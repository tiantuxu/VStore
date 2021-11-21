# VStore: A Data Store for Analytics on Large Videos

VStore is a data store for supporting fast, resource efficient analytics over large archival videos.
Please read our paper in EuroSys '19: [VStore: A Data Store for Analytics on Large Videos](https://web.ics.purdue.edu/~xu944/eurosys19.pdf)
and visit our [website](https://thexsel.github.io/p/vstore/) for more details.

## Overview
This source code orchestrates VStore's ingestion, storage, retrieval, and consumption based on 2 modern computer vision pipelines, [OpenALPR](https://github.com/openalpr/openalpr) and [NoScope](https://github.com/stanford-futuredata/noscope).

VStore's configurations, including the derivation of consumption formats, the derivation of storage formats, and data erosion are not included.

**In this repo, for the codebases (two sinks) inherited from third-parties, i.e., [OpenALPR](https://github.com/openalpr/openalpr) and [NoScope](https://github.com/stanford-futuredata/noscope), are NOT provided.**

## Build VStore

#### 1. VStore Requirements
To build VStore, you need the following to be installed
* ZeroMQ 2.2.x
* lmdb

#### 2. Benchmark Requirements

Follow the requirement of [OpenALPR](https://github.com/openalpr/openalpr) and [NoScope](https://github.com/stanford-futuredata/noscope).

#### 3. Build

Before building VStore, merge the sink into the current code:

 - For OpenALPR: Under ./VStore-OpenALPR, download [OpenALPR](https://github.com/openalpr/openalpr); merge ./VStore-OpenALPR/src into [OpenALPR](https://github.com/openalpr/openalpr) src directory.

 - For NoScope: Under ./VStore-NoScope/vstore, download [OpenALPR](https://github.com/openalpr/openalpr); merge ./VStore-NoScope/vstore/src into [OpenALPR](https://github.com/openalpr/openalpr) src directory. Under ./VStore-NoScope/, download [NoScope](https://github.com/stanford-futuredata/noscope) code from their repo, and follow their instructions for details.

Now do the build:
```
cd ./src
source env-teddy.sh
mkdir cmake-build-debug; cd cmake-build-debug; cmake .. -DCMAKE_BUILD_TYPE=Debug; cd ..
mkdir cmake-build-release; cd cmake-build-release; cmake .. -DCMAKE_BUILD_TYPE=Release; cd ..
build-all
```

## Run VStore
#### 1. Ingestion (Build video footage by transcoding)
The first step is to transcode the videos to the target formats: Follow the guides [here](https://gist.github.com/tiantuxu/6dca1b86f5ad5f7386d242f001a1cf08).

To generate raw video footage:

```
ffmpeg -i input.mp4 -c:v rawvideo -pix_fmt yuv420p video-raw.yuv
```

To generate encoded video chunks:
```
ffmpeg -i input.mp4 -acodec copy -f segment -vcodec copy -reset_timestamps 1 -map 0 ./video-chunks/output%04d.mp4
```

#### 2. Storage
Build the footage and store into lmdb:

 - for raw video footage:
```
/tmp/teddyxu/Debug/test-db-build.bin -r --dpath=/path/to/database --vpath=/path/to/video-raw.yuv width height
```
 - for encoded video chunks:
```
 /tmp/teddyxu/Debug/test-db-build.bin -e --dpath=/path/to/database --vpath=/path/to/video-chunks width height
``` 

#### 3. Retrieval (Run the source and invoke the decoder)
```
/tmp/teddyxu/Debug/test-source.bin
/tmp/teddyxu/Debug/decode-serv.bin
```

#### 4. Consumption (Run the sink)
For OpenALPR, invoke OpenALPR sink by running
```
/tmp/teddyxu/Debug/test-sink.bin
```

For NoScope, follow their instructions and configure the data directory.
After building NoScope sink, invoke the sink by running the sample using a command like:
```
./VStore-NoScope/data/experiments/jackson-town-square/0.02/run_testset.sh 0 VIDEO_LEN RESOLUTION
```
