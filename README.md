# VStore: A Data Store for Analytics on Large Videos
Please read our paper in EuroSys '19: [VStore: A Data Store for Analytics on Large Videos](https://web.ics.purdue.edu/~xu944/eurosys19.pdf)
and visit out [website](https://thexsel.github.io/p/vstore/).

## Overview
VStore is a data store for supporting fast, resource efficient analytics over large archival videos. 
The code is built opon 2 modern computer vision pipelines, [OpenALPR](https://github.com/openalpr/openalpr) and [NoScope](https://github.com/stanford-futuredata/noscope).

## Build VStore
### VStore Requirements
To build VStore, you need the following to be installed
* ZeroMQ 2.2.x
* lmdb
### Benchmark Requirements
Follow the requirement of [OpenALPR](https://github.com/openalpr/openalpr) and [NoScope](https://github.com/stanford-futuredata/noscope).
### Build
```
cd ./src
source env-teddy.sh
mkdir cmake-build-debug; cd cmake-build-debug; cmake .. -DCMAKE_BUILD_TYPE=Debug; cd ..
mkdir cmake-build-release; cd cmake-build-release; cmake .. -DCMAKE_BUILD_TYPE=Release; cd ..
build-all
```

## Run VStore
### Build Video Footage
#### 1. Transcode: Follow the guides [here](https://gist.github.com/tiantuxu/6dca1b86f5ad5f7386d242f001a1cf08)
#### 2. Build the footage to lmdb
For raw video footage,
```
/tmp/vstore/Release/test-db-build.bin -r --dpath=/path/to/database --vpath=/path/to/video.yuv width height
```
For encoded video chunks,
```
 /tmp/vstore/Release/test-db-build.bin -e --dpath=/path/to/database --vpath=/path/to/video.yuv width height
``` 
