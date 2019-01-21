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

    def stage1(self, gdata):
        print("Started Stage1: ")
        spamutils = SpamUtils()
        fileutils = FileUtils()

        data = gdata[0]
        path = gdata[1]
        obs_no = []

        # print(len(data))
        co=0
        for each_obs in data:
            co+=1
            file_path = data[each_obs]['file_path']
            dest_path = path+str(int(each_obs))+'/'+file_path.split('/')[-2]
            lta_file = ""
            lta_list = glob.glob(file_path + '/*.lta*')
            lta_list.sort()
            status = "unprocessed"
            if lta_list:
                if len(lta_list) > 1:
                    checked = []
                    for each_lta in lta_list:
                        if each_lta not in checked:
                            to_comb_lta = glob.glob(each_lta+'*')
                            for x in to_comb_lta:
                                checked.append(x)
                            if len(to_comb_lta) > 1:
                                to_comb_lta.sort()
                                # print(dest_path, to_comb_lta)
                                status = spamutils.run_ltacomb(to_comb_lta, dest_path)
                                lta_file = to_comb_lta[0]
                            else:
                                lta_file = each_lta
                                fileutils.copy_files(each_lta, dest_path)
                                print(each_lta, dest_path)
                else:
                    lta_file = lta_list[0]
                    fileutils.copy_files(lta_file, dest_path)
                    # print("---------------------------", lta_list)
                if co == 955:
                    break
            fileutils.insert_details([lta_file], dest_path, 'false', data[each_obs]['cycle_id'], status, each_obs)
                # lta_file_16s = (s for s in lta_list if "_16s" in s)
                # lta_file_8s = (s for s in lta_list if "_8s" in s)
                # lta_file_4s = (s for s in lta_list if "_4s" in s)
                # lta_file_2s = (s for s in lta_list if "_2s" in s)
                # if lta_file_8s.__sizeof__():
                #     print("8s")
                # elif lta_file_16s.__sizeof__():
                #     print("16s")
                # elif lta_file_4s.__sizeof__():
                #     print("4s")
                # elif lta_file_2s.__sizeof__():
                #     print("4s")
                # else:
                #     print("copying only NORMAL", lta_list)



            #
            # try:
            #     # fileutils.copy_files(file_path, dest_path)
            #     print("copying", file_path, dest_path)
            # except Exception as ex:
            #     print(ex)
            #
            #
            # # lta_list = glob.glob(dest_path+'/*.lta*')
            #
            # if len(lta_list) > 1:
            #     print(lta_list)
            # else:
            #     if lta_list:
            #         print(glob.glob(file_path+'/*'))


        """

            obs_no.append(each_obs)
            print(data[each_obs])
            file_path = data[each_obs]['file_path']
            print(file_path)
            dir_path = file_path.split('/')[-2]
            project_path = path+str(int(each_obs))+'/'+dir_path
            print("********"+file_path+";;;;;;;;;"+dir_path+"------"+project_path)
            lta_list = glob.glob(file_path+'/*lta*')
            status = "processing"
            if len(lta_list) > 1:
                print(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>")
                lta_list.sort()
                lta_tracker = []
                for each_lta in lta_list:
                    split_lta = each_lta.split('.')[0]
                    if each_obs not in lta_tracker:
                        lta_tracker.append(each_obs)
                        this_lta_list = glob.glob(split_lta+'.lta*')
                        this_lta_list.sort()
                        if not os.path.exists(project_path):
                            os.makedirs(project_path)
                        status = spamutils.run_ltacomb(this_lta_list, project_path)
                        print(this_lta_list[0], project_path, 'false', data[each_obs]['cycle_id'],status, each_obs)
                        fileutils.insert_details([this_lta_list[0]], project_path, 'false', data[each_obs]['cycle_id'], status, each_obs)
            elif lta_list:
                print("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<")
                print(lta_list[0], project_path)
                # fileutils.copy_files(lta_list[0], project_path)
                if not os.path.exists(project_path):
                    print([lta_list[0]], project_path, 'false', data[each_obs]['cycle_id'], status, each_obs)
                    fileutils.insert_details([lta_list[0]], project_path, 'false', data[each_obs]['cycle_id'], status,
                                              each_obs)
        """


    def stage1_ghb(self, gdata):
        print("Started Stage1: ")
        spamutils = SpamUtils()
        fileutils = FileUtils()

        data = gdata[0]
        path = gdata[1]

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

    def __prerequisites(self):
        CYCLE_ID = str(26)
        CYCLE_PATH = '/GARUDATA/IMAGING'+CYCLE_ID+'/CYCLE'+CYCLE_ID+'/'
        print(CYCLE_PATH)

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


    def __prerequisites_ghb(self):
        CYCLE_ID = 15
        CYCLE_PATH = '/GARUDATA/IMAGING15/CYCLE15/'
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
        self.stage1(self.__prerequisites()) # LTACOMB

Pipeline()
