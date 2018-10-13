from FileUtils import FileUtils
from DBUtils import DBUtils
from SpamUtils import SpamUtils
import time
import datetime
import glob
import os
# import spam

class Pipeline:


    def stage1(self, gdata):
        """
        RUN LTACOMB
        1) Identify the GSB and GHB observations
        2) Do LTACOMB if required
        :param gadpu-data observation_no from napsgoadb:
        :return: None, does the LTACOMB
        """

        print("Started Stage1: ")

        spamutils = SpamUtils()
        fileutils = FileUtils()

        data = gdata[0]
        path = gdata[1]


        for each_obs in data:
            proposal_id = data[each_obs]['proposal_id']
            file_path = data[each_obs]['file_path']
            backend_type = data[each_obs]['backend_type']
            cycle_id = data[each_obs]['cycle_id']

            project_path = path+"/".join(file_path.split('/')[-3:])

            lta, ltb, gsb = None, None, None
            isghb = False
            status = "cycle17"

            files_list = glob.glob(file_path+'*.lt*')
            files_list.sort()

            if len(files_list) == 1:
                lta = files_list
            elif any("gsb" in lt_list for lt_list in files_list):
                gsb = [x for x in files_list if "gsb" in x][0]
                cgsb = fileutils.check_for_multiples(gsb)
                if cgsb:
                    # print("LTACOMB: "+str(cgsb)+" "+project_path+""+os.path.basename(gsb))
                    status = spamutils.run_ltacomb(cgsb, project_path)
            elif any("ltb" in lt_list for lt_list in files_list):
                # print(files_list)
                isghb = True
                lta = [x for x in files_list if "lta" in x][0]
                ltb = [x for x in files_list if "ltb" in x][0]
                clta = fileutils.check_for_multiples(lta)
                cltb = fileutils.check_for_multiples(ltb)
                if clta:
                    # print("LTACOMB: "+str(clta)+" "+project_path+""+os.path.basename(lta))
                    status = spamutils.run_ltacomb(clta, project_path)
                if cltb:
                    # print("LTACOMB: "+str(cltb)+" "+project_path+""+os.path.basename(ltb))
                    status = spamutils.run_ltacomb(cltb, project_path)

            # print(isghb, lta, ltb, gsb)

            if isghb:
                fileutils.copy_files(lta, project_path)
                fileutils.copy_files(ltb, project_path)
                fileutils.insert_details([lta, ltb], project_path, isghb, cycle_id, status)
            else:
                if gsb:
                    fileutils.copy_files(gsb, project_path)
                    fileutils.insert_details(gsb, project_path, isghb, cycle_id, status)
                if lta:
                    fileutils.copy_files(lta, project_path)
                    fileutils.insert_details(lta, project_path, isghb, cycle_id, status)


    def stage2(self):
        """
        Generating Uncalibrated UVFITS using GVFITS
        :param gsb_observations, ghb_observations, observations_list:
        :return None, generates uncalibrated UVFITS:
        """

        dbutils = DBUtils()
        spamutils = SpamUtils()
        fileutils = FileUtils()

        currentTimeInSec = time.time()
        current_date_timestamp = datetime.datetime.fromtimestamp(currentTimeInSec).strftime('%Y-%m-%d %H:%M:%S')

        columnKeys = {"project_id", "ltacomb_file", "lta_id"}
        whereKeys = {"comments": "cycle17"}

        lta_details = dbutils.select_from_table("ltadetails", columnKeys, whereKeys, None)

        print(lta_details)

        for each_lta in lta_details:
            print(each_lta)
            project_id = each_lta["project_id"]
            #project_id = each_lta[0]
            lta_file = each_lta["ltacomb_file"]
            #lta_file = each_lta[1]
            #lta_id = each_lta[2]
            lta_id = each_lta["lta_id"]
            columnKeys = {"file_path"}
            whereKeys = {"project_id": project_id}
            lta_path_details = dbutils.select_test_table("projectobsno", columnKeys, whereKeys, 0)
            print(lta_path_details)
            base_path = lta_path_details[0]
            print(base_path)
            uvfits_file = lta_file+'.UVFITS'
            base_lta = base_path+'/'+lta_file
            if os.path.exists(base_lta):
                base_uvfits = base_path+'/'+uvfits_file
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


    def stage3(self):
        dbutils = DBUtils()
        fileutils = FileUtils()

        columnKeys = {"calibration_id", "project_id", "uvfits_file", "observation_no"}
        whereData = {"comments": "c17", "status": "success"}
        uncalibrated_uvfits = dbutils.select_from_table("calibrationinput", columnKeys, whereData, 0)

        calibration_id = uncalibrated_uvfits[0]
        project_id = uncalibrated_uvfits[1]
        uvfits_file = uncalibrated_uvfits[2]
        observation_no = uncalibrated_uvfits[3]

        columnKeys = {"file_path"}
        whereData = {"project_id": project_id, "cycle_id": 17}
        project_details = dbutils.select_from_table("projectobsno", columnKeys, whereData, 0)

        base_path = project_details[0]

        """
        TODO: 
        
            1 Update the projectobsno's project_id status to 'processing'
            and comments to 'running precalibrate_target'
            
            2 Update the calibrationinput's calibration_id status to 'processing'
            
            3 Start precalibrate_target
            
            4 depending on the process status, update projectobsno and calibrationinput
               
        """

        current_time_in_sec = time.time()
        current_date_timestamp = datetime.datetime.fromtimestamp(current_time_in_sec).strftime('%Y-%m-%d %H:%M:%S')

        projectobsno_update_data = {
            "set": {
                "status": "processing",
                "comments": "running precalibrate_target, calibration_id = "+str(calibration_id),
                "start_time": current_date_timestamp
            },
            "where": {
                "project_id": project_id,
                "status": "unprocessed"
            }
        }

        calibration_update_data = {
            "set": {
                "status": "processing",
                "start_time": current_date_timestamp
            },
            "where": {
                "calibration_id": calibration_id,
                "status": "success"
            }
        }

        dbutils.update_table(projectobsno_update_data, "projectobsno")
        dbutils.update_table(calibration_update_data, "calibrationinput")

        UVFITS_FILE_NAME = uvfits_file
        UVFITS_BASE_DIR = base_path
        is_fits_dir = os.getcwd().split('/')
        SPAM_WORKING_DIR = os.getcwd()
        SPAM_THREAD_DIR = ""
        for num in range(1, 4):
            SPAM_THREAD_DIR += "/" + is_fits_dir[num]
        if 'fits' not in is_fits_dir:
            SPAM_THREAD_DIR = os.getcwd()
            SPAM_WORKING_DIR = os.getcwd() + "/fits/"
        fileutils.copy_files(UVFITS_BASE_DIR+'/'+UVFITS_FILE_NAME, SPAM_WORKING_DIR)
        print "Copying done ==> Moving to pre_cal_target"
        fileutils.run_spam_precalibration_stage(UVFITS_BASE_DIR, SPAM_WORKING_DIR, UVFITS_FILE_NAME, OBSNO)

        current_time_in_sec = time.time()
        current_date_timestamp = datetime.datetime.fromtimestamp(current_time_in_sec).strftime('%Y-%m-%d %H:%M:%S')

        check_status_file = glob.glob(base_path+"/failed_log.txt")

        if check_status_file:
            status = "failed"
        else:
            status = "success"

        projectobsno_update_data = {
            "set": {
                "status": status,
                "comments": "precalibrate_target done, calibration_id = "+str(calibration_id),
                "end_time": current_date_timestamp
            },
            "where": {
                "project_id": project_id
            }
        }

        calibration_update_data = {
            "set": {
                "status": status,
                "end_time": current_date_timestamp
            },
            "where": {
                "calibration_id": calibration_id
            }
        }

        dbutils.update_table(projectobsno_update_data, "projectobsno")
        dbutils.update_table(calibration_update_data, "calibrationinput")

    def stage4(self):
        pass


    def stage5(self):
        pass


    def __prerequisites(self):
        CYCLE_ID = 17
        CYCLE_PATH = '/GARUDATA/IMAGING17/CYCLE17/'
        CYCLE_ID = str(CYCLE_ID)

        sql_query = "select distinct o.observation_no, p.proposal_id, g.file_path, p.backend_type from " \
                    "gmrt.proposal p inner join das.observation o on p.proposal_id = o.proj_code " \
                    "inner join das.scangroup g on g.observation_no = o.observation_no " \
                    "inner join das.scans s on s.scangroup_id = g.scangroup_id " \
                    "inner join gmrt.sourceobservationtype so on p.proposal_id = so.proposal_id " \
                    "where p.cycle_id ='" + CYCLE_ID + "' " \
                    "and so.obs_type not like 'pulsar%' " \
                    "and so.obs_type not like 'phased array'" \
                    "and s.sky_freq1=s.sky_freq2 " \
                    "and s.sky_freq1 < 900000000 " \
                    "and s.chan_width >= 62500 " \
                    "and o.proj_code not like '16_279' " \
                    "and o.proj_code not like '17_072' " \
                    "and o.proj_code not like '18_031' " \
                    "and o.proj_code not like '19_043' " \
                    "and o.proj_code not like '20_083' " \
                    "and o.proj_code not like '21_057';"

        dbutils = DBUtils()

        data = dbutils.select_query(sql_query)
        gadpudata = {}
        for each_row in data:
            gadpudata[each_row[0]] = {
                "proposal_id": each_row[1],
                "file_path": each_row[2],
                "backend_type": each_row[3],
                "cycle_id": CYCLE_ID
            }
        return (gadpudata, CYCLE_PATH)

    def __init__(self):
        #self.stage1(self.__prerequisites())
        self.stage3()

Pipeline()
