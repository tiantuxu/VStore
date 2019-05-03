//
// Created by xzl on 12/24/17.
//
// tester: push a video clip to the server

// file xfer over zmq
// cf:
// http://zguide.zeromq.org/php:chapter7#toc22
//
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <sstream> // std::ostringstream

#include <zmq.h>
#include <zmq.hpp>

#include "mm.h"
#include "config.h"
#include "vs-types.h"
#include "mydecoder.h"
#include "log.h"
#include "measure.h"
#include "rxtx.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include "rt-config.h"

using namespace std;
using namespace vs;

atomic<int> flag(0);
//MDB_env *env;

void send_chunks(const char *fname, int k)
{
	/* send the file k times */
	for (int i = 0; i < k; i++) {

	}
}

/* send the entier file content as two msgs:
 * a chunk desc; a chunk */
void send_chunk_from_file(const char *fname, zmq::socket_t & sender)
{

	/* send desc */
	struct data_desc desc(TYPE_CHUNK);
	desc.cid.as_uint = 100; /* XXX more */

	ostringstream oss;
	boost::archive::text_oarchive oa(oss);

	oa << desc;
	string s = oss.str();

	/* desc msg. explicit copy content over */
	zmq::message_t dmsg(s.size());
	memcpy(dmsg.data(), s.c_str(), s.size());
	//zmq::message_t dmsg(s.begin(), s.end());
	//zmq::message_t dmsg(s.size());
	sender.send(dmsg, ZMQ_SNDMORE); /* multipart msg */

	I("desc sent");

	/* send chunk */
	uint8_t * buf;
	size_t sz;
	map_file(fname, &buf, &sz);

	auto hint = new my_alloc_hint(USE_MMAP, sz);
	zmq::message_t cmsg(buf, sz, my_free, hint);
	sender.send(cmsg, 0); /* no more msg */

	I("chunk sent");
}

/* load all the chunks in a key range from the db, and send them out as
 * msgs (desc + chunk for each)
 *
 * this can be used for sending either encoded chunks or raw frames.
 *
 */
void test_send_multi_from_db(const char *dbpath, zmq::socket_t & sender, int type, int f_start, int f_end)
//void test_send_multi_from_db(const char *dbpath, zmq::socket_t & sender, int type)
{
	int rc;
	MDB_env *env;
	MDB_dbi dbi;
	MDB_txn *txn;

	rc = mdb_env_create(&env);
	xzl_bug_on(rc != 0);

	rc = mdb_env_set_mapsize(env, 1UL * 1024UL * 1024UL * 1024UL); /* 1 GiB */
	xzl_bug_on(rc != 0);

	rc = mdb_env_open(env, dbpath, 0, 0664);
	xzl_bug_on(rc != 0);

	rc = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
	xzl_bug_on(rc != 0);

	rc = mdb_dbi_open(txn, NULL, MDB_INTEGERKEY, &dbi);
	xzl_bug_on(rc != 0);

	mdb_txn_commit(txn); /* done open the db */

	data_desc temp_desc(type);
	temp_desc.cid.stream_id = 1001;
	temp_desc.c_seq = 0;

//	data_desc temp_desc(type);

	//unsigned cnt = send_multi_from_db(env, dbi, 0, UINT64_MAX, sender, temp_desc);
    unsigned cnt = send_multi_from_db(env, dbi, f_start, f_end, sender, temp_desc);
//	unsigned cnt = send_multi_from_db(env, dbi, 0, 1000 * 1000, sender, temp_desc);

	/* -- wait for all outstanding? -- */
	sleep (6);

	EE("total %u loaded & sent", cnt);

	mdb_dbi_close(env, dbi);
	mdb_env_close(env);
}

void test_send_chunk_from_db(MDB_env *env, const char *dbpath, zmq::socket_t & sender, int type, int f_start, int f_end, int id)
{
    int rc;
    MDB_dbi dbi;
    MDB_txn *txn;

    rc = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
	I("%s",mdb_strerror(rc));
	if(rc != 0){
		I("rc = %d", rc);
	}
	xzl_bug_on(rc != 0);

    rc = mdb_dbi_open(txn, NULL, MDB_INTEGERKEY, &dbi);
    I("%s",mdb_strerror(rc));
    if(rc != 0){
        I("rc = %d", rc);
    }

    mdb_txn_commit(txn); /* done open the db */
    xzl_bug_on(rc != 0);

    data_desc temp_desc(type);
    temp_desc.cid.stream_id = 1001;
    temp_desc.c_seq = 0;

    //	data_desc temp_desc(type);

    //unsigned cnt = send_multi_from_db(env, dbi, 0, UINT64_MAX, sender, temp_desc);
    //unsigned cnt = send_chunk_from_db(env, dbi, 0, UINT64_MAX, sender, temp_desc, id);
    unsigned cnt = send_chunk_from_db(env, dbi, f_start, f_end, sender, temp_desc, id);
	I("id = %d", id);
	//unsigned cnt = send_chunk_from_db(env, dbi, 0, 20, sender, temp_desc);

    /* -- wait for all outstanding? -- */
    //sleep (2);

    EE("total %u loaded & sent", cnt);

    //mdb_dbi_close(env, dbi);
    //mdb_env_close(env);
}

void test_send_one_from_db(const char *dbpath, zmq::socket_t & sender, int type, int f_start, int f_end)
{

	int rc;

	MDB_env *env;
	MDB_dbi dbi;
	MDB_txn *txn;

	rc = mdb_env_create(&env);
	xzl_bug_on(rc != 0);

	rc = mdb_env_set_mapsize(env, 1UL * 1024UL * 1024UL * 1024UL); /* 1 GiB */
	xzl_bug_on(rc != 0);

	flag = 1;

	rc = mdb_env_set_maxdbs(env, 4);
    xzl_bug_on(rc != 0);

    mdb_env_set_maxreaders(env, 30);

	rc = mdb_env_open(env, dbpath, 0, 0664);
	xzl_bug_on(rc != 0);

	rc = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
	xzl_bug_on(rc != 0);

	rc = mdb_dbi_open(txn, NULL, MDB_INTEGERKEY, &dbi);
	xzl_bug_on(rc != 0);

	mdb_txn_commit(txn); /* done open the db */

	data_desc temp_desc(type);
	temp_desc.cid.stream_id = 1001;
	temp_desc.c_seq = 1;

//	data_desc temp_desc(type);

	unsigned cnt = send_multi_from_db(env, dbi, f_start, f_end, sender, temp_desc);
    EE("frame %u loaded & sent from db %s", f_start, dbpath);

    //std::atomic_thread_fence(std::memory_order_release);

    asm volatile("" ::: "memory");
	//free(env);

    //mdb_dbi_close(env, dbi);
	//mdb_env_close(env);
}

/* send raw frames (from a raw video file) over. */
void send_raw_frames_from_file(const char *fname, zmq::socket_t &s,
													 int height, int width, int yuv_mode,
													 data_desc const & fdesc_temp)
{
	size_t frame_sz = (size_t) width * (size_t) height;
	size_t frame_w;

	if (yuv_mode == 420)
		frame_w = ((frame_sz * 3) / 2);
	else if (yuv_mode == 422)
		frame_w = (frame_sz * 2);
	else if (yuv_mode == 444)
		frame_w = (frame_sz * 3);
	else
		xzl_bug("unsupported yuv");

	/* map file */
	uint8_t * buf = nullptr;
	size_t sz;
	map_file(fname, &buf, &sz);
	xzl_bug_on(!buf);
	auto n_frames = sz / frame_w;  /* # of frames we have */
	xzl_bug_on(n_frames == 0);

	/* all frames come from the same mmap'd file. they share the same @hint */
	auto h = new my_alloc_hint(USE_MMAP_REFCNT, sz, buf, n_frames); /* will be free'd when refcnt drops to 0 */
	for (auto i = 0u; i < n_frames; i++) {
		data_desc fdesc(fdesc_temp);
		fdesc.f_seq= i;
		send_one_frame_mmap(buf + i * frame_w, frame_w, s, fdesc, h);
	}

	I("total %lu raw frames from %s", n_frames, fname);
}

void test_send_raw_frames_from_file(const char *fname, zmq::socket_t &s_frame)
{
	data_desc fdesc(TYPE_RAW_FRAME);
	send_raw_frames_from_file(fname, s_frame, 320, 240, 420, fdesc);
}

static zmq::context_t context (1 /* # io threads */);

#if 0 /* tbd */
static void *serv_stream_info(void *t) {

	zmq::socket_t si_sender(context, ZMQ_REP);
	si_sender.setsockopt(ZMQ_SNDHWM, 1);
	si_sender.bind(STREAMINFO_PUSH_ADDR);

	EE("launched.");

	while (1) {
		zmq::message_t req;
		si_sender.recv(&req);

		send_all_stream_info(si_sender);
	}

	return nullptr;
}
#endif

void testing_send_one_from_db(MDB_env *env, const char *dbpath, zmq::socket_t & sender, int type, int f_start, int f_end)
{

    int rc;

    MDB_dbi dbi;
    MDB_txn *txn;

    rc = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
	I("%s",mdb_strerror(rc));
    if(rc != 0){
		I("rc = %d", rc);
	}
	xzl_bug_on(rc != 0);

    rc = mdb_dbi_open(txn, NULL, MDB_INTEGERKEY, &dbi);
    xzl_bug_on(rc != 0);

    rc =  mdb_txn_commit(txn); /* done open the db */
	xzl_bug_on(rc != 0);

    data_desc temp_desc(type);
    temp_desc.cid.stream_id = 1001;
    temp_desc.c_seq = 1;

    unsigned cnt = send_multi_from_db(env, dbi, f_start, f_end, sender, temp_desc);

    EE("%u frames loaded & sent from db %s", cnt, dbpath);
	//mdb_txn_abort(txn);
    mdb_dbi_close(env, dbi);
    //mdb_env_close(env);
}

/* argv[1] -- the video file name */
int main (int argc, char *argv[])
{

	zmq::socket_t sender(context, ZMQ_PUSH);
//	sender.connect(CLIENT_PUSH_TO_ADDR);
	sender.bind(CHUNK_PUSH_ADDR);

	zmq::socket_t fb_recv(context, ZMQ_PULL);
	fb_recv.bind(FB_PULL_ADDR);

	//I("bound to %s (fb %s). wait for workers to pull ...", CHUNK_PUSH_ADDR, FB_PULL_ADDR);

	zmq::socket_t s_frame(context, ZMQ_REP);
	s_frame.bind(REQUEST_PULL_ADDR);  /* push raw frames */

	//I("connect to %s. ready to push raw frames", WORKER_PUSH_TO_ADDR);

    zmq::socket_t frame_range(context, ZMQ_PUSH);
    frame_range.connect(WORKER_PUSH_TO_ADDR);  /* pull query requests */

    //I("connect to %s. ready to get new requests", WORKER_REQUEST);
	// The port setup for the raw case
	zmq::socket_t *worker_socket[50];
	for(int worker_count = 0; worker_count < THREAD_NUM; worker_count++){
		worker_socket[worker_count]= new zmq::socket_t(context, ZMQ_PUSH);
		worker_socket[worker_count]->connect(raw_push_address[worker_count].c_str());
	}

	zmq::socket_t *worker_socket3[50];
	for(int worker_count = 0; worker_count < THREAD_NUM; worker_count++){
		worker_socket3[worker_count]= new zmq::socket_t(context, ZMQ_PUSH);
		worker_socket3[worker_count]->connect(raw3_push_address[worker_count].c_str());
	}

//	pthread_t si_server;
//	int rc = pthread_create(&si_server, NULL, serv_stream_info, NULL);
//	xzl_bug_on(rc != 0);
#if 0
	for (int i = 0; i < 20; i++) {

		I("to send chunk %d/20...", i);
		send_chunk_from_file(argv[1], sender);

		/* check fb */
		I("to recv fb");
		feedback fb;
		bool ret = recv_one_fb(fb_recv, &fb, true /* blocking */);
		if (ret)
			I("got one fb. id %lu", fb.fid);
		else
			I("failed to get fb");
	}
#endif
#if 1
	std::string temp[3] = {"hot", "warm", "cold"};

	int rc;
    MDB_env *env[27];
	MDB_env *env_decode[9];
	MDB_env *env_decode_720_nvme[THREAD_NUM * 3];
	MDB_env *env_decode_720_ssd[THREAD_NUM * 3];
	MDB_env *env_decode_720_hdd[THREAD_NUM * 3];
	MDB_env *env_decode_540_nvme[THREAD_NUM * 3];
	MDB_env *env_decode_540_ssd[THREAD_NUM * 3];
	MDB_env *env_decode_540_hdd[THREAD_NUM * 3];

	//MDB_dbi dbi;
    //MDB_txn *txn;
	int i, j, k;
	string rpath_list[9] = {"/mnt/nvme/180-raw/", "/ssd/180-raw/", "/shared/videos/hdd/180-raw/",
							"/mnt/nvme/720-raw/", "/ssd/720-raw/", "/shared/videos/hdd/720-raw/",
							"/mnt/nvme/540-raw/", "/ssd/540-raw/", "/shared/videos/hdd/540-raw/"};

	for(i = 0; i < 9; i++){
		for (k = 0; k < 3; k ++) {
			rc = mdb_env_create(&env[3 * i + k]);
			xzl_bug_on(rc != 0);

			rc = mdb_env_set_mapsize(env[3 * i + k], 1UL * 1024UL * 1024UL * 1024UL); /* 1 GiB */
			xzl_bug_on(rc != 0);

			string path = rpath_list[i] + temp[k].c_str();
			//cout << path << endl;
			rc = mdb_env_open(env[3 * i + k], path.c_str(), 0, 0664);
			xzl_bug_on(rc != 0);
		}
	}

	string epath_list[6] = {"/mnt/nvme/720-chunk/", "/ssd/720-chunk/", "/shared/videos/hdd/720-chunk/",
							"/mnt/nvme/540-chunk/", "/ssd/540-chunk/", "/shared/videos/hdd/540-chunk/"};
#if 0
	for(j = 0; j < THREAD_NUM; j++){
		rc = mdb_env_create(&env_decode[j]);
		xzl_bug_on(rc != 0);

		rc = mdb_env_set_mapsize(env_decode[j], 1UL * 1024UL * 1024UL); /* 1 MiB */
		xzl_bug_on(rc != 0);

		rc = mdb_env_open(env_decode[j], DB_PATH_CHUNK_NVME_180, 0, 0664);
		xzl_bug_on(rc != 0);

	}
#endif

	string epath_180[3] = {"/mnt/nvme/180-chunk/", "/ssd/180-chunk/", "/shared/videos/hdd/180-chunk/"};

	for(j = 0; j < TIER_NUM; j++){
		for (k = 0; k < 3; k ++) {
			rc = mdb_env_create(&env_decode[3 * j + k]);
			xzl_bug_on(rc != 0);

			rc = mdb_env_set_mapsize(env_decode[3 * j + k], 1UL * 1024UL * 1024UL); /* 32 MiB */
			xzl_bug_on(rc != 0);

			string path = epath_180[j] + temp[k].c_str();
			//cout << path << endl;
			rc = mdb_env_open(env_decode[3 * j + k], path.c_str(), 0, 0664);
			xzl_bug_on(rc != 0);
		}
	}

	for(j = Q_TEMP; j < Q_TEMP+1; j++){
		for (k = 0; k < THREAD_NUM; k ++) {
			rc = mdb_env_create(&env_decode_720_nvme[k]);
			xzl_bug_on(rc != 0);

			rc = mdb_env_set_mapsize(env_decode_720_nvme[k], 1UL * 1024UL * 1024UL); /* 32 MiB */
			xzl_bug_on(rc != 0);

			string path = epath_list[0] + temp[j].c_str();
			//cout << path << endl;
			rc = mdb_env_open(env_decode_720_nvme[k], path.c_str(), 0, 0664);
			xzl_bug_on(rc != 0);
		}
	}

	for(j = Q_TEMP; j < Q_TEMP+1; j++){
		for (k = 0; k < THREAD_NUM; k ++) {
			rc = mdb_env_create(&env_decode_720_ssd[k]);
			xzl_bug_on(rc != 0);

			rc = mdb_env_set_mapsize(env_decode_720_ssd[k], 1UL * 1024UL * 1024UL); /* 32 MiB */
			xzl_bug_on(rc != 0);

			string path = epath_list[1] + temp[j].c_str();
			//cout << path << endl;
			rc = mdb_env_open(env_decode_720_ssd[k], path.c_str(), 0, 0664);
			xzl_bug_on(rc != 0);
		}
	}

	for(j = Q_TEMP; j < Q_TEMP+1; j++){
		for (k = 0; k < THREAD_NUM; k ++) {
			rc = mdb_env_create(&env_decode_720_hdd[k]);
			xzl_bug_on(rc != 0);

			rc = mdb_env_set_mapsize(env_decode_720_hdd[k], 1UL * 1024UL * 1024UL); /* 32 MiB */
			xzl_bug_on(rc != 0);

			string path = epath_list[2] + temp[j].c_str();
			//cout << path << endl;
			rc = mdb_env_open(env_decode_720_hdd[k], path.c_str(), 0, 0664);
			I("%s",mdb_strerror(rc));
			if(rc != 0){
				I("rc = %d", rc);
			}
			xzl_bug_on(rc != 0);
		}
	}

	for(j = Q_TEMP; j < Q_TEMP+1; j++){
		for (k = 0; k < THREAD_NUM; k ++) {
			rc = mdb_env_create(&env_decode_540_nvme[k]);
			xzl_bug_on(rc != 0);

			rc = mdb_env_set_mapsize(env_decode_540_nvme[k], 1UL * 1024UL * 1024UL * 32UL); /* 32 MiB */
			xzl_bug_on(rc != 0);

			string path = epath_list[3] + temp[j].c_str();
			//cout << path << endl;
			rc = mdb_env_open(env_decode_540_nvme[k], path.c_str(), 0, 0664);
			xzl_bug_on(rc != 0);
		}
	}

	for(j = Q_TEMP; j < Q_TEMP+1; j++){
		for (k = 0; k < THREAD_NUM; k ++) {
			rc = mdb_env_create(&env_decode_540_ssd[k]);
			xzl_bug_on(rc != 0);

			rc = mdb_env_set_mapsize(env_decode_540_ssd[k], 1UL * 1024UL * 1024UL * 32UL); /* 32 MiB */
			xzl_bug_on(rc != 0);

			string path = epath_list[4] + temp[j].c_str();
			//cout << path << endl;
			rc = mdb_env_open(env_decode_540_ssd[k], path.c_str(), 0, 0664);
			xzl_bug_on(rc != 0);
		}
	}

	for(j = Q_TEMP; j < Q_TEMP+1; j++){
		for (k = 0; k < THREAD_NUM; k ++) {
			rc = mdb_env_create(&env_decode_540_hdd[k]);
			xzl_bug_on(rc != 0);

			rc = mdb_env_set_mapsize(env_decode_540_hdd[k], 1UL * 1024UL * 1024UL * 32UL); /* 32 MiB */
			xzl_bug_on(rc != 0);

			string path = epath_list[5] + temp[j].c_str();
			//cout << path << endl;
			rc = mdb_env_open(env_decode_540_hdd[k], path.c_str(), 0, 0664);
			xzl_bug_on(rc != 0);
		}
	}

#endif

    while(1){
        request_desc *rd;
		vs::sv *s;

        string db_path;
        string dbpath;

        zmq::message_t s_request(sizeof(vs::sv));
        zmq::message_t reply(5);

        I("bound to %s. ready to pull new requests", REQUEST_PULL_ADDR);

        s_frame.recv(&s_request);
		// Send feedback back to the scheduler
		memcpy (reply.data (), "reply", 5);
		s_frame.send(reply);

		s = static_cast<vs::sv *>(s_request.data());
		rd = &s->rq;

        I("got desc msg. type = %d, db_seq = %d. start = %d frame_num = %d thread_id = %d", s->type, s->rq.db_seq, s->rq.start_fnum, s->rq.total_fnum, s->rq.id);

        switch (s->rq.db_seq) {
            case NVME_RAW_180:
                EE("ready to push raw frames. DB = NVME_RAW_180");
                //memcpy (reply.data (), "World", 5);
                //s_frame.send(reply);
                db_path = DB_RAW_FRAME_PATH_NVME_180;
				dbpath = db_path + temp[rd->temp].c_str();
				cout << dbpath << endl;
				EE("start=%d, end=%d", 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 2 * rd->total_fnum);
				//test_send_multi_from_db(dbpath.c_str(), frame_range, TYPE_RAW_FRAME, 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 2 * rd->total_fnum);
				testing_send_one_from_db(env[rd->temp], dbpath.c_str(), *worker_socket[rd->id], TYPE_RAW_FRAME, 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 2 * rd->total_fnum + 1);
				break;
            case NVME_RAW_720:
                EE("ready to push raw frames. DB = NVME_RAW_720");
                db_path = DB_RAW_FRAME_PATH_NVME_720;
				dbpath = db_path + temp[rd->temp].c_str();
				cout << dbpath << endl;
				testing_send_one_from_db(env[9 + rd->temp], dbpath.c_str(), *worker_socket[rd->id], TYPE_RAW_FRAME, 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 2 * rd->total_fnum + 1);
                //test_send_multi_from_db(DB_RAW_FRAME_PATH, s_frame, TYPE_RAW_FRAME);
                //testing_send_one_from_db(env[4], DB_RAW_FRAME_PATH, s_frame, TYPE_RAW_FRAME, 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 2 * rd->total_fnum + 1);
                break;
            case NVME_RAW_540:
                EE("ready to push raw frames. DB = NVME_RAW_540");
                db_path = DB_RAW_FRAME_PATH_NVME_540;
				dbpath = db_path + temp[rd->temp].c_str();
				cout << dbpath << endl;
				testing_send_one_from_db(env[18 + rd->temp], dbpath.c_str(), *worker_socket3[rd->id], TYPE_RAW_FRAME, 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 2 * rd->total_fnum + 1);
                //test_send_multi_from_db(DB_RAW_FRAME_PATH, s_frame, TYPE_RAW_FRAME);
                //testing_send_one_from_db(env[4], DB_RAW_FRAME_PATH, s_frame, TYPE_RAW_FRAME, 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 2 * rd->total_fnum + 1);
                break;
            case SSD_RAW_180:
                EE("ready to push raw frames. DB = SSD_RAW_180");
				EE("start=%d, end=%d", 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 2 * rd->total_fnum);
				//memcpy (reply.data (), "World", 5);
                //s_frame.send(reply);
                db_path = DB_RAW_FRAME_PATH_SSD_180;
				dbpath = db_path + temp[rd->temp].c_str();
				cout << dbpath << endl;
                //test_send_multi_from_db(dbpath.c_str(), frame_range, TYPE_RAW_FRAME, 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 2 * rd->total_fnum);
				testing_send_one_from_db(env[3 + rd->temp], dbpath.c_str(), *worker_socket[rd->id], TYPE_RAW_FRAME, 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 2 * rd->total_fnum + 1);
				break;
            case SSD_RAW_720:
                EE("ready to push raw frames. DB = SSD_RAW_720");
                //test_send_multi_from_db(DB_PATH, sender, TYPE_CHUNK);
                db_path = DB_RAW_FRAME_PATH_SSD_720;
				dbpath = db_path + temp[rd->temp].c_str();
                testing_send_one_from_db(env[12 + rd->temp], dbpath.c_str(), *worker_socket[rd->id], TYPE_RAW_FRAME, 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 2 * rd->total_fnum + 1);
                I("send frames returned");
                break;
            case SSD_RAW_540:
                EE("ready to push raw frames. DB = SSD_RAW_540");
                //	test_send_multi_from_db(DB_PATH, sender, TYPE_CHUNK);
                //test_send_one_from_db(DB_RAW_FRAME_PATH_SSD_540, s_frame, TYPE_RAW_FRAME, 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 3);
                db_path = DB_RAW_FRAME_PATH_SSD_540;
				dbpath = db_path + temp[rd->temp].c_str();
				testing_send_one_from_db(env[21 + rd->temp], dbpath.c_str(), *worker_socket3[rd->id], TYPE_RAW_FRAME, 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 2 * rd->total_fnum + 1);
				break;
            case HDD_RAW_180:
				EE("ready to push raw frames. DB = HDD_RAW_180");
				//memcpy (reply.data (), "World", 5);
				//s_frame.send(reply);
                db_path = DB_RAW_FRAME_PATH_180;
				dbpath = db_path + temp[rd->temp].c_str();
				cout << dbpath << endl;
                //test_send_multi_from_db(dbpath.c_str(), frame_range, TYPE_RAW_FRAME, 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 2 * rd->total_fnum);
				testing_send_one_from_db(env[6 + rd->temp], dbpath.c_str(), *worker_socket[rd->id], TYPE_RAW_FRAME, 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 2 * rd->total_fnum + 1);
				break;
            case HDD_RAW_720:
				EE("ready to push raw frames. DB = HDD_RAW_720");
                //	test_send_multi_from_db(DB_PATH, sender, TYPE_CHUNK);
                //test_send_one_from_db(DB_RAW_FRAME_PATH_SSD_540, s_frame, TYPE_RAW_FRAME, 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 3);
                db_path = DB_RAW_FRAME_PATH_720;
				dbpath = db_path + temp[rd->temp].c_str();
                testing_send_one_from_db(env[15 + rd->temp], dbpath.c_str(), *worker_socket[rd->id], TYPE_RAW_FRAME, 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 2 * rd->total_fnum + 1);
                break;
            case HDD_RAW_540:
				EE("ready to push raw frames. DB = HDD_RAW_540");
                //	test_send_multi_from_db(DB_PATH, sender, TYPE_CHUNK);
                //test_send_one_from_db(DB_RAW_FRAME_PATH_SSD_540, s_frame, TYPE_RAW_FRAME, 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 3);
                db_path = DB_RAW_FRAME_PATH_540;
				dbpath = db_path + temp[rd->temp].c_str();
                testing_send_one_from_db(env[24 + rd->temp], dbpath.c_str(), *worker_socket3[rd->id], TYPE_RAW_FRAME, 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 2 * rd->total_fnum + 1);
                break;
            case NVME_CHUNK_180:
				EE("ready to push encoded frames. DB = NVME_CHUNK_180");
                //memcpy (reply.data (), "World", 5);
                //s_frame.send(reply);
                db_path = DB_PATH_CHUNK_NVME_180;
				dbpath = db_path + temp[rd->temp].c_str();
				EE("start = %d, end = %d", 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 2 * rd->total_fnum);
				test_send_chunk_from_db(env_decode[rd->temp], dbpath.c_str(), sender, TYPE_CHUNK, 2 * int(rd->start_fnum/CHUNK_LENGTH) + 2, 2 * int(rd->start_fnum/CHUNK_LENGTH) + 3, rd->id);
                break;
            case NVME_CHUNK_720:
				EE("ready to push encoded frames. DB = NVME_CHUNK_720");
                //memcpy (reply.data (), "World", 5);
                //s_frame.send(reply);
                db_path = DB_PATH_CHUNK_NVME_720;
				dbpath = db_path + temp[rd->temp].c_str();
				test_send_chunk_from_db(env_decode_720_nvme[rd->id], dbpath.c_str(), sender, TYPE_CHUNK, 2 * int(rd->start_fnum/CHUNK_LENGTH) + 2, 2 * int(rd->start_fnum/CHUNK_LENGTH) + 3, rd->id);
                break;
			case NVME_CHUNK_540:
				EE("ready to push encoded frames. DB = NVME_CHUNK_540");
                //memcpy (reply.data (), "World", 5);
                //s_frame.send(reply);
                I("Request for frames in chunk %d", 2 * int(rd->start_fnum/CHUNK_LENGTH));
                I("tid = %d", rd->id);
                db_path = DB_PATH_CHUNK_NVME_540;
				dbpath = db_path + temp[rd->temp].c_str();
                //testing_send_one_from_db(env[3], DB_PATH_CHUNK_SSD_540, sender, TYPE_CHUNK, 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 2 * rd->total_fnum + 1);
				test_send_chunk_from_db(env_decode_540_nvme[rd->id], dbpath.c_str(), sender, TYPE_CHUNK, 2 * int(rd->start_fnum/CHUNK_LENGTH) + 2, 2 * int(rd->start_fnum/CHUNK_LENGTH) + 3, rd->id);
                break;
            case SSD_CHUNK_180:
				EE("ready to push encoded frames. DB = SSD_CHUNK_180");
                //memcpy (reply.data (), "World", 5);
                //s_frame.send(reply);
                db_path = DB_PATH_CHUNK_SSD_180;
				dbpath = db_path + temp[rd->temp].c_str();
				cout << dbpath << endl;
                test_send_chunk_from_db(env_decode[3 + rd->temp], dbpath.c_str(), sender, TYPE_CHUNK, 2 * int(rd->start_fnum/CHUNK_LENGTH) + 2, 2 * int(rd->start_fnum/CHUNK_LENGTH) + 3, rd->id);
                break;
            case SSD_CHUNK_720:
                EE("ready to push raw frames. DB = SSD_CHUNK_720");
                //memcpy (reply.data (), "World", 5);
                //s_frame.send(reply);
                db_path = DB_PATH_CHUNK_SSD_720;
				dbpath = db_path + temp[rd->temp].c_str();
				I("start = %d, end = %d", 2 * int(rd->start_fnum/CHUNK_LENGTH), 2 * int(rd->start_fnum/CHUNK_LENGTH) + 1);
                test_send_chunk_from_db(env_decode_720_ssd[rd->id], dbpath.c_str(), sender, TYPE_CHUNK, 2 * int(rd->start_fnum/CHUNK_LENGTH) + 2, 2 * int(rd->start_fnum/CHUNK_LENGTH) + 3, rd->id);
                break;
            case SSD_CHUNK_540:
                EE("ready to push raw frames. DB = SSD_CHUNK_540");
                //memcpy (reply.data (), "World", 5);
                //s_frame.send(reply);
                I("Request for frames in chunk %d", 2 * int(rd->start_fnum/CHUNK_LENGTH));
                I("tid = %d", rd->id);
                db_path = DB_PATH_CHUNK_SSD_540;
				dbpath = db_path + temp[rd->temp].c_str();
                //testing_send_one_from_db(env[3], DB_PATH_CHUNK_SSD_540, sender, TYPE_CHUNK, 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 2 * rd->total_fnum + 1);
                test_send_chunk_from_db(env_decode_540_ssd[rd->id], dbpath.c_str(), sender, TYPE_CHUNK, 2 * int(rd->start_fnum/CHUNK_LENGTH) + 2, 2 * int(rd->start_fnum/CHUNK_LENGTH) + 3, rd->id);
                break;
            case HDD_CHUNK_180:
				EE("ready to push encoded frames. DB = HDD_CHUNK_180");
                //memcpy (reply.data (), "World", 5);
                //s_frame.send(reply);
                db_path = DB_PATH_CHUNK_HDD_180;
				dbpath = db_path + temp[rd->temp].c_str();
				cout << dbpath << endl;
                test_send_chunk_from_db(env_decode[6 + rd->temp], dbpath.c_str(), sender, TYPE_CHUNK, 2 * int(rd->start_fnum/CHUNK_LENGTH) + 2, 2 * int(rd->start_fnum/CHUNK_LENGTH) + 3, rd->id);
                break;
            case HDD_CHUNK_720:
                EE("ready to push raw frames. DB = HDD_CHUNK_720");
                //memcpy (reply.data (), "World", 5);
                //s_frame.send(reply);
                db_path = DB_PATH_CHUNK_HDD_720;
				dbpath = db_path + temp[rd->temp].c_str();
                test_send_chunk_from_db(env_decode_720_hdd[rd->id], dbpath.c_str(), sender, TYPE_CHUNK, 2 * int(rd->start_fnum/CHUNK_LENGTH) + 2, 2 * int(rd->start_fnum/CHUNK_LENGTH) + 3, rd->id);
                break;
            case HDD_CHUNK_540:
                EE("ready to push raw frames. DB = HDD_CHUNK_540");
                //memcpy (reply.data (), "World", 5);
                //s_frame.send(reply);
                I("Request for frames in chunk %d", 2 * int(rd->start_fnum/CHUNK_LENGTH));
                I("tid = %d", rd->id);
                db_path = DB_PATH_CHUNK_HDD_540;
				dbpath = db_path + temp[rd->temp].c_str();
                //testing_send_one_from_db(env[3], DB_PATH_CHUNK_SSD_540, sender, TYPE_CHUNK, 2 * rd->start_fnum + 2, 2 * rd->start_fnum + 2 * rd->total_fnum + 1);
                test_send_chunk_from_db(env_decode_540_hdd[rd->id], dbpath.c_str(), sender, TYPE_CHUNK, 2 * int(rd->start_fnum/CHUNK_LENGTH) + 2, 2 * int(rd->start_fnum/CHUNK_LENGTH) + 3, rd->id);
                break;
            default:
                break;
        }
    }

	//	rc = pthread_join(si_server, nullptr); /* will never join.... XXX */
	//	xzl_bug_on(rc != 0);
	return 0;
}