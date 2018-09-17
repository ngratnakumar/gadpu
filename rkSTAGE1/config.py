import logging
import logging.config
import datetime
import psycopg2
import glob
import os
import re
import copy
import subprocess

DBHOST = '192.168.118.48'
DBUSER = 'postgres'
DBNAME = 'napsgoadb'

CYCLE_ID = '18'

CYCLE_NAME = 'CYCLE' + CYCLE_ID

INVALID_CYCLE_NAME = 'INVALID_CYCLE' + CYCLE_ID
UNCOMB_CYCLE_NAME = 'UNCOMB_CYCLE' + CYCLE_ID
FILTERED_CYCLE_NAME = 'FILTERED_CYCLE' + CYCLE_ID
ORIGINAL_CYCLE_NAME = 'ORIGINAL_CYCLE' + CYCLE_ID

GARUDATA = '/GARUDATA/IMAGING' + CYCLE_ID + '/'

BASE_DIR = GARUDATA + 'CYCLE' + CYCLE_ID + '/'
INVALID_DIR = GARUDATA + 'INVALID_CYCLE' + CYCLE_ID + '/'
UNCOMB_DIR = GARUDATA + 'UNCOMB_CYCLE' + CYCLE_ID + '/'
FILTERED_DIR = GARUDATA + 'FILTERED_CYCLE' + CYCLE_ID + '/'
ORIGINAL_DIR = GARUDATA + 'ORIGINAL_CYCLE' + CYCLE_ID + '/'

print "BASE_DIR: " + BASE_DIR

SCRIPT_PRE_RUN_FLAG_FILE = BASE_DIR + 'processed.flag'
SCRIPT_PRE_RUN_SPAM_FLAG_FILE = BASE_DIR + 'spam_processed.flag'

# QUERY1 = "select distinct s.observation_no, s.proj_code,  s.sky_freq1, s.sky_freq2, g.lta_file, g.file_path from das.scangroup g inner join das.scans s on g.observation_no = s.observation_no inner join gmrt.proposal p on s.proj_code = p.proposal_id where s.sky_freq1=s.sky_freq2 and s.sky_freq1 < 900000000 and s.chan_width >= 62500 and  p.cycle_id = '" + CYCLE_ID + "'"
QUERY1 = "select distinct o.observation_no, p.proposal_id, s.sky_freq1, s.sky_freq2, g.lta_file, g.file_path, p.backend_type from " \
                    "gmrt.proposal p inner join das.observation o on p.proposal_id = o.proj_code " \
                    "inner join das.scangroup g on g.observation_no = o.observation_no " \
                    "inner join das.scans s on s.scangroup_id = g.scangroup_id " \
                    "inner join gmrt.sourceobservationtype so on p.proposal_id = so.proposal_id where p.cycle_id ='" + CYCLE_ID + "' and so.obs_type not like 'pulsar' " \
                                                     "and s.sky_freq1=s.sky_freq2 and s.sky_freq1 < 900000000 " \
                                                     "and s.chan_width >= 62500 " \
                                                     "and o.proj_code not like '16_279' " \
                                                     "and o.proj_code not like '17_072' " \
                                                     "and o.proj_code not like '18_031' " \
                                                     "and o.proj_code not like '19_043' " \
                                                     "and o.proj_code not like '20_083' " \
                                                     "and o.proj_code not like '21_057';"

LTA_DIR = BASE_DIR

todayDateTime = datetime.datetime.today().strftime("%d%b%Y-%H%M")
todayDate = datetime.date.today().strftime("%d%b%Y")

ENV_FILE = '/GARUDATA/rkSTAGE1/setenv.sh'

# SPAM ENV EXPORTS
setEnvParams = [['export', 'SPAM_PATH=/export/spam'], ['export', 'SPAM_HOST=GADPU'], ['export', 'PYTHON=/export/spam/Python-2.7/bin/python'], ['export', 'PYTHONPATH=${SPAM_PATH}/python:${PYTHONPATH}'], ['export', 'LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${SPAM_PATH}/lib'], ['export', 'PATH=${SPAM_PATH}/bin:${PATH}']]

# Logger Config
# Logger_Name = '\n===========GADPU STAGE1 - STEP1 ===========\n'
# Log_File = GARUDATA + 'logs/gadpu-cycle' + CYCLE_ID + '-s1-' + todayDateTime + '.log'
#
# logging.basicConfig(filename=Log_File, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p', filemode='w', level=logging.DEBUG)
# # create logger
# logger = logging.getLogger(Logger_Name)
# logger.setLevel(logging.DEBUG)
#
# # Add a FileHandler
# fileHandler = logging.FileHandler(Log_File)
# fileHandler.setLevel(logging.DEBUG)
#
# # Create ConsoleHandler and set level to debug
# consoleHandler = logging.StreamHandler()
# consoleHandler.setLevel(logging.DEBUG)
#
# # Create Formatter
# formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p')
#
# # Adding formatter to handlers
# fileHandler.setFormatter(formatter)
# consoleHandler.setFormatter(formatter)
#
# # Generic way to Run a SH Command on BASH Shell
#
# def runCommand(cmd, outfile):
#     print("Running Command: " + str(cmd))
#     err = ""
#     try:
#         if cmd is not None:
#             cmdStr = str(cmd)
#         proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=False)
#         (out, err) = proc.communicate()
#         print(cmdStr + " Success")
#         logging.info("Running command " + cmd[0] + " on file " + cmd[2] + "\t: SUCCESS!")
#     except Exception, er:
#         print("Error: " + cmdStr + "\n" + str(err) + "\n")
#         logging.error("Error: " + cmdStr + "\n" + str(err) + "\n" + str(er))
#
#
# def moveFiles(src, dest):
#     cmd = ['mv', src, dest]
#     print(cmd)
#     dest_dir = os.path.dirname(dest)
#     if os.path.isdir(src):
#         runCommand(cmd, None)
#     elif os.path.isfile(src):
#         if not os.path.exists(dest_dir):
#             os.makedirs(dest_dir)
#             logging.info("Creating " + dest_dir)
#         runCommand(cmd, None)
#         print "Moving " + src + " ==> " + dest
#         logging.info("Moving " + src + " ==> " + dest)
#     else:
#         logging.error(src + " Does Not Exists")
#
#
# def checkFileSize(fileName):
#     fileInfo = os.stat(fileName)
#     return ((fileInfo.st_size) / 1024) / 1024
#
#
# def checkPreviousRunFlag():
#     return os.path.isfile(SCRIPT_PRE_RUN_FLAG_FILE)
#
#
def checkSpamPreviousRunFlag():
    return os.path.isfile(SCRIPT_PRE_RUN_SPAM_FLAG_FILE)
