//
// Created by xzl on 12/25/17.
//

// act like the server.
// simply pull a chunk. do nothing else

#include <sstream> // std::ostringstream

#include <zmq.hpp>

#include "mm.h"
#include "config.h"
#include "vs-types.h"
#include "log.h"
#include "rxtx.h"

using namespace vs;

void recv_one_file(const char *fname /* to save */) {
	zmq::context_t context(1 /* # io threads */);

	zmq::socket_t recv(context, ZMQ_PULL);
	recv.bind(SERVER_PULL_ADDR);

	I("bound to socket");

	{
		/* recv the desc */
		zmq::message_t dmsg;
		recv.recv(&dmsg);
		I("got desc msg. msg size =%lu", dmsg.size());

		std::string s((char const *)dmsg.data(), dmsg.size()); /* copy over */
		data_desc desc;
		std::istringstream ss(s);
		boost::archive::text_iarchive ia(ss);

		ia >> desc;
		I("%s", desc.to_string().c_str());
	}

	{
		/* recv the chunk */
		zmq::message_t cmsg;
		recv.recv(&cmsg);
		I("got chunk msg. size =%lu", cmsg.size());

		/* write the chunk to file */
		FILE *f = fopen(fname, "wb");
		xzl_bug_on(!f);
		auto ret = fwrite(cmsg.data(), cmsg.size(), 1, f);
		xzl_bug_on(ret != 1);
		fclose(f);
		I("written chunk to file %s", fname);
/*
		if (cmsg.more())
			I("there's more");
		else
			I("no more");
*/
	}

}

void test_recv_one_chunk(const char *fname /* to save */)
{
	zmq::context_t context(1 /* # io threads */);

	zmq::socket_t recv(context, ZMQ_PULL);
	recv.bind(SERVER_PULL_ADDR);

	I("bound to socket");

	data_desc desc;
	recv_one_chunk_tofile(recv, &desc, fname);

	I("written to file %s", fname);
}

int main (int argc, char *argv[])
{
//		recv_one_file(argv[1]);
	test_recv_one_chunk(argv[1]);
}
