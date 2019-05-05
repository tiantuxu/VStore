//===========================================From sink====================================
//
// Created by xzl on 12/24/17.
//
// tester: pull frames from a

#ifdef DEBUG
#define GLIBCXX_DEBUG 1 // debugging
#endif
/*
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
#include "rt-config.h"*/

//=========================================From NoScope==================================
#include <zmq.hpp>

#include <sys/mman.h>

#include <chrono>
#include <ctime>
#include <random>
#include <algorithm> //luis: duplicated
#include <iterator> //luis: duplicated
#include <memory>

#include "tensorflow/core/public/session.h"
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/graph/default_device.h"
#include "tensorflow/core/util/command_line_flags.h"

#include "tensorflow/noscope/mse.h"
#include "tensorflow/noscope/filters.h"
#include "tensorflow/noscope/MemoryTests.h"
#include "tensorflow/noscope/noscope_labeler.h"
#include "tensorflow/noscope/noscope_data.h"
#include "tensorflow/noscope/darknet/src/yolo.h"


#include "tensorflow/noscope/config.h"
#include "tensorflow/noscope/rt-config.h"
#include "tensorflow/noscope/vs-types.h"
//#include "mydecoder.h"
#include "tensorflow/noscope/log.h"
#include "tensorflow/noscope/rxtx.h"
#include "tensorflow/noscope/RxManager.h"
#include "tensorflow/noscope/measure.h"
//#include <iostream> //luis: duplicated
//#include <fstream>

//===========================================From sink====================================
extern "C"{
#include <omp.h>
};

using namespace vs;
//size_t reso;

//=========================================From NoScope==================================
//using namespace std;
using tensorflow::Flag;
/*
static bool file_exists(const std::string& name) {
  std::ifstream f(name.c_str());
  return f.good();
}
*/
//===========================================From sink====================================
const std::string MAIN_WINDOW_NAME = "ALPR main window";

const bool SAVE_LAST_VIDEO_STILL = false;
const std::string LAST_VIDEO_STILL_LOCATION = "/tmp/laststill.jpg";
const std::string WEBCAM_PREFIX = "/dev/video";
//MotionDetector motiondetector;
//MotionDetector motiondetector[64];
//bool do_motiondetection = true;

/** Function Headers */
//bool detectandshow(Alpr* alpr, cv::Mat frame, std::string region, bool writeJson);
//bool is_supported_image(std::string image_file);

//bool measureProcessingTime = true;
//std::string templatePattern;

// This boolean is set to false when the user hits terminates (e.g., CTRL+C )
// so we can end infinite loops for things like video processing.
bool program_active = true;

bool flag_180[50];
bool flag_720[50]={true};                           //flag indicate that it is currently feeding 720p video chunk to the client
bool flag_540[50];
const int chunk_num = 1;
const int chunk_num_encode = 1;

static std::shared_ptr<zmq::message_t> pre_buf_180[50][PREFETCH_BUF];
static std::shared_ptr<zmq::message_t> pre_buf_720[50][PREFETCH_BUF];
static std::shared_ptr<zmq::message_t> pre_buf_540[50][PREFETCH_BUF];

static std::shared_ptr<zmq::message_t> pre_enbuf_180[50][CHUNK_LENGTH];
static std::shared_ptr<zmq::message_t> pre_enbuf_720[50][CHUNK_LENGTH];
static std::shared_ptr<zmq::message_t> pre_enbuf_540[50][CHUNK_LENGTH];
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
vs::Frame frame[3][3600];

std::atomic<int> mycount(0);
std::atomic<int> mycount_rr(0);
std::atomic<int> mycount_ocr(0);

std::atomic<int> count1(0);
std::atomic<int> count2(0);
std::atomic<int> count3(0);

std::atomic<int> chunk_count(0);
#if 0
std::vector<AlprRegionOfInterest> detectmotion( Alpr* alpr, cv::Mat frame, std::string region, bool writeJson, int proc_id, int f_seq)
//bool detectandshow( Alpr* alpr, cv::Mat frame, std::string region, bool writeJson)
{
    timespec startTime;
    timespec endTime;

    //count1 ++;

    std::vector<AlprRegionOfInterest> regionsOfInterest;
#if 1
    int ratio = 720 / 180;
#endif
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
        //regionsOfInterest.push_back(AlprRegionOfInterest(ratio*rectan.x, ratio*rectan.y, ratio*rectan.width, ratio*rectan.height));
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
    timespec startTime;
    timespec endTime;
    /* Recognition phase */
    getTimeMonotonic(&startTime);
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

    //I("%d platequeue.size = %lu", f_seq, plateQueue.size());
    //cout << f_seq << " " << plateQueue.size() << endl;

    //results = alpr->recognize(frame_720.data, frame_720.elemSize(), frame_720.cols, frame_720.rows, regionsOfInterest);

    /* print the ROI */
    //cout << regionsOfInterest[0].x << " " << regionsOfInterest[0].y << " " << regionsOfInterest[0].width << " " << regionsOfInterest[0].height << " " << endl;

    getTimeMonotonic(&endTime);
    //std::cout << "Total Plate Recognition: " << diffclock(startTime, endTime) << "ms." << std::endl;
    count2 ++;

    return plateQueue;

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

    timespec startTime;
    timespec endTime;

    getTimeMonotonic(&startTime);

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

    getTimeMonotonic(&endTime);
    count3 ++;

    return results.plates.size() > 0;
}


void init_raw_chunks(int j, int id, int *status, shared_ptr<zmq::message_t> prefetch[][chunk_num*CHUNK_SIZE], zmq::socket_t **work_skt, int db_seq, request_desc *rd_one_frame, bool *flag){

    zmq::message_t request(sizeof(request_desc));

    memcpy(request.data(), rd_one_frame, sizeof(request_desc));

    /********* Initializing 5 CHUNKS of Raw Frames **********/
    if(j-status[id] > chunk_num*CHUNK_SIZE){
        for(int i=0;i<chunk_num;i++)
        {
            work_skt[id]->send(request);
            request_desc *rd;
            rd = (request_desc *)(request.data()), request.size();
            I("Initial request sent, start: %d, frame: %d", rd->start_fnum, rd->total_fnum);

            for (int iter = 0; iter < rd->total_fnum; iter++) {
                auto msg_ptr = recv_one_chunk_720(*work_skt[id]);
                prefetch[id][iter+CHUNK_SIZE*i] = msg_ptr;
            }
            rd->db_seq = db_seq;
            rd->start_fnum = j + CHUNK_SIZE*(i+1);
            rd->total_fnum = CHUNK_SIZE;
            memcpy(request.data(), &rd, sizeof(request_desc));

        }
        status[id] = j;
        flag[id]=false;
    }

    if(j-status[id]<=CHUNK_SIZE)
        flag[id]=false;

}
#endif
//=====================================From NoScope======================================


static bool file_exists(const std::string& name) {
  std::ifstream f(name.c_str());
  return f.good();
}

static void SpeedTests() {
  const size_t kImgSize = 100 * 100 * 3;
  const size_t kDelay = 10;
  const size_t kFrames = 100000;
  const size_t kNumThreads = 32;
  std::vector<uint8_t> speed_tests(kFrames * kImgSize);
  {
    auto start = std::chrono::high_resolution_clock::now();
    #pragma omp parallel for num_threads(kNumThreads)
    for (size_t i = kDelay; i < kFrames; i++) {
      noscope::filters::GlobalMSE(&speed_tests[i * kImgSize], &speed_tests[(i - kDelay) * kImgSize]);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "BlockedMSE: " << diff.count() << " s" << std::endl;
  }
  {
    auto start = std::chrono::high_resolution_clock::now();
    #pragma omp parallel for num_threads(kNumThreads)
    for (size_t i = kDelay; i < kFrames; i++) {
      noscope::filters::BlockedMSE(&speed_tests[i * kImgSize], &speed_tests[100 * kImgSize]);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "GlobalMSE: " << diff.count() << " s" << std::endl;
  }
}

static tensorflow::Session* InitSession(const std::string& graph_fname) {
  tensorflow::Session *session;
  tensorflow::SessionOptions opts;
  tensorflow::GraphDef graph_def;
  // YOLO needs some memory
  opts.config.mutable_gpu_options()->set_per_process_gpu_memory_fraction(0.9);
  // opts.config.mutable_gpu_options()->set_allow_growth(true);
  tensorflow::Status status = NewSession(opts, &session);
  TF_CHECK_OK(status);

  status = tensorflow::ReadBinaryProto(
      tensorflow::Env::Default(),
      graph_fname, &graph_def);
  tensorflow::graph::SetDefaultDevice("/gpu:0", &graph_def);
  TF_CHECK_OK(status);

  status = session->Create(graph_def);
  TF_CHECK_OK(status);

  return session;
}

static noscope::NoscopeData* LoadVideo(vs::Frame frame[][3600], const std::string& dumped_videos,
                                 const int kSkip, const int kNbFrames, const int kStartFrom) {
  auto start = std::chrono::high_resolution_clock::now();
  noscope::NoscopeData *data = NULL;

  if (dumped_videos == "/dev/null") {
    data = new noscope::NoscopeData(frame, kSkip, kNbFrames, kStartFrom);
  } /*else {
    if (file_exists(dumped_videos)) {
      std::cerr << "Loading dumped video\n";
      data = new noscope::NoscopeData(dumped_videos);
    } else { //Luis: This is always running
      std::cerr << "Dumping video\n";
      data = new noscope::NoscopeData(frame, kSkip, kNbFrames, kStartFrom);
      data->DumpAll(dumped_videos);
    }
  }*/
  auto end = std::chrono::high_resolution_clock::now();
  std::cout << "Loaded video\n";
  std::chrono::duration<double> diff = end - start;
  std::cout << "Time to load (and resize) video: " << diff.count() << " s" << std::endl;
  return data;
}

/*
static noscope::NoscopeData* LoadVideo(const std::string& video, const std::string& dumped_videos,
                                 const int kSkip, const int kNbFrames, const int kStartFrom) {
  auto start = std::chrono::high_resolution_clock::now();
  noscope::NoscopeData *data = NULL;
  if (dumped_videos == "/dev/null") {
    data = new noscope::NoscopeData(video, kSkip, kNbFrames, kStartFrom);
  } else {
    if (file_exists(dumped_videos)) {
      std::cerr << "Loading dumped video\n";
      data = new noscope::NoscopeData(dumped_videos);
    } else {
      std::cerr << "Dumping video\n";
      data = new noscope::NoscopeData(video, kSkip, kNbFrames, kStartFrom);
      data->DumpAll(dumped_videos);
    }
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::cout << "Loaded video\n";
  std::chrono::duration<double> diff = end - start;
  std::cout << "Time to load (and resize) video: " << diff.count() << " s" << std::endl;
  return data;
}
*/
noscope::filters::DifferenceFilter GetDiffFilter(const bool kUseBlocked,
                                              const bool kSkipDiffDetection) {
  noscope::filters::DifferenceFilter nothing{noscope::filters::DoNothing, "DoNothing"};
  noscope::filters::DifferenceFilter blocked{noscope::filters::BlockedMSE, "BlockedMSE"};
  noscope::filters::DifferenceFilter global{noscope::filters::GlobalMSE, "GlobalMSE"};

  if (kSkipDiffDetection) {
    return nothing;
  }
  if (kUseBlocked) {
    return blocked;
  } else {
    return global;
  }
}

int main (int argc, char *argv[]){
    //Continue working here (luis)
//======================================+From noscope====================================================
  std::string graph, video;
  std::string yolo_cfg, yolo_weights;
  std::string avg_fname;
  std::string confidence_csv;
  std::string diff_thresh_str;
  std::string distill_thresh_lower_str, distill_thresh_upper_str;
  std::string skip;
  std::string nb_frames;
  std::string start_from;
  std::string yolo_class;
  std::string skip_small_cnn;
  std::string skip_diff_detection;
  std::string dumped_videos;
  std::string diff_detection_weights;
  std::string use_blocked;
  std::string ref_image;
  std::string resolution;
  std::vector<Flag> flag_list = {
      Flag("graph", &graph, "Graph to be executed"),
      Flag("video", &video, "Video to load"),
      Flag("yolo_cfg", &yolo_cfg, "YOLO config file"),
      Flag("yolo_weights", &yolo_weights, "YOLO weights file"),
      Flag("avg_fname", &avg_fname, "Filename with the average (txt)"),
      Flag("confidence_csv", &confidence_csv, "CSV to output confidences to"),
      Flag("diff_thresh", &diff_thresh_str, "Difference filter threshold"),
      Flag("distill_thresh_lower", &distill_thresh_lower_str, "Distill threshold (lower)"),
      Flag("distill_thresh_upper", &distill_thresh_upper_str, "Distill threshold (upper)"),
      Flag("skip", &skip, "Number of frames to skip"),
      Flag("nb_frames", &nb_frames, "Number of frames to read"),
      Flag("start_from", &start_from, "Where to start from"),
      Flag("yolo_class", &yolo_class, "YOLO class"),
      Flag("skip_small_cnn", &skip_small_cnn, "0/1 skip small CNN or not"),
      Flag("skip_diff_detection", &skip_diff_detection, "0/1 skip diff detection or not"),
      Flag("dumped_videos", &dumped_videos, ""),
      Flag("diff_detection_weights", &diff_detection_weights, "Difference detection weights"),
      Flag("use_blocked", &use_blocked, "0/1 whether or not to use blocked DD"),
      Flag("ref_image", &ref_image, "reference image"),
      Flag("resolution", &resolution, "resolution")
  };

  std::string usage = tensorflow::Flags::Usage(argv[0], flag_list);
  const bool parse_result = tensorflow::Flags::Parse(&argc, argv, flag_list);
  const float diff_thresh = std::stof(diff_thresh_str);
  const float distill_thresh_lower = std::stof(distill_thresh_lower_str);
  const float distill_thresh_upper = std::stof(distill_thresh_upper_str);
  const size_t kSkip = std::stoi(skip);
  const size_t kNbFrames = std::stoi(nb_frames);
  const size_t kStartFrom = std::stoi(start_from);
  const int kYOLOClass = std::stoi(yolo_class);
  const bool kSkipSmallCNN = std::stoi(skip_small_cnn);
  const bool kSkipDiffDetection = std::stoi(skip_diff_detection);
  const bool kUseBlocked = std::stoi(use_blocked);
  const size_t kRefImage = std::stoi(ref_image);
  const size_t res = std::stoi(resolution);
    if (!parse_result) {
    LOG(ERROR) << usage;
    return -1;
  }

  //reso = res;
  //I("================== resolution = %d x %d ====================", reso, reso);

//========================================================From sink==================================================================
	zmq::context_t context (1 /* # io threads */);

//	zmq::socket_t receiver(context, ZMQ_PULL);
//	receiver.connect(CLIENT_PULL_FROM_ADDR);
//	receiver.bind(FRAME_PULL_ADDR);

    zmq::socket_t q_request(context, ZMQ_REQ);
    q_request.connect(WORKER_REQUEST);

    zmq::socket_t s_request(context, ZMQ_REQ);
    s_request.connect(SERVICE_PUSH_ADDR);

    zmq::socket_t *service_socket[1];
    for(int worker_count = 0; worker_count < 1; worker_count++){
        service_socket[worker_count]= new zmq::socket_t(context, ZMQ_REQ);
        service_socket[worker_count]->connect(SERVICE_PUSH_ADDR);
    }


	//CallBackTimer stat_collector;
	RxManager mg;

	//stat_collector.start(200 /*ms, check interval */, &getStatistics);

	/* consumption phase */
	//Frame f;
	//int rc;
	//seq_t wm_c, wm_f;

    //bool debug_mode = false;

    int cont_flag = 0;

    zmq::socket_t *worker_socket[THREAD_NUM];
    for(int worker_count = 0; worker_count < THREAD_NUM; worker_count++){
        worker_socket[worker_count]= new zmq::socket_t(context, ZMQ_PULL);
        worker_socket[worker_count]->bind(raw_pull_address[worker_count].c_str());
    }

    zmq::socket_t *worker_socket3[THREAD_NUM];
    for(int worker_count = 0; worker_count < THREAD_NUM; worker_count++){
        worker_socket3[worker_count]= new zmq::socket_t(context, ZMQ_PULL);
        worker_socket3[worker_count]->bind(raw3_pull_address[worker_count].c_str());
    }

    zmq::socket_t *worker_chunk_socket[THREAD_NUM];
    for(int worker_count = 0; worker_count < THREAD_NUM; worker_count++){
        worker_chunk_socket[worker_count]= new zmq::socket_t(context, ZMQ_PULL);
        worker_chunk_socket[worker_count]->bind(en_pull_address[worker_count].c_str());
        std::cout << en_pull_address[worker_count].c_str() << std::endl;
    }

    //k2_measure("alpr begins");

    I("Start to receive frames");

    while(1){
        vs::Frame f;
        //vs::Frame frame[3][3000];

        request_desc rd;
        vs::sv ser;

        for(int ds = 0; ds < 1; ds++) {

            rd.db_seq = 18 + 2 * ds + 2;

            rd.start_fnum = 0;
            rd.total_fnum = std::stoi(nb_frames);
            std::cout << ds << " " << std::stoi(nb_frames) << std::endl;
            rd.id = 0;
            rd.temp = 0;
#if 1
            // Sending requests using service type
            zmq::message_t rq_desc(sizeof(sv));
            ser.type = RETRIEVAL;
            memcpy(&ser.rq, &rd, sizeof(request_desc));
            memcpy(rq_desc.data(), &ser, sizeof(sv));
            s_request.send(rq_desc);

            EE("Request sent, db: %d, frames: %d", ser.rq.db_seq, ser.rq.total_fnum);

            //k2_measure("receive begins");
            // Receiving frames
            for (int iter = 0; iter < rd.total_fnum; iter++) {
                data_desc desc;
                std::shared_ptr<zmq::message_t> msg_ptr;

                if(stage[ds] == 1) {
                    msg_ptr = recv_one_frame(*worker_socket[rd.id], &desc);
                }
                else{
                    msg_ptr = recv_one_frame(*worker_chunk_socket[rd.id], &desc);
                }

                if (!msg_ptr) {
                    //mg.DepositADesc(desc);
                    frame[ds][count1+iter].msg_p = nullptr;
                    //I("got desc: %s", desc.to_string().c_str());
                    if (desc.type == TYPE_CHUNK_EOF || desc.type == TYPE_RAW_FRAME_EOF ||
                        desc.type == TYPE_DECODED_FRAME_EOF) {
                        if (chunk_count == rd.total_fnum/CHUNK_LENGTH - 1) {
                            break;
                        }
                        else{
                            chunk_count ++;
                            continue;
                        }
                    }
                } else {
                    f.desc = desc;
                    f.msg_p = msg_ptr;
                    frame[ds][count1+iter] = f;
                    //mg.DepositAFrame(desc, msg_ptr);
                    //stat.inc_byte_counter((int) msg_ptr->size());
                    //stat.inc_rec_counter(1);
                }
            }
            //k2_measure("receive ends"); //Stops receiving frames

            zmq::message_t reply(5);
            s_request.recv(&reply);

            //count1 += int(stagePer[ds] * TEMP_NUM[ds / 3]);
        }
#endif
 
        //Second request================================================================
        
        //Third request==============================================================
/*
        int height_180 = 180;
        int width_180 = 320;
        int framesize_180 = height_180 * width_180;

        int height_720 = 720;
        int width_720 = 1280;
        int framesize_720 = height_720 * width_720;

        int height_ocr = 540;
        int width_ocr = 960;
        int framesize_ocr = height_ocr * width_ocr;
*/
        Frame fp[64];

        int status_180[50];
        int status_720[50];
        int status_540[50];

        int status_en_180[50];
        int status_en_720[50];
        int status_en_540[50];

        for(int iter = 0; iter < 50; iter ++){
            status_180[iter] = -PREFETCH_BUF-1;
            status_720[iter] = -PREFETCH_BUF-1;
            status_540[iter] = -PREFETCH_BUF-1;
        }

        for(int iter = 0; iter < 50; iter ++){
            status_en_180[iter] = -CHUNK_LENGTH-1;
            status_en_720[iter] = -CHUNK_LENGTH-1;
            status_en_540[iter] = -CHUNK_LENGTH-1;
        }

        //#pragma omp parallel for schedule(dynamic, CHUNK_LENGTH * 6) num_threads(THREAD_NUM)
        //#pragma omp parallel for schedule(dynamic, 250) num_threads(THREAD_NUM)

        //==============================From noscope (initializaions)=====================================================================//

  	if (diff_detection_weights != "/dev/null" && !kSkipDiffDetection) {  //Load weights
   	 noscope::filters::LoadWeights(diff_detection_weights);
  	}



  	tensorflow::Session *session = InitSession(graph);    //Initialize tensorflow


  	yolo::YOLO *yolo_classifier = new yolo::YOLO(yolo_cfg, yolo_weights, kYOLOClass);  //Initialize YOLO


//========================================Load video======================================================
        //+++++++ This is the function I need to change (luis) fix noscope_data.cc
        //I("loadvideo");
        noscope::NoscopeData *data = LoadVideo(frame, dumped_videos, kSkip, kNbFrames, kStartFrom);
        //I("video loaded");
        noscope::filters::DifferenceFilter df = GetDiffFilter(kUseBlocked, kSkipDiffDetection);

        noscope::NoscopeLabeler labeler = noscope::NoscopeLabeler(
           session,
           yolo_classifier,
           df,
           avg_fname,
           *data);

  	std::cerr << "Loaded NoscopeLabeler\n";
 
  	auto start = std::chrono::high_resolution_clock::now();

  	labeler.RunDifferenceFilter(diff_thresh, 10000000, kUseBlocked, kRefImage); //Difference filter

  	auto diff_end = std::chrono::high_resolution_clock::now();

    //continue;

  	if (!kSkipSmallCNN) {
  	  labeler.PopulateCNNFrames();    //Populate small CNN frames
   	  labeler.RunSmallCNN(distill_thresh_lower, distill_thresh_upper);  //Run Small CNN
  	}

    continue;

  	auto dist_end = std::chrono::high_resolution_clock::now();

    labeler.RunYOLO(true); //Run YOLO

  	auto yolo_end = std::chrono::high_resolution_clock::now();
 	 std::vector<double> runtimes(4);
 	 {
  	  std::chrono::duration<double> diff = yolo_end - start;
  	  std::cout << "Total time: " << diff.count() << " s" << std::endl;

   	 diff = diff_end - start;
  	  runtimes[0] = diff.count();
  	  diff = dist_end - start;
   	 runtimes[1] = diff.count();
   	 diff = yolo_end - start;
   	 runtimes[2] = diff.count();
   	 runtimes[3] = diff.count();
 	 }
  	runtimes[2] -= runtimes[1];
  	runtimes[1] -= runtimes[0];

  	labeler.DumpConfidences(confidence_csv,
                          graph,
                          kSkip,
                          kSkipSmallCNN,
                          diff_thresh,
                          distill_thresh_lower,
                          distill_thresh_upper,
                          runtimes);
    

    exit(0);


    }//End while(1) loop

    return 0;
}