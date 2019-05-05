#!/bin/bash

if [[ -z $1 || -z $2 || -z $3 || -z $4 || -z $5 ]]; then
    echo "Usage: $0 GPU_NUM SKIP DIFF_THRES CNN_LOWER_THRES CNN_UPPER_THRES"
    exit 1
fi

export CUDA_VISIBLE_DEVICES="$1"

time ../bazel-bin/tensorflow/vuse/vuse \
    --diff_thresh=$3 \
    --distill_thresh_lower=$4 \
    --distill_thresh_upper=$5 \
    --skip=$2 \
    --skip_small_cnn=0 \
    --avg_fname=/root/vuse-data/cnn-avg/gates-elevator.txt \
    --graph=/root/vuse-data/cnn-models/gates-elevator_mnist_256_32.pb \
    --video=/root/vuse-data/videos/gates-elevator.mp4 \
    --confidence_csv=/root/vuse-experiments/gates-elevator/gates-elevator_mnist_256_32.csv \
    --yolo_cfg=/root/darknet_ddkang/cfg/yolo.cfg \
    --yolo_weights=/root/darknet_ddkang/weights/yolo.weights \
    --yolo_class=0 \
    --start_from=250000 \
     --nb_frames=100020
