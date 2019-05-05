#include <sys/mman.h>

#include <chrono>
#include <ctime>
#include <random>
#include <algorithm>
#include <iterator>
#include <memory>

#include "tensorflow/core/util/command_line_flags.h"

#include "tensorflow/noscope/noscope_data.h"
#include <iostream>
#include <fstream>
using namespace std;
using tensorflow::Flag;

static bool file_exists(const std::string& name) {
  std::ifstream f(name.c_str());
  return f.good();
}

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

int main(int argc, char* argv[]) {
  std::string video;
  std::string skip;
  std::string nb_frames;
  std::string start_from;
  std::string dumped_videos;
  std::vector<Flag> flag_list = {
      Flag("video", &video, "Video to load"),
      Flag("skip", &skip, "Number of frames to skip"),
      Flag("nb_frames", &nb_frames, "Number of frames to read"),
      Flag("start_from", &start_from, "Where to start from"),
      Flag("dumped_videos", &dumped_videos, ""),
  };
  std::string usage = tensorflow::Flags::Usage(argv[0], flag_list);
  const bool parse_result = tensorflow::Flags::Parse(&argc, argv, flag_list);
  const size_t kSkip = std::stoi(skip);
  const size_t kNbFrames = std::stoi(nb_frames);
  const size_t kStartFrom = std::stoi(start_from);

  if (!parse_result) {
    // LOG(ERROR) << usage;
    std::cerr << "Parsing arguments failed\n";
    return -1;
  }

  noscope::NoscopeData *data = LoadVideo(video, dumped_videos, kSkip, kNbFrames, kStartFrom);
  return 0;
}
