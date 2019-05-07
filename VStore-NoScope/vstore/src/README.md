# to build
sudo apt install libpostproc-dev

using CMakefile. Check out the script
env-xzl.sh
And modify path as needed.

# 3rd party lib
db - lmdb
transport - zmq

# WORKFLOW

see diagram

# db schema

## encoded chunks:
key = vtime (video time)
v = chunk

## raw frame
key = vtime
v = frame

# stream
a runtime concept

a sequence of chunks/frames to be consumed by CV
- dynamically composed by subscribing from multiple dbs on a given vtime range
- from the same camera
- have the same quality
- no overlap on vtime axis
- there might be holes on vtime (deleted?)

# CODE

## data structure defs:
vs-types.h

## Decoder
mydecoder.[cpp|h]
    can do both sw/hw decoding. Can scale horizontally.

## Source
test-source.cpp
    example source code that loads from db and send over zmq

## Sink
RxManager.h
    received frame management:
test-sink.cpp
    example sink code

## Data move
rxtx.[cpp|h]
    data transfer code. load chunks/frames from db/file/mmap and send over zmq. also includes the receiver functions.
    only -db code will be truely useful.
mm.[cpp|h]
    memory management code for deferred deallocation w.r.t zmq transmission.
    this is to implement zero-copy for data load + network transmission.

## helps/utils
    StatCollector.h
    CallBackTimer.h
    ...
