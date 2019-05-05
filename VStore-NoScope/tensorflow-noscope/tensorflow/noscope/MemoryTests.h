#ifndef TENSORFLOW_VUSE_MEMORYTESTS_H_
#define TENSORFLOW_VUSE_MEMORYTESTS_H_

#include "tensorflow/core/public/session.h"
#include "tensorflow/core/platform/env.h"

#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/graph/default_device.h"

namespace noscope {

using namespace tensorflow;

void MemoryLocationTests();

void GPUCopyTest(Session* session);

} // namespace noscope

#endif // TENSORFLOW_VUSE_MEMORYTESTS_H_
