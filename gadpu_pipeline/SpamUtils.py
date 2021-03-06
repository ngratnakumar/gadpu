import os
import spam
import glob

from FileUtils import FileUtils


class SpamUtils:

    def run_ltacomb(self, files_list, destination):
        print("##########run_ltacomb########")
        fileutils = FileUtils()
        lta_list = []
        self.status = "failed"
        print(files_list, destination)

        for each_file in files_list:
            print("Copying "+each_file+" "+destination)
            fileutils.copy_files(each_file, destination)
            lta_list.append(destination+'/'+os.path.basename(each_file))
        lta_list.sort()
        to_comb = ",".join(lta_list)
        print(".......... joining ", to_comb)
        os.chdir(destination)
        print("::::::::", lta_list)
        try:
            print("/home/gadpu/gadpu_pipeline/ltacomb -i "+to_comb)
            os.system("/home/gadpu/gadpu_pipeline/ltacomb -i "+to_comb)
            self.status = "success"
        except Exception as ex:
            print(ex)
            self.status = ex
        print("?????????????????????????????????????????????")
        print("rm "+to_comb.replace(',',' '))
        os.system("rm "+to_comb.replace(',',' '))
        print("mv ltacomb_out.lta "+os.path.basename(lta_list[0]))
        os.system("mv ltacomb_out.lta "+os.path.basename(lta_list[0]))
        return str(self.status)

    def run_ltacomb_ghb(self, files_list, destination):
        print("##########run_ltacomb########")
        fileutils = FileUtils()
        lta_list = []
        self.status = "failed"
        print(files_list, destination)
        if len(files_list) > 1:
            for each_file in files_list:
                print("Copying "+each_file+" "+destination)
                fileutils.copy_files(each_file, destination)
                lta_list.append(destination+'/'+os.path.basename(each_file))
            lta_list.sort()
            to_comb = ",".join(lta_list)
        else:
            to_comb = files_list[0]
        os.chdir(destination)
        print("::::::::", lta_list)
        try:
            print("/home/gadpu/gadpu_pipeline/ltacomb -i "+to_comb)
            os.system("/home/gadpu/gadpu_pipeline/ltacomb -i "+to_comb)
            self.status = "success"
        except Exception as ex:
            print(ex)
            self.status = ex
        print("mv ltacomb_out.lta "+os.path.basename(lta_list[0]))
        os.system("mv ltacomb_out.lta "+os.path.basename(lta_list[0]))
        print("rm "+to_comb.replace(',',' '))
        os.system("rm "+to_comb.replace(',',' '))
        return str(self.status)

    def run_gvfits(self, filename, destination):
        base_path = os.path.dirname(filename)
        lta_file = os.path.basename(filename)
        uvfits_file = os.path.basename(destination)
        print("Running GVFITS")
        fileutils = FileUtils()
        fileutils.copy_files(filename, 'fits/')
        status = "gvfits processing for "+filename
        try:
            print("Before spam")
            spam.convert_lta_to_uvfits("fits/"+lta_file)
            print("After spam")
            fileutils.move_files('fits/*.UVFITS*', base_path)
            os.system('rm fits/'+lta_file)
        except Exception as ex:
            print(ex)
            status = ex
        return status   

    def run_precalibrate_targets(self, filename):
        print("spam.precalibrate_targets")
        # spam.set_aips_userid(11)
        project_code = os.path.dirname(filename)
        precalib_dir = project_code+'/PRECALIB'
        if glob.glob(precalib_dir+'/failed_log.txt'):
            print(filename)
            os.popen('cp '+filename+' fits/')
            print('cp '+filename+' fits/')
            uvfits_file = filename.split('/')[-1]
            try:
                # spam.pre_calibrate_targets("fits/"+uvfits_file)
                os.popen('rm fits/'+uvfits_file)
                print(uvfits_file, project_code)
            except Exception as ex:
                print(ex)
            if not os.path.exists(precalib_dir):
                print(precalib_dir)
                os.mkdir(precalib_dir)
            os.popen('mv datfil/* fits/')
            os.popen('mv fits/* '+precalib_dir+'/')

    def get_uvfits_integration_time(self, filename):
        uv = spam.get_aips_file( 0, 'UV', 'UV', -1, 'UV' )
        spam.read_fits_uv(filename, uv)
        integration_time = spam.find_integration_time(uv)
        uv.zap()
        return float("{0:.2f}".format(integration_time))
