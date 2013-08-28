#!/usr/bin/env python

# you must have ffmpeg in $PATH
import os, sys, glob

for i in range(1,14):
    flist = "|".join(glob.glob("%02d??-A.mp3" % i ))
    os.system('ffmpeg -y -i "concat:%s" -acodec copy %02d.mp3 ' % (flist,i) )
