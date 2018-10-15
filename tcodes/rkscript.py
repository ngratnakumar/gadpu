import os
import re
import glob
import json
import fnmatch
from astropy.io import fits

# Modified key-value pair as per ESO standard (max 8 characters allowed for keyword) 
eightchar = {u'ant_mask': u'ant_mask',
 u'band_mask': u'bandmask',
 u'calcode': u'calcode',
 u'chan_width': u'chnwidth',
 u'corr_version': u'corrver',
 u'date_obs': u'date_obs',
 u'ddec': u'ddec',
 u'dec_2000': u'dec_2000',
 u'dec_date': u'dec_date',
 u'dra': u'dra',
 u'fileSize_val': u'filesize',
 u'logfile': u'logfile',
 u'lsr_vel1': u'lsr_vel1',
 u'lsr_vel2': u'lsr_vel2',
 u'lta_time': u'lta_time',
 u'net_sign1': u'netsign1',
 u'net_sign2': u'netsign2',
 u'net_sign3': u'netsign3',
 u'net_sign4': u'netsign4',
 u'num_chans': u'numchans',
 u'num_pols': u'num_pols',
 u'observation_no': u'obsno',
 u'onsrc_time': u'onsrctim',
 u'proj_code': u'projcode',
 u'qual': u'qual',
 u'ra_2000': u'ra_2000',
 u'ra_date': u'ra_date',
 u'rest_freq1': u'rstfreq1',
 u'rest_freq2': u'rstfreq2',
 u'sky_freq1': u'skyfreq1',
 u'sky_freq2': u'skyfreq2',
 u'source': u'source',
 u'source_position': u'src_pos',
 u'sta_time': u'sta_time'}

band_mask, calcode, chan_width, corr_version, date_obs, ddec, dec_2000, dec_date, dra, fileSize_val, logfile, lsr_vel1, lsr_vel2, lta_time, net_sign1, net_sign2, net_sign3, net_sign4, num_chans, num_pols, observation_no, onsrc_time, proj_code, qual, ra_2000, ra_date, rest_freq1, rest_freq2, sky_freq1, sky_freq2, source, source_position, sta_time

# Need to assign these paths accordingly before running this script (this example is for Cycle 23)
#IMAGES_DIR='/FITS/data/CYCLE_23_IMAGES'
#UVFITS_DIR='/FITS/data/SPLIT_CYCLE_23'
#LTA_DIR = '/GARUDATA/LTA/CYCLE23'
#XINFO_PATH  = '/home/gadpu/gadpu-xinfo/xinfo'

IMAGES_DIR = '/GARUDATA/backup/partitions/FITS/data/CYCLE_23_IMAGES/'
UVFITS_DIR='/FITS/data/SPLIT_CYCLE_23/'
LTA_DIR = '/GARUDATA/LTA/CYCLE23/'
XINFO_PATH  = '/home/gadpu/tcodes/gadpu-xinfo/xinfo'

def find(pattern, path):
	result = []
	for root, dirs, files in os.walk(path):
		for name in files:
			if fnmatch.fnmatch(name, pattern):
				result.append(os.path.join(root, name))
	return result

# Generates the output 'json' file for each LTA file using xinfo
def xinfo(lta_file, output_name, obs_num, obs_log):
	try:
		os.system(XINFO_PATH+" -l "+lta_file+" -o "+output_name+" -n "+obs_num+" -f "+ obs_log)
	except Exception as e:
		print "xinfo error: ", e
 
# This script sits outside FITS images directory and runs the processing inside of the FITS IMAGES directory
os.chdir(IMAGES_DIR) 
for dir in os.listdir("."):
	os.chdir(dir)
	# Picking only the first spam log as the corresponding LTA file would be the same for the rest as well
	spamlog = glob.glob("spam*")[0]
	spamlog = spamlog.split("_")
	UVFITS_NAME = '_'.join(spamlog[1:7])
	lta_dir = find(UVFITS_NAME+"*", UVFITS_DIR)[0].split('/')[-2]  # 'lta_dir' points to the corresponding lta directory
	lta_dir = lta_dir.split("_lta") # strip out the trailing '_lta' to match lta file pattern
	lta_pointer, extension = lta_dir[0], lta_dir[1]
	if not extension:
		extension = ".lta"
	else:
		extension = ".lta"+extension.replace("_",".")
	# Getting LTA file's full path
	lta_file = find(lta_pointer+extension, LTA_DIR)[0]

	# Getting output name of the json file
	output_name = lta_file.split("/")[-1]+".json"

	# Getting obslog file
	dirname = os.path.dirname(lta_file)
	obs_log = glob.glob(dirname+"/*obslog")[0]

	# Getting obs_num from obs_log
	obs_num = obs_log.split("/")[-1].split(".")[0]

	# Calling xinfo function to generate json file
	xinfo(lta_file, output_name, obs_num, obs_log)

	with open(output_name, "r") as f:
		data = json.load(f)

	# Getting all the PBCOR FITS files
	fits_files = [f for f in os.listdir(".") if "PBCOR" in f]

	# All the sources and their corresponding fits files
	source_list = { f.split('.')[0]:f for f in fits_files }

	# Merging the common key-value pairs i.e. 'observation_details' and 'scangroup'
	merged = dict(data['observation_details'].items() + data['scangroup'].items())

	# Mapping of files source to all the sources, corresponding to it, present in the scans produced by JSON
	mapping = dict()
	for file_source in source_list.keys():
		for i in range(len(data['scans'])):
			scan_source = data['scans'][i]['source']
			if file_source in scan_source:
				mapping[file_source] = scan_source

	# Adding the 'onsrc_time' and appending the key-value pairs to FITS Header
	for key, value in mapping.iteritems():
		total_onsrc_time = 0.0
		for i in range(len(data['scans'])):
			if value==data['scans'][i]['source']:
				total_onsrc_time += float(data['scans'][i]['onsrc_time'])
				index = i

		final_dict = data['scans'][index]
		final_dict['onsrc_time'] = total_onsrc_time

		# Final key-value pair to appended to the FITS Header
		final_dict = dict(merged.items() + final_dict.items())

		# Opening the fits header here
		filename = source_list[key]
		hdulist = fits.open(filename, mode='update')
		header = hdulist[0].header

		for key, value in final_dict.iteritems():
			header.set(eightchar[key], value)
		hdulist.flush()
	os.chdir("..")
