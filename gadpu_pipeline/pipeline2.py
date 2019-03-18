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


class Pipeline:
    """
    Stage2: Applies GVFITS generates Uncalibrated UVFITS files
    """

    def stage2(self):
        dbutils = DBUtils()
        spamutils = SpamUtils()
        fileutils = FileUtils()

        currentTimeInSec = time.time()
        current_date_timestamp = datetime.datetime.fromtimestamp(currentTimeInSec).strftime('%Y-%m-%d %H:%M:%S')

        columnKeys = {"project_id", "ltacomb_file", "lta_id"}
        whereKeys = {"comments": "cycle26"}

        lta_details = dbutils.select_from_table("ltadetails", columnKeys, whereKeys, None)

        print("================================")
        print(lta_details)
        print("********************************")
        print(len(lta_details))
        print("********************************")

        for each_lta in lta_details:
            print(each_lta)
            project_id = each_lta["project_id"]
            # project_id = each_lta[0]
            lta_file = each_lta["ltacomb_file"]
            # lta_file = each_lta[1]
            # lta_id = each_lta[2]
            lta_id = each_lta["lta_id"]
            columnKeys = {"file_path"}
            whereKeys = {"project_id": project_id}
            lta_path_details = dbutils.select_test_table("projectobsno", columnKeys, whereKeys, 0)
            print(lta_path_details)
            base_path = lta_path_details[0]
            print(base_path)
            uvfits_file = lta_file + '.UVFITS'
            base_lta = base_path + '/' + lta_file
            print(base_lta)
            if os.path.exists(base_lta):
                base_uvfits = base_path + '/' + uvfits_file
                gvfits_status = spamutils.run_gvfits(base_lta, base_uvfits)
                if os.path.exists(base_uvfits):
                    status = "success"
                else:
                    status = "failed"

                calibration_data = {
                    "project_id": project_id,
                    "lta_id": lta_id,
                    "uvfits_file": uvfits_file,
                    "status": status,
                    "comments": gvfits_status,
                    "uvfits_size": fileutils.calculalate_file_sizse_in_MB(base_uvfits),
                    "start_time": current_date_timestamp
                }

                dbutils.insert_into_table("calibrationinput", calibration_data, "calibration_id")
            else:
                project_update_data = {
                    "set": {
                        "status": "failed",
                        "comments": "ltacomb failed"
                    },
                    "where": {
                        "project_id": project_id
                    }
                }
                lta_details_update_data = {
                    "set": {
                        "status": "failed",
                        "comments": "ltacomb failed"
                    },
                    "where": {
                        "lta_id": lta_id
                    }
                }
                dbutils.update_table(project_update_data, "projectobsno")
                dbutils.update_table(lta_details_update_data, "ltadetails")


    def stage2_ghb(self):
        dbutils = DBUtils()
        spamutils = SpamUtils()
        fileutils = FileUtils()

        currentTimeInSec = time.time()
        current_date_timestamp = datetime.datetime.fromtimestamp(currentTimeInSec).strftime('%Y-%m-%d %H:%M:%S')

        columnKeys = {"project_id", "ltacomb_file", "lta_id"}
        whereKeys = {"comments": "c26"}

        lta_details = dbutils.select_from_table("ltadetails", columnKeys, whereKeys, None)

        print(lta_details)

        for each_lta in lta_details:
            print(each_lta)
            project_id = each_lta["project_id"]
            # project_id = each_lta[0]
            lta_file = each_lta["ltacomb_file"]
            # lta_file = each_lta[1]
            # lta_id = each_lta[2]
            lta_id = each_lta["lta_id"]
            columnKeys = {"file_path"}
            whereKeys = {"project_id": project_id}
            lta_path_details = dbutils.select_test_table("projectobsno", columnKeys, whereKeys, 0)
            print(lta_path_details)
            base_path = lta_path_details[0]
            print(base_path)
            uvfits_file = lta_file + '.UVFITS'
            base_lta = base_path + '/' + lta_file
            if os.path.exists(base_lta):
                base_uvfits = base_path + '/' + uvfits_file
                gvfits_status = spamutils.run_gvfits(base_lta, base_uvfits)
                if os.path.exists(base_uvfits):
                    status = "success"
                else:
                    status = "failed"

                calibration_data = {
                    "project_id": project_id,
                    "lta_id": lta_id,
                    "uvfits_file": uvfits_file,
                    "status": status,
                    "comments": gvfits_status,
                    "uvfits_size": fileutils.calculalate_file_sizse_in_MB(base_uvfits),
                    "start_time": current_date_timestamp
                }

                dbutils.insert_into_table("calibrationinput", calibration_data, "calibration_id")

            else:
                project_update_data = {
                    "set": {
                        "status": "failed",
                        "comments": "ltacomb failed"
                    },
                    "where": {
                        "project_id": project_id
                    }
                }
                lta_details_update_data = {
                    "set": {
                        "status": "failed",
                        "comments": "ltacomb failed"
                    },
                    "where": {
                        "lta_id": lta_id
                    }
                }
                dbutils.update_table(project_update_data, "projectobsno")
                dbutils.update_table(lta_details_update_data, "ltadetails")

    def __init__(self):
        self.stage2() # GVFITS

Pipeline()
