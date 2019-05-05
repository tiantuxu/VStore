# pkg-config --cflags opencv
# pkg-config --libs opencv

load(
    "@local_config_cuda//cuda:build_defs.bzl",
    "if_cuda",
    "if_cuda_is_configured"
)

load(
    "//tensorflow:tensorflow.bzl",
    "tf_cc_test",
    "tf_cuda_library"
)

# tf_cuda_library(
cc_binary(
    name = "noscope",
    copts = [
	"-I/usr/local/include",
	"-I/usr/local/include/libavutil",
	"-I/usr/local/include/libavdevice",
	"-I/usr/local/include/libavfilter",
	"-I/usr/local/include/libavcodec",
	"-I/usr/local/include/libavformat",
	"-I/usr/local/include/libswresample",
	"-I/usr/local/include/libswscale",
 	"-I/usr/local/include/opencv2",
 	"-I/usr/local/include/opencv",
    "-O3",
    #"-O0",
    #"-g",
    #"-Wall",
    "-fopenmp"
    ] + if_cuda(["-DGOOGLE_CUDA=1"]),
    linkopts = [
        "-fopenmp",
        "-L/usr/local/lib",
        "-lopencv_core",
        "-lopencv_imgcodecs",
        "-lopencv_imgproc",
        "-lopencv_highgui",
        "-lopencv_videoio",
        "-lavcodec",
        "-lavutil",
        "-lavformat",
        "-lavdevice",
        "-lavfilter",
        "-lswresample",
        "-lswscale",
        "-lzmq",
        "-lboost_serialization"
    ],
    srcs =glob(["*.h"]) + glob(["*.c"]) + glob(["*.cc"], exclude=["dumper.cc"]),
    deps = [
        "//tensorflow/core:tensorflow",
        "//tensorflow/cc:cc_ops",
        "//tensorflow/cc:ops",
        "//tensorflow/cc:scope",
        "//tensorflow/noscope/darknet:yolo",
        "//tensorflow/core:cuda",
        "//tensorflow/core:gpu_lib",
        #"//tensorflow/noscope/ffmpeg/include:ffmpeg",
        "@local_config_cuda//cuda:cuda_headers"
    ]
)

cc_binary(
    name = "dumper",
    copts = [
        "-I/usr/local/include", "-I/usr/local/include/opencv2", "-I/usr/local/include/opencv",
        "-O3", "-fopenmp"
    ],
    linkopts = [
        "-fopenmp",
        "-L/usr/local/lib",
        "-lopencv_core",
        "-lopencv_imgcodecs",
        "-lopencv_imgproc",
        "-lopencv_highgui",
        "-lopencv_videoio",
    ],
    srcs = glob(["noscope_data.h"]) + glob(["noscope_data.cc", "dumper.cc"]),
    deps = [
        "//tensorflow/core:tensorflow",
        "//tensorflow/cc:scope",
    ]
)
