//
// Created by xzl on 12/23/17.
//

#ifndef VIDEO_STREAMER_DECODER_H
#define VIDEO_STREAMER_DECODER_H

#include <zmq.hpp>
#include <lmdb.h>

#include "tensorflow/noscope/vs-types.h"
#include "tensorflow/noscope/mm.h"
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/imgutils.h>
}

void init_decoder(bool use_hw);


int decode_one_file(const char *fname, zmq::socket_t &sender,
										vs::data_desc const &desc);

__attribute__((deprecated))
int decode_one_file_sw(const char *fname, zmq::socket_t &sender,
											 vs::data_desc const &desc);

#endif //VIDEO_STREAMER_DECODER_H
