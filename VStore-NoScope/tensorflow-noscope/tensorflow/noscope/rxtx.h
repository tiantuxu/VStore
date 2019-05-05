//
// Created by xzl on 12/29/17.
//

#ifndef VIDEO_STREAMER_RXTX_H
#define VIDEO_STREAMER_RXTX_H

#include <memory>

#include <zmq.hpp>
#include "tensorflow/noscope/vs-types.h"
#include "tensorflow/noscope/mm.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

/* send/recv perf feedback */
int send_one_fb(vs::feedback const & fb, zmq::socket_t &sender);
bool recv_one_fb(zmq::socket_t &s, vs::feedback * fb, bool blocking);

unsigned send_multi_from_db(MDB_env *env, MDB_dbi dbi, vs::cid_t start, vs::cid_t end, zmq::socket_t &s,
														vs::data_desc const &desc);

unsigned send_chunk_from_db(MDB_env *env, MDB_dbi dbi, vs::cid_t start, vs::cid_t end, zmq::socket_t &s, vs::data_desc const &temp_desc /* template. */, int id);


int send_one_frame(uint8_t *buffer, int size, zmq::socket_t &sender,
									 vs::data_desc const &desc);

int send_one_frame_mmap(uint8_t *buffer, size_t sz, zmq::socket_t &sender,
												vs::data_desc const & fdesc, my_alloc_hint * hint);

int send_one_from_db(uint8_t * buffer, size_t sz, zmq::socket_t &sender,
													 vs::data_desc const & cdesc, my_alloc_hint * hint);
int send_one_720_from_db(uint8_t * buffer, size_t sz, zmq::socket_t &sender,
					 vs::data_desc const & cdesc, my_alloc_hint * hint);
std::shared_ptr<zmq::message_t> recv_one_chunk_withtid(zmq::socket_t & s, vs::data_desc *desc);
int send_one_chunk_from_db(uint8_t * buffer, size_t sz, zmq::socket_t &sender,
						   vs::data_desc const & desc, my_alloc_hint * hint, int id);
unsigned recv_one_frame(zmq::socket_t & recv, size_t* sz = nullptr, vs::data_desc *fdesc = nullptr);

std::shared_ptr<zmq::message_t> recv_one_frame(zmq::socket_t & recv, vs::data_desc *fdesc);
std::shared_ptr<zmq::message_t> recv_one_frame_720(zmq::socket_t & recv);

void send_chunk_eof(vs::cid_t const & cid, unsigned int chunk_seq, zmq::socket_t & sender);
void send_raw_eof(vs::cid_t const & cid, unsigned int chunk_seq, unsigned int frame_seq, zmq::socket_t & sender);
void send_decoded_eof(vs::cid_t const & cid, unsigned int chunk_seq, unsigned int frame_seq, zmq::socket_t & sender);

//int send_all_stream_info(zmq::socket_t & sender);

std::shared_ptr<zmq::message_t> recv_one_chunk(zmq::socket_t &s,
																							 vs::data_desc *desc);

std::shared_ptr<zmq::message_t> recv_one_chunk_720(zmq::socket_t & s);

void recv_one_chunk_to_buf(zmq::socket_t & s, vs::data_desc *desc,
													 char **p, size_t *sz);

int recv_one_chunk_tofile(zmq::socket_t &s, vs::data_desc *desc,
													const char *fname);


#endif //VIDEO_STREAMER_RXTX_H
