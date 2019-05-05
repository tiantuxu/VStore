//
// Created by teddyxu on 7/26/18.
//

#ifndef VIDEO_STREAMER_RT_CONFIG_H
#define VIDEO_STREAMER_RT_CONFIG_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <iostream>
#include <sstream>
#include <zmq.h>
#include <zmq.hpp>
#include <queue>

#define INGESTION 3
#define RETRIEVAL 2
#define MIGRATION 1

#define SERVICE_PUSH_ADDR "tcp://localhost:44444"
#define SERVICE_PULL_ADDR "tcp://*:44444"

#define MIGRATION_PUSH_ADDR "tcp://localhost:44445"
#define MIGRATION_PULL_ADDR "tcp://*:44445"

#define MIGRATION_CHUNK_PUSH_ADDR "tcp://localhost:44446"
#define MIGRATION_CHUNK_PULL_ADDR "tcp://*:44446"

#define MIGRATION_FRAME_PUSH_ADDR "tcp://localhost:44447"
#define MIGRATION_FRAME_PULL_ADDR "tcp://*:44447"

#define MIGRATE_TID 8888

namespace vs{

    typedef struct request_desc{
        int db_seq;
        int start_fnum;
        int total_fnum;
        int id;
        int temp;
    }request_desc;

    typedef struct migration_request{
        // mode 0: no codec, mode 1: decode first, mode: encode first
        int mode;
        int stage; // stage 0-2
        int source;
        int destination;
        int chunkNum;
        // Record the temperature of the migration
        int temperature;
    }migration_request;

    typedef struct service{
        int type;
        request_desc rq;
        migration_request mrq;
    }sv;
}

int getOffset(int type){
    return !type;
}

#endif //VIDEO_STREAMER_RT_CONFIG_H
