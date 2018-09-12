#!/bin/bash

export SPAM_PATH=/export/spam
export SPAM_HOST=GADPU
export PYTHON=/export/spam/Python-2.7/bin/python
export PYTHONPATH=${SPAM_PATH}/python:${PYTHONPATH}
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${SPAM_PATH}/lib
export PATH=${SPAM_PATH}/bin:${PATH}

i=11
while [ $i -lt 19 ]
do
	cat 'proctarget'$i.py | sed s/$i/$[$i+1]/ > 'proctarget'$[$i+1].py
	i=$[$i+1]
done


i=11
while [ $i -lt 18 ]
do
	$PYTHON runStage3.py $i &
   i=$[$i+1]
	sleep 10
done

while true;
do
    sleep 60
    ls /home/gadpu/rkSTAGE3/THREAD*
    if [ $? != 0 ]
    then
        sleep 60
        sudo reboot
        sleep 120
    fi
    sleep 180
done