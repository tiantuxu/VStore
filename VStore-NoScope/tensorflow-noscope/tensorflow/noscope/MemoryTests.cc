#include <sys/mman.h>

#include <chrono>
#include <ctime>
#include <random>
#include <algorithm>
#include <iterator>

#include "tensorflow/core/public/session.h"
#include "tensorflow/core/platform/env.h"

#include "tensorflow/cc/ops/standard_ops.h"
#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/graph/default_device.h"

#include "cuda/include/cuda.h"

#include "tensorflow/noscope/MemoryTests.h"
#include <iostream>
#include <fstream>
using namespace std;
namespace noscope {

using namespace tensorflow;

void MemoryLocationTests() {
  const int N = 2500;
  Tensor input(DT_FLOAT, TensorShape({N, 50, 50, 3}));
  auto input_mapped = input.tensor<float, 4>();

  std::cout << &input_mapped(1, 0, 0, 0) - &input_mapped(0, 0, 0, 0) << "\n";
}

void GPUCopyTest(Session* session) {
  const size_t N = 100000;
  Tensor input(DT_FLOAT, TensorShape({N, 50, 50, 3}));

  Scope root = Scope::NewRootScope();
  using namespace ::tensorflow::ops;
  auto inp = Placeholder(root.WithOpName("input"), DT_FLOAT);
  auto id = Identity(root.WithOpName("output"), inp);

  GraphDef def;
  TF_CHECK_OK(root.ToGraphDef(&def));
  tensorflow::graph::SetDefaultDevice("/gpu:0", &def);
  session->Create(def);

  std::vector<tensorflow::Tensor> outputs;
  std::vector<std::pair<string, tensorflow::Tensor> > inputs = {
    {"input", input}
  };

  if (mlock(&(input.tensor<float, 4>()(0, 0, 0, 0)), N * 50 * 50 * 3 * 4) != 0) {
    int errsv = errno;
    std::cout << "mlock failed: " << errsv << "\n";
  }

  auto start = std::chrono::high_resolution_clock::now();
  session->Run(inputs, {"output:0"}, {}, &outputs);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end-start;
  std::cout << "Time: " << diff.count() << " s" << std::endl;

  std::cout << outputs[0].DebugString() << "\n";
  std::cout << outputs.size() << "\n";
}

} // namespace noscope
