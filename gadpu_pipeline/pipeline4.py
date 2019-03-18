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
    Stage4: This is only if the CYCLE is GHB,
    After running pre_calibrate targets,
    this will combine the LSB and USB calibrated UVFITS,
    and populates teh imagainginoput tables
    """

    def stage4(self):
        spam.set_aips_userid(11)
        dbutils = DBUtils()
        fileutils = FileUtils()
        status = "failed"
        comments = "combine usb lsb failed"
        # query conditions for projectobsno
        columnKeys = {"project_id", "file_path", "observation_no"}
        whereKeys = {"isghb": "false" , "cycle_id": 26, "status": "cycle26"}

        project_data = dbutils.select_from_table("projectobsno", columnKeys, whereKeys, 0)
        print(project_data)

        project_id = project_data[1]
        base_path = project_data[2]
        obsno = project_data[0]

        start_time = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')
        # print(project_id, base_path, obsno)

        # query conditions for calibrationinput
        columnKeys = {"calibration_id", "uvfits_file"}
        whereKeys = {"project_id": project_id}
        calibration_data = dbutils.select_from_table("calibrationinput", columnKeys, whereKeys, None)
        print(calibration_data)

        if not calibration_data:
            print("All the data is processed ... OR \n ==> please check the DB for combinelsbusb")
            spam.exit()

        print(len(calibration_data))
        if len(calibration_data) < 2:
            status = "success"
            comments = "single file combinelsbusb not required"
            usb_lsb_file = glob.glob(base_path+"/PRECALIB/*GMRT*.UVFITS")
            if calibration_data:
                projectobsno_update_data = {
                    "set": {
                        "status": status,
                        "comments": comments
                    },
                    "where": {
                        "project_id": project_id
                    }
                }
                print("Updating the projectobsno ... ")
                dbutils.update_table(projectobsno_update_data, "projectobsno")
                if calibration_data:
                    calibration_update_data = {
                        "set": {
                            "status": status,
                            "comments": comments
                        },
                        "where": {
                            "calibration_id": calibration_data[0]["calibration_id"]
                        }
                    }
                    dbutils.update_table(calibration_update_data, "calibrationinput")
            else:
                projectobsno_update_data = {
                    "set": {
                        "status": "failed",
                        "comments": "Failed Error: Something went wrong"
                    },
                    "where": {
                        "project_id": project_id
                    }
                }
                print("Updating the projectobsno ... ")
                dbutils.update_table(projectobsno_update_data, "projectobsno")
        else:
            print("Values > 2")
            print("*************"+str(os.getcwd()))
            for each_uvfits in calibration_data:
                precalib_files = glob.glob(base_path+"/PRECALIB/*")
                lsb_list = glob.glob(base_path + "/PRECALIB/*_LL_*.UVFITS")
                usb_list = glob.glob(base_path + "/PRECALIB/*_RR_*.UVFITS")

                if len(lsb_list) == 0 or len(usb_list) == 0:
                    print(len(lsb_list), len(usb_list))
                    lsb_list = glob.glob(base_path + "/PRECALIB/*LSB*.UVFITS")
                    usb_list = glob.glob(base_path + "/PRECALIB/*USB*.UVFITS")

                projectobsno_update_data = {
                    "set": {
                        "status": "processing",
                        "comments": "combining_lsb_usb"
                    },
                    "where": {
                        "project_id": project_id
                     }
                }
                dbutils.update_table(projectobsno_update_data, "projectobsno")
                calibration_id = each_uvfits["calibration_id"]
                uvfits_file = each_uvfits["uvfits_file"]
                calibration_update_data = {
                    "set": {
                        "status": "processing",
                        "comments": "combining_lsb_usb",
                        "start_time": start_time
                    },
                    "where": {
                        "calibration_id": calibration_id
                    }
                }
                dbutils.update_table(calibration_update_data, "calibrationinput")
                print("lsb_list : "+str(len(lsb_list)))
                print("usb_list : "+str(len(usb_list)))
                status = "failed"
                comments ="combining lsb usb"
                if len(lsb_list) == len(usb_list):
                    print(">>>>>>COMBINE_LSB_USB<<<<<<<")
                    usb_list.sort()
                    lsb_list.sort()
                    print(usb_list)
                    print(lsb_list)
                    to_spam = list(zip(usb_list, lsb_list))
                    file_size = 0
                    print(to_spam)
                    for each_pair in to_spam:
                        print("-------------------------")
                        comb = each_pair[0].replace('USB', 'COMB')
                        data = each_pair, comb
                        print("++++++++++++++++"+comb)
                        currentTimeInSec = time.time()
                        fits_comb = comb.split('/')[-1]
                        check_comb_file = glob.glob("fits/"+fits_comb)
                        if not check_comb_file:
                            status, comments = fileutils.run_spam_combine_usb_lsb(data)
                            print("__________________________________________")
                            print(glob.glob("fits/*"))
                            print("__________________________________________")
                            end_time = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')
                            if not comments:
                                comments = "done combining usb lsb"
                            if glob.glob(comb):
                                file_size = fileutils.calculalate_file_sizse_in_MB(comb)
                            imagininput_data = {
                                "project_id": project_id,
                                "calibration_id": calibration_id,
                                "calibrated_fits_file": os.path.basename(comb),
                                "file_size": file_size,
                                "start_time": start_time,
                                "end_time": end_time,
                                "comments": "c16 "+comments,
                            }
                            dbutils.insert_into_table("imaginginput", imagininput_data, "imaging_id")
                            print("-------------------------")
                end_time = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')

                if status == 'success':
                    comments = 'done'

                calibration_update_data = {
                    "set": {
                        "status": status,
                        "comments": comments,
                        "start_time": start_time,
                        "end_time": end_time
                    },
                    "where": {
                        "calibration_id": calibration_id
                    }
                }
                dbutils.update_table(calibration_update_data, "calibrationinput")


                projectobsno_update_data = {
                    "set": {
                        "status": status,
                        "comments": comments
                    },
                    "where": {
                        "project_id": project_id
                     }
                }
                dbutils.update_table(projectobsno_update_data, "projectobsno")

    def __init__(self):
        self.stage4() # COMBINE_LSB_USB

Pipeline()
