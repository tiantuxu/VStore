//
// Created by xzl on 12/26/17.
//

#include <zmq.hpp>
#include "log.h"
#include "vs-types.h"
#include "config.h"
#include "rxtx.h"


using namespace vs;

int main(int ac, char * av[])
{
	zmq::context_t context (1 /* # of io threads */);

	// for feedback
	zmq::socket_t fb_sender(context, ZMQ_PUSH);
//	fb_sender.connect(WORKER_PUSHFB_TO_ADDR);
	fb_sender.connect("tcp://localhost:10001");
	EE("connected to fb");

	int fb_cnt = 0;

	for (int k = 0; k < 20; k++) {
#if 0
		zmq::message_t msg;
		fb_sender.send(msg);
		I("%d...", k);
#else
		data_desc desc(TYPE_RAW_FRAME);

		/* send some fb back */
		feedback fb(desc.cid, fb_cnt++);
		send_one_fb(fb, fb_sender);
//		feedback fb1(desc.id, fb_cnt++);
//		send_one_fb(fb1, fb_sender);

#endif

	}

	return 0;

}