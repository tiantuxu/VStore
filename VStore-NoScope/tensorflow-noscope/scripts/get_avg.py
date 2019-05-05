#! /usr/bin/env python

import itertools
import argparse
import vuse
import numpy as np
from keras.utils import np_utils

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--csv_in', required=True, help='CSV input filename')
    parser.add_argument('--video_in', required=True, help='Video input filename')
    parser.add_argument('--objects', required=True, help='Objects to classify. Comma separated')
    parser.add_argument('--num_frames', type=int, help='Number of frames')
    args = parser.parse_args()

    objects = args.objects.split(',')
    # for now, we only care about one object, since
    # we're only focusing on the binary task
    assert len(objects) == 1

    print 'Preparing data....'
    data, nb_classes = vuse.DataUtils.get_data(
            args.csv_in, args.video_in,
            binary=True,
            num_frames=args.num_frames,
            OBJECTS=objects,
            regression=False,
            resol=(50, 50),
            center=False)
    X_train, Y_train, X_test, Y_test = data

    np.save('avg.npy', np.mean(X_train, axis=0))
    np.save('train.npy', X_train)
    np.save('test.npy', X_test)


if __name__ == '__main__':
    main()
