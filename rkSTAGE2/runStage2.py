import sys
import subprocess
import glob
import socket

hostname = socket.gethostname()

thread_no = sys.argv[1]
#path = "/GARUDATA/IMAGING25/CYCLE25/*/precalibration.*"
path = "ls -ltrh /GARUDATA/IMAGING19/CYCLE19/CYCLE19/*/*/PRECALIB/*UVFITS/precalibration.*"
while 1:
    tobeProcessed = len(glob.glob(path))
    if tobeProcessed >= 213:
        print "Processed all files"
        sys.exit()
    else:
        cmd = ['start_parseltongue.sh', 'THREAD' + str(thread_no) + '/', str(thread_no), '../precalib' + str(thread_no) + '.py']
        print cmd
        start_process = subprocess.Popen(cmd)
        start_process.wait()
