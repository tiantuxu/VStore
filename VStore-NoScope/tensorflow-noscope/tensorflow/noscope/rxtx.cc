//
// Created by xzl on 12/23/17.
//

#include "tensorflow/noscope/mydecoder.h"
#include <lmdb.h>
#include <sstream>
#include <zmq.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

extern "C" {
#include <libavutil/mem.h>
#include "tensorflow/noscope/measure.h"
}
#include "tensorflow/noscope/config.h"
#include "tensorflow/noscope/vs-types.h"
#include "tensorflow/noscope/log.h"
#include "tensorflow/noscope/mm.h"
#include "tensorflow/noscope/rxtx.h"

using namespace std;
using namespace vs;

#if 0
/* free frame data */
static void my_free_av (void *data, void *hint)
{
	xzl_bug_on(!data);
	av_freep(&data);
}
#endif

int send_one_fb(feedback const & fb, zmq::socket_t &sender)
{
	/* send frame desc */
	ostringstream oss;
	boost::archive::text_oarchive oa(oss);

	oa << fb;
	string s = oss.str();

	/* desc msg. explicit copy content over */
	zmq::message_t dmsg(s.size());
	memcpy(dmsg.data(), s.c_str(), s.size());
	//zmq::message_t dmsg(s.begin(), s.end());
	auto sz = dmsg.size();

	auto ret = sender.send(dmsg);
	if (!ret)
		EE("send failure");
	else
		I("send fb. id = %d. msg size %lu", fb.fid, sz);

	return 0;
}

/* return: true if a feedback is recv'd */
bool recv_one_fb(zmq::socket_t &s, feedback * fb, bool blocking = false)
{
	zmq::message_t dmsg;
	xzl_bug_on(!fb);

	bool ret = s.recv(&dmsg, blocking ? 0 : ZMQ_NOBLOCK);
//	bool ret = s.recv(&dmsg);

	if (ret) {
		I("got fb msg. msg size =%lu", dmsg.size());

		string ss((char const *) dmsg.data(), dmsg.size()); /* copy over */
		istringstream iss(ss);
		boost::archive::text_iarchive ia(iss);

		ia >> *fb;
	}

	return ret;
}

int send_one_720_from_db(uint8_t * buffer, size_t sz, zmq::socket_t &sender,
                     data_desc const & desc, my_alloc_hint * hint) {
    xzl_bug_on(!buffer || !hint);

    /* send frame desc */
    ostringstream oss;
    boost::archive::text_oarchive oa(oss);

    oa << desc;
    string s = oss.str();

    /* desc msg. explicit copy content over */
    zmq::message_t dmsg(s.size());
    memcpy(dmsg.data(), s.c_str(), s.size());
    //zmq::message_t dmsg(s.begin(), s.end());
    //sender.send(dmsg, ZMQ_SNDMORE); /* multipart msg */

    VV("desc sent");
    I("send_one_from_db");

    /* send the chunk */
    zmq::message_t cmsg(buffer, sz, my_free /* our deallocation func */, (void *) hint);
    I("%d", sz);
    auto ret = sender.send(cmsg, 0); /* no more msg */
    xzl_bug_on(!ret);

    VV("frame sent");

    return 0;
}


/* send multiple chunks/frames from the db.
 *
 * caller must ensure: no tx is for the db is alive as of now
 *
 * @dbi: must be opened already.
 *
 * @start/end: inclusive/exclusive
 * @desc: template desc. its .type indicate whether this is for raw frames or for chunks
 *
 * @return: total chunks sent.
 * */
unsigned send_multi_from_db(MDB_env *env, MDB_dbi dbi, cid_t start, cid_t end,
														 zmq::socket_t &s,
														 data_desc const &temp_desc /* template. */)
{
	MDB_txn *txn;
	MDB_val key, data;
    //vs::chunk_info chunk;
	cv::Mat chunk[CHUNK_SIZE];
	MDB_cursor *cursor;
	MDB_cursor_op op;
	int rc;

	int height_720 = 720;
	int width_720 = 1280;
	int framesize_720 = height_720 * width_720;

	xzl_assert(env);

	rc = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
	xzl_bug_on(rc != 0);
	if(rc != 0){
		I("rc = %d", rc);
		I("%s",mdb_strerror(rc));
	}

	rc = mdb_cursor_open(txn, dbi, &cursor);
	xzl_bug_on(rc != 0);

	my_alloc_hint *hint = new my_alloc_hint(USE_LMDB_REFCNT);
	hint->txn = txn;
	hint->cursor = cursor;

	unsigned cnt = 0; /* also used as seq number */

	/* once we start to send, the refcnt can be dec by the async sender,
	 * which means it can go neg. */

	while (1) {
		rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT);
		if (rc == MDB_NOTFOUND)
			break;

		cid_t id = *(cid_t *)key.mv_data;

        if (id.as_uint < start.as_uint)
            continue;

		if (id.as_uint >= end.as_uint)
			break;

		I("loaded one k/v. key: %lu, sz %zu data sz %zu",
			*(uint64_t *)key.mv_data, key.mv_size,
			data.mv_size);

		data_desc desc(temp_desc); /* make a desc out of the template */

		desc.cid = id;

		switch (desc.type) {
			case TYPE_RAW_FRAME:
				desc.f_seq = cnt;
				break;
			case TYPE_CHUNK:
				desc.c_seq = cnt;
				break;
			default:
				xzl_bug("???");
		}

		/* XXX more. check desc.type and fill in the rest of the desc... */
        if (start.as_uint + 1 == end.as_uint){
            send_one_720_from_db((uint8_t *) data.mv_data, data.mv_size, s, desc, hint);
        }
        else if(start.as_uint + 1 + (CHUNK_SIZE - 1) * 2 == end.as_uint){
            /*
			char *imagedata_720 = NULL;
			imagedata_720 = (char*)(data.mv_data), data.mv_size;
			chunk[cnt].create(height_720, width_720, CV_8UC1);
			memcpy(chunk[cnt].data, imagedata_720, framesize_720);
            //cout << chunk[cnt] << endl;
             */
            if(cnt < CHUNK_SIZE - 1){
                zmq::message_t cmsg((uint8_t *) data.mv_data, data.mv_size, my_free /* our deallocation func */, (void *) hint);
                auto ret = s.send(cmsg, ZMQ_SNDMORE);
            }
            else{
                zmq::message_t cmsg((uint8_t *) data.mv_data, data.mv_size, my_free /* our deallocation func */, (void *) hint);
                auto ret = s.send(cmsg, 0);
                //send_one_chunk_from_db(chunk, CHUNK_SIZE * data.mv_size, s, desc, hint);
            }
        }
        else {
            send_one_from_db((uint8_t *) data.mv_data, data.mv_size, s, desc, hint);
        }
        cnt ++;
	}

    /*
    //asm volatile("" ::: "memory");
    if(start.as_uint + 1 + (CHUNK_SIZE - 1) * 2 == end.as_uint){
        //I("sent from here, size = %d", sizeof(*(uint8_t *)(data.mv_data)));
        data_desc desc(temp_desc);
        send_one_chunk_from_db(chunk, CHUNK_SIZE * data.mv_size, s, desc, hint);
        mdb_txn_abort(txn);
        I("abort here2");
        return cnt;
    }
    */
    if(start.as_uint == end.as_uint - 1 || start.as_uint + 1 + (CHUNK_SIZE - 1) * 2 == end.as_uint){
        I("abort here1");
        mdb_txn_abort(txn);
        return cnt;
	}

	I("Any more");

	/* send the eof -- depending whether we are sending frames or chunks */
	switch (temp_desc.type) {
		case TYPE_RAW_FRAME:
			send_raw_eof(temp_desc.cid, temp_desc.c_seq, cnt, s);
			break;
		case TYPE_CHUNK:
			send_chunk_eof(temp_desc.cid, cnt, s);
			break;
		default:
			xzl_bug("???");
	}

	/* for mm
	 * bump refcnt one time */
	long before = hint->refcnt.fetch_add(cnt);
	xzl_bug_on(before < - (long)cnt); /* impossible */

	if (before == - (long)cnt) {
		/* meaning that refcnt reaches 0... all outstanding chunks are sent */
		W("close the tx");
		mdb_cursor_close(cursor);
		mdb_txn_abort(txn);
	}
	return cnt;
}

unsigned send_chunk_from_db(MDB_env *env, MDB_dbi dbi, cid_t start, cid_t end, zmq::socket_t &s, data_desc const &temp_desc /* template. */, int tid)
{
    MDB_txn *txn;
    MDB_val key, data;
    MDB_cursor *cursor;
    MDB_cursor_op op;
    int rc;

    xzl_assert(env);

    rc = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
    xzl_bug_on(rc != 0);

    rc = mdb_cursor_open(txn, dbi, &cursor);
    xzl_bug_on(rc != 0);

    my_alloc_hint *hint = new my_alloc_hint(USE_LMDB_REFCNT);
    hint->txn = txn;
    hint->cursor = cursor;

    int cnt = 0;

    /* once we start to send, the refcnt can be dec by the async sender,
     * which means it can go neg. */
    while (1) {
        rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT);
        if (rc == MDB_NOTFOUND)
            break;

        cid_t id = *(cid_t *)key.mv_data;

		if (id.as_uint < start.as_uint){
            continue;
            I("skip here id = %d, start = %d, end = %d", id.as_uint, start.as_uint, end.as_uint);
        }

        if (id.as_uint >= end.as_uint)
            break;

        I("loaded one k/v. key: %lu, sz %zu data sz %zu",
          *(uint64_t *)key.mv_data, key.mv_size,
          data.mv_size);

        data_desc desc(temp_desc); /* copy ctor */
		desc.tid = tid;
		I("tid = %d", desc.tid);

		switch (desc.type) {
            case TYPE_RAW_FRAME:
                desc.c_seq = cnt;
                desc.cid = id;
                break;
            case TYPE_CHUNK:
                desc.c_seq = cnt;
                desc.cid = id;
				break;
            default:
                xzl_bug("???");
        }

        /* XXX more. check desc.type and fill in the rest of the desc... */
        send_one_chunk_from_db((uint8_t *)data.mv_data, data.mv_size, s, desc, hint, tid);
        cnt ++;
    }

	//mdb_txn_abort(txn);

	/* bump refcnt one time */
    int before = hint->refcnt.fetch_add(cnt);
    xzl_bug_on(before < - cnt ); /* impossible */

    if (before == - cnt) { /* meaning that refcnt reaches 0... all outstanding chunks are sent */
        W("close the tx");
        mdb_cursor_close(cursor);
        mdb_txn_abort(txn);
    }

    return cnt;
}

/* send a decoded frame as one message with two parts,
 * in which the frame buffer is allocated by avmalloc.
 *
 * the ownership of the frame is moved to zmq, which will free the frame later.
 *
 * @fdesc: frame descriptor, content from which will be copied & serailized into the msg
 * @buffer allocated from av_malloc. zmq has to free it
 *
 * if buffer == nullptr, send the desc only (and indicate no more msg)
 */
int send_one_frame(uint8_t *buffer, int size, zmq::socket_t &sender,
									 data_desc const & fdesc)
{
	int ret;

	if (!buffer) {
		/* send frame desc */
		ostringstream oss;
		boost::archive::text_oarchive oa(oss);

		oa << fdesc;
		string s = oss.str();

		/* desc msg. explicit copy content over */
		zmq::message_t dmsg(s.size());
		memcpy(dmsg.data(), s.c_str(), s.size());
		//zmq::message_t dmsg(s.begin(), s.end());
		sender.send(dmsg, 0); /* no more */

		return 0;
	}

	{
		/* send frame desc */
		ostringstream oss;
		boost::archive::text_oarchive oa(oss);

		oa << fdesc;
		string s = oss.str();

		/* desc msg. explicit copy content over */
		zmq::message_t dmsg(s.size());
		memcpy(dmsg.data(), s.c_str(), s.size());
		//zmq::message_t dmsg(s.begin(), s.end());
		sender.send(dmsg, ZMQ_SNDMORE); /* multipart msg */

		VV("desc sent");

		/* send frame */
		my_alloc_hint *hint = new my_alloc_hint(USE_AVMALLOC, size);
		zmq::message_t cmsg(buffer, size, my_free, hint);
		ret = sender.send(cmsg, 0); /* no more msg */
		//ret = sender.send(cmsg, ZMQ_SNDMORE); /* no more msg */
		xzl_bug_on(!ret);

		VV("frame sent");
	}

	/* will free @buffer internally */
//	zmq::message_t msg(buffer, size, my_free_av);
//	ret = sender.send(msg);
//	xzl_bug_on(!ret);

	return 0;
}

#if 0 /* tbd */
/* return: # of stream info sent */
extern stream_desc all_streams[]; /* in stream-info.cpp */
int send_all_stream_info(zmq::socket_t & sender)
{
	int cnt;
	stream_desc * sd = all_streams;

	for (cnt = 0; sd[cnt].stream_id == -1; cnt++) {
		ostringstream oss;
		boost::archive::text_oarchive oa(oss);

		oa << *sd;
		string s = oss.str();

		zmq::message_t dmsg(s.begin(), s.end());
		if (sd[cnt+1].stream_id == -1) /* this is the last one */
			sender.send(dmsg, 0); /* multipart msg */
		else
			sender.send(dmsg, ZMQ_SNDMORE); /* multipart msg */

		VV("one stream info sent");
	}
	return cnt;
}
#endif

/* send a raw frame from a mmap'd buffer.
 * @hint: the info about the mmap'd buffer (inc refcnt) for zmq to perform unmapping
 *
 * @others: see above.
 */
int send_one_frame_mmap(uint8_t *buffer, size_t sz, zmq::socket_t &sender,
												data_desc const & fdesc, my_alloc_hint * hint)
{
	xzl_bug_on(!buffer || !hint);

	/* send frame desc */
	ostringstream oss;
	boost::archive::text_oarchive oa(oss);

	oa << fdesc;
	string s = oss.str();

	/* desc msg. explicit copy content over */
	zmq::message_t dmsg(s.size());
	memcpy(dmsg.data(), s.c_str(), s.size());
	//zmq::message_t dmsg(s.begin(), s.end());
	sender.send(dmsg, ZMQ_SNDMORE); /* multipart msg */

	VV("desc sent");

	/* send the frame */

	zmq::message_t cmsg(buffer, sz, my_free, (void *)hint);
	auto ret = sender.send(cmsg, 0); /* no more msg */
	xzl_bug_on(!ret);

	VV("frame sent");

	return 0;
}

/* send one buf (chunk or frame) returned from a live lmdb transaction.  *
 *
 * @desc: already filled in.
 */
int send_one_from_db(uint8_t * buffer, size_t sz, zmq::socket_t &sender,
												data_desc const & desc, my_alloc_hint * hint) {
	xzl_bug_on(!buffer || !hint);

	/* send frame desc */
	ostringstream oss;
	boost::archive::text_oarchive oa(oss);

	oa << desc;
	string s = oss.str();

	/* desc msg. explicit copy content over */
	zmq::message_t dmsg(s.size());
	memcpy(dmsg.data(), s.c_str(), s.size());
	//zmq::message_t dmsg(s.begin(), s.end());
	sender.send(dmsg, ZMQ_SNDMORE); /* multipart msg */

	VV("desc sent");
    I("send_one_from_db");

	/* send the chunk */
	zmq::message_t cmsg(buffer, sz, my_free /* our deallocation func */, (void *) hint);
    //I("size = %d", sizeof(uint8_t *));
	auto ret = sender.send(cmsg, 0); /* no more msg */
	xzl_bug_on(!ret);

	VV("frame sent");

	return 0;
}

int send_one_chunk_from_db(uint8_t * buffer, size_t sz, zmq::socket_t &sender,
                     data_desc const & desc, my_alloc_hint * hint, int id) {
    xzl_bug_on(!buffer || !hint);

    /* send frame desc */
    ostringstream oss;
    boost::archive::text_oarchive oa(oss);

    oa << desc;
    string s = oss.str();

    /* desc msg. explicit copy content over */
    zmq::message_t dmsg(s.size());
    memcpy(dmsg.data(), s.c_str(), s.size());
    //zmq::message_t dmsg(s.begin(), s.end());
    sender.send(dmsg, ZMQ_SNDMORE); /* multipart msg */

    VV("desc sent");
    I("send_one_from_db");

    /* send the chunk */
    zmq::message_t cmsg(buffer, sz, my_free /* our deallocation func */, (void *) hint);
    //I("size = %d", sizeof(uint8_t *));
    auto ret = sender.send(cmsg, 0); /* no more msg */
    xzl_bug_on(!ret);

    VV("frame sent");

    return 0;
}
#if 0
int send_one_720_from_db(uint8_t * buffer, size_t sz, zmq::socket_t &sender,
                     data_desc const & desc, my_alloc_hint * hint) {
    xzl_bug_on(!buffer || !hint);

    /* send frame desc */
    ostringstream oss;
    boost::archive::text_oarchive oa(oss);

    oa << desc;
    string s = oss.str();

    /* desc msg. explicit copy content over */
    zmq::message_t dmsg(s.size());
    memcpy(dmsg.data(), s.c_str(), s.size());
    //zmq::message_t dmsg(s.begin(), s.end());
    //sender.send(dmsg, ZMQ_SNDMORE); /* multipart msg */

    VV("desc sent");
    I("send_one_from_db");

    /* send the chunk */
    zmq::message_t cmsg(buffer, sz, my_free /* our deallocation func */, (void *) hint);
    I("%d", sz);
    auto ret = sender.send(cmsg, 0); /* no more msg */
    xzl_bug_on(!ret);

    VV("frame sent");

    return 0;
}

int send_one_chunk_from_db(cv::Mat chunk[5], size_t sz, zmq::socket_t &sender,
                         data_desc const & desc, my_alloc_hint * hint, int id) {
    xzl_bug_on(!chunk || !hint);
    /* send frame desc */
    ostringstream oss;
    boost::archive::text_oarchive oa(oss);

    oa << desc;
    string s = oss.str();

    /* desc msg. explicit copy content over */
    zmq::message_t dmsg(s.size());
    memcpy(dmsg.data(), s.c_str(), s.size());
    //zmq::message_t dmsg(s.begin(), s.end());
    //sender.send(dmsg, ZMQ_SNDMORE); /* multipart msg */

    //VV("desc sent");
    I("send_one_chunk_from_db");

    /* send the chunk */
//    zmq::message_t cmsg(sz);
//    memcpy(cmsg.data(), &chunk, sz);
    for(int i = 0; i < CHUNK_SIZE; i++){
        if(i < CHUNK_SIZE - 1){
            I("%d, size = %d", i, sizeof(chunk[i]));
            zmq::message_t cmsg(&chunk[i], 1382400, my_free /* our deallocation func */, (void *) hint);
            auto ret = sender.send(cmsg, ZMQ_SNDMORE); /* no more msg */
            xzl_bug_on(!ret);
        }
        else{
            I("%d", i);
            zmq::message_t cmsg(&chunk[i], 1382400, my_free /* our deallocation func */, (void *) hint);
            auto ret = sender.send(cmsg, 0); /* no more msg */
            xzl_bug_on(!ret);
        }
    }

    I("frame sent size = %d", sz);

    return 0;
}
#endif
/* send the final desc with no frame, marking the end of a frame seq (e.g. a chunk?) */

void send_chunk_eof(cid_t const & cid, unsigned int chunk_seq, zmq::socket_t & sender) {
	data_desc desc(TYPE_CHUNK_EOF);

	desc.cid.stream_id = cid.stream_id; /* only stream id matters */
	desc.c_seq = chunk_seq;

	auto ret = send_one_frame(nullptr, 0, sender, desc);
	xzl_bug_on(ret != 0);
}

/* we still need @chunk_seq, even a dummy one (like 0) */
void send_raw_eof(cid_t const & cid, unsigned int chunk_seq, unsigned int frame_seq, zmq::socket_t & sender) {
	data_desc desc(TYPE_RAW_FRAME_EOF);

	desc.cid.stream_id = cid.stream_id; /* only stream id matters */
	/* chunk seq does not matter? */
	desc.c_seq = chunk_seq;
	desc.f_seq = frame_seq;

	auto ret = send_one_frame(nullptr, 0, sender, desc);
	xzl_bug_on(ret != 0);
}

void send_decoded_eof(cid_t const & cid, unsigned int chunk_seq, unsigned int frame_seq, zmq::socket_t & sender) {

	data_desc desc(TYPE_DECODED_FRAME_EOF);

	desc.cid.stream_id = cid.stream_id; /* only stream id matters */
	desc.c_seq = chunk_seq; /* inherit chunk seq */
	desc.f_seq = frame_seq;

	auto ret = send_one_frame(nullptr, 0, sender, desc);
	xzl_bug_on(ret != 0);
}

/* XXX: do something to the frame.
 * @sz: [out] the recv'd frame size, in bytes. 0 if there is no actual frame
 * @fdesc: [out]
 *
 * return: frame seq extracted from the desc.
 * */
unsigned recv_one_frame(zmq::socket_t & recv, size_t* sz, data_desc *fdesc) {

	//I("start to rx msgs...");

	data_desc desc;
	zmq::message_t dmsg;
    int rc;

	{
		/* recv the desc */
		recv.recv(&dmsg);
		//I("got desc msg. msg size =%lu", dmsg.size());

		std::string s((char const *)dmsg.data(), dmsg.size()); /* copy over */
		std::istringstream ss(s);
		boost::archive::text_iarchive ia(ss);

		ia >> desc;
		//I("%s", desc.to_string().c_str());
	}

	//if (dmsg.more())
    //teddyxu:check if we have more parts
    rc = zmq_getsockopt (recv, ZMQ_RCVMORE, &dmsg, sz);
    if(rc == 1)
    {	/* there's a frame for the desc. get it. */
		zmq::message_t cmsg;
		recv.recv(&cmsg);
		//I("got frame msg. size =%lu", cmsg.size());

		//xzl_bug_on_msg(cmsg.more(), "there should be no more");

		if (sz)
			*sz = cmsg.size();
	} else {
		if (sz)
			*sz = 0;
	}

	if (fdesc)
		*fdesc = desc;

	return desc.f_seq;
}

/* return a shared_ptr of the frame, which can be free'd later.
 * 	return nullptr if only desc but no frame is recv'd.
 * */
shared_ptr<zmq::message_t> recv_one_frame(zmq::socket_t & recv, data_desc *fdesc) {

	//I("start to rx msgs...");

	data_desc desc;
	zmq::message_t dmsg;

	shared_ptr<zmq::message_t> cmsg = nullptr;

    int rc;

	{
		/* recv the desc */
		recv.recv(&dmsg);
        //I("got desc msg. msg size =%lu", dmsg.size());

		std::string s((char const *)dmsg.data(), dmsg.size()); /* copy over */
		std::istringstream ss(s);
		boost::archive::text_iarchive ia(ss);

		ia >> desc;
		//I("type %s cid %lu c_seq %d f_seq %d",
		//	data_type_str[desc.type], desc.cid.as_uint, desc.c_seq, desc.f_seq);
	}

	//if (dmsg.more())
    //size_t sz = dmsg.size();
    //rc = zmq_getsockopt (recv, ZMQ_RCVMORE, &dmsg, &sz);
    //cout << "rc = " << rc << endl;
    //if(rc == 0)
    /*

    int more = 0;

	size_t more_size = sizeof (more);
	recv.getsockopt (ZMQ_RCVMORE, &more, &more_size);
    cout << "more = " << more << endl;
    */
    //if (dmsg.more())
    if(desc.type == TYPE_RAW_FRAME || desc.type == TYPE_CHUNK || desc.type == TYPE_DECODED_FRAME)
    {	/* there's a frame for the desc. get it. */
		cmsg = make_shared<zmq::message_t>();
		xzl_bug_on(!cmsg);

		auto ret = recv.recv(cmsg.get());
        //if(ret != 1) cout << "ret != 1" << endl;
        //cout << ret << endl;

		xzl_bug_on(!ret); /* EAGAIN? */
		//I("got chunk msg. size =%lu", cmsg->size());

		//xzl_bug_on_msg(cmsg->more(), "there should be no more");
	} else {
		xzl_bug_on(desc.type != TYPE_CHUNK_EOF && desc.type != TYPE_DECODED_FRAME_EOF && desc.type != TYPE_RAW_FRAME_EOF);
	}

	if (fdesc)
		*fdesc = desc;

	return cmsg;
}

/* return a shared_ptr of the frame, which can be free'd later.
 * 	return nullptr if only desc but no frame is recv'd.
 * */
shared_ptr<zmq::message_t> recv_one_frame_720(zmq::socket_t & recv) {

    I("start to rx msgs...");

    data_desc desc;
    zmq::message_t dmsg;

    shared_ptr<zmq::message_t> cmsg = nullptr;

    cmsg = make_shared<zmq::message_t>();
    xzl_bug_on(!cmsg);

    auto ret = recv.recv(cmsg.get());

    xzl_bug_on(!ret); /* EAGAIN? */
    I("got chunk msg. size =%lu", cmsg->size());

    return cmsg;
}

/* recv a desc msg and a chunk msg.
 *
 * for the chunk msg,
 * return a shared ptr of msg, so that we can access its data() later
 * [ there seems no safe way of moving out its data. ]
 *
 * if the desc indicates eof of a chunk seq, there will be no chunk data.
 * in that case, @desc will be filled but nullptr will be returned.
 *
 */
shared_ptr<zmq::message_t> recv_one_chunk(zmq::socket_t & s, data_desc *desc) {
//	zmq::context_t context(1 /* # io threads */);

	{
		/* recv the desc */
		zmq::message_t dmsg;
		s.recv(&dmsg);

		I("got desc msg. msg size =%lu", dmsg.size());

		string s((char const *)dmsg.data(), dmsg.size()); /* copy over */
		istringstream ss(s);
		boost::archive::text_iarchive ia(ss);

		ia >> *desc;
		I("desc->tid = %d", desc->tid);

		I("%s", desc->to_string().c_str());

		if (desc->type == TYPE_CHUNK_EOF) {
			//xzl_bug_on(dmsg.more());
			return nullptr;
		} else {
			xzl_bug_on(desc->type != TYPE_CHUNK); /* can't be anything else */
			//xzl_bug_on(!dmsg.more()); /* there must be data */
		}
	}

	{
		/* recv the chunk */
		auto cmsg = make_shared<zmq::message_t>();
		xzl_bug_on(!cmsg);
		auto ret = s.recv(cmsg.get());
		xzl_bug_on(!ret); /* EAGAIN? */
		I("got chunk msg. size =%lu", cmsg->size());

		//xzl_bug_on_msg(cmsg->more(), "multipart msg should end");

		return cmsg;
	}
}

/* recv a desc msg and a chunk msg.
 *
 * for the chunk msg,
 * return a shared ptr of msg, so that we can access its data() later
 * [ there seems no safe way of moving out its data. ]
 *
 * if the desc indicates eof of a chunk seq, there will be no chunk data.
 * in that case, @desc will be filled but nullptr will be returned.
 *
 */
shared_ptr<zmq::message_t> recv_one_chunk_720(zmq::socket_t & s) {
//	zmq::context_t context(1 /* # io threads */);
    I("start to rx msgs...");

    data_desc desc;
    zmq::message_t dmsg;

    shared_ptr<zmq::message_t> cmsg = nullptr;

    cmsg = make_shared<zmq::message_t>();
    xzl_bug_on(!cmsg);

    auto ret = s.recv(cmsg.get());

    xzl_bug_on(!ret); /* EAGAIN? */
    I("got chunk msg. size =%lu", cmsg->size());

    return cmsg;
}

/* recv a desc and a chunk from socket.
 *
 * this does memcpy since we assume encoded chunks won't be too large.
 * However, @recv_one_chunk() enables zero-copy if that's desirable.
 *
 * @p: [OUT] mem buffer from malloc(). to be free'd by the caller
 */
void recv_one_chunk_to_buf(zmq::socket_t &s, data_desc *desc,
													 char **p, size_t *sz) {

	auto cmsg = recv_one_chunk(s, desc);

	//k2_measure("chunk recv'd");

	if (cmsg) {
		char *buf = (char *) malloc(cmsg->size());
		xzl_bug_on(!buf);

		memcpy(buf, cmsg->data(), cmsg->size());

		*p = buf;
		*sz = cmsg->size();
		/* cmsg will be auto destroyed */
	} else { /* we only got a desc. no chunk data */
		*p = nullptr;
		*sz = 0;
	}
}

/* recv one chunk, and save it as a new file.
 *
 * return: 0 if the chunk is saved.
 * -1 if no chunk recv'd and this is just eof.
 *
 * XXX directly use msg.data(), instead of a copied buffer */
int recv_one_chunk_tofile(zmq::socket_t &s, data_desc *desc,
													const char *fname) {

	xzl_bug_on(!fname);

	char * buf = nullptr;
	size_t sz;
	recv_one_chunk_to_buf(s, desc, &buf, &sz);
	int rc;

	if (buf) {
		I("going to write to file. sz = %lu", sz);

		/* write the chunk to file */
		FILE *f = fopen(fname, "wb");
		xzl_bug_on_msg(!f, "failed to cr file");
		auto ret = fwrite(buf, 1, sz, f);
		if (ret != sz)
			perror("failed to write");

		xzl_bug_on(ret != sz);
		fclose(f);
		I("written chunk to file %s", fname);

		free(buf);
		rc = 0;

		//k2_measure("file written");

	} else {
		I("desc only. nothing to write to file");
		xzl_assert(sz == 0);
		rc = -1;
	}

	I("return %d", rc);
	//return rc;
	I("tid = %d",desc->tid);
	return desc->tid;
}
