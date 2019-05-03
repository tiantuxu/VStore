//
// Created by teddyxu on 8/3/18.
//

#include <sstream> // std::ostringstream

#include <zmq.hpp>
#include <cstdio>
#include <sstream>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <unistd.h>
#include "config.h"
#include "vs-types.h"
#include "mydecoder.h"
#include "log.h"
#include "rxtx.h"
#include "RxManager.h"
#include "measure.h"
#include <stdlib.h>

#include "StatCollector.h"
#include "CallBackTimer.h"
#include <opencv2/opencv.hpp>

#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/operations.hpp"
#include "opencv2/core/core.hpp"

#include "tclap/CmdLine.h"
#include <queue>
#include "rt-config.h"


using namespace std;
using namespace vs;

static zmq::context_t context (1 /* # io threads */);

/* load the given file contents to mem
 * @p: allocated by this func, to be free'd by caller */

int load_file(const char *fname, char **p, size_t *sz)
{
    /* get file sz */
    I("load file %s...", fname);

    struct stat finfo;
    int fd = open(fname, O_RDONLY);
    I("fd = %d", fd);
    xzl_bug_on(fd < 0);
    int ret = fstat(fd, &finfo);
    xzl_bug_on(ret != 0);

    char *buff = (char *)malloc(finfo.st_size);
    xzl_bug_on(!buff);

    auto s = pread(fd, buff, finfo.st_size, 0);
    xzl_bug_on(s != finfo.st_size);

    *p = buff;
    *sz = s;
    return fd;
}

int getTier(int count, int temp, int type, int stage){
    // type 0 means old, type 1 means new
    int tierNum = 0;
    int base = stage * 9 + temp * 3;
    float percent = 0.0;

    switch (type){
        case 0:
            // Source
            for(int i = 0; i < 3; i++){
                percent += stagePer[base + i];
                if((count+1) * CHUNK_LENGTH > percent * TEMP_NUM[temp]){
                    tierNum ++;
                }
                else{
                    return base + tierNum;
                }
            }
            break;
        case 1:
            // Destination
            for(int i = 0; i < 3; i++){
                percent += stagePerNew[base + i];
                if((count+1) * CHUNK_LENGTH > percent * TEMP_NUM[temp]){
                    tierNum ++;
                }
                else{
                    return base + tierNum;
                }
            }
            break;
        default:
            break;
    }
}


void videoMove(vs::sv migrationService){

    int rc;
    MDB_env *envSource, *envDest;
    MDB_dbi dbiSource, dbiDest;
    MDB_val keySource, dataSource, keyDest, dataDest;
    MDB_txn *txnSource, *txnDest;
    MDB_stat mstSource, mstDest;
    MDB_cursor *cursorSource, *cursorDest;

    // Path construction:  6 *(stage - 1) + 3
    string sourceDBPath = db_address[6*migrationService.mrq.stage + 2*(migrationService.mrq.source%3) + getOffset(stage[migrationService.mrq.source])];
    string destDBPath = db_address[6*migrationService.mrq.stage + 2*(migrationService.mrq.destination%3) + getOffset(stageNew[migrationService.mrq.destination])];

    string sourcePath = sourceDBPath + temp[migrationService.mrq.temperature];
    string destPath = destDBPath + temp[migrationService.mrq.temperature];

    rc = mdb_env_create(&envSource);
    xzl_bug_on(rc != 0);
    rc = mdb_env_create(&envDest);
    xzl_bug_on(rc != 0);

    rc = mdb_env_set_mapsize(envSource, 1UL * 1024UL * 1024UL * 1024UL * 1024UL); /* 1 GiB */
    xzl_bug_on(rc != 0);
    rc = mdb_env_set_mapsize(envDest, 1UL * 1024UL * 1024UL * 1024UL * 1024UL); /* 1 GiB */
    xzl_bug_on(rc != 0);

    rc = mdb_env_set_maxdbs(envSource, 1); /* required for named db */
    xzl_bug_on(rc != 0);
    rc = mdb_env_set_maxdbs(envDest, 1); /* required for named db */
    xzl_bug_on(rc != 0);

    rc = mdb_env_open(envSource, sourcePath.c_str(), 0, 0664);
    xzl_bug_on(rc != 0);
    rc = mdb_env_open(envDest, destPath.c_str(), 0, 0664);
    xzl_bug_on(rc != 0);

    rc = mdb_txn_begin(envSource, NULL, 0, &txnSource);
    xzl_bug_on(rc != 0);
    rc = mdb_txn_begin(envDest, NULL, 0, &txnDest);
    xzl_bug_on(rc != 0);

    rc = mdb_dbi_open(txnSource, NULL, MDB_INTEGERKEY | MDB_CREATE, &dbiSource);
    xzl_bug_on(rc != 0);
    rc = mdb_dbi_open(txnDest, NULL, MDB_INTEGERKEY | MDB_CREATE, &dbiDest);
    xzl_bug_on(rc != 0);

    int fps = 30;
    vs::cid_t lastest_keySource;
    vs::cid_t last_key;

    EE("going to move chunk %d from db path %s to db path %s", migrationService.mrq.chunkNum, sourcePath.c_str(), destPath.c_str());

    switch (stage[9 * migrationService.mrq.stage + 3 * migrationService.mrq.temperature + (migrationService.mrq.source%3)]){
        case 0:
            // Open the cursor of the source database
            rc = mdb_cursor_open(txnSource, dbiSource, &cursorSource);
            xzl_bug_on(rc != 0);
            rc = mdb_cursor_get(cursorSource, &keySource, &dataSource, MDB_FIRST);
            xzl_bug_on(rc != 0);

            if (rc == MDB_NOTFOUND)
                lastest_keySource = 0;

            // Open the cursor of the destinaton database
            rc = mdb_cursor_open(txnDest, dbiDest, &cursorDest);
            xzl_bug_on(rc != 0);
            rc = mdb_cursor_get(cursorDest, &keyDest, &dataDest, MDB_LAST);
            xzl_bug_on(rc != 0);
            // Get last key of destination database
            if (rc == MDB_NOTFOUND)
                last_key = 0;
            else {
                xzl_bug_on(rc != 0);
                last_key = *(vs::cid_t *) keyDest.mv_data;
            }

            // key values for source and destination
            last_key.as_uint += 1;
            lastest_keySource.as_uint = migrationService.mrq.chunkNum;

            // The frame to be retrieved as specified
            keySource.mv_data = &lastest_keySource;
            keySource.mv_size = sizeof(lastest_keySource);

            // Get the specific frame from the source
            rc = mdb_get(txnSource, dbiSource, &keySource, &dataSource);
            xzl_bug_on(rc != 0);

            // The frame to be stored should be appended to the end of the destination database
            keyDest.mv_data = &last_key;
            keyDest.mv_size = sizeof(last_key);

            // Put the frame to the destination database
            rc = mdb_cursor_put(cursorDest, &keyDest, &dataSource, MDB_NOOVERWRITE);
            xzl_bug_on(rc != 0);

            I("lmdb moved frame %d to %d in chunk %d, from source tier %d to tier %d",
              CHUNK_LENGTH * migrationService.mrq.chunkNum, CHUNK_LENGTH * (migrationService.mrq.chunkNum + 1),
              migrationService.mrq.chunkNum, migrationService.mrq.source, migrationService.mrq.destination);

            // TODO:Remember to delete the key from source (done)
            rc = mdb_del(txnSource, dbiSource, &keySource, &dataSource);
            xzl_bug_on(rc != 0);

            break;

        case 1:
            // Open the cursor of the source database
            rc = mdb_cursor_open(txnSource, dbiSource, &cursorSource);
            xzl_bug_on(rc != 0);
            rc = mdb_cursor_get(cursorSource, &keySource, &dataSource, MDB_FIRST);
            xzl_bug_on(rc != 0);

            if (rc == MDB_NOTFOUND)
                lastest_keySource = 0;
            else {
                xzl_bug_on(rc != 0);
                lastest_keySource = *(vs::cid_t *) keySource.mv_data;
            }

            //int latest = 2 * CHUNK_LENGTH * migrationService.mrq.chunkNum;
            //lastest_keySource.as_uint = (uint64_t)latest;
            //I("%" PRId64, lastest_keySource.as_uint);

            // Open the cursor of the destinaton database
            rc = mdb_cursor_open(txnDest, dbiDest, &cursorDest);
            xzl_bug_on(rc != 0);
            rc = mdb_cursor_get(cursorDest, &keyDest, &dataDest, MDB_LAST);
            xzl_bug_on(rc != 0);

            if (rc == MDB_NOTFOUND)
                last_key = 0;
            else {
                xzl_bug_on(rc != 0);
                last_key = *(vs::cid_t *) keyDest.mv_data;
            }

            for (int f = 0; f < CHUNK_LENGTH; f++){
                last_key.as_uint += (60/fps);
                //lastest_keySource.as_uint += (60/fps);

                // The frame to be retrieved as specified
                keySource.mv_data = &lastest_keySource;
                keySource.mv_size = sizeof(lastest_keySource);

                // Get the specific frame from the source
                rc = mdb_get(txnSource, dbiSource, &keySource, &dataSource);
                xzl_bug_on(rc != 0);

                I("loaded one k/v. key: %lu, sz %zu data sz %zu",
                  *(uint64_t *)keySource.mv_data, keySource.mv_size,
                  dataSource.mv_size);

                // The frame to be stored should be appended to the end of the destination database
                keyDest.mv_data = &last_key;
                keyDest.mv_size = sizeof(last_key);

                dataDest.mv_data = dataSource.mv_data;
                dataDest.mv_size = dataSource.mv_size;

                //I("going to store one k/v. key: %lu, sz %zu data sz %zu",
                //  *(uint64_t *)keyDest.mv_data, keyDest.mv_size,
                //  dataDest.mv_size);

                //cout << last_key.as_uint << endl;

                // Put the frame to the destination database
                rc = mdb_put(txnDest, dbiDest, &keyDest, &dataDest, MDB_NOOVERWRITE);
                xzl_bug_on(rc != 0);

                // TODO: Remember to delete the key from source (done)
                //rc = mdb_del(txnSource, dbiSource, &keySource, &dataSource);
                //xzl_bug_on(rc != 0);

                rc = mdb_cursor_del(cursorSource, 0);
                xzl_bug_on(rc != 0);

                lastest_keySource.as_uint += (60/fps);
                if(f != CHUNK_LENGTH -1){
                    rc = mdb_cursor_get(cursorSource, &keySource, &dataSource, MDB_NEXT);
                    xzl_bug_on(rc != 0);
                }
            }

            I("lmdb moved frame %d to %d in chunk %d, from source tier %d to tier %d",
              CHUNK_LENGTH * migrationService.mrq.chunkNum, CHUNK_LENGTH * (migrationService.mrq.chunkNum + 1),
              migrationService.mrq.chunkNum, migrationService.mrq.source, migrationService.mrq.destination);

            break;
        default:
            break;
    }

    rc = mdb_txn_commit(txnSource);
    xzl_bug_on(rc != 0);
    rc = mdb_env_stat(envSource, &mstSource);
    xzl_bug_on(rc != 0);

    rc = mdb_txn_commit(txnDest);
    xzl_bug_on(rc != 0);
    rc = mdb_env_stat(envSource, &mstSource);
    xzl_bug_on(rc != 0);

    mdb_dbi_close(envSource, dbiSource);
    mdb_env_close(envSource);

    mdb_dbi_close(envDest, dbiDest);
    mdb_env_close(envDest);
}

void videoDecodeMove(vs::sv migrationService, zmq::socket_t &chunk_push, zmq::socket_t &frame_pull){
    // This function deals witht the situation that the migrating chunk will first be decoded
    int rc;
    MDB_env *envSource, *envDest;
    MDB_dbi dbiSource, dbiDest;
    MDB_val keySource, dataSource, keyDest, dataDest;
    MDB_txn *txnSource, *txnDest;
    MDB_stat mstSource, mstDest;
    MDB_cursor *cursorSource, *cursorDest;

    // Path construction: (stage - 1) * 3
    string sourceDBPath = db_address[6*migrationService.mrq.stage + 2*(migrationService.mrq.source%3) + getOffset(stage[migrationService.mrq.source])];
    string destDBPath = db_address[6*migrationService.mrq.stage + 2*(migrationService.mrq.destination%3) + getOffset(stageNew[migrationService.mrq.destination])];

    string sourcePath = sourceDBPath + temp[migrationService.mrq.temperature];
    string destPath = destDBPath + temp[migrationService.mrq.temperature];

    // Prepare the environments for movements
    rc = mdb_env_create(&envSource);
    xzl_bug_on(rc != 0);
    rc = mdb_env_create(&envDest);
    xzl_bug_on(rc != 0);

    rc = mdb_env_set_mapsize(envSource, 1UL * 1024UL * 1024UL * 1024UL * 1024UL); /* 1 GiB */
    xzl_bug_on(rc != 0);
    rc = mdb_env_set_mapsize(envDest, 1UL * 1024UL * 1024UL * 1024UL * 1024UL); /* 1 GiB */
    xzl_bug_on(rc != 0);

    rc = mdb_env_set_maxdbs(envSource, 1); /* required for named db */
    xzl_bug_on(rc != 0);
    rc = mdb_env_set_maxdbs(envDest, 1); /* required for named db */
    xzl_bug_on(rc != 0);

    rc = mdb_env_open(envSource, sourcePath.c_str(), 0, 0664);
    xzl_bug_on(rc != 0);
    rc = mdb_env_open(envDest, destPath.c_str(), 0, 0664);
    xzl_bug_on(rc != 0);
    EE("going to move from db path %s to db path %s", sourcePath.c_str(), destPath.c_str());

    rc = mdb_txn_begin(envSource, NULL, 0, &txnSource);
    xzl_bug_on(rc != 0);
    rc = mdb_txn_begin(envDest, NULL, 0, &txnDest);
    xzl_bug_on(rc != 0);

    rc = mdb_dbi_open(txnSource, NULL, MDB_INTEGERKEY | MDB_CREATE, &dbiSource);
    xzl_bug_on(rc != 0);
    rc = mdb_dbi_open(txnDest, NULL, MDB_INTEGERKEY | MDB_CREATE, &dbiDest);
    xzl_bug_on(rc != 0);

    int fps = 30;
    vs::cid_t lastest_keySource;
    vs::cid_t last_key;

    // Open the cursor of the souce database
    rc = mdb_cursor_open(txnSource, dbiSource, &cursorSource);
    xzl_bug_on(rc != 0);
    rc = mdb_cursor_get(cursorSource, &keySource, &dataSource, MDB_FIRST);
    xzl_bug_on(rc != 0);

    if (rc == MDB_NOTFOUND)
        lastest_keySource = 0;
    /*
    else {
        while(cnt < CHUNK_LENGTH * migrationService.mrq.chunkNum * CHUNK_LENGTH){
            // Scan from the first key, and stop when it get the number
            rc = mdb_cursor_get(cursorSource, &keySource, &dataSource, MDB_NEXT);
            xzl_bug_on(rc != 0);

            // Save the current last_key
            last_keySource = *(vs::cid_t *) keySource.mv_data;
            cnt += 2;
        }

    }
    */

    // Send the chunk in source to decoder, and the decoder send it to the destination
    // Open the cursor of the destinaton database
    rc = mdb_cursor_open(txnDest, dbiDest, &cursorDest);
    xzl_bug_on(rc != 0);
    rc = mdb_cursor_get(cursorDest, &keyDest, &dataDest, MDB_LAST);
    xzl_bug_on(rc != 0);

    if (rc == MDB_NOTFOUND)
        last_key = 0;
    else {
        xzl_bug_on(rc != 0);
        last_key = *(vs::cid_t *) keyDest.mv_data;
    }

    lastest_keySource.as_uint = migrationService.mrq.chunkNum;

    // The frame to be retrieved as specified
    keySource.mv_data = &lastest_keySource;
    keySource.mv_size = sizeof(lastest_keySource);

    // Get the specific frame from the source
    rc = mdb_get(txnSource, dbiSource, &keySource, &dataSource);
    xzl_bug_on(rc != 0);

    // copy the video chunk to type zmq::message_t
    my_alloc_hint *hint = new my_alloc_hint(USE_LMDB_REFCNT);
    hint->txn = txnSource;
    hint->cursor = cursorSource;
    data_desc desc(TYPE_CHUNK);
    desc.cid.stream_id = 1001;
    desc.c_seq = 0;
    desc.tid = MIGRATE_TID;
    I("tid = %d", desc.tid);

    send_one_chunk_from_db((uint8_t *)dataSource.mv_data, dataSource.mv_size, chunk_push, desc, hint, MIGRATE_TID);

    // Receiving frames that has been decoded by the decoder
    for (int count = 0; count < CHUNK_LENGTH; count++){
        data_desc pull_desc;
        shared_ptr<zmq::message_t> msg_ptr;

        last_key.as_uint += (60/fps);

        msg_ptr = recv_one_frame(frame_pull, &pull_desc);
        char *framedata = NULL;
        framedata = static_cast<char *>(msg_ptr->data()), msg_ptr->size();

        // The frame to be stored should be appended to the end of the destination database
        keyDest.mv_data = &last_key;
        keyDest.mv_size = sizeof(last_key);

        // Store the key & data to destination database
        rc = mdb_put(txnDest, dbiDest, &keyDest, &dataDest, MDB_NOOVERWRITE);
        xzl_bug_on(rc != 0);
    }

    I("lmdb moved frame %d to %d in chunk %d, from source tier %d to tier %d",
      CHUNK_LENGTH * migrationService.mrq.chunkNum, CHUNK_LENGTH * (migrationService.mrq.chunkNum + 1),
      migrationService.mrq.chunkNum, migrationService.mrq.source, migrationService.mrq.destination);

    // TODO: Remember to delete the key from source (done)
    rc = mdb_del(txnSource, dbiSource, &keySource, &dataSource);
    xzl_bug_on(rc != 0);

    rc = mdb_txn_commit(txnSource);
    rc = mdb_txn_commit(txnDest);

    mdb_dbi_close(envSource, dbiSource);
    mdb_env_close(envSource);

    mdb_dbi_close(envDest, dbiDest);
    mdb_env_close(envDest);

    return;
}

void videoEncodeMove(vs::sv migrationService){

    // This function deals witht the situation that the migrating chunk will first be decoded
    int rc;
    MDB_env *envSource, *envDest;
    MDB_dbi dbiSource, dbiDest;
    MDB_val keySource, dataSource, keyDest, dataDest;
    MDB_txn *txnSource, *txnDest;
    MDB_stat mstSource, mstDest;
    MDB_cursor *cursorSource, *cursorDest;

    // Specify the path that is going to move from and to
    // Path construction: (stage - 1) * 3
    string sourceDBPath = db_address[6*migrationService.mrq.stage + 2*(migrationService.mrq.source%3) + getOffset(stage[migrationService.mrq.source])];
    string destDBPath = db_address[6*migrationService.mrq.stage + 2*(migrationService.mrq.destination%3) + getOffset(stageNew[migrationService.mrq.destination])];

    string sourcePath = sourceDBPath + temp[migrationService.mrq.temperature];
    string destPath = destDBPath + temp[migrationService.mrq.temperature];

    EE("going to move from db path %s to db path %s", sourcePath.c_str(), destPath.c_str());

    // Prepare the environments for movements
    rc = mdb_env_create(&envSource);
    xzl_bug_on(rc != 0);
    rc = mdb_env_create(&envDest);
    xzl_bug_on(rc != 0);

    rc = mdb_env_set_mapsize(envSource, 1UL * 1024UL * 1024UL * 1024UL * 1024UL); /* 1 GiB */
    xzl_bug_on(rc != 0);
    rc = mdb_env_set_mapsize(envDest, 1UL * 1024UL * 1024UL * 1024UL * 1024UL); /* 1 GiB */
    xzl_bug_on(rc != 0);

    rc = mdb_env_set_maxdbs(envSource, 1); /* required for named db */
    xzl_bug_on(rc != 0);
    rc = mdb_env_set_maxdbs(envDest, 1); /* required for named db */
    xzl_bug_on(rc != 0);

    rc = mdb_env_open(envSource, sourcePath.c_str(), 0, 0664);
    xzl_bug_on(rc != 0);
    rc = mdb_env_open(envDest, destPath.c_str(), 0, 0664);
    xzl_bug_on(rc != 0);
    EE("going to move from db path %s to db path %s", sourcePath.c_str(), destPath.c_str());

    rc = mdb_txn_begin(envSource, NULL, 0, &txnSource);
    xzl_bug_on(rc != 0);
    rc = mdb_txn_begin(envDest, NULL, 0, &txnDest);
    xzl_bug_on(rc != 0);

    rc = mdb_dbi_open(txnSource, NULL, MDB_INTEGERKEY | MDB_CREATE, &dbiSource);
    xzl_bug_on(rc != 0);
    rc = mdb_dbi_open(txnDest, NULL, MDB_INTEGERKEY | MDB_CREATE, &dbiDest);
    xzl_bug_on(rc != 0);

    int fps = 30;
    vs::cid_t lastest_keySource;
    vs::cid_t last_key;

    // Open the cursor of the souce database
    rc = mdb_cursor_open(txnSource, dbiSource, &cursorSource);
    xzl_bug_on(rc != 0);
    rc = mdb_cursor_get(cursorSource, &keySource, &dataSource, MDB_FIRST);
    xzl_bug_on(rc != 0);

    if (rc == MDB_NOTFOUND)
        lastest_keySource = 0;
    /*
    else {
        while(cnt < CHUNK_LENGTH * migrationService.mrq.chunkNum * CHUNK_LENGTH){
            // Scan from the first key, and stop when it get the number
            rc = mdb_cursor_get(cursorSource, &keySource, &dataSource, MDB_NEXT);
            xzl_bug_on(rc != 0);

            // Save the current last_key
            last_keySource = *(vs::cid_t *) keySource.mv_data;
            cnt += 2;
        }

    }
    */

    // Send the chunk in source to decoder, and the decoder send it to the destination
    // Open the cursor of the destinaton database
    rc = mdb_cursor_open(txnDest, dbiDest, &cursorDest);
    xzl_bug_on(rc != 0);
    rc = mdb_cursor_get(cursorDest, &keyDest, &dataDest, MDB_LAST);
    xzl_bug_on(rc != 0);

    if (rc == MDB_NOTFOUND)
        last_key = 0;
    else {
        xzl_bug_on(rc != 0);
        last_key = *(vs::cid_t *) keyDest.mv_data;
    }

    lastest_keySource.as_uint = 2 * CHUNK_LENGTH * migrationService.mrq.chunkNum;

    // The frame to be retrieved as specified
    keySource.mv_data = &lastest_keySource;
    keySource.mv_size = sizeof(lastest_keySource);

    // Get the specific frame from the source
    rc = mdb_get(txnSource, dbiSource, &keySource, &dataSource);
    xzl_bug_on(rc != 0);

    // Prepare the directory to encode videos
    system("mkdir /tmp/encode_image/");

    // Get all the raw frames to be encoded to the proper directory
    for (int i = 0; i < CHUNK_LENGTH; i++){
        std::stringstream ss;
        ss << i;
        std::string imagename;
        ss >> imagename;

        // Add padding 0 chars
        int length = imagename.length();
        for (int j = 0; j < 5 - length; j++){
            imagename = "0" + imagename;
        }

        lastest_keySource.as_uint += (60/fps);

        // The frame to be retrieved as specified
        keySource.mv_data = &lastest_keySource;
        keySource.mv_size = sizeof(lastest_keySource);

        // Get the specific frame from the source
        rc = mdb_get(txnSource, dbiSource, &keySource, &dataSource);
        xzl_bug_on(rc != 0);

        imagename = "/tmp/encode_image" + imagename + ".png";
        cv::imwrite(imagename, reinterpret_cast<const cv::_InputArray &>(dataSource.mv_data));

        // TODO: Remember to delete the key from source
        rc = mdb_del(txnSource, dbiSource, &keySource, &dataSource);
        xzl_bug_on(rc != 0);
    }

    std::string mergePrefix = "ffmpeg -r 30 -s ";
    std::string mergeSuffix = " -i '%05d.png' -pix_fmt yuv420p output.mp4";
    std::string mergeCommand = mergePrefix + resolution[migrationService.mrq.stage] + mergeSuffix;
    system("cd /tmp/encode_image");
    system(mergeCommand.c_str());
    system("rm -rf /tmp/encode_image");

    char *buf = nullptr;
    size_t sz;

    int fd = load_file("/tmp/encode_image/output/mp4", &buf, &sz);

    keyDest.mv_data = &last_key;
    keyDest.mv_size = sizeof(last_key);

    dataDest.mv_data = buf;
    dataDest.mv_size = sz;

    // Put the encoded chunk to the destination database
    rc = mdb_put(txnDest, dbiDest, &keyDest, &dataDest, MDB_NOOVERWRITE);
    xzl_bug_on(rc != 0);

    I("lmdb moved frame %d to %d in chunk %d, from source tier %d to tier %d",
      CHUNK_LENGTH * migrationService.mrq.chunkNum, CHUNK_LENGTH * (migrationService.mrq.chunkNum + 1),
      migrationService.mrq.chunkNum, migrationService.mrq.source, migrationService.mrq.destination);

    rc = mdb_txn_commit(txnSource);
    rc = mdb_txn_commit(txnDest);

    mdb_dbi_close(envSource, dbiSource);
    mdb_env_close(envSource);

    mdb_dbi_close(envDest, dbiDest);
    mdb_env_close(envDest);

    return;
}

int main(int argc, char *argv[]){

    std::queue<vs::sv> queue;
    vs::migration_request migrationRequest;

    zmq::socket_t sv_send(context, ZMQ_REQ);
    sv_send.connect(SERVICE_PUSH_ADDR);

    zmq::socket_t mr_recv(context, ZMQ_REP);
    mr_recv.bind(MIGRATION_PULL_ADDR);

    zmq::socket_t chunk_push(context, ZMQ_PUSH);
    chunk_push.connect(MIGRATION_CHUNK_PUSH_ADDR);

    zmq::socket_t frame_pull(context, ZMQ_PULL);
    frame_pull.bind(MIGRATION_FRAME_PULL_ADDR);

    // Constructing tasks for stage 1, 250 frames per task
    for(int stageNum = 0; stageNum < 3; stageNum++){
        for (int i = 0; i < TEMP; i++) {
            EE("%s %d", temp[i].c_str(), TEMP_NUM[i]);
            int count = 0;
            float segment;
            int chunkNum = TEMP_NUM[i]/CHUNK_LENGTH;
            // Go through the whole footage to see if it needs data movements
            while(count < chunkNum){
                int oldTier = getTier(count, i, 0, 0);
                int newTier = getTier(count, i, 1, 0);

                vs::sv service;

                // Only get whether it is NVME, SSD or HDD
                service.mrq.source = oldTier;
                service.mrq.destination = newTier;
                service.mrq.temperature = i;

                service.type = MIGRATION;
                service.mrq.stage = stageNum;

                string sourceDBPath = db_address[3*stageNum + 2*(oldTier%3) + getOffset(stage[2*(oldTier%3)%2])];
                string destDBPath = db_address[3*stageNum + 2*(newTier%3) + getOffset(stage[2*(newTier%3)%2])];

                string sourcePath = sourceDBPath + temp[i];
                string destPath = destDBPath + temp[i];

                // Figure out whether a chunk needs codec operations
                switch(stage[oldTier] ^ stageNew[newTier]){
                    case 0:
                        if(oldTier == newTier){
                            I("chunk %d, No movement needed", count);
                            break;
                        }
                        else{
                            service.mrq.mode = 0;
                            service.mrq.chunkNum = count;
                            for(int tierCount = 0; tierCount < oldTier % 3; tierCount ++){
                                service.mrq.chunkNum -= stagePer[stageNum * 9 + 3 * i + tierCount] * float(TEMP_NUM[stageNum]/CHUNK_LENGTH);
                            }

                            I("decode chunk %d, sequence chunk %d on %s, from frame %d to %d, migrate from %s to %s mode %d",
                              count, service.mrq.chunkNum, sourcePath.c_str(), count * CHUNK_LENGTH, (count + 1) * CHUNK_LENGTH,
                              sourcePath.c_str(), destPath.c_str(), service.mrq.mode);

                            queue.push(service);
                        }
                        break;
                    case 1:
                        if((stage[oldTier] == 0) && (stageNew[newTier] == 1)){
                            service.mrq.chunkNum = count;
                            for(int stageCount = 0; stageCount < stageNum; stageCount ++){
                                service.mrq.chunkNum -= stagePer[stageNum * 9 + 3 * i + oldTier] * TEMP_NUM[stageNum]/CHUNK_LENGTH;
                            }
                            service.mrq.mode = 1;
                            I("decode chunk %d, from frame %d to %d, migrate from %s to %s mode %d",
                              count, count * CHUNK_LENGTH, (count + 1) * CHUNK_LENGTH,
                              sourcePath.c_str(), destPath.c_str(), service.mrq.mode);

                            queue.push(service);
                        }
                        else{
                            service.mrq.chunkNum = count;
                            for(int stageCount = 0; stageCount < stageNum; stageCount ++){
                                service.mrq.chunkNum -= stagePer[stageNum * 9 + 3 * i + oldTier] * TEMP_NUM[stageNum]/CHUNK_LENGTH;
                            }
                            service.mrq.mode = 2;
                            I("encode chunk %d, from frame %d to %d, migrate from %s to %s mode %d",
                              count, count * CHUNK_LENGTH, (count + 1) * CHUNK_LENGTH,
                              sourcePath.c_str(), destPath.c_str(), service.mrq.mode);

                            queue.push(service);
                        }
                        break;
                    default:
                        break;
                }
                count += 1;
            }
        }
        break;
    }


    while(1){
        //Sending tasks to rt-scheduler
        if(!queue.empty()) {
            zmq::message_t mrrq(sizeof(vs::sv));
            mempcpy(mrrq.data(), &queue.front(), sizeof(vs::sv));
            sv_send.send(mrrq);

            zmq::message_t reply(5);
            sv_send.recv(&reply);

            // Get the migration service details and do the migration
            //const vs::sv migrationService = queue.front();
            queue.pop();
        }

        // receiving permission from rt-scheduler to issue the migration operations
        vs::sv *s;
        zmq::message_t mr_rq(sizeof(vs::sv));

        mr_recv.recv(&mr_rq);
        s = static_cast<vs::sv *>(mr_rq.data());
        //I("Get permission from the runtime scheduler, moving chunk %d", s->mrq.chunkNum);

        zmq::message_t reply(5);
        memcpy (reply.data (), "reply", 5);
        mr_recv.send(reply);

        switch (s->mrq.mode){
            case 0:
                videoMove(*s);
                break;
            case 1:
                videoDecodeMove(*s, chunk_push, frame_pull);
                break;
            case 2:
                videoEncodeMove(*s);
                break;
            default:
                break;
        }
    }
    return 0;
}