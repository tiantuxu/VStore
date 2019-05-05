#!/bin/bash

# to be execute *locally* on remote or local

# stop on error
set -e 

# no trailing /. will add later
SRC=/home/xzl/video-streamer
DEST=/home/xzl/video-streamer-`date +"%m%d%y-%H_%M_%S"`

mkdir ${DEST}

rsync -a -vv --exclude='*.bin' --exclude='*.o' ${SRC}/ ${DEST}; \
touch ${DEST}/BACKUP; \
mv ${DEST} ${DEST}-complete; 

echo "[[Finished]]: backed up ${SRC} to ${DEST}"
