# from FileUtils import FileUtils
from DBUtils import DBUtils
# from SpamUtils import SpamUtils
import time
import datetime
import glob
import os
from astropy.io import fits
import random

class Pipeline:

    def stage1(self, gdata):
        print("Started Stage1: ")
        # spamutils = SpamUtils()
        # fileutils = FileUtils()

        data = gdata[0]
        path = gdata[1]

        print(data)
        print(path)

        obs_no = []
        cnt = 0
        for each_obs in data:
            obs_no.append(each_obs)
            project_path = path
            lta_list = glob.glob(data[each_obs]['file_path']+'/*lta*')
            if len(lta_list) >1:
                lta_list.sort()
                lta_tracker = []
                for each_lta in lta_list:
                    split_lta = each_lta.split('.')[0]
                    if split_lta not in lta_tracker:
                        lta_tracker.append(split_lta)
                        this_lta_list = glob.glob(split_lta+'.lta*')
                        this_lta_list.sort()
                        cnt+=1
                        # status = spamutils.run_ltacomb(this_lta_list, project_path)
                        # fileutils.insert_details(this_lta_list[0], project_path, 'false', data[each_obs]['cycle_id'], status)
            if lta_list:
                print(str(each_obs)+' -- '+lta_list[0])
                cnt+=1
                # fileutils.insert_details(lta_list[0], project_path, 'false', data[each_obs]['cycle_id'], status)
        print(cnt)

        '''
        for each_obs in data:
            print("=========================================")
            print(each_obs)
            print("=========================================")
            proposal_id = data[each_obs]['proposal_id']
            file_path = data[each_obs]['file_path']
            backend_type = data[each_obs]['backend_type']
            cycle_id = data[each_obs]['cycle_id']
            print("********"+file_path)
            project_path = path + "/".join(file_path.split('/')[-3:])
            print("---------"+project_path)
            lta, ltb, gsb = None, None, None
            isghb = False
            status = "cycle15"

            files_list = glob.glob(file_path + '*.lt*')
            files_list.sort()
            print("Processing .... " + str(project_path))

            if len(files_list) == 1:
                lta = files_list[0]
            elif any("gsb" in lt_list for lt_list in files_list):
                gsb = [x for x in files_list if "gsb" in x][0]
                print(os.path.basename(gsb))
                cgsb = fileutils.check_for_multiples(gsb)
                if cgsb:
                    print("LTACOMB: "+str(cgsb)+" "+project_path+" "+os.path.basename(gsb))
                    status = spamutils.run_ltacomb(cgsb, project_path)
            elif any("ltb" in lt_list for lt_list in files_list):
                # print(files_list)
                isghb = True
                lta = [x for x in files_list if "lta" in x][0]
                ltb = [x for x in files_list if "ltb" in x][0]
                clta = fileutils.check_for_multiples(lta)
                cltb = fileutils.check_for_multiples(ltb)
                if clta:
                    print("LTACOMB: "+str(clta)+" "+project_path+""+os.path.basename(lta))
                    status = spamutils.run_ltacomb(clta, project_path)
                if cltb:
                    print("LTACOMB: "+str(cltb)+" "+project_path+""+os.path.basename(ltb))
                    status = spamutils.run_ltacomb(cltb, project_path)

            print(isghb, lta, ltb, gsb)

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
        '''

    def __prerequisites(self):
        CYCLE_ID = str(26)
        CYCLE_PATH = '/GARUDATA/IMAGING'+CYCLE_ID+'/CYCLE'+CYCLE_ID+'/'

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
                                                       "and o.proj_code not like '23_066' " \
                                                       "and o.proj_code not like '26_063' " \
                                                       "and o.proj_code not like 'ddtB014' " \
                                                       "and o.proj_code not like 'ddtB015' " \
                                                       "and o.proj_code not like 'ddtB028' " \
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
        # print(self.__prerequisites())
        self.stage1(self.__prerequisites()) # LTACOMB


Pipeline()