import DBUtils
import os
import spam
import glob

def initialize(cycle_id):
    cycle_id = str(cycle_id)

    sql_query = "select distinct o.observation_no, p.proposal_id, g.file_path, g.lta_file, g.ltb_file, g.lta_gsb_file from " \
                "gmrt.proposal p inner join das.observation o on p.proposal_id = o.proj_code " \
                "inner join das.scangroup g on g.observation_no = o.observation_no " \
                "inner join das.scans s on s.scangroup_id = g.scangroup_id " \
                "where cycle_id ='"+cycle_id+"' " \
                "and s.sky_freq1=s.sky_freq2 and s.sky_freq1 < 900000000 " \
                "and s.chan_width >= 62500 " \
                "and o.proj_code not like '16_279' " \
                "and o.proj_code not like '17_072' " \
                "and o.proj_code not like '18_031' " \
                "and o.proj_code not like '19_043' " \
                "and o.proj_code not like '20_083' " \
                "and o.proj_code not like '21_057';"

    dbutils = DBUtils.DBUtils()

    data = dbutils.select_query(sql_query)
    gadpudata = {}
    for each_row in data:
        obs_no = each_row[0]
        proposal_id = each_row[0]
        file_path = each_row[2]
        lta = each_row[3]
        ltb = each_row[4]
        gsb = each_row[5]
        # if lta or ltb or gsb:
        #     files = lta, ltb, gsb
        if lta and ltb and gsb:
            files = gsb
        elif lta and ltb and not gsb:
            files = lta, ltb
        elif not lta and not ltb and gsb:
            files = gsb
        elif lta and not ltb and not gsb:
            files = lta
        elif lta and not ltb and gsb:
            files = lta

        gadpudata[int(obs_no)] = {
            "proposal_id": proposal_id,
            "files": files,
            "file_path": file_path
        }

    return gadpudata


def run_gvfits(file):
    dir_name = os.path.dirname(file)
    base_name = os.path.basename(file)
    if not glob.glob('fits/'+base_name+'.UVFITS'):
        try:
            print("Working Directory:"+ str(os.getcwd()))
            print("Runnning GVFITS for "+dir_name)
            print("Processing: "+base_name)
            print("Copying "+base_name+" from "+dir_name+" to fits/")
            os.popen('cp ' + file + ' fits/')
            print(" ****** SPAM process started **** ")
            spam.convert_lta_to_uvfits('fits/'+base_name, 'fits/'+base_name+'.UVFITS')
            print(" ****** SPAM process ended **** ")
            print("Removing the file : fits/"+base_name)
            os.popen('rm -rf fits/'+base_name)
            print("moving the processed UVFITS files")
            os.popen('mv fits/*.UVFITS '+dir_name+'/')
        except Exception as ex:
            print("  ******************  "+str(file))
            print(ex)


def run_precalibrate_targets(filename):
    print("spam.precalibrate_targets")
    # spam.set_aips_userid(11)
    project_code = os.path.dirname(filename)
    copy_uvfits = os.system('cp ' + filename + ' fits/')
    print('cp ' + filename + ' fits/')
    uvfits_file = filename.split('/')[-1]
    try:
        spam.pre_calibrate_targets("fits/" + uvfits_file, keep_channel_one=True)
        rm_tmp = os.system('rm fits/' + uvfits_file)
        print(uvfits_file, project_code)
    except Exception as ex:
        print(ex)
    precalib_dir = project_code + '/PRECALIB'
    if not os.path.exists(precalib_dir):
        print(precalib_dir)
        os.mkdir(precalib_dir)
    mv_datfil = os.system('mv datfil/* fits/')
    mv_fits = os.system('mv fits/* ' + precalib_dir + '/')


if __name__ == '__main__':
    data = initialize(18)
    lta = 0
    ghb = 0
    print(len(list(set(data.keys()))))
    for every_obs in data:
        file_path = str(data[every_obs]["file_path"]).replace('data1','GARUDATA/IMAGING18')
        if len(str(data[every_obs]["files"]).split(',')) == 2:
            ghb+=1
            tfile = file_path+data[every_obs]["files"][0]
            dfile = os.path.dirname(tfile)
            afile = os.path.basename(tfile)
            afile = afile.split('.')
            file1 = glob.glob(dfile+'/'+afile[0]+'.'+afile[1]+'.*')
            if len(file1) > 1:
                file1.sort()
                for each_lta in file1:
                    run_gvfits(each_lta)
                print(file1)
                print(file1)
            else:
                run_gvfits(file_path+data[every_obs]["files"][0])

            tfile2 = file_path+data[every_obs]["files"][1]
            dfile2 = os.path.dirname(tfile2)
            afile2 = os.path.basename(tfile2)
            afile2 = afile2.split('.')
            file2 = glob.glob(dfile2+'/'+afile2[0]+'.'+afile2[1]+'.*')
            if len(file2) > 1:
                file2.sort()
                for each_lta2 in file2:
                    run_gvfits(each_lta2)
                print(file2)
            else:
                run_gvfits(file_path+data[every_obs]["files"][1])

            # run_gvfits(file_path+data[every_obs]["files"][0])
            # run_gvfits(file_path+data[every_obs]["files"][1])
        else:
            lta+=1
            tfile = file_path+data[every_obs]["files"]
            dfile = os.path.dirname(tfile)
            afile = os.path.basename(tfile)
            afile = afile.split('.')
            file1 = glob.glob(dfile+'/'+afile[0]+'.'+afile[1]+'.*')
            if len(file1) > 1:
                file1.sort()
                for each_lta in file1:
                    run_gvfits(each_lta)
                print(file1)
            else:
                run_gvfits(file_path+data[every_obs]["files"])

    print("ghb: "+str(ghb),"lta: "+str(lta))

    #     uvfits = glob.glob(file_path+'/*.UVFITS')
    #     if uvfits:
    #         run_precalibrate_targets(uvfits[0])
