from FileUtils import FileUtils
from DBUtils import DBUtils
from SpamUtils import SpamUtils
import time
import datetime
import glob
import os
from astropy.io import fits
import random
import spam
from time import sleep


class Pipeline:
    """
    Stage3: Check for Haslam correction and populates clibrationinput table
    """

    def stage3(self):
        spam.set_aips_userid(33)
        dbutils = DBUtils()
        fileutils = FileUtils()

        # while True:
        #     columnKeys = {"calibration_id"}
        #     whereData = {"comments": "c15", "status": "copying"}
        #     uncalibrated_uvfits = dbutils.select_from_table("calibrationinput", columnKeys, whereData, 0)
        #     if not uncalibrated_uvfits:
        #         break
        #     print("Waiting for bandwidth ... ")
        #     time.sleep(50)

        columnKeys = {"calibration_id", "project_id", "uvfits_file"}
        whereData = {"status": "cycle226"}
        uncalibrated_uvfits = dbutils.select_from_table("calibrationinput", columnKeys, whereData, 0)

        if not uncalibrated_uvfits:
            print("All for the data is processed ... please check the DB for pre_calib")
            spam.exit()

        calibration_id = uncalibrated_uvfits[0]
        project_id = uncalibrated_uvfits[1]
        uvfits_file = uncalibrated_uvfits[2]

        columnKeys = {"file_path", "observation_no"}
        whereData = {"project_id": project_id}
        project_details = dbutils.select_from_table("projectobsno", columnKeys, whereData, 0)


        base_path = project_details[1]
        observation_no = project_details[0]

        current_date_timestamp = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')

        projectobsno_update_data = {
            "set": {
                "status": "processing",
                "comments": "running precalibrate_target, calibration_id = " + str(calibration_id),
            },
            "where": {
                "project_id": project_id
            }
        }

        calibration_update_data = {
            "set": {
                "status": "copying",
                "start_time": current_date_timestamp
            },
            "where": {
                "calibration_id": calibration_id
            }
        }

        dbutils.update_table(projectobsno_update_data, "projectobsno")
        dbutils.update_table(calibration_update_data, "calibrationinput")

        UVFITS_FILE_NAME = uvfits_file
        UVFITS_BASE_DIR = base_path
        is_fits_dir = os.getcwd().split('/')
        print(is_fits_dir)
        SPAM_WORKING_DIR = os.getcwd()
        print(SPAM_WORKING_DIR)
        # for num in range(1, 3):
        #     SPAM_THREAD_DIR += "/" + is_fits_dir[num]
        # if 'fits' not in is_fits_dir:
        #     SPAM_THREAD_DIR = os.getcwd()
        SPAM_WORKING_DIR = os.getcwd() + "/fits/"
        print(SPAM_WORKING_DIR, UVFITS_BASE_DIR, UVFITS_FILE_NAME)
        UVFITS_FILE_PATH = UVFITS_BASE_DIR + "/" + UVFITS_FILE_NAME
        print(UVFITS_FILE_PATH)
        print(SPAM_WORKING_DIR)
        fileutils.copy_files(UVFITS_FILE_PATH, SPAM_WORKING_DIR)
        print("Copying done ==> Moving to pre_cal_target")
        current_date_timestamp = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')
        calibration_update_data = {
            "set": {
                "status": "processing",
                "start_time": current_date_timestamp
            },
            "where": {
                "calibration_id": calibration_id
            }
        }
        dbutils.update_table(calibration_update_data, "calibrationinput")

        fileutils.run_spam_precalibration_stage(UVFITS_BASE_DIR, SPAM_WORKING_DIR, UVFITS_FILE_NAME, observation_no)
        current_time_in_sec = time.time()
        current_date_timestamp = datetime.datetime.fromtimestamp(current_time_in_sec).strftime('%Y-%m-%d %H:%M:%S')

        check_status_file = glob.glob(base_path + "/PRECALIB/failed_log.txt")
        comments = "failed"
        if check_status_file:
            status = "failed"
            comments = str(open(check_status_file[0], 'r').read())
        else:
            status = "success"
            comments = "precalibrate_target done, calibration_id = " + str(calibration_id)

        projectobsno_update_data = {
            "set": {
                "status": status,
                "comments": comments
            },
            "where": {
                "project_id": project_id
            }
        }

        calibration_update_data = {
            "set": {
                "status": status,
                "end_time": current_date_timestamp,
                "comments": comments
            },
            "where": {
                "calibration_id": calibration_id
            }
        }

        dbutils.update_table(projectobsno_update_data, "projectobsno")
        dbutils.update_table(calibration_update_data, "calibrationinput")

        if not check_status_file:
            split_files = glob.glob(base_path +"/PRECALIB/*GMRT*.UVFITS")

            for each_split_file in split_files:
                current_time_in_sec = time.time()
                calibrated_uvfits = {
                    'start_time': datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S'),
                    'end_time': datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S'),
                    'project_id': project_id,
                    'calibration_id': calibration_id,
                    "file_size": fileutils.calculalate_file_sizse_in_MB(each_split_file),
                    "calibrated_fits_file": each_split_file.split('/')[-1],
                    "status": "success"
                }
                print dbutils.insert_into_table("imaginginput", calibrated_uvfits, "project_id")


    def __init__(self):
        rand_sec = int(random.random()*100)
        print("Waiting for "+str(rand_sec)+" seconds ... ")
        sleep(rand_sec)
        print("Starting process ... ")
        self.stage3() # PRE_CALIB

Pipeline()
