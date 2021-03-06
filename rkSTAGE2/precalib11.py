import os
import spam
import glob
import socket
import sys
import commands
import subprocess
import time
import random

from random import shuffle

hostname = socket.gethostname()
spam.set_aips_userid(11)

UVFITS_DATA = '/GARUDATA/IMAGING18/CYCLE18/'
PRECAL_PROCESSING = 'precalibration.processing'
PRECAL_SUCCESS = 'precalibration.success'
PRECAL_FAILED = 'precalibration.failed'


def check_haslam_flag(project):
    '''
    # Definition: check_haslam_flag() - takes PROPOSAL_CODE_DATE
    # Returns: Boolean
    # Description: This function checks for the all obslog files and 
    gets the is ALC set to OFF.
        If th ALC at 13. in obslog if FLAGGED as OFF, then we set apply_tsys to 
    spam.pre_calibrate_targets(apply_tsys=False), apply_tsys is True by default
    # Code: spam.pre_calibrate_targets(UVFITS_FILE_NAME, apply_tsys=False)
    -----------------------------------------------------------------------------------
    $fgrep '13.' /GARUDATA/IMAGING24/CYCLE24/*/*.obslog | grep 'OFF' | cut -d ':' -f 1 
    -----------------------------------------------------------------------------------
    # Reference: /export/spam/python/spam/gmrt.py:846,42

    '''
    #cmd = "fgrep '13.' " + UVFITS_DATA + "*/*.obslog | grep 'OFF' | cut -d ':' -f 1"
    cmd = "fgrep '14.' " + UVFITS_DATA + "*/*/*.obslog | grep 'OFF' | cut -d ':' -f 1"
    output = subprocess.check_output(cmd, shell=True)
    obs_list = output.split('\n')
    obs_list.remove('')
    for is_haslam_flagged in obs_list:
        return any(project in string for string in obs_list)


def check_pipeline_flag(DIR):
    ''' 
    # Description: Checking if any precalibration.* exists
    if Processsing precalibration.processing
    if Success precalibration.success
    if failed precalibration.failed
    '''
    FLAG_LIST = glob.glob(DIR + "precalibration*")
    return bool(FLAG_LIST)


def set_flag(DIR, FLAG):
    ''' 
    # Description: Setting if any precalibration.* exists
    if Processsing precalibration.processing
    if Success precalibration.success
    if failed precalibration.failed
    '''
    print "Setting " + FLAG + " flag @ " + DIR
    if check_pipeline_flag(DIR):
        delete_file(DIR + PRECAL_PROCESSING)
    pipeline_flag_file = DIR + "/" + FLAG
    os.system('touch ' + pipeline_flag_file)


def run_spam_precalibration_stage(UVFITS_BASE_DIR, DIR, UVFITS):
    print "Running SPAM pre_calibrate_targets on " + DIR
    os.chdir(DIR)
    print(DIR)
    UVFITS_FILE_NAME = glob.glob(DIR + "/*.UVFITS")
    print(UVFITS_FILE_NAME)
    if UVFITS_FILE_NAME:
        original_stdout = sys.stdout
        original_stderr = sys.stderr
        precal_log = open('precal_stdout.log', 'a+')
        precal_log.write('\n\n******PRECALIBRATION STARTED******\n\n')
        sys.stdout = precal_log
        sys.stderr = precal_log
        PROJECT_CODE = UVFITS_BASE_DIR.split('/')[4]
        print PROJECT_CODE
        if check_haslam_flag(PROJECT_CODE):
            print(PROJECT_CODE + " Flagging apply_tsys=False")
            try:
                spam.pre_calibrate_targets(UVFITS_FILE_NAME, apply_tsys=False, keep_channel_one = True)
                set_flag(UVFITS_BASE_DIR, PRECAL_SUCCESS)
            except Exception as e:
                failed_log = open('failed_log.txt', 'a+')
                failed_log.write("Failed Error: " + str(e))
                failed_log.flush()
                set_flag(UVFITS_BASE_DIR, PRECAL_FAILED)
        else:
            print(PROJECT_CODE + " Flagging apply_tsys=True")
            try:
                spam.pre_calibrate_targets(UVFITS_FILE_NAME)
                set_flag(UVFITS_BASE_DIR, PRECAL_SUCCESS)
            except Exception as e:
                failed_log = open('failed_log.txt', 'a+')
                failed_log.write("Failed Error: " + str(e))
                failed_log.flush()
                set_flag(UVFITS_BASE_DIR, PRECAL_FAILED)
        delete_file(UVFITS_FILE_NAME[0])
        PROCCEED_FILE_LIST = glob.glob(DIR + "/*")
        sys.stdout = original_stdout
        sys.stderr = original_stderr
        precal_log.flush()
        if PROCCEED_FILE_LIST:
            for EACH_FILE in PROCCEED_FILE_LIST:
                move_files(EACH_FILE, UVFITS_BASE_DIR + "/PRECALIB")
    else:
        set_flag(UVFITS_BASE_DIR, PRECAL_FAILED)


def copy_files(src, dest):
    cmd = "cp " + src + " " + dest
    (status, output) = commands.getstatusoutput(cmd)
    print output + "Copying " + src + " to " + dest + " : " + str(status)
    time.sleep(10)


def move_files(src, dest):
    print "Moving " + src + " ===> " + dest
    makedir = os.system("mkdir -p " + dest)
    movefile = os.system("mv " + src + " " + dest)


def delete_file(file_dir):
    print "Removing " + file_dir
    removefiles = os.system("rm " + file_dir)


def delete_dir(ddir):
    print "Removing " + ddir
    removedir = os.system("rm -r " + ddir)
    return True


def __main__():
    UVFITS_FILE_LIST = glob.glob("/GARUDATA/IMAGING18/CYCLE18/*/*/*.UVFITS")
    print UVFITS_DATA
    shuffle(UVFITS_FILE_LIST)
    if not UVFITS_FILE_LIST:
        print "UVFITS_FILE_LIST is empty"
        print UVFITS_FILE_LIST
    for EACH_UVFITS_FILE in UVFITS_FILE_LIST:
        UVFITS_FILE_NAME = os.path.basename(EACH_UVFITS_FILE)
        UVFITS_BASE_DIR = os.path.dirname(EACH_UVFITS_FILE) + "/"
        if not check_pipeline_flag(UVFITS_BASE_DIR):
            set_flag(UVFITS_BASE_DIR, PRECAL_PROCESSING)
            is_fits_dir = os.getcwd().split('/')
            SPAM_WORKING_DIR = os.getcwd()
            SPAM_THREAD_DIR = ""
            for num in range(1, 4):
                SPAM_THREAD_DIR += "/" + is_fits_dir[num]
            if 'fits' not in is_fits_dir:
                SPAM_THREAD_DIR = os.getcwd()
                SPAM_WORKING_DIR = os.getcwd() + "/fits/"
            copy_files(EACH_UVFITS_FILE, SPAM_WORKING_DIR)
            print "Copying done ==> Moving to pre_cal_target"
            run_spam_precalibration_stage(UVFITS_BASE_DIR, SPAM_WORKING_DIR, EACH_UVFITS_FILE)
            delete_dir(SPAM_THREAD_DIR)
            spam.exit()



if __name__ == '__main__':
    time.sleep(int(random.random()*300))
    __main__()











