//
// Created by xzl on 12/24/17.
//

#ifndef VIDEO_STREAMER_CONFIG_H
#define VIDEO_STREAMER_CONFIG_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <sstream>
#include <zmq.h>
#include <zmq.hpp>
#include <queue>
#include "tensorflow/noscope/log.h"
#include "tensorflow/noscope/mm.h"
#define SERVER_PULL_ADDR "tcp://*:5557"
#define SERVER_PUSH_ADDR "tcp://*:5558"

#define CLIENT_PULL_FROM_ADDR "tcp://localhost:5558"
#define CLIENT_PUSH_TO_ADDR "tcp://localhost:5557"

/* ---- */

#define WORKER_PUSH_TO_ADDR "tcp://localhost:5557"
#define WORKER_PULL_FROM_ADDR "tcp://localhost:5558"
#define WORKER_PUSHFB_TO_ADDR "tcp://localhost:5559"
#define WORKER_REQUEST "tcp://localhost:5560"
#define WORKER_REQUEST_CHUNK "tcp://localhost:5561"
//#define WORKER_PULL_STREAMINFO_FROM_ADDR "tcp://localhost:5561"
//#define WORKER720_REQUEST "tcp://localhost:5562"


#define FRAME_PULL_ADDR 						"tcp://*:5557"
#define CHUNK_PUSH_ADDR 						"tcp://*:5558"
#define FB_PULL_ADDR 								"tcp://*:5559"
#define REQUEST_PULL_ADDR 								"tcp://*:5560"
#define REQUEST_CHUNK_PULL_ADDR 								"tcp://*:5561"
//#define STREAMINFO_PUSH_ADDR 				"tcp://*:5561"

#define DB_PATH 							"/shared/videos/lmdb/"
#define DB_RAW_FRAME_PATH                   "/shared/videos/lmdb-rf"

#define DB_PATH_CHUNK_NVME_720     			"/mnt/nvme/720-chunk/"
#define DB_PATH_CHUNK_NVME_540     			"/mnt/nvme/540-chunk/"
#define DB_PATH_CHUNK_NVME_180     			"/mnt/nvme/180-chunk/"
#define DB_PATH_CHUNK_SSD_720     			"/ssd/720-chunk/"
#define DB_PATH_CHUNK_SSD_540     			"/ssd/540-chunk/"
#define DB_PATH_CHUNK_SSD_180     			"/ssd/180-chunk/"
#define DB_PATH_CHUNK_HDD_720     			"/shared/videos/hdd/720-chunk/"
#define DB_PATH_CHUNK_HDD_540     			"/shared/videos/hdd/540-chunk/"
#define DB_PATH_CHUNK_HDD_180     			"/shared/videos/hdd/180-chunk/"

#define DB_RAW_FRAME_PATH_180 		"/shared/videos/hdd/180-raw/"
#define DB_RAW_FRAME_PATH_540 		"/shared/videos/hdd/540-raw/"
#define DB_RAW_FRAME_PATH_720       "/shared/videos/hdd/720-raw/"
#define DB_RAW_FRAME_PATH_SSD_720   "/ssd/720-raw/"
#define DB_RAW_FRAME_PATH_SSD_540   "/ssd/540-raw/"
#define DB_RAW_FRAME_PATH_SSD_180   "/ssd/180-raw/"
#define DB_RAW_FRAME_PATH_NVME_720   "/mnt/nvme/720-raw/"
#define DB_RAW_FRAME_PATH_NVME_540   "/mnt/nvme/540-raw/"
#define DB_RAW_FRAME_PATH_NVME_180   "/mnt/nvme/180-raw/"

enum db_sequence{
    NVME_RAW_180 = 0,
    NVME_CHUNK_180,
    SSD_RAW_180,
    SSD_CHUNK_180,
    HDD_RAW_180,
    HDD_CHUNK_180,

    NVME_RAW_720,
    NVME_CHUNK_720,
    SSD_RAW_720,
    SSD_CHUNK_720,
    HDD_RAW_720,
    HDD_CHUNK_720,

    NVME_RAW_540,
    NVME_CHUNK_540,
    SSD_RAW_540,
    SSD_CHUNK_540,
    HDD_RAW_540,
    HDD_CHUNK_540,
};

static std::string db_address[20] = {
        "/mnt/nvme/180-raw/", "/mnt/nvme/180-chunk/",
        "/ssd/180-raw/", "/ssd/180-chunk/",
        "/shared/videos/hdd/180-raw/", "/shared/videos/hdd/180-chunk/",
        "/mnt/nvme/720-raw/", "/mnt/nvme/720-chunk/",
        "/ssd/720-raw/", "/ssd/720-chunk/",
        "/shared/videos/hdd/720-raw/", "/shared/videos/hdd/720-chunk/",
        "/mnt/nvme/540-raw/", "/mnt/nvme/540-chunk/",
        "/ssd/540-raw/", "/ssd/540-chunk/",
        "/shared/videos/hdd/540-raw/", "/shared/videos/hdd/540-raw/",
        "/ssd/raw/", "/ssd/encode/"
};

#define CHUNK_SIZE 5
#define PREFETCH_BUF 1
#define CHUNK_LENGTH 250

#define THREAD_NUM 1
#define THREAD_LIMIT 40

static std::string en_pull_address[THREAD_LIMIT] = {
        "tcp://*:16000", "tcp://*:16001", "tcp://*:16002", "tcp://*:16003", "tcp://*:16004",
        "tcp://*:16005", "tcp://*:16006", "tcp://*:16007", "tcp://*:16008", "tcp://*:16009",
        "tcp://*:16010", "tcp://*:16011", "tcp://*:16012", "tcp://*:16013", "tcp://*:16014",
        "tcp://*:16015", "tcp://*:16016", "tcp://*:16017", "tcp://*:16018", "tcp://*:16019",
        "tcp://*:16020", "tcp://*:16021", "tcp://*:16022", "tcp://*:16023", "tcp://*:16024",
        "tcp://*:16025", "tcp://*:16026", "tcp://*:16027", "tcp://*:16028", "tcp://*:16029",
        "tcp://*:16030", "tcp://*:16031", "tcp://*:16032", "tcp://*:16033", "tcp://*:16034",
        "tcp://*:16035", "tcp://*:16036", "tcp://*:16037", "tcp://*:16038", "tcp://*:16039"
};

static std::string en_push_address[THREAD_LIMIT] = {
        "tcp://localhost:16000", "tcp://localhost:16001", "tcp://localhost:16002", "tcp://localhost:16003", "tcp://localhost:16004",
        "tcp://localhost:16005", "tcp://localhost:16006", "tcp://localhost:16007", "tcp://localhost:16008", "tcp://localhost:16009",
        "tcp://localhost:16010", "tcp://localhost:16011", "tcp://localhost:16012", "tcp://localhost:16013", "tcp://localhost:16014",
        "tcp://localhost:16015", "tcp://localhost:16016", "tcp://localhost:16017", "tcp://localhost:16018", "tcp://localhost:16019",
        "tcp://localhost:16020", "tcp://localhost:16021", "tcp://localhost:16022", "tcp://localhost:16023", "tcp://localhost:16024",
        "tcp://localhost:16025", "tcp://localhost:16026", "tcp://localhost:16027", "tcp://localhost:16028", "tcp://localhost:16029",
        "tcp://localhost:16030", "tcp://localhost:16031", "tcp://localhost:16032", "tcp://localhost:16033", "tcp://localhost:16034",
        "tcp://localhost:16035", "tcp://localhost:16036", "tcp://localhost:16037", "tcp://localhost:16038", "tcp://localhost:16039"
};

static std::string raw_pull_address[THREAD_LIMIT] = {
        "tcp://*:26000", "tcp://*:26001", "tcp://*:26002", "tcp://*:26003", "tcp://*:26004",
        "tcp://*:26005", "tcp://*:26006", "tcp://*:26007", "tcp://*:26008", "tcp://*:26009",
        "tcp://*:26010", "tcp://*:26011", "tcp://*:26012", "tcp://*:26013", "tcp://*:26014",
        "tcp://*:26015", "tcp://*:26016", "tcp://*:26017", "tcp://*:26018", "tcp://*:26019",
        "tcp://*:26020", "tcp://*:26021", "tcp://*:26022", "tcp://*:26023", "tcp://*:26024",
        "tcp://*:26025", "tcp://*:26026", "tcp://*:26027", "tcp://*:26028", "tcp://*:26029",
        "tcp://*:26030", "tcp://*:26031", "tcp://*:26032", "tcp://*:26033", "tcp://*:26034",
        "tcp://*:26035", "tcp://*:26036", "tcp://*:26037", "tcp://*:26038", "tcp://*:26039"
};

static std::string raw_push_address[THREAD_LIMIT] = {
        "tcp://localhost:26000", "tcp://localhost:26001", "tcp://localhost:26002", "tcp://localhost:26003", "tcp://localhost:26004",
        "tcp://localhost:26005", "tcp://localhost:26006", "tcp://localhost:26007", "tcp://localhost:26008", "tcp://localhost:26009",
        "tcp://localhost:26010", "tcp://localhost:26011", "tcp://localhost:26012", "tcp://localhost:26013", "tcp://localhost:26014",
        "tcp://localhost:26015", "tcp://localhost:26016", "tcp://localhost:26017", "tcp://localhost:26018", "tcp://localhost:26019",
        "tcp://localhost:26020", "tcp://localhost:26021", "tcp://localhost:26022", "tcp://localhost:26023", "tcp://localhost:26024",
        "tcp://localhost:26025", "tcp://localhost:26026", "tcp://localhost:26027", "tcp://localhost:26028", "tcp://localhost:26029",
        "tcp://localhost:26030", "tcp://localhost:26031", "tcp://localhost:26032", "tcp://localhost:26033", "tcp://localhost:26034",
        "tcp://localhost:26035", "tcp://localhost:26036", "tcp://localhost:26037", "tcp://localhost:26038", "tcp://localhost:26039"
};

static std::string raw3_pull_address[THREAD_LIMIT] = {
        "tcp://*:36000", "tcp://*:36001", "tcp://*:36002", "tcp://*:36003", "tcp://*:36004",
        "tcp://*:36005", "tcp://*:36006", "tcp://*:36007", "tcp://*:36008", "tcp://*:36009",
        "tcp://*:36010", "tcp://*:36011", "tcp://*:36012", "tcp://*:36013", "tcp://*:36014",
        "tcp://*:36015", "tcp://*:36016", "tcp://*:36017", "tcp://*:36018", "tcp://*:36019",
        "tcp://*:36020", "tcp://*:36021", "tcp://*:36022", "tcp://*:36023", "tcp://*:36024",
        "tcp://*:36025", "tcp://*:36026", "tcp://*:36027", "tcp://*:36028", "tcp://*:36029",
        "tcp://*:36030", "tcp://*:36031", "tcp://*:36032", "tcp://*:36033", "tcp://*:36034",
        "tcp://*:36035", "tcp://*:36036", "tcp://*:36037", "tcp://*:36038", "tcp://*:36039"
};

static std::string raw3_push_address[THREAD_LIMIT] = {
        "tcp://localhost:36000", "tcp://localhost:36001", "tcp://localhost:36002", "tcp://localhost:36003", "tcp://localhost:36004",
        "tcp://localhost:36005", "tcp://localhost:36006", "tcp://localhost:36007", "tcp://localhost:36008", "tcp://localhost:36009",
        "tcp://localhost:36010", "tcp://localhost:36011", "tcp://localhost:36012", "tcp://localhost:36013", "tcp://localhost:36014",
        "tcp://localhost:36015", "tcp://localhost:36016", "tcp://localhost:36017", "tcp://localhost:36018", "tcp://localhost:36019",
        "tcp://localhost:36020", "tcp://localhost:36021", "tcp://localhost:36022", "tcp://localhost:36023", "tcp://localhost:36024",
        "tcp://localhost:36025", "tcp://localhost:36026", "tcp://localhost:36027", "tcp://localhost:36028", "tcp://localhost:36029",
        "tcp://localhost:36030", "tcp://localhost:36031", "tcp://localhost:36032", "tcp://localhost:36033", "tcp://localhost:36034",
        "tcp://localhost:36035", "tcp://localhost:36036", "tcp://localhost:36037", "tcp://localhost:36038", "tcp://localhost:36039"
};

/* Frame numbers of hot, warm, cold data */
static int TEMP_NUM[3] = {36000, 72000, 108000};

#if 0
/* Old i-stream configuration */
static float stagePer[27] = {0.25, 0.25, 0.50, 0.00, 0.00, 1.00, 0.50, 0.50, 0.00, // stage 1
                             0.00, 0.75, 0.25, 0.25, 0.75, 0.00, 0.25, 0.50, 0.25, // stage 2
                             0.00, 0.25, 0.75, 0.50, 0.50, 0.00, 0.25, 0.75, 0.00};// stage 3

// 0 means encoded, 1 means raw
static int stage[27] = {1, 1, 1, 1, 0, 1, 0, 1, 1, // stage 1
                        1, 0, 0, 0, 0, 0, 1, 0, 0, // stage 2
                        0, 1, 1, 1, 0, 1, 0, 1, 1};// stage 3
#endif

#if 1
/* Old i-stream configuration */
static float stagePer[27] = {0.25, 0.25, 0.50, 0.00, 0.00, 1.00, 0.50, 0.50, 0.00, // stage 1
                             0.00, 0.75, 0.25, 0.25, 0.75, 0.00, 0.25, 0.50, 0.25, // stage 2
                             0.00, 0.25, 0.75, 0.50, 0.50, 0.00, 0.25, 0.75, 0.00};// stage 3

// 0 means encoded, 1 means raw
static int stage[27] = {1, 1, 1, 1, 0, 1, 0, 1, 1, // stage 1
                        1, 0, 0, 0, 0, 0, 1, 0, 0, // stage 2
                        0, 1, 1, 1, 0, 1, 0, 1, 1};// stage 3
#endif

/* New i-stream configuration */
//static float stagePerNew[27] = {1.00, 0.00, 0.00, 0.00, 0.50, 0.50, 0.50, 0.50, 0.00, // stage 1
                                //0.25, 0.75, 0.00, 0.60, 0.00, 0.40, 0.50, 0.50, 0.00, // stage 2
                                //0.00, 0.50, 0.50, 0.25, 0.75, 0.00, 0.75, 0.25, 0.00};// stage 3

// 0 means encoded, 1 means raw
//static int stageNew[27] = {1, 0, 1, 1, 0, 0, 1, 1, 1, // stage 1
                           //0, 1, 1, 0, 1, 0, 1, 1, 0, // stage 2
                           //0, 1, 1, 1, 1, 1, 1, 0, 1};// stage 3

static std::string temp[3] = {"hot", "warm", "cold"};
//static std::string resolution[3] = {"320x180", "1280x720", "960x540"};
#define RESO 50

#define HOT 0
#define WARM 1
#define COLD 2

#define TIER_NUM 3
#define STAGE_NUM 3
#define TEMP 3
#define Q_TEMP 0

#endif //VIDEO_STREAMER_CONFIG_H
