//
// Created by xzl on 12/23/17.
//
// cf: hw_decode.c

/*
 * Copyright (c) 2017 Jun Zhao
 * Copyright (c) 2017 Kaixuan Liu
 *
 * HW Acceleration API (video decoding) decode sample
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <zmq.hpp>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/imgutils.h>
}
#include "tensorflow/noscope/measure.h"
#include "tensorflow/noscope/log.h"
#include "tensorflow/noscope/mydecoder.h"
#include "tensorflow/noscope/rxtx.h"

using namespace vs;

/* this is okay: we don't support mix hw/sw in one program. */
static bool is_use_hw = false;

/* one time ops */
void init_decoder(bool use_hw)
{
	is_use_hw = use_hw;

	av_register_all();
	avcodec_register_all();
}

static AVBufferRef *hw_device_ctx = NULL;
static enum AVPixelFormat hw_pix_fmt;
static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
																				const enum AVPixelFormat *pix_fmts)
{
	const enum AVPixelFormat *p;

	for (p = pix_fmts; *p != -1; p++) {
		if (*p == hw_pix_fmt)
			return *p;
	}

	fprintf(stderr, "Failed to get HW surface format.\n");
	return AV_PIX_FMT_NONE;
}

static int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type)
{
	int err = 0;

	if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
																		NULL, NULL, 0)) < 0) {
		fprintf(stderr, "Failed to create specified HW device.\n");
		return err;
	}
	ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

	return err;
}

/* decode a @packet into a set of frames, write each frame as a msg to @sender
 *
 * the hw decoder workflow
 */
static int decode_write_hw(AVCodecContext *avctx, AVPacket *packet,
												zmq::socket_t & sender, data_desc const & cdesc)
{
	AVFrame *frame = NULL, *sw_frame = NULL;
	AVFrame *tmp_frame = NULL;
	uint8_t *buffer = NULL;
	int size;
	int ret = 0;

	ret = avcodec_send_packet(avctx, packet);
	if (ret < 0) {
		fprintf(stderr, "Error during decoding\n");
		return ret;
	}

	/* xzl: XXX why allocate new frame every time? expensive? */
	while (ret >= 0) {
		if (!(frame = av_frame_alloc()) || !(sw_frame = av_frame_alloc())) {
			fprintf(stderr, "Can not alloc frame\n");
			ret = AVERROR(ENOMEM);
			goto fail;
		}

		ret = avcodec_receive_frame(avctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			av_frame_free(&frame);
			av_frame_free(&sw_frame);
			return 0;
		} else if (ret < 0) {
			fprintf(stderr, "Error while decoding\n");
			goto fail;
		}

		if (is_use_hw) {
			if (frame->format == hw_pix_fmt) {
				/* retrieve data from GPU to CPU */
				if ((ret = av_hwframe_transfer_data(sw_frame, frame, 0)) < 0) {
					fprintf(stderr, "Error transferring the data to system memory\n");
					goto fail;
				}
				tmp_frame = sw_frame;
			} else
				tmp_frame = frame;
		} else
			tmp_frame = frame;

		size = av_image_get_buffer_size((enum AVPixelFormat) tmp_frame->format,
																		tmp_frame->width,
																		tmp_frame->height, 1);

		buffer = (uint8_t *)av_malloc(size);
		if (!buffer) {
			fprintf(stderr, "Can not alloc buffer\n");
			ret = AVERROR(ENOMEM);
			goto fail;
		}
		ret = av_image_copy_to_buffer(buffer, size,
																	(const uint8_t * const *)tmp_frame->data,
																	(const int *)tmp_frame->linesize,
																	(enum AVPixelFormat) tmp_frame->format,
																	tmp_frame->width, tmp_frame->height, 1);
		if (ret < 0) {
			fprintf(stderr, "Can not copy image to buffer\n");
			goto fail;
		}

		{
			data_desc fdesc(TYPE_DECODED_FRAME);
			fdesc.cid = cdesc.cid; /* inherit the chunk's cid */
			fdesc.c_seq = cdesc.c_seq; /* inherit chunk seq */
			fdesc.f_seq = avctx->frame_number;
#if 1
			if ((ret = send_one_frame(buffer, size, sender, fdesc)) < 0) { /* will free buffer */
				fprintf(stderr, "Failed to dump raw data.\n");
				goto fail;
			}
#endif
#if 0
            I("image size = %d", size);
            zmq::message_t cmsg(size);
            memcpy(cmsg.data(), buffer, size);
            auto ret = sender.send(cmsg, ZMQ_SNDMORE);
#endif
        }
fail:
		av_frame_free(&frame);
		av_frame_free(&sw_frame);
//		if (buffer)
//			av_freep(&buffer);
		if (ret < 0)
			return ret;
	}

	return 0;
}

//static AVCodec *decoder = NULL;
//static int video_stream = -1;
//static AVCodecContext *decoder_ctx = NULL; /* okay to reuse across files? */

/* decode one file and write frames to @sender.
 * will send the last desc of frame seq with no data.
 *
 * can use both sw and hw, depending on @is_use_hw
 * */
int decode_one_file(const char *fname, zmq::socket_t &sender,
										const data_desc &cdesc /* chunk info */) {

	AVFormatContext *input_ctx = NULL;
	int video_stream = -1, ret;
	AVStream *video = NULL;
	AVCodecContext *decoder_ctx = NULL;
	AVCodec *decoder = NULL;
	AVPacket packet;

	//k2_measure("decode start");

//	av_register_all();

	enum AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
	const char * devname = "cuda"; // "cuda" "vdpau"

	if (is_use_hw) {
		type = av_hwdevice_find_type_by_name(devname);
		if (type == AV_HWDEVICE_TYPE_NONE) {
			fprintf(stderr, "Device type %s is not supported.\n", devname);
			fprintf(stderr, "Available device types:");
			while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
				fprintf(stderr, " %s", av_hwdevice_get_type_name(type));
			fprintf(stderr, "\n");
			return -1;
		}
	}

	/* open the input file */
	xzl_assert(fname);
	ret = avformat_open_input(&input_ctx, fname, NULL, NULL);
	xzl_bug_on_msg(ret != 0, "Cannot find input stream information");

//	once = !decoder;

//	if (once) {
//	xzl_bug_on(video_stream != -1); /* must be invalid */

	ret = avformat_find_stream_info(input_ctx, NULL);
	xzl_bug_on_msg(ret < 0, "Cannot find a video stream in the input file");

	/* find the video stream information */
	ret = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
	if (ret < 0) {
		fprintf(stderr, "Cannot find a video stream in the input file\n");
		return -1;
	}
	video_stream = ret;

	if (is_use_hw) {
		for (int i = 0;; i++) {
			const AVCodecHWConfig *config = avcodec_get_hw_config(decoder, i);
			if (!config) {
				fprintf(stderr, "Decoder %s does not support device type %s.\n",
								decoder->name, av_hwdevice_get_type_name(type));
				return -1;
			}
			if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
					config->device_type == type) {
				hw_pix_fmt = config->pix_fmt;
				break;
			}
		}
	}

	decoder_ctx = avcodec_alloc_context3(decoder);
	xzl_bug_on_msg(!decoder_ctx, "no mem for avcodec ctx");
//}

	video = input_ctx->streams[video_stream];

//	if (once) {
		/* fill in the decoder ctx with stream's codec parameters... */
		ret = avcodec_parameters_to_context(decoder_ctx, video->codecpar);
		xzl_bug_on(ret < 0);

	if (is_use_hw)
		decoder_ctx->get_format  = get_hw_format;

		av_opt_set_int(decoder_ctx, "refcounted_frames", 1, 0);

	if (is_use_hw) {
		ret = hw_decoder_init(decoder_ctx, type);
		xzl_bug_on_msg(ret < 0, "hw decoder init failed");
	}

		ret = avcodec_open2(decoder_ctx, decoder, NULL);
		xzl_bug_on_msg(ret < 0, "Failed to open codec for stream");
//	}

	//k2_measure("decoder init'd");

	/* actual decoding and dump the raw data */
	while (ret >= 0) {
		if ((ret = av_read_frame(input_ctx, &packet)) < 0)
			break;

		if (video_stream == packet.stream_index) {
			//I("decode here");
			ret = decode_write_hw(decoder_ctx, &packet, sender, cdesc);
		}
		av_packet_unref(&packet);
	}

	/* flush the decoder */
	packet.data = NULL;
	packet.size = 0;
	ret = decode_write_hw(decoder_ctx, &packet, sender, cdesc);

#if 0
	{
		/* send the final desc with no frame, marking the end of the chunk */
		data_desc fdesc(TYPE_DECODED_FRAME);

		fdesc.cid = cdesc.cid; /* inherit the chunk's cid */
		fdesc.c_seq = cdesc.c_seq; /* inherit chunk seq */
		fdesc.f_seq = decoder_ctx->frame_number + 1;

		ret = send_one_frame(nullptr, 0, sender, fdesc);
		xzl_bug_on(ret != 0);
	}
#endif
	send_decoded_eof(cdesc.cid, cdesc.c_seq, decoder_ctx->frame_number + 1, sender);

	// may skew the measurement
//	EE("total %d frames", decoder_ctx->frame_number);

	av_packet_unref(&packet);
	avcodec_free_context(&decoder_ctx);
	avformat_close_input(&input_ctx);

	if (is_use_hw)
		av_buffer_unref(&hw_device_ctx);

	//k2_measure("decode/send end");

	return 0;
}
