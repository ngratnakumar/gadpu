import os
import spam

class SpamUtils:


    def run_ltacomb(self, files_list, destination):
	to_comb = ",".join(files_list)
        os.chdir('/home/gadpu/')
        os.system("ltacomb -i "+to_comb)
        os.system("rm "+to_comb.replace(',',' '))
        os.system("mv ltacomb_out.lta "+files_list[0])

    def run_gvfits(self, filename, destination):
        copying_lta_file = os.system('cp '+filename+' .')
        print(filename)
	ne_name = filename.split('/')[-1]
        spam.set_aips_userid(11)
        try:
            spam.convert_lta_to_uvfits(ne_name)
            #os.system('mv fits/*.UVFITS '+destination+'/')
	    print(destination)
        except Exception as ex:
            print(ex)  
	os.system('rm '+ne_name)
	os.system('mv fits/*.UVFITS '+destination+'/')
	

    def run_precalibrate_targets(self, filename):
        print("spam.precalibrate_targets")
	#spam.set_aips_userid(11)
	project_code = os.path.dirname(filename)
	copy_uvfits = os.system('cp '+filename+' fits/')
	print('cp '+filename+' fits/')
	uvfits_file = filename.split('/')[-1]
	try:
		spam.pre_calibrate_targets("fits/"+uvfits_file, keep_channel_one = True)
		rm_tmp=os.system('rm fits/'+uvfits_file)
		print(uvfits_file, project_code)
	except Exception as ex:
		print(ex)
	precalib_dir = project_code+'/PRECALIB'
	if not os.path.exists(precalib_dir):
		print(precalib_dir)
		os.mkdir(precalib_dir)
	mv_datfil=os.system('mv datfil/* fits/')
	mv_fits=os.system('mv fits/* '+precalib_dir+'/')


    def run_process_target(self, filename):
        print("spam.process_traget()")


    def run_spam_summary(self, filename):
        pass


    def run_combine_usb_lsb(self, filelist):
        pass


    def run_haslam_correction(self, obslog_file):
        pass
