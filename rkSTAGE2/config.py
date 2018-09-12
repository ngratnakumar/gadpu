import logging
import logging.config
import datetime
import psycopg2
import glob
import os
import re
import copy
import subprocess
import time
import random
import trace
import sys
import socket

DBHOST = '192.168.118.48'
DBUSER = 'postgres'
DBNAME = 'napsgoadb'

CYCLE_ID = '25'

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

GADPU_HOME = '/home/gadpu/'

print "BASE_DIR: " + BASE_DIR

STAGE2_PROCESSING_FILE_LIST = GARUDATA + "stage2_processing_files.list"

# writeToFile(STAGE2_PROCESSING_FILE_LIST, for uvfits in glob.glob(BASE_DIR + '*/*.UVFITS'): print uvfits + '\n')

SCRIPT_PRE_RUN_FLAG_FILE = BASE_DIR + 'processed.flag'
SCRIPT_PRE_RUN_SPAM_FLAG_FILE = BASE_DIR + 'spam_processed.flag'

QUERY1 = "select distinct s.observation_no, s.proj_code,  s.sky_freq1, s.sky_freq2, g.lta_file, g.file_path from das.scangroup g inner join das.scans s on g.observation_no = s.observation_no inner join gmrt.proposal p on s.proj_code = p.proposal_id where s.sky_freq1=s.sky_freq2 and s.sky_freq1 < 900000000 and p.cycle_id = '" + CYCLE_ID + "'"

LTA_DIR = BASE_DIR

todayDateTime = datetime.datetime.today().strftime("%d%b%Y-%H%M")
todayDate = datetime.date.today().strftime("%d%b%Y")

# Logger Config
Logger_Name = '\n===========GADPU STAGE2 ===========\n'
COMPUTE_NODE = socket.gethostname()
# Log_File = GARUDATA + 'logs/gadpu-cycle' + CYCLE_ID + '-s2-' + todayDateTime + '_' + str(fileNo) + '_' + COMPUTE_NODE + '.log'
Log_File = GARUDATA + 'logs/s2c' + CYCLE_ID + '-' + todayDate + '-' + COMPUTE_NODE + '.log'

logging.basicConfig(filename=Log_File, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p', filemode='a', level=logging.DEBUG)
# create logger
logger = logging.getLogger(Logger_Name)
logger.setLevel(logging.DEBUG)

# Add a FileHandler
fileHandler = logging.FileHandler(Log_File)
fileHandler.setLevel(logging.DEBUG)

# Create ConsoleHandler and set level to debug
consoleHandler = logging.StreamHandler()
consoleHandler.setLevel(logging.DEBUG)

# Create Formatter
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p')

# Adding formatter to handlers
fileHandler.setFormatter(formatter)
consoleHandler.setFormatter(formatter)


# Generic way to Run a SH Command on BASH Shell


def writeToFile(filename, data):
    try:
        writeFile = open(filename, 'a+')
        writeFile.write(data)
        writeFile.close()
    except Exception as e:
        logging.error(e)


def readFileToList(filename):
    fileName = open(filename, 'r')
    data = fileName.readlines()
    dataList = []
    for eachline in data:
        dataList.append(eachline.strip())
    fileName.close()
    return dataList


def runCommand(cmd, outfile):
    print("Running Command: " + str(cmd))
    err = ""
    try:
        if cmd is not None:
            cmdStr = str(cmd)
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=False)
        (out, err) = proc.communicate()
        print(cmdStr + " Success")
        logging.info("Running command " + cmd[0] + " on file " + cmd[2] + "\t: SUCCESS!")
    except Exception, er:
        print("Error: " + cmdStr + "\n" + str(err) + "\n")
        logging.error("Error: " + cmdStr + "\n" + str(err) + "\n" + str(er))


def moveFiles(src, dest):
    cmd = ['mv', src, dest]
    print(cmd)
    dest_dir = os.path.dirname(dest)
    if os.path.isdir(src):
        runCommand(cmd, None)
    elif os.path.isfile(src):
        if not os.path.exists(dest_dir):
            os.makedirs(dest_dir)
            logging.info("Creating " + dest_dir)
        runCommand(cmd, None)
        print "Moving " + src + " ==> " + dest
        logging.info("Moving " + src + " ==> " + dest)
    else:
        logging.error(src + " Does Not Exists")


def checkFileSize(fileName):
    fileInfo = os.stat(fileName)
    # return ((fileInfo.st_size) / 1024) / 1024
    return fileInfo.st_size


def checkPreviousRunFlag():
    return os.path.isfile(SCRIPT_PRE_RUN_FLAG_FILE)


def checkSpamPreviousRunFlag():
    return os.path.isfile(SCRIPT_PRE_RUN_SPAM_FLAG_FILE)


'''
def genThreadList():
    if checkFileSize(THREAD_TRACKER) == 0:
        for threadNum in range(1, 8):
            writeToFile(THREAD_TRACKER, str(threadNum) + '\n')


def getCurrentThread():
    genThreadList()
    threadNo = 0
    threadList = readFileToList(THREAD_TRACKER)
    threadNo = threadList[0]
    removeThreadEntry(threadNo)
    # for number in range(1, 8):
    #     if number not in threadList:
    #         threadNo = number
    #         writeToFile(THREAD_TRACKER, threadNo)
    return threadNo
'''
