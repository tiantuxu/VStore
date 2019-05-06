
echo "Running script (actually inside script) +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
date
echo "dd_thres = 1.46152452452"

#!/bin/bash

# 2018-07-20 17:32:25.192176

if [[ -z $1 ]]; then
    echo "Usage: $0 GPU_ID"
    exit 1
fi

export CUDA_VISIBLE_DEVICES="$1"

START_FRAME=540000
LEN=270000
END_FRAME=$((START_FRAME + LEN))

time /home/luis/noscopeFolder/tensorflow-noscope//bazel-bin/tensorflow/noscope/noscope \
    --diff_thresh=1.46152452452 \
    --distill_thresh_lower=0.196167355355 \
    --distill_thresh_upper=0.867740291291 \
    --skip_small_cnn=0 \
    --skip_diff_detection=0 \
    --skip=30 \
    --avg_fname=/home/luis/noscopeFolder/data/cnn-avg/jackson-town-square.txt \
    --graph=/home/luis/noscopeFolder/data/cnn-models/jackson-town-square_convnet_128_32_2.pb \
    --video=/home/luis/noscopeFolder/data/videos/jackson-town-square.mp4 \
    --yolo_cfg=/home/luis/noscopeFolder/tensorflow-noscope/tensorflow/noscope/darknet/cfg/yolo.cfg \
    --yolo_weights=/home/luis/noscopeFolder/tensorflow-noscope/tensorflow/noscope/darknet/weights/yolo.weights \
    --yolo_class=2 \
    --confidence_csv=/home/luis/noscopeFolder/data/experiments/jackson-town-square/0.01/test_${START_FRAME}_${END_FRAME}.csv \
    --start_from=${START_FRAME} \
    --nb_frames=$LEN \
    --dumped_videos=/home/luis/noscopeFolder/data/video-cache/jackson-town-square_540000_270000_30.bin \
    --diff_detection_weights=/dev/null \
    --use_blocked=0 \
    --ref_image=0 \
    &> /home/luis/noscopeFolder/data/experiments/jackson-town-square/0.01/test_${START_FRAME}_${END_FRAME}.log
