import os
import spam
import glob

from FileUtils import FileUtils


class SpamUtils:
    

    def run_ltacomb(self, files_list, destination):
        fileutils = FileUtils()
        lta_list = []
        self.status = "cycle17"
        for each_file in files_list:
            print("Copying "+each_file+" "+destination)
            fileutils.copy_files(each_file, destination)
            lta_list.append(destination+'/'+os.path.basename(each_file))

        lta_list.sort()
        to_comb = ",".join(lta_list)
        os.chdir(destination)
        try:
            print("ltacomb -i "+to_comb)
            os.system("ltacomb -i "+to_comb)
        except Exception as ex:
            print(ex)
            self.status = ex
        print("rm "+to_comb.replace(',',' '))
        os.system("rm "+to_comb.replace(',',' '))
        print("mv ltacomb_out.lta "+os.path.basename(lta_list[0]))
        os.system("mv ltacomb_out.lta "+os.path.basename(lta_list[0]))
        return str(self.status)

    def run_gvfits(self, filename, destination):
        base_path = os.path.dirname(filename)
        lta_file = os.path.basename(filename)
        uvfits_file = os.path.basename(destination)

        fileutils = FileUtils()
        fileutils.copy_files(filename, 'fits/')
        status = "gvfits processing for "+filename
        try:
            spam.convert_lta_to_uvfits(lta_file, uvfits_file)
            fileutils.move_files('fits/*.UVFITS*', base_path)
        except Exception as ex:
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


    def run_process_target(self, filename):
        print("spam.process_traget()")


    def run_spam_summary(self, filename):
        pass


    def run_combine_usb_lsb(self, filelist):
        pass


    def run_haslam_correction(self, obslog_file):
        pass
