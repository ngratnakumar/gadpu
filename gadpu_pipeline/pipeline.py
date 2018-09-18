#from gadpu_pipeline.FileUtils import FileUtils
from DBUtils import DBUtils
from SpamUtils import SpamUtils

import glob
import os
import spam

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
        
    data = gdata[0]
    path = gdata[1]

    gsb_obs = []
    ghb_obs = []

    for each_obs in data:
        proposal_id = data[each_obs]['proposal_id']
        file_path = data[each_obs]['file_path']
        backend_type = data[each_obs]['backend_type']

        #print(each_obs, file_path)
        project_path = path+"/".join(file_path.split('/')[-3:])

        #print(project_path)

        files_list = glob.glob(project_path+'/*.lt*')
        comb_list = []

        print("Checking GHB and GSB listings: "+proposal_id)

        for each_file in files_list:
            lta = each_file
            ltb = lta.replace('lta', 'ltb')

            if ltb is each_file:
                ghb_obs.append(each_obs)
            else:
                if '_gsb.lta' in each_file:
                    gsb_obs.append(each_obs)

        gsb_obs = list(set(gsb_obs))
        ghb_obs = list(set(ghb_obs))
        print(len(gsb_obs), len(ghb_obs))

        print("Running LTACOMB: ")

        for each_gsb in gsb_obs:
            file_path = data[each_gsb]['file_path']
            project_path = path+"/".join(file_path.split('/')[-3:])
            gsb_files = glob.glob(project_path+"/*_gsb*")
            gsb_files.sort()
            #print(gsb_files)
            if len(gsb_files) > 1:
                print("Running LTACOMB")
                spamutils.run_ltacomb(gsb_files, project_path)
            else:
                #print("LTACOMB not required for "+ str(gsb_files))
		pass

        self.stage2((gsb_obs, ghb_obs, data, path))

    def stage2(self, data):
        """
        Generating Uncalibrated UVFITS using GVFITS
        :param gsb_observations, ghb_observations, observations_list:
        :return None, generates uncalibrated UVFITS:
        """
	
    spamutils = SpamUtils()

        gsb_data, ghb_data, obs_data, cycle_path = data
        print(gsb_data, ghb_data)

        for each_gsb in gsb_data:
            file_path = obs_data[each_gsb]["file_path"]
            project_path = cycle_path+"/".join(file_path.split('/')[-3:])
            files_list = glob.glob(project_path+"/*gsb*")
            if len(files_list) == 1:
                lta = files_list[0].split('/')[-1]
                uvfits = lta+'.UVFITS'
                os.chdir(project_path)
            check_uvfits_exisits = glob.glob(project_path+'/*.UVFITS')
            print(check_uvfits_exisits)
            if not check_uvfits_exisits:
                spamutils.run_gvfits(lta,uvfits)
            else:
                print("UVFITS already exists!")
			

        for each_ghb in ghb_data:
            file_path = obs_data[each_ghb]["file_path"]
            project_path = cycle_path+"/".join(file_path.split('/')[-3:])
            lta_list = glob.glob(project_path+"/*lta*")
            lta = lta_list[0]
            uvfits = lta+'.UVFITS'
            if 'gsb' not in lta:
                os.chdir(project_path)
                check_uvfits_exisits = glob.glob(project_path+'/*.UVFITS')
                print(check_uvfits_exisits)
                if not check_uvfits_exisits:
                    spamutils.run_gvfits(lta, uvfits)
        else:
            print("UVFITS already exists!")

            ltb_list = glob.glob(project_path+"/*ltb*")
            if len(ltb_list) == 1:
                ltb = ltb_list[0]
                uvfits = ltb+'.UVFITS'
                gsb_uvfits = glob.glob(project_path+'/*gsb*.UVFITS')
                os.chdir(project_path)
                check_uvfits_exisits = glob.glob(project_path+'/*B.UVFITS')
                print(check_uvfits_exisits)
                if not gsb_uvfits:
                    if not check_uvfits_exisits:
                        pamutils.run_gvfits(ltb, project_path)
            else:
                print("UVFITS already exists! -- "+str(check_uvfits_exisits)+" | "+str(gsb_uvfits))

    def stage3(self):
        uvfits_list = glob.glob('/GARUDATA/IMAGING19/CYCLE19/*/*/*.UVFITS')
        for each_uvfits in uvfits_list:
            spamutils = SpamUtils()
            spam.set_aips_userid(11)
            project_code = os.path.dirname(each_uvfits)
            copy_uvfits = os.system('cp '+each_uvfits+' fits/')
            uvfits_file = each_uvfits.split('/')[-1]
            spamutils.run_precalibrate_targets(each_uvfits)
        spam.exit()


    def stage4(self):
        pass


    def stage5(self):
        pass



    def __prerequisites(self):
        CYCLE_ID = 18
        CYCLE_PATH = '/GARUDATA/IMAGING18/CYCLE18/'
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
                "backend_type": each_row[3]
            }

        return (gadpudata, CYCLE_PATH)
        #
        # stagger = {
        #     1: self.stage1,
        #     2: self.stage2,
        #     3: self.stage3,
        #     4: self.stage4,
        #     5: self.stage5
        # }
        # jumper = stagger.get(STAGE_FLAG, lambda: "Invalid Stage")


    def __init__(self):
        self.stage1(self.__prerequisites())
        #print(self.__prerequisites())
	#self.stage3()

Pipeline()
