from FileUtils import FileUtils
from DBUtils import DBUtils
from SpamUtils import SpamUtils
import tableSch as tableSchema

from astropy.io import fits
from time import sleep

import os
import spam
import time
import glob
import datetime
import sys
import random


class Pipeline:

    def stage5(self):
        """
        Stage5: Running SPAM's process_target and populates dataproducts table
        """
        fileutils = FileUtils()
        # aips_id = int(random.random()*100)
        spam.set_aips_userid(11)
        # Setting the Process Start Date Time
        start_time = str(datetime.datetime.now())
        # Taking system's in/out to backup variable
        original_stdout = sys.stdout
        original_stderr = sys.stderr
        thread_dir = os.getcwd()
        # Changing directory to fits/
        os.chdir("fits/")
        datfil_dir = thread_dir + "/datfil/"
        fits_dir = thread_dir + "/fits/"
        curr_dir = thread_dir + "/fits/"
        process_status = False
        db_model = DBUtils()
        # Get random imaging_id & project_id
        column_keys = [tableSchema.imaginginputId, tableSchema.projectobsnoId, "calibrated_fits_file"]
        where_con = {
            "status": "cycle226"
        }
        to_be_processed = db_model.select_from_table("imaginginput", column_keys, where_con, None)
        print(len(to_be_processed))
        print(to_be_processed)
        imaginginput_details = random.choice(to_be_processed)
        print(imaginginput_details)
        imaging_id = imaginginput_details["imaging_id"]

        # Update status for imaginginput for selected imaging_id
        current_time_in_sec = time.time()
        current_date_timestamp = datetime.datetime.fromtimestamp(current_time_in_sec).strftime('%Y-%m-%d %H:%M:%S')
        update_data = {
            "set": {
                "status": "processing",
                "start_time": current_date_timestamp,
                "comments": "processing_cycle20_process_target ",
                "end_time": current_date_timestamp
            },

            "where": {
                "imaging_id": imaging_id,
            }
        }
        db_model.update_table(update_data, "imaginginput")

        project_id = imaginginput_details["project_id"]
        calibrated_fits_file = imaginginput_details["calibrated_fits_file"]

        # Using the above project_id, fetch base_path
        column_keys = ["file_path"]
        where_con = {
            "project_id": project_id
        }
        process_target_log = open('process_target.log', 'a+')
        process_target_log.write('\n\n\n******PROCESS TARGET STARTED******\n\n\n')
        process_target_log.write("--->  Start Time " + start_time)
        # Logging all Standard In/Output
        sys.stdout = process_target_log
        sys.stderr = process_target_log
        base_path = db_model.select_from_table("projectobsno", column_keys, where_con, 0)
        base_path = base_path[0]
        uvfits_full_path = base_path + "/PRECALIB/" + calibrated_fits_file
        # uvfits_full_path = base_path+"/PRECALIB/"+calibrated_fits_file
        print "Copying " + uvfits_full_path + " to " + fits_dir
        copying_fits = os.system("cp " + uvfits_full_path + " " + fits_dir)
        uvfits_file = calibrated_fits_file
        # Starting spam.process_target(SPLIT_FITS_FILE)
        try:
            spam.process_target(uvfits_file, allow_selfcal_skip=True, add_freq_to_name=True)
            # If this process_target is success call
            # GADPU API setSuccessStatus for the current fits_id
            current_time_in_sec = time.time()
            current_date_timestamp = datetime.datetime.fromtimestamp(current_time_in_sec).strftime('%Y-%m-%d %H:%M:%S')
            success_update_data = {
                "set": {
                    "status": "checking",
                    "end_time": current_date_timestamp,
                    "comments": "processing done, checking"
                },
                "where": {
                    "imaging_id": imaging_id
                }
            }
            db_model.update_table(success_update_data, "imaginginput")
        except Exception, e:
            process_target_log.write("Error: " + str(e))
            # If this process_target is a failure call
            # GADPU API setFailedStatus for the current fits_id
            current_date_timestamp = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')
            success_update_data = {
                "set": {
                    "status": "failed",
                    "end_time": current_date_timestamp,
                },
                "where": {
                    "imaging_id": imaging_id
                }
            }
            db_model.update_table(success_update_data, "imaginginput")
            print("Error: spam.process_tagret Failed " + uvfits_file)
            # Even the process is Success/Failed we remove
            # the Initially copied SPLIT_FITS_file, to save
            # disk space
        os.system('rm ' + uvfits_file)
        # recording the process end time to Log
        end_time = str(datetime.datetime.now())
        image_base_dir = base_path + "/FITS_IMAGE/"
        # Creating a new dir at BASE_DIR - FITS_IMAGES
        print "Make dir at " + image_base_dir
        os.system("mkdir -p " + image_base_dir)
        # STDIN/OUT controls are reverted.
        process_target_log.write("End Time " + end_time)
        # Flushing the all processed out log to the log_file
        process_target_log.flush()
        # reverting stdin/out controls
        sys.stdout = original_stdout
        sys.stderr = original_stderr
        # Getting the list of datfil/spam_logs for summarize the process
        spam_log = glob.glob(datfil_dir + "spam*.log")
        # if spam_log is non-empty list, proceed
        print spam_log
        failed_msg = "something went wrong"
        try:
            if spam_log:
                # for every spam*.log file in the datfile file
                for each_spam_log in spam_log:
                    original_stdout = sys.stdout
                    original_stderr = sys.stderr
                    failed = os.popen('grep "processing of field" ' + each_spam_log + ' | grep "failed" | wc -l').read()
                    if int(failed.strip()) > 0:
                        failed_msg = os.popen('fgrep "Error:" ' + each_spam_log + '').read()
                        current_date_timestamp = datetime.datetime.fromtimestamp(time.time()).strftime(
                            '%Y-%m-%d %H:%M:%S')
                        failed_update_data = {
                            "set": {
                                "status": "failed",
                                "end_time": current_date_timestamp,
                                "comments": failed_msg
                            },
                            "where": {
                                "imaging_id": imaging_id
                            }
                        }
                        db_model.update_table(failed_update_data, "imaginginput")
                    else:
                        process_status = True
                    print each_spam_log
                    # getting summary of the log file
                    summ_file = each_spam_log.replace(".log", ".summary")
                    print summ_file
                    summary_filename = open(summ_file, 'a+')
                    # making the spam*.summary file and write the
                    # summarize_spam_log output
                    summary_filename.write('\n\n******SUMMARY LOG******\n\n')
                    sys.stdout = summary_filename
                    sys.stderr = summary_filename
                    spam.summarize_spam_log(each_spam_log)
                    sys.stdout = original_stdout
                    sys.stderr = original_stderr
                    summary_filename.flush()
        except Exception as ex:
            print(ex)
        # Once the summary file is created inside the fits/
        # Moving all the files from datfil/ to fits/
        # Moving all the processed files from fits/ to FITS_IMAGE@BASE_DIR
        print "moving back the processed files from " + fits_dir + " to " + image_base_dir
        # The below print statement is only for recording purpose,
        # actual removing the THREAD directory is done after the
        # Move all the fits/ to FITS_IMAGE@BASE_DIR
        print "Moving datfil/ to fits/"
        current_date_timestamp = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')

        moving_update_data = {
            "set": {
                "status": "moving",
                "end_time": current_date_timestamp,
                "comments": "Moving to NAS"
            },
            "where": {
                "imaging_id": imaging_id
            }
        }

        db_model.update_table(moving_update_data, "imaginginput")
        movedata = os.system('mv ' + datfil_dir + '* ' + fits_dir)
        if process_status:
            self.update_datproducts(curr_dir, project_id, imaging_id, db_model)
            sleep(5)
        movefits = os.system("mv " + fits_dir + "* " + image_base_dir)
        sleep(5)
        # current THREAD dir
        # Changing the directory to /home/gadpu, inorder to delete the
        os.chdir('../../')
        print "Changed to " + os.getcwd()

        # Removing the current THREAD directory
        removethread = os.system('rm -rf ' + thread_dir)
        current_date_timestamp = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')
        if process_status:
            status = "success"
        else:
            status = "failed"
        done_update_data = {
            "set": {
                "status": status,
                "end_time": current_date_timestamp,
                "comments": failed_msg
            },
            "where": {
                "imaging_id": imaging_id
            }
        }
        db_model.update_table(done_update_data, "imaginginput")
        # exiting the SPAM process and cleaning the cache memory
        spam.exit()

    def update_datproducts(self, curr_dir, project_id, imaging_id, db_model):
        """
        Used by stage5 process_target functionality
        :param project_id:
        :param imaging_id:
        :param db_model:
        :return:
        """
        fileutils = FileUtils()
        products_list = glob.glob(curr_dir + '/*')
        for each_product in products_list:
            current_time_in_sec = time.time()
            product_data = {
                'project_id': project_id,
                'imaging_id': imaging_id,
                "file_size": fileutils.calculalate_file_sizse_in_MB(each_product),
                "file_type": each_product.split('.')[-1],
                "file_name": each_product.split('/')[-1],
                "generated": datetime.datetime.fromtimestamp(current_time_in_sec).strftime('%Y-%m-%d %H:%M:%S'),
                "status": "success"
            }
            print db_model.insert_into_table("dataproducts", product_data, tableSchema.dataproductsId)

    def __init__(self):
        rand_sec = int(random.random()*100)
        print("Waiting for "+str(rand_sec)+" seconds ... ")
        sleep(rand_sec)
        print("Starting process ... ")
        self.stage5() # PROCESS_TARGET

Pipeline()
