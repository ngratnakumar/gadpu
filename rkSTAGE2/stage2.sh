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
	cat 'precalib'$i.py | sed s/$i/$[$i+1]/ > 'precalib'$[$i+1].py
	i=$[$i+1]
	chmod +x precalib*.py
done

i=11
while [ $i -lt 17 ]
do
	$PYTHON runStage2.py $i &
	i=$[$i+1]
	sleep 10
done
