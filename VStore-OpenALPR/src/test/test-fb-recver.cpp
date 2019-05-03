//
// Created by xzl on 12/26/17.
//

#include <zmq.hpp>
#include "log.h"
#include "vs-types.h"
#include "config.h"
#include "rxtx.h"


using namespace vs;

int main (int argc, char *argv[])
{
	zmq::context_t context (1 /* # io threads */);

	zmq::socket_t fb_recv(context, ZMQ_PULL);
//	fb_recv.bind(FB_PULL_ADDR);
	fb_recv.bind("tcp://*:10001");

	printf ("Press Enter when the workers are ready: ");
	getchar ();
	printf ("Sending tasks to workers…\n");

	for (int i = 0; i < 20; i++) {
		I("%d...", i);
#if 0
		zmq::message_t msg;
		fb_recv.recv(&msg);
#else
		/* check fb */
		feedback fb;
		bool ret = recv_one_fb(fb_recv, &fb, true /* blocking */);
		if (ret)
			I("got one fb. id %d", fb.fid);
		else
			I("failed to get fb");
#endif
	}

}