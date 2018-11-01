import os
import glob
from DBUtils import DBUtils
import getScangroup as gS
import time
import datetime
from config import Config
import tableSch as tableSchema
import subprocess
import sys
import spam

currentTimeInSec = time.time()

class FileUtils:

    def check_network_connectivity(self):
        pass


    def calculalate_file_sizse_in_MB(self, filename):
        return ((os.path.getsize(filename))/1024)/1024


    def tail(f, lines=1, _buffer=4098):
        """Tail a file and get X lines from the end"""
        # place holder for the lines found
        lines_found = []

        # block counter will be multiplied by buffer
        # to get the block size from the end
        block_counter = -1

        # loop until we find X lines
        while len(lines_found) < lines:
            try:
                f.seek(block_counter * _buffer, os.SEEK_END)
            except IOError:  # either file is too small, or too many lines requested
                f.seek(0)
                lines_found = f.readlines()
                break

            lines_found = f.readlines()
            block_counter -= 1

        return lines_found[-lines:]


    def copy_files(self, src, dest):
        try:
            print("Copying "+src+"* to "+dest)
            if not os.path.exists(dest):
                os.system('mkdir -p '+dest)

            if os.path.isfile(src):
                os.system('cp '+src+' '+dest)
            else:
                os.system('cp -r '+src+'* '+dest)
        except Exception as ex:
            print(ex)


    def move_files(self, src, dest):
        try:
            print("Moving "+src+" to "+dest)
            if not os.path.exists(dest):
                os.system('mkdir -p '+dest)
            os.system('mv '+src+' '+dest)
        except Exception as ex:
            print(ex)

    def delete_file_dir(self, path):
        try:
            print("Deleting "+path)
            if os.path.isdir(path):
                os.system('rm -r '+path)
            if os.path.isfile(path):
                os.system('rm '+path)
        except Exception as ex:
            print(ex)

    def check_for_multiples(self, filename):
        files_list = glob.glob(filename+"*")
        files_list.sort()
        if len(files_list) > 1:
            return files_list

    def insert_details(self, data, project_path, isghb, cycle_id, status):
        print(data)
        dbutils = DBUtils()
        for each_rec in data:
            lta_file = os.path.basename(each_rec)
            try:
                current_date_timestamp = datetime.datetime.fromtimestamp(currentTimeInSec).strftime('%Y-%m-%d %H:%M:%S')
                lta_details = gS.get_naps_scangroup_details(lta_file)
                utils = self
                lta_details["ltacomb_size"] = int(utils.calculalate_file_sizse_in_MB(each_rec))
                lta_details["status"] = "unprocessed"
                lta_details["file_path"] = project_path
                lta_details["start_time"] = current_date_timestamp
                lta_details["proposal_dir"] = project_path.split('/')[-2]
                lta_details["pipeline_id"] = 1
                lta_details["comments"] = status
                lta_details["counter"] = 0
                lta_details["ltacomb_file"] = lta_file
                lta_details["isghb"] = isghb
                lta_details["cycle_id"] = cycle_id


                projectobsno_data = {}
                for key in tableSchema.projectobsnoData.iterkeys():
                    if key in lta_details.iterkeys():
                        projectobsno_data[key] = lta_details[key]

                ltadetails_data = {}
                for key in tableSchema.ltadetailsData.iterkeys():
                    if key in lta_details.iterkeys():
                        ltadetails_data[key] = lta_details[key]
                #print ltadetails_data
                # print("ltadetails_data")
                # print(ltadetails_data)

                columnKeys = {"project_id"}
                whereKeys = {"proposal_dir": lta_details["proposal_dir"], "cycle_id": cycle_id}

                project_id = dbutils.select_test_table("projectobsno", columnKeys, whereKeys, 0)

                if project_id:
                    project_id = project_id[0]
                else:
                    project_id = dbutils.insert_into_table("projectobsno", projectobsno_data, tableSchema.projectobsnoId)

                ltadetails_data["project_id"] = project_id

                lta_id = dbutils.insert_into_table("ltadetails", ltadetails_data, tableSchema.ltadetailsId)
                print lta_id
                print("projectobsno")
                print(projectobsno_data)
            except Exception as e:
                print e

    def insert_calibrationinput(self):
        pass


    def check_haslam_flag(self, project, obsno):
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

        UVFITS_DATA = "/data1/CYCLE17/"

        # cmd = "fgrep '13.' " + UVFITS_DATA + "*/*.obslog | grep 'OFF' | cut -d ':' -f 1"
        # cmd = "fgrep '14.' " + UVFITS_DATA + "*/*/"+str(obsno)+".obslog | grep 'OFF' | cut -d ':' -f 1"
        cmd = "cat /GARUDATA/C17_ACL.list"
        output = subprocess.check_output(cmd, shell=True)
        obs_list = output.split('\n')
        obs_list.remove('')
        for is_haslam_flagged in obs_list:
            return any(project in string for string in obs_list)


    def run_spam_combine_usb_lsb(self, data):
        (usb,lsb), comb_name = data
        print("================> "+str(os.getcwd())+" <==============")
        usb_cp = os.system('cp '+usb+' '+"fits/")
        print('cp '+usb+' '+'fits/')
        lsb_cp = os.system('cp '+lsb+' '+"fits/")
        print('cp '+lsb+' '+'fits/')
        status = "failed"
        comments = "something went wrong!"
        base_path = os.path.dirname(usb)+"/PRECALIB/"
        usb = "fits/"+os.path.basename(usb)
        lsb = "fits/"+os.path.basename(lsb)
        comb_name = "fits/"+os.path.basename(comb_name)
        original_stdout = sys.stdout
        original_stderr = sys.stderr
        combine_usb_lsb_log = open('combusblsb_stdout.log', 'a+')
        combine_usb_lsb_log.write('\n\n******COMB USB LSB STARTED******\n\n')
        sys.stdout = combine_usb_lsb_log
        sys.stderr = combine_usb_lsb_log
        try:
            print(usb, lsb, comb_name)
            spam.combine_usb_lsb(usb, lsb, comb_name)
            status = "success"
            comments = "done combining usb lsb"
        except Exception as ex:
            comments = str(ex)
        sys.stdout = original_stdout
        sys.stderr = original_stderr
        # os.system('rm '+usb+' '+lsb)
        # os.system('cp datfil/* '+base_path)
        # os.system('cp fits/* '+base_path)
        return status, comments

    def run_spam_precalibration_stage(self, UVFITS_BASE_DIR, DIR, UVFITS_FILE_NAME, OBSNO):
        print "Running SPAM pre_calibrate_targets on " + DIR
        os.chdir(DIR)
        print(DIR)
        # UVFITS_FILE_NAME = glob.glob(DIR + "/*.UVFITS")
        print(UVFITS_FILE_NAME)
        if UVFITS_FILE_NAME:
            original_stdout = sys.stdout
            original_stderr = sys.stderr
            precal_log = open('precal_stdout.log', 'a+')
            precal_log.write('\n\n******PRECALIBRATION STARTED******\n\n')
            sys.stdout = precal_log
            sys.stderr = precal_log
            PROJECT_CODE = UVFITS_BASE_DIR.split('/')[-1]
            print(PROJECT_CODE, OBSNO)
            print(UVFITS_FILE_NAME)
            if self.check_haslam_flag(PROJECT_CODE, OBSNO):
                print(PROJECT_CODE + " Flagging apply_tsys=False")
                try:
                    spam.pre_calibrate_targets(UVFITS_FILE_NAME, apply_tsys=False, keep_channel_one=True)
                except Exception as e:
                    failed_log = open('failed_log.txt', 'a+')
                    failed_log.write("Failed Error: " + str(e))
                    failed_log.flush()
            else:
                print(PROJECT_CODE + " Flagging apply_tsys=True")
                try:
                    spam.pre_calibrate_targets(UVFITS_FILE_NAME, keep_channel_one=True)
                except Exception as e:
                    failed_log = open('failed_log.txt', 'a+')
                    failed_log.write("Failed Error: " + str(e))
                    failed_log.flush()
            self.delete_file_dir(UVFITS_FILE_NAME)
            PROCCEED_FILE_LIST = glob.glob(DIR + "/*")
            sys.stdout = original_stdout
            sys.stderr = original_stderr
            precal_log.flush()
            if PROCCEED_FILE_LIST:
                for EACH_FILE in PROCCEED_FILE_LIST:
                    self.move_files(EACH_FILE, UVFITS_BASE_DIR + "/PRECALIB")

