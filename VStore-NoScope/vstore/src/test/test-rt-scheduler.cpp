//
// Created by teddyxu on 7/26/18.
//

#include <sstream> // std::ostringstream

#include <zmq.hpp>
#include <cstdio>
#include <sstream>
#include <iostream>
#include <iterator>
#include <algorithm>

#include "config.h"
#include "vs-types.h"
#include "mydecoder.h"
#include "log.h"
#include "rxtx.h"
#include "RxManager.h"
#include "measure.h"

#include "StatCollector.h"
#include "CallBackTimer.h"

#include "tclap/CmdLine.h"
#include "video/videobuffer.h"
#include "openalpr/motiondetector.h"
#include "openalpr/alpr.h"
#include <queue>
#include "rt-config.h"


using namespace std;
static zmq::context_t context (1 /* # io threads */);

int main(int argc, char *argv[]){
    // The request pulling queue
    std::queue<vs::sv> pq_ingestion;
    std::queue<vs::sv> pq_retrieval;
    std::queue<vs::sv> pq_migration;

    zmq::socket_t sv_recv(context, ZMQ_REP);
    sv_recv.bind(SERVICE_PULL_ADDR);

    I("bound to %s. ready to pull new requests", SERVICE_PULL_ADDR);

    zmq::socket_t rt_send(context, ZMQ_REQ);
    rt_send.connect(WORKER_REQUEST);

    zmq::socket_t mr_send(context, ZMQ_REQ);
    mr_send.connect(MIGRATION_PUSH_ADDR);

    std::string temp[3] = {"hot", "warm", "cold"};

    while(1){
        vs::sv *s;
        zmq::message_t ser_rq(sizeof(vs::sv));

        sv_recv.recv(&ser_rq);
        s = static_cast<vs::sv *>(ser_rq.data());
        //I("Get service type %d", s->type);

        zmq::message_t reply(5);
        memcpy (reply.data (), "reply", 5);
        sv_recv.send(reply);

        std::string oldDBPath;
        std::string newDBPath;

        std::string oldPath;
        std::string newPath;

        switch(s->type){
            case INGESTION:
                I("Got INGESTION request");
                pq_ingestion.push(*s);
                break;

            case RETRIEVAL:
                I("Got RETRIEVAL request, db_seq = %d, start = %d, total = %d, id = %d, temperature = %d",
                s->rq.db_seq, s->rq.start_fnum, s->rq.total_fnum, s->rq.id, s->rq.temp);
                pq_retrieval.push(*s);
                break;

            case MIGRATION:
                oldDBPath = db_address[2*(s->mrq.source%3) + getOffset(stage[s->mrq.source])];
                newDBPath = db_address[2*(s->mrq.destination%3) + getOffset(stageNew[s->mrq.destination])];

                oldPath = oldDBPath + temp[s->mrq.temperature];
                newPath = newDBPath + temp[s->mrq.temperature];

                I("Got MIGRATION request, migrate from %s to %s", oldPath.c_str(), newPath.c_str());
                pq_migration.push(*s);
                break;

            default:
                break;
        }

        zmq::message_t ser_send(sizeof(vs::sv));
        // Scan each queue to get the next request with the highest priority
        if (!pq_ingestion.empty()){
            mempcpy(ser_send.data(), &pq_ingestion.front(), sizeof(vs::sv));
            rt_send.send(ser_send);
            rt_send.recv(&reply);
            pq_ingestion.pop();
        }
        else if(!pq_retrieval.empty()){
            mempcpy(ser_send.data(), &pq_retrieval.front(), sizeof(vs::sv));
            rt_send.send(ser_send);
            rt_send.recv(&reply);
            //s = static_cast<vs::sv *>(ser_send.data());
            //I("Get sv type %d", s->type);
            pq_retrieval.pop();
        }
        else if(!pq_migration.empty()){
            mempcpy(ser_send.data(), &pq_migration.front(), sizeof(vs::sv));
            mr_send.send(ser_send);
            mr_send.recv(&reply);
            pq_migration.pop();
        }
        else{
            continue;
        }

    }
    return 0;
}