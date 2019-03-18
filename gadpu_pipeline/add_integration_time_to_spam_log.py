import glob
import os
from SpamUtils import SpamUtils

CYCLE_PATH = "/GARUDATA/IMAGING21/CYCLE21/*/*/FITS_IMAGE/spam_*.log"

spam_logs = glob.glob(CYCLE_PATH)
# print(spam_logs)
# print(len(spam_logs))
success_log_counter = 0
for each_log in spam_logs:
    tail_log = open(each_log,'r').readlines()[-5:]
    # print(tail_log)
    print(each_log)
    if not 'time resolution' in open(each_log).read():
        if "successfully" in tail_log[1]:
            log_filename = each_log.split('/')[-1]
            source = log_filename.split('_')[1]
            dirname = os.path.dirname(each_log)
            precalib_path = dirname.replace("FITS_IMAGE","PRECALIB")
            # print(precalib_path)
            print(source, precalib_path)
            uvfits = glob.glob(precalib_path+'/'+source+'*.UVFITS')
            print(each_log)
            print(uvfits)
            spamutils = SpamUtils()
            if len(uvfits) > 0:
                time_resolution = spamutils.get_uvfits_integration_time(uvfits[0])
                if time_resolution > 0:
                    print("++ Writing to the log file: ",each_log)
                    # with open(each_log, "a") as log_file:
                    #     log_file.write("time resolution = "+str(time_resolution))
                    print(time_resolution)
                    print("++++++ appended time_resolution value +++++++")
                success_log_counter+=1
            if success_log_counter == 1000:
                break
print(success_log_counter)


def das_headers(table_name, data):
# observation_no | scangroup_id | lta_file | corr_version | sta_time
# num_pols | num_chans | lta_time | ltb_file | lta_file_size | ltb_file_size
# file_path | lta_gsb_file_size | lta_gsb_file
    das_observation_header = [ "observation_no" ,"scangroup_id" ,"lta_file" ,"corr_version" ,
                               "sta_time", "num_pols", "num_chans", "lta_time", "ltb_file",
                               "lta_file_size", "ltb_file_size", "file_path",
                               "lta_gsb_file_size", "lta_gsb_file"]
