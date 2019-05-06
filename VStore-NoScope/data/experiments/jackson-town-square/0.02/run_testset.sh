
echo "Running script (actually inside script) +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
date
echo "dd_thres = 1.24647935936"

#!/bin/bash

# 2018-07-25 17:33:25.479857

if [[ -z $1 ]]; then
    echo "Usage: $0 GPU_ID"
    exit 1
fi

export CUDA_VISIBLE_DEVICES="$1"

START_FRAME=540000
#LEN=270000
LEN=3000
END_FRAME=$((START_FRAME + LEN))

echo "start the script"

time /home/teddyxu/noscopeFolder/tensorflow-noscope//bazel-bin/tensorflow/noscope/noscope \
    --diff_thresh=1.24647935936 \
    --distill_thresh_lower=0.0169753763764 \
    --distill_thresh_upper=0.520245358358 \
    --skip_small_cnn=0 \
    --skip_diff_detection=0 \
    --skip=30 \
    --avg_fname=/home/teddyxu/noscopeFolder/data/cnn-avg/jackson-town-square.txt \
    --graph=/home/teddyxu/noscopeFolder/data/cnn-models/jackson-town-square_convnet_128_32_0.pb \
    --video=/home/teddyxu/noscopeFolder/data/videos/jackson-town-square.mp4 \
    --yolo_cfg=/home/teddyxu/noscopeFolder/tensorflow-noscope/tensorflow/noscope/darknet/cfg/yolo.cfg \
    --yolo_weights=/home/teddyxu/noscopeFolder/tensorflow-noscope/tensorflow/noscope/darknet/weights/yolo.weights \
    --yolo_class=2 \
    --confidence_csv=/home/teddyxu/noscopeFolder/data/experiments/jackson-town-square/0.02/test_${START_FRAME}_${END_FRAME}.csv \
    --start_from=${START_FRAME} \
    --nb_frames=$LEN \
    --dumped_videos=/dev/null \
    --diff_detection_weights=/dev/null \
    --use_blocked=0 \
    --ref_image=0
    #&> /home/teddyxu/noscopeFolder/data/experiments/jackson-town-square/0.02/test_${START_FRAME}_${END_FRAME}.log
