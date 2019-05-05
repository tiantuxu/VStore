//
// Created by xzl on 12/24/17.
//
// tester: pull frames from a

/*
 * Copyright (c) 2015 OpenALPR Technology, Inc.
 * Open source Automated License Plate Recognition [http://www.openalpr.com]
 *
 * This file is part of OpenALPR.
 *
 * OpenALPR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License
 * version 3 as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef DEBUG
#define GLIBCXX_DEBUG 1 // debugging
#endif

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

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/operations.hpp"
#include "opencv2/core/core.hpp"

#include "tclap/CmdLine.h"
#include "openalpr/support/filesystem.h"
#include "openalpr/support/timing.h"
#include "openalpr/support/platform.h"
#include "video/videobuffer.h"
#include "openalpr/motiondetector.h"
#include "openalpr/alpr.h"
#include <queue>

extern "C"{
#include <omp.h>
};

using namespace vs;
using namespace alpr;

const std::string MAIN_WINDOW_NAME = "ALPR main window";

const bool SAVE_LAST_VIDEO_STILL = false;
const std::string LAST_VIDEO_STILL_LOCATION = "/tmp/laststill.jpg";
const std::string WEBCAM_PREFIX = "/dev/video";
//MotionDetector motiondetector;
MotionDetector motiondetector[64];
bool do_motiondetection = true;

/** Function Headers */
bool detectandshow(Alpr* alpr, cv::Mat frame, std::string region, bool writeJson);
bool is_supported_image(std::string image_file);

bool measureProcessingTime = true;
std::string templatePattern;

// This boolean is set to false when the user hits terminates (e.g., CTRL+C )
// so we can end infinite loops for things like video processing.
bool program_active = true;

/*
static StatCollector stat;

static void getStatistics(void) {
	Statstics s;
	if (!stat.ReportStatistics(&s))
		return;

	if (s.rec_persec < 1) // nothing?
		return;

	printf("%.2f MB/sec \t %.0f fps \n", s.byte_persec/1024/1024, s.rec_persec);
};
*/

/* pull the source to get info for all streams */
void getAllStreamInfo() {



}

atomic<int> mycount(0);

std::vector<AlprRegionOfInterest> detectmotion( Alpr* alpr, cv::Mat frame, std::string region, bool writeJson, int proc_id, int f_seq)
//bool detectandshow( Alpr* alpr, cv::Mat frame, std::string region, bool writeJson)
{
    timespec startTime;
    timespec endTime;

    mycount++;

    std::vector<AlprRegionOfInterest> regionsOfInterest;

    int ratio = 720 / 180;

    /*
    if (do_motiondetection)
    {
        cout << "motion detection" << endl;
        cv::Rect rectan = motiondetector.MotionDetect(&frame);
        if (rectan.width>0) regionsOfInterest.push_back(AlprRegionOfInterest(rectan.x, rectan.y, rectan.width, rectan.height));
    }
    else regionsOfInterest.push_back(AlprRegionOfInterest(0, 0, frame.cols, frame.rows));
    */

    /* Motion detection phase */
    cv::Rect rectan = motiondetector[proc_id].MotionDetect(&frame);
    if (rectan.width > 0) {
        //cout << "here" << endl;
        //regionsOfInterest.push_back(AlprRegionOfInterest(rectan.x, rectan.y, rectan.width, rectan.height));
        regionsOfInterest.push_back(AlprRegionOfInterest(ratio*rectan.x, ratio*rectan.y, ratio*rectan.width, ratio*rectan.height));
    }

    //cout << rectan.x << " " << rectan.y << " " << rectan.width << " " << rectan.height << " " << endl;
    //std::cout << "Motion Detection: " << diffclock(startTime, endTime) << "ms." << std::endl;

    return regionsOfInterest;
}

//bool recognizeplate( Alpr* alpr, cv::Mat frame_720, std::string region, bool writeJson, int proc_id, int f_seq, std::vector<AlprRegionOfInterest> regionsOfInterest){
std::queue<PlateRegion> recognizeRegion( Alpr* alpr, cv::Mat frame_720, std::string region, bool writeJson, int proc_id, int f_seq, std::vector<AlprRegionOfInterest> regionsOfInterest) {

    AlprResults results;
    std::queue<PlateRegion> plateQueue;

    /* Recognition phase */
    //getTimeMonotonic(&startTime);
    /*
    int arraySize = frame_720.cols * frame_720.rows * frame_720.elemSize();
    cv::Mat imgData = cv::Mat(arraySize, 1, CV_8U, frame_720.data);
    cv::Mat img = imgData.reshape(frame_720.elemSize(), frame_720.elemSize());

    if (regionsOfInterest.size() == 0) {
        AlprRegionOfInterest fullFrame(0, 0, img.cols, img.rows);
        regionsOfInterest.push_back(fullFrame);
    }
    */

    //results = alpr->recognize(frame_720.data, frame_720.elemSize(), frame_720.cols, frame_720.rows, frame_ocr, regionsOfInterest);
    //splateQueue = alpr->recognizeregion(frame_720.data, frame_720.elemSize(), frame_720.cols, frame_720.rows, regionsOfInterest);

    plateQueue =  alpr->recognizeregion(frame_720.data, frame_720.elemSize(), frame_720.cols, frame_720.rows, regionsOfInterest);

    I("platequeue.size = %d", plateQueue.size());

    return plateQueue;
    //results = alpr->recognize(frame_720.data, frame_720.elemSize(), frame_720.cols, frame_720.rows, regionsOfInterest);

    /* print the ROI */
    //cout << regionsOfInterest[0].x << " " << regionsOfInterest[0].y << " " << regionsOfInterest[0].width << " " << regionsOfInterest[0].height << " " << endl;

    //getTimeMonotonic(&endTime);
    //std::cout << "Total Plate Recognition: " << diffclock(startTime, endTime) << "ms." << std::endl;

    /*
    if (measureProcessingTime) {
        std::cout << "Total Time to process image: " << totalProcessingTime << "ms." << std::endl;
    }
    */
}

bool recognizeplate( Alpr* alpr, cv::Mat frame_720, std::string region, bool writeJson, int proc_id, int f_seq, std::vector<AlprRegionOfInterest> regionsOfInterest)
{
    AlprResults results;

    results = alpr->recognize(frame_720.data, frame_720.elemSize(), frame_720.cols, frame_720.rows, regionsOfInterest);
    if (writeJson)
    {
        std::cout << alpr->toJson( results ) << std::endl;
    }
    else
    {
        for (int i = 0; i < results.plates.size(); i++)
        {
            std::cout << "plate" << i << ": " << results.plates[i].topNPlates.size() << " results";
            if (measureProcessingTime)
                std::cout << " -- Processing Time = " << results.plates[i].processing_time_ms << "ms.";
            std::cout << std::endl;

            if (results.plates[i].regionConfidence > 0)
                std::cout << "State ID: " << results.plates[i].region << " (" << results.plates[i].regionConfidence << "% confidence)" << std::endl;

            for (int k = 0; k < results.plates[i].topNPlates.size(); k++)
            {
                // Replace the multiline newline character with a dash
                std::string no_newline = results.plates[i].topNPlates[k].characters;
                std::replace(no_newline.begin(), no_newline.end(), '\n','-');

                std::cout << "    - " << no_newline << "\t confidence: " << results.plates[i].topNPlates[k].overall_confidence;
                if (templatePattern.size() > 0 || results.plates[i].regionConfidence > 0)
                    std::cout << "\t pattern_match: " << results.plates[i].topNPlates[k].matches_template;

                std::cout << std::endl;
            }
        }
    }

    return results.plates.size() > 0;
}

bool recognizePlate( Alpr* alpr, cv::Mat frame_ocr, cv::Mat frame_ocr_gray, std::string region, bool writeJson, int proc_id, int f_seq, queue<PlateRegion> plateQueue)
{
    AlprResults results;

    //cout << plateQueue.front().rect.x << endl;
    //cout << frame_ocr.cols << endl;
    //for(auto iter = plateQueue.front(); iter != plateQueue.back(); iter ++){
    //    cout << plateQueue.front().rect.x << endl;
    //    cout << plateQueue.front().rect.y << endl;
    //    cout << plateQueue.front().rect.width << endl;
    //    cout << plateQueue.front().rect.height << endl;

    //}

    results = alpr->recognizeresult(frame_ocr, frame_ocr_gray, plateQueue);

    if (writeJson)
    {
        std::cout << alpr->toJson( results ) << std::endl;
    }
    else
    {
        for (int i = 0; i < results.plates.size(); i++)
        {
            std::cout << "plate" << i << ": " << results.plates[i].topNPlates.size() << " results";
            if (measureProcessingTime)
                std::cout << " -- Processing Time = " << results.plates[i].processing_time_ms << "ms.";
            std::cout << std::endl;

            if (results.plates[i].regionConfidence > 0)
                std::cout << "State ID: " << results.plates[i].region << " (" << results.plates[i].regionConfidence << "% confidence)" << std::endl;

            for (int k = 0; k < results.plates[i].topNPlates.size(); k++)
            {
                // Replace the multiline newline character with a dash
                std::string no_newline = results.plates[i].topNPlates[k].characters;
                std::replace(no_newline.begin(), no_newline.end(), '\n','-');

                std::cout << "    - " << no_newline << "\t confidence: " << results.plates[i].topNPlates[k].overall_confidence;
                if (templatePattern.size() > 0 || results.plates[i].regionConfidence > 0)
                    std::cout << "\t pattern_match: " << results.plates[i].topNPlates[k].matches_template;

                std::cout << std::endl;
            }
        }
    }

    return results.plates.size() > 0;
}

int main (int argc, char *argv[])
{
	zmq::context_t context (1 /* # io threads */);

	zmq::socket_t receiver(context, ZMQ_PULL);
//	receiver.connect(CLIENT_PULL_FROM_ADDR);
	receiver.bind(FRAME_PULL_ADDR);

    zmq::socket_t q_request(context, ZMQ_REQ);
    q_request.connect(WORKER_REQUEST);

    EE("bound to %s. wait for workers to push ...", FRAME_PULL_ADDR);

	CallBackTimer stat_collector;
	RxManager mg;

	//stat_collector.start(200 /*ms, check interval */, &getStatistics);

	/* consumption phase */
	//Frame f;
	int rc;
	seq_t wm_c, wm_f;

    /* teddyxu: alpr starts here */
    //std::vector<std::string> filenames;
    std::string configFile = "";
    bool outputJson = false;
    //int seektoms = 0;
    //bool detectRegion = false;
    std::string country;
    int topn;

    timespec startTime;
    timespec endTime;
    //bool debug_mode = false;

    TCLAP::CmdLine cmd("OpenAlpr Command Line Utility", ' ', Alpr::getVersion());
    TCLAP::UnlabeledMultiArg<std::string> fileArg("image_file", "Image containing license plates", true, "",
                                                      "image_file_path");
    TCLAP::ValueArg<std::string> countryCodeArg("c", "country",
                                                "Country code to identify (either us for USA or eu for Europe).  Default=us",
                                                false, "us", "country_code");
    TCLAP::ValueArg<int> seekToMsArg("", "seek", "Seek to the specified millisecond in a video file. Default=0",
                                     false, 0, "integer_ms");
    TCLAP::ValueArg<std::string> configFileArg("", "config", "Path to the openalpr.conf file", false, "",
                                               "config_file");
    TCLAP::ValueArg<std::string> templatePatternArg("p", "pattern",
                                                    "Attempt to match the plate number against a plate pattern (e.g., md for Maryland, ca for California)",
                                                    false, "", "pattern code");
    TCLAP::ValueArg<int> topNArg("n", "topn", "Max number of possible plate numbers to return.  Default=10",
                                 false, 10, "topN");

    TCLAP::SwitchArg jsonSwitch("j", "json", "Output recognition results in JSON format.  Default=off", cmd,
                                false);
    TCLAP::SwitchArg debugSwitch("", "debug", "Enable debug output.  Default=off", cmd, false);
    TCLAP::SwitchArg detectRegionSwitch("d", "detect_region",
                                        "Attempt to detect the region of the plate image.  [Experimental]  Default=off",
                                        cmd, false);
    TCLAP::SwitchArg clockSwitch("", "clock",
                                 "Measure/print the total time to process image and all plates.  Default=off",
                                 cmd, false);
    TCLAP::SwitchArg motiondetect("", "motion", "Use motion detection on video file or stream.  Default=off",
                                  cmd, false);


    try {
        cmd.add(templatePatternArg);
        cmd.add(seekToMsArg);
        cmd.add(topNArg);
        cmd.add(configFileArg);
        cmd.add(fileArg);
        cmd.add(countryCodeArg);

        //filenames = fileArg.getValue();

        country = countryCodeArg.getValue();
        //seektoms = seekToMsArg.getValue();
        outputJson = jsonSwitch.getValue();
        //debug_mode = debugSwitch.getValue();
        configFile = configFileArg.getValue();
        //detectRegion = detectRegionSwitch.getValue();
        templatePattern = templatePatternArg.getValue();
        topn = topNArg.getValue();
        measureProcessingTime = clockSwitch.getValue();
        do_motiondetection = motiondetect.getValue();
    }
    catch (TCLAP::ArgException &e)    // catch any exceptions
    {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
        //return 1;
    }

    //Alpr alpr(country, configFile);
    //alpr.setTopN(topn);

    Alpr *alpr[64];

    for(int m = 0; m < 64; m++){
        alpr[m] = new Alpr(country, configFile);
        alpr[m]->setTopN(topn);
    }

    int height = 180;
    int width = 320;

    //Frame frame[72005];
    int framesize = height * width;

    int cont_flag = 0;
    //for(int iter = 0; iter < 3; iter++) {
    while(1){
        Frame f;
        Frame frame[50000];

        request_desc rd;
        zmq::message_t rq_desc(sizeof(request_desc));

        if (cont_flag == 1)
            break;

        EE("connect to %s. Enter your target database: (1-n)", WORKER_REQUEST);
        //cin >> rd.db_seq;
        rd.db_seq = HDD_RAW_180;

        EE("connect to %s. Enter start frame number: ", WORKER_REQUEST);
        //cin >> rd.start_fnum;
        rd.start_fnum = 0;

        EE("connect to %s. Enter total frame numbers: ", WORKER_REQUEST);
        //cin >> rd.total_fnum;
        rd.total_fnum = 40000;

        /* Sending requests */
        memcpy(rq_desc.data(), &rd, sizeof(request_desc));
        q_request.send(rq_desc);

        I("Request sent, db: %d, frames: %d", rd.db_seq, rd.total_fnum);

        k2_measure("receive begins");
        /* Receiving frames */
        for(int iter = 0; iter < rd.total_fnum; iter++) {
            data_desc desc;
            auto msg_ptr = recv_one_frame(receiver, &desc);
            //auto msg_ptr = recv_one_frame(q_request, &desc);

            if (!msg_ptr) {
                mg.DepositADesc(desc);
                I("got desc: %s", desc.to_string().c_str());
                if (desc.type == TYPE_CHUNK_EOF || desc.type == TYPE_RAW_FRAME_EOF || desc.type == TYPE_DECODED_FRAME_EOF) {
                    break;
                }
            } else {
                mg.DepositAFrame(desc, msg_ptr);
                //stat.inc_byte_counter((int) msg_ptr->size());
                //stat.inc_rec_counter(1);
            }
        }
        k2_measure("receive ends");

        k2_measure("retrieve begins");
        /* Retrieving frames */
        for(int i = rd.start_fnum; i < rd.start_fnum + rd.total_fnum; i++) {
            mg.GetWatermarks(&wm_c, &wm_f);

            //I("watermarks:  chunk %u frame %u", wm_c, wm_f);
            rc = mg.RetrieveAFrame(&f);

            frame[i] = f;

            if (rc == VS_ERR_EOF_CHUNKS)
                break;

            //xzl_bug_on(rc != 0);
        }
        k2_measure("retrieve ends");

        zmq::message_t reply(5);
        q_request.recv(&reply);

        //Frame frame_720p[72000];
        int height_720 = 720;
        int width_720 = 1280;
        int framesize_720 = height_720 * width_720;

        int height_ocr = 540;
        int width_ocr = 960;
        int framesize_ocr = height_ocr * width_ocr;

        /* Processing Frames*/
        k2_measure("alpr begins");

        zmq::socket_t *worker_socket[50];
        for(int worker_count = 0; worker_count < 50; worker_count++){
            worker_socket[worker_count]= new zmq::socket_t(context, ZMQ_REQ);
            worker_socket[worker_count]->connect(WORKER_REQUEST);
        }

        Frame fp[64];
        shared_ptr<zmq::message_t> pre_buf_720[50][2*CHUNK_SIZE];
        shared_ptr<zmq::message_t> pre_buf_540[50][2*CHUNK_SIZE];

        int status_720[50];
        int status_540[50];

        for(int iter = 0; iter < 50; iter ++){
            status_720[iter] = -2*CHUNK_SIZE-1;
            status_540[iter] = -2*CHUNK_SIZE-1;
        }

        #pragma omp parallel for schedule(dynamic, 100) num_threads(30)
        for (int j = rd.start_fnum; j < rd.start_fnum + rd.total_fnum; j++) {
            int id = omp_get_thread_num();
            cv::Mat frm;
            char *imagedata = nullptr;

            imagedata = static_cast<char *>(frame[j].msg_p->data()), frame[j].msg_p->size();
            //imagedata[id] = (char *)(frame[id*1440+j].msg_p->data()), frame[id*1440+j].msg_p->size();

            frm.create(height, width, CV_8UC1);
            memcpy(frm.data, imagedata, framesize);

            //bool plate_found = detectandshow(&alpr, frm, "", outputJson, id);
            std::vector<AlprRegionOfInterest> regionsOfInterest = detectmotion(alpr[id], frm, "", outputJson, id, j);

            zmq::message_t rq_desc720(sizeof(request_desc));
            request_desc rd_one_frame;
            zmq::message_t rq_descocr(sizeof(request_desc));
            request_desc rd_one_frame_ocr;

            //if (regionsOfInterest.size() > 0 && (j % 3 == 0)){
            if (regionsOfInterest.size() > 0){
                //Specify the frame wants to retrieve
                rd_one_frame.db_seq = NVME_RAW_720;
                rd_one_frame.start_fnum = j;
                rd_one_frame.total_fnum = CHUNK_SIZE;

                cv::Mat frm_720;
                Frame f720;

                //Specify the frame wants to retrieve
                rd_one_frame_ocr.db_seq = NVME_RAW_540;
                rd_one_frame_ocr.start_fnum = j;
                rd_one_frame_ocr.total_fnum = CHUNK_SIZE;

                cv::Mat frm_ocr;
                Frame focr;

                memcpy(rq_desc720.data(), &rd_one_frame, sizeof(request_desc));
                memcpy(rq_descocr.data(), &rd_one_frame_ocr, sizeof(request_desc));


                shared_ptr<zmq::message_t> msg_ptr;

                cv::Mat chunk;
                cv::Mat chunk_ocr;

                //worker_socket[id]->send(rq_desc720);
                //I("Request sent, db: %d, frame: %d", rd_one_frame.db_seq, rd_one_frame.start_fnum);

                /********** Initializing 2 CHUNKS of New Frames (720p)  **********/
#if 1
                if(j-status_720[id] > 2*CHUNK_SIZE){
                    worker_socket[id]->send(rq_desc720);
                    I("Initial request sent, db: %d, frame: %d", rd_one_frame.db_seq, rd_one_frame.start_fnum);
                    for (int iter = 0; iter < rd_one_frame.total_fnum; iter++) {
                        msg_ptr = recv_one_chunk_720(*worker_socket[id]);
                        I("Initial prefetch %d, thread %d", j, id);
                        pre_buf_720[id][iter] = msg_ptr;
                    }
                    //Specify the frame wants to retrieve
                    rd_one_frame.db_seq = NVME_RAW_720;
                    rd_one_frame.start_fnum = j + CHUNK_SIZE;
                    rd_one_frame.total_fnum = CHUNK_SIZE;
                    memcpy(rq_desc720.data(), &rd_one_frame, sizeof(request_desc));

                    worker_socket[id]->send(rq_desc720);
                    I("Initial request sent, db: %d, frame: %d", rd_one_frame.db_seq, rd_one_frame.start_fnum);
                    for (int iter = 0; iter < rd_one_frame.total_fnum; iter++) {
                        msg_ptr = recv_one_chunk_720(*worker_socket[id]);
                        I("Initial prefetch %d, thread %d", j, id);
                        pre_buf_720[id][iter+CHUNK_SIZE] = msg_ptr;
                    }
                    status_720[id] = j;
                }


                /********** Requesting 720 New Frames if exceeds 50% already **********/
                if(j - status_720[id] == CHUNK_SIZE) {
                    rd_one_frame.db_seq = NVME_RAW_720;
                    rd_one_frame.start_fnum = j+CHUNK_SIZE;
                    rd_one_frame.total_fnum = CHUNK_SIZE;
                    memcpy(rq_desc720.data(), &rd_one_frame, sizeof(request_desc));
                    worker_socket[id]->send(rq_desc720);
                    I("------prefetch720p %d, thread %d------", j, id);
                    I("Request sent, db: %d, frame: %d", rd_one_frame.db_seq, rd_one_frame.start_fnum);
                }

                msg_ptr = pre_buf_720[id][j%(2*CHUNK_SIZE)];

#endif

#if 0
                if(j - status_720[id] < CHUNK_SIZE){
                    msg_ptr = pre_buf_720[id][j%status_720[id]];
                }
                else {
                    status_720[id] = j;
                    worker_socket[id]->send(rq_desc720);
                    I("Request sent, db: %d, frame: %d", rd_one_frame.db_seq, rd_one_frame.start_fnum);

                    for (int iter = 0; iter < rd_one_frame.total_fnum; iter++) {
                        msg_ptr = recv_one_chunk_720(*worker_socket[id]);
                        //chunk = * (cv::Mat *)msg_ptr->data();
                        I("prefetch here %d, thread %d", j, id);
                        pre_buf_720[id][iter] = msg_ptr;
                    }
                    msg_ptr = pre_buf_720[id][j - status_720[id]];
                }
                //msg_ptr = recv_one_frame_720(*worker_socket[id]);
#endif
                char *imagedata_720 = NULL;

                imagedata_720 = (char*)(msg_ptr->data()), msg_ptr->size();

                frm_720.create(height_720, width_720, CV_8UC1);
                //frm_720.create(height_ocr, width_ocr, CV_8UC1);
                memcpy(frm_720.data, imagedata_720, framesize_720);


                queue<PlateRegion> plateQueue = recognizeRegion(alpr[id], frm_720, "", outputJson, id, j, regionsOfInterest);
                I("plateQueue size = %d", plateQueue.size());

                /********** Prefetching 720 New Frames after the computation **********/
                if(j - status_720[id] == CHUNK_SIZE) {
                    for (int iter = 0; iter < rd_one_frame.total_fnum; iter++) {
                        msg_ptr = recv_one_chunk_720(*worker_socket[id]);
                        I("Initial prefetch %d, thread %d", j, id);
                        pre_buf_720[id][(j - status_720[id] + iter) % (2*CHUNK_SIZE)] = msg_ptr;
                    }
                    status_720[id] = j;
                }


                if(plateQueue.size() > 0) {
                    //I("size = %d",plateQueue.size());
#if 1
                    /********** Initializing 2 CHUNKS of New Frames (540p)  **********/

                    if(j-status_540[id] > 2*CHUNK_SIZE){
                        worker_socket[id]->send(rq_descocr);
                        I("Initial request sent, db: %d, frame: %d", rd_one_frame_ocr.db_seq, rd_one_frame_ocr.start_fnum);
                        for (int iter = 0; iter < rd_one_frame_ocr.total_fnum; iter++) {
                            msg_ptr = recv_one_chunk_720(*worker_socket[id]);
                            I("Initial prefetch %d, thread %d", j, id);
                            pre_buf_540[id][iter] = msg_ptr;
                        }
                        //Specify the frame wants to retrieve
                        rd_one_frame_ocr.db_seq = NVME_RAW_540;
                        rd_one_frame_ocr.start_fnum = j + CHUNK_SIZE;
                        rd_one_frame_ocr.total_fnum = CHUNK_SIZE;
                        memcpy(rq_descocr.data(), &rd_one_frame_ocr, sizeof(request_desc));

                        worker_socket[id]->send(rq_descocr);
                        I("Initial request sent, db: %d, frame: %d", rd_one_frame_ocr.db_seq, rd_one_frame_ocr.start_fnum);
                        for (int iter = 0; iter < rd_one_frame_ocr.total_fnum; iter++) {
                            msg_ptr = recv_one_chunk_720(*worker_socket[id]);
                            I("Initial prefetch %d, thread %d", j, id);
                            pre_buf_540[id][iter+CHUNK_SIZE] = msg_ptr;
                        }
                        status_540[id] = j;
                    }

                    /********** Requesting 540 New Frames if exceeds 50% already **********/
                    if (j - status_540[id] == CHUNK_SIZE) {
                        rd_one_frame_ocr.db_seq = NVME_RAW_540;
                        rd_one_frame_ocr.start_fnum = j + CHUNK_SIZE;
                        rd_one_frame_ocr.total_fnum = CHUNK_SIZE;
                        memcpy(rq_descocr.data(), &rd_one_frame_ocr, sizeof(request_desc));
                        worker_socket[id]->send(rq_descocr);
                        I("------prefetch540p %d, thread %d------", j, id);
                        I("Request sent, db: %d, frame: %d", rd_one_frame_ocr.db_seq, rd_one_frame_ocr.start_fnum);
                    }

                    msg_ptr = pre_buf_540[id][j % (2 * CHUNK_SIZE)];

#endif
                    /********** Exactly the same retrieving process **********/

                    memcpy(rq_descocr.data(), &rd_one_frame_ocr, sizeof(request_desc));

                    //worker_socket[id]->send(rq_descocr);
                    //I("Request sent, db: %d, frame: %d", rd_one_frame_ocr.db_seq, rd_one_frame_ocr.start_fnum);
#if 0
                    if(j - status_540[id] < CHUNK_SIZE){
                        I("enter here status = %d", status_540[id]);
                        msg_ptr = pre_buf_540[id][j-status_540[id]];
                    }
                    else {
                        status_540[id] = j;
                        worker_socket[id]->send(rq_descocr);
                        I("Request sent, db: %d, frame: %d", rd_one_frame_ocr.db_seq, rd_one_frame_ocr.start_fnum);

                        for (int iter = 0; iter < rd_one_frame_ocr.total_fnum; iter++) {
                            msg_ptr = recv_one_chunk_720(*worker_socket[id]);
                            //chunk = * (cv::Mat *)msg_ptr->data();
                            I("prefetch here %d, thread %d", j, id);
                            pre_buf_540[id][iter] = msg_ptr;
                        }
                        msg_ptr = pre_buf_540[id][j - status_540[id]];
                    }
                    //msg_ptr = recv_one_frame_720(*worker_socket[id]);
#endif
                    char *imagedata_ocr = NULL;

                    //imagedata_ocr = static_cast<char *>(fp[id].msg_p->data()), fp[id].msg_p->size();
                    imagedata_ocr = (char *) (msg_ptr->data()), msg_ptr->size();

                    frm_ocr.create(height_ocr, width_ocr, CV_8UC1);
                    //frm_ocr.create(height_720, width_720, CV_8UC1);
                    memcpy(frm_ocr.data, imagedata_ocr, framesize_ocr);

                    I("OCR frame height: %d, width: %d", frm_ocr.rows, frm_ocr.cols);

                    getTimeMonotonic(&startTime);
                    //bool plate_found = recognizeplate(alpr[id], frm_720, "", outputJson, id, j, regionsOfInterest);
                    //bool plate_found = recognizeplate(alpr[id], frm_720, frm_ocr, "", outputJson, id, j, regionsOfInterest);
                    //plate_found = recognizeplate(alpr[id], frm_ocr, frm_720, "", outputJson, id, j, regionsOfInterest);

                    cv::Mat frm_ocr_gray = frm_ocr;
                    if (frm_ocr.channels() > 2) {
                        cvtColor(frm_ocr, frm_ocr_gray, CV_BGR2GRAY);
                    }
                    //I("OCR frame height: %d, width: %d", frm_ocr_gray.rows, frm_ocr_gray.cols);

                    bool plate_found = recognizePlate(alpr[id], frm_ocr, frm_ocr_gray, "", outputJson, id, j, plateQueue);

                    if (!plate_found && !outputJson) {
                        std::cout << "No license plates found." << std::endl;
                    }
                    getTimeMonotonic(&endTime);
                    std::cout << "Plate Recognition: " << diffclock(startTime, endTime) << "ms." << std::endl;
#if 1
                    /********** Prefetching 540 New Frames after thr computation **********/
                    if (j - status_540[id] == CHUNK_SIZE) {
                        for (int iter = 0; iter < rd_one_frame_ocr.total_fnum; iter++) {
                            I("Initial prefetch %d, thread %d", j, id);
                            msg_ptr = recv_one_chunk_720(*worker_socket[id]);
                            pre_buf_540[id][(j - status_540[id] + iter) % (2*CHUNK_SIZE)] = msg_ptr;
                        }
                        status_540[id] = j;
                    }
#endif
                }

            }
        }
        k2_measure("alpr ends");
        k2_measure_flush();
        cout << "count = " << mycount << endl;
    }
    //}
	/* XXX stop the stat collector */
	stat_collector.stop();

	return 0;
}
