import os
import glob
import spam

# in_dir = ["/GARUDATA/IMAGING16/CYCLE16/MIXCYCLE/16_146_17MAY09/PRECALIB/GMRT*UVFITS", \
#          "/GARUDATA/IMAGING16/CYCLE16/MIXCYCLE/16_146_19MAY09/PRECALIB/GMRT*UVFITS",
#           "/GARUDATA/IMAGING16/CYCLE16/4382/16_146_02JUL09/PRECALIB/GMRT*UVFITS",
#          "/GARUDATA/IMAGING16/CYCLE16/MIXCYCLE/16_146_18MAY09/PRECALIB/GMRT*UVFITS"]

usb = glob.glob("/GARUDATA/IMAGING16/CYCLE16/*/16_278*/PRECALIB/*GMRT*USB*UVFITS")
lsb = glob.glob("/GARUDATA/IMAGING16/CYCLE16/*/16_278*/PRECALIB/*GMRT*LSB*UVFITS")

# e = list(set(e))
# e.sort()
# uvfits_list = []

count = 0
for each_dir in usb:
    if each_dir.replace('USB', 'LSB') in lsb:
        print each_dir
        count+=1
    print count

