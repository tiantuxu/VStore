#!/bin/bash -e

bazel build -c opt --copt=-mavx2 --config=cuda :noscope
