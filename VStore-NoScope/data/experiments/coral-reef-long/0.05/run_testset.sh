
echo "Running script (actually inside script) +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
date
echo "dd_thres = 288.498"

#!/bin/bash

# 2018-07-20 14:23:25.004351

if [[ -z $1 ]]; then
    echo "Usage: $0 GPU_ID"
    exit 1
fi

export CUDA_VISIBLE_DEVICES="$1"

START_FRAME=1188000
LEN=270000
END_FRAME=$((START_FRAME + LEN))

time /home/luis/noscopeFolder/tensorflow-noscope//bazel-bin/tensorflow/noscope/noscope \
    --diff_thresh=288.498 \
    --distill_thresh_lower=0.998998998999 \
    --distill_thresh_upper=1.0 \
    --skip_small_cnn=0 \
    --skip_diff_detection=0 \
    --skip=30 \
    --avg_fname=/home/luis/noscopeFolder/data/cnn-avg/coral-reef-long.txt \
    --graph=/home/luis/noscopeFolder/data/cnn-models/coral-reef-long_convnet_32_32_2.pb \
    --video=/home/luis/noscopeFolder/data/videos/coral-reef-long.mp4 \
    --yolo_cfg=/home/luis/noscopeFolder/tensorflow-noscope/tensorflow/noscope/darknet/cfg/yolo.cfg \
    --yolo_weights=/home/luis/noscopeFolder/tensorflow-noscope/tensorflow/noscope/darknet/weights/yolo.weights \
    --yolo_class=0 \
    --confidence_csv=/home/luis/noscopeFolder/data/experiments/coral-reef-long/0.05/test_${START_FRAME}_${END_FRAME}.csv \
    --start_from=${START_FRAME} \
    --nb_frames=$LEN \
    --dumped_videos=/home/luis/noscopeFolder/data/video-cache/coral-reef-long_1188000_270000_30.bin \
    --diff_detection_weights=/dev/null \
    --use_blocked=0 \
    --ref_image=0 \
    &> /home/luis/noscopeFolder/data/experiments/coral-reef-long/0.05/test_${START_FRAME}_${END_FRAME}.log
