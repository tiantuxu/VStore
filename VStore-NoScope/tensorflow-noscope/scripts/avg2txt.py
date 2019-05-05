#!/usr/bin/env python

import numpy as np
import sys

if len(sys.argv):
    print "Usage: " + sys.argv[0] + " AVG.npy"
    sys.exit(1)

a = np.load(sys.argv[1])
for b in np.nditer(a):
    print b
