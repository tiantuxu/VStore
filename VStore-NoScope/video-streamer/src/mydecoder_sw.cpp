//
// Created by xzl on 12/24/17.
//
// obsoleted. can't decode h264 properly. see mydecoder_hw.cpp

#include "mydecoder.h"
#include "vs-types.h"
#include "log.h"

using namespace vstreamer;

#define INBUF_SIZE (4 * 1024  * 1024)
//#define INBUF_SIZE (1024)

/* cf: decode_video.c in ffmpeg */

/* sw decoding workflow.
 * @frame allocated once by the caller. */
static int decode_write_sw(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt,
														zmq::socket_t & sender, chunk_desc const & cdesc)
{
	int ret, size;
	uint8_t *buffer = NULL;

//	AVFrame *frame;

	ret = avcodec_send_packet(dec_ctx, pkt);
	if (ret < 0) {
		fprintf(stderr, "Error sending a packet for decoding\n");
		exit(1);
	}

	while (ret >= 0) {

//		frame = av_frame_alloc();
//		xzl_bug_on(!frame);

		ret = avcodec_receive_frame(dec_ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return -1; /* this is okay */

		xzl_bug_on_msg(ret < 0, "Error during decoding");

//		printf("saving frame %3d\n", dec_ctx->frame_number);
//		fflush(stdout);

		I("decoded one frame");

		size = av_image_get_buffer_size((enum AVPixelFormat) frame->format,
																		frame->width,
																		frame->height, 1);

		buffer = (uint8_t *)av_malloc(size);
		xzl_bug_on(!buffer);

		/* copy image to buffer for tx
		 * XXX direct use frame->data[0]?
		 * */
		ret = av_image_copy_to_buffer(buffer, size,
																	(const uint8_t *const *) frame->data,
																	(const int *) frame->linesize,
																	(enum AVPixelFormat) frame->format,
																	frame->width, frame->height, 1);

		xzl_bug_on(ret < 0);

		frame_desc fdesc (cdesc.id, dec_ctx->frame_number); /* XXX: gen fdesc */
		ret = send_one_frame(buffer, size, sender, fdesc); /* will free buffer */
		xzl_bug_on_msg(ret < 0, "Failed to dump raw data.\n");

		I("sent out one frame -- %d", dec_ctx->frame_number);

		/* the picture is allocated by the decoder. no need to
			 free it */

//	char buf[1024];
//		snprintf(buf, sizeof(buf), "%s-%d", filename, dec_ctx->frame_number);
//		pgm_save(frame->data[0], frame->linesize[0],
//						 frame->width, frame->height, buf);
	}

	return 0;
}

static uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];

/* xzl: obsoleted. can't do h264. see above */
int decode_one_file_sw(const char *fname, zmq::socket_t &sender,
											 chunk_desc const &cdesc /* chunk info */) {
	const AVCodec *codec;
	AVCodecParserContext *parser;
	AVCodecContext *c= NULL;
	FILE *f;
	AVFrame *frame;		// to be allocated by writer
	int ret;
	AVPacket *pkt;
	size_t   data_size;
	uint8_t *data;

//	avcodec_register_all();

	pkt = av_packet_alloc();
	xzl_bug_on(!pkt);

	/* find the MPEG-1 video decoder */
	codec = avcodec_find_decoder(AV_CODEC_ID_MPEG1VIDEO);
//	codec = avcodec_find_decoder(AV_CODEC_ID_H264);	/* xzl: does not work */
	xzl_bug_on(!codec);

	parser = av_parser_init(codec->id);
	xzl_bug_on(!parser);

	c = avcodec_alloc_context3(codec);
	xzl_bug_on(!c);

	/* For some codecs, such as msmpeg4 and mpeg4, width and height
		 MUST be initialized there because this information is not
		 available in the bitstream. */
	xzl_bug_on(avcodec_open2(c, codec, NULL) < 0);

	f = fopen(fname, "rb");
	xzl_bug_on(!f);

	frame = av_frame_alloc();
	xzl_bug_on(!frame);

	I("codec init done");

	while (!feof(f)) {
		/* read raw data from the input file */
		data_size = fread(inbuf, 1, INBUF_SIZE, f);
		if (!data_size)
			break;

		/* use the parser to split the data into frames
		 *
		 * xzl: see http://roxlu.com/2014/039/decoding-h264-and-yuv420p-playback
		 * */
		data = inbuf;
		while (data_size > 0) {
			ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size,
														 data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
			/* XXX: if &pkt->size == 0 && ret > 0, larger @data_size is needed */
//			xzl_bug_on(pkt->size == 0);
			if (ret < 0) {
				fprintf(stderr, "Error while parsing\n");
				exit(1);
			}
			data      += ret;
			data_size -= ret;

			if (pkt->size)
				decode_write_sw(c, frame, pkt, sender, cdesc);
		}
	}

	/* flush the decoder */
	decode_write_sw(c, frame, NULL, sender, cdesc);

	fclose(f);

	av_parser_close(parser);
	avcodec_free_context(&c);
	av_frame_free(&frame);
	av_packet_free(&pkt);

	return 0;
}