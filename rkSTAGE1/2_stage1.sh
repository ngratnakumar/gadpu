#!/bin/bash

export SPAM_PATH=/export/spam
export SPAM_HOST=GADPU
export PYTHONPATH=${SPAM_PATH}/python:${PYTHONPATH}
export PYTHON=/export/spam/Python-2.7/bin/python
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${SPAM_PATH}/lib
export PATH=${SPAM_PATH}/bin:${PATH}

start_parseltongue.sh CYCLE19ST1/ 11 ../2_stage1.py
