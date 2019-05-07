//
// Created by xzl on 12/27/17.
//
// standalone test for the decode functions
// it loads a file, decodes it, and sends frames over zmq
// sink_func in a separate thread recvs the frames
//


#include <zmq.hpp>
#include <iostream>
#include <boost/program_options.hpp>

#include "config.h"
#include "mydecoder.h"

#include "log.h"
#include "measure.h"
#include "vs-types.h"
#include "rxtx.h"

using namespace std;
using namespace vs;

/* main func for recv thread */
static void *sink_func(void *t) {
	/* a sep context */
	zmq::context_t context (1 /* # io threads */);

	zmq::socket_t receiver(context, ZMQ_PULL);
	receiver.bind(FRAME_PULL_ADDR);

	I("sink: bound to %s. wait for workers to push ...", FRAME_PULL_ADDR);

	int i = 0;
	int fid;

	while (1) {
		fid = recv_one_frame(receiver);
		I("recv'd %d frame. fid = %d", i++, fid);
		if (fid == -1)
			break;
	}

	return nullptr;
}

int main(int ac, char * av[])
{
	const char *fname_in = av[2];
	const char *fname_out = av[3];

	if (ac < 4) {
		fprintf(stderr, "Usage: %s {sw|hw} <input file> <output file>\n", av[0]);
		return -1;
	}

	if (strncmp(av[1], "sw", 5) == 0)
		init_decoder(false);
	else if (strncmp(av[1], "hw", 5) == 0)
		init_decoder(true);
	else {
		fprintf(stderr, "Usage: %s {sw|hw} <input file> <output file>\n", av[0]);
		return -1;
	}

	zmq::context_t context (1 /* # of io threads */);

	// for outgoing decoded frames
	zmq::socket_t s(context, ZMQ_PUSH);
//	sender.bind(SERVER_PUSH_ADDR);
	s.connect(WORKER_PUSH_TO_ADDR);
	EE("connected to sink (frames)");

	pthread_t sink;
	int rc = pthread_create(&sink, NULL, sink_func, NULL);
	xzl_bug_on(rc != 0);

	data_desc desc;
	decode_one_file(fname_in, s, desc);

	rc = pthread_join(sink, nullptr);

	xzl_bug_on(rc != 0);

	I("all done");

	return 0;
}