from FileUtils import FileUtils
from DBUtils import DBUtils
from SpamUtils import SpamUtils
import tableSch as tableSchema

from astropy.io import fits
from time import sleep

import os
import spam
import time
import glob
import datetime
import sys
import random


class Pipeline:

    def stage6(self):
        """
        Post processing, extract RMS from the summary file for corresponding PBCOR_IFTS file
        extract BMIN, BMAJ, BPA from the HISTORY keyword from the PBCOR FITS header and put
        KEY Value pairs in the same PBCOR FITS header using astropy.io
        :return:
        """
        dbutils = DBUtils()
        fits_images_list = glob.glob('/GARUDATA/IMAGING16/CYCLE16/*/*/FITS_IMAGE/*PBCOR*.FITS')
        # fits_images_list = ['/GARUDATA/IMAGING17/CYCLE17/4575/17_024_04NOV09/FITS_IMAGE/A3376-W.GMRT325.SP2B.PBCOR.FITS']
        # fits_images_list = ['/GARUDATA/IMAGING17/CYCLE17/4572/17_024_03NOV09/FITS_IMAGE/A3376-E.GMRT325.SP2B.PBCOR.FITS']
        counter = 1
        for fits_file in fits_images_list:
            counter += 1
            # fits_file = '/GARUDATA/IMAGING19/CYCLE19/5164/19_085_27DEC10/FITS_IMAGE/1445+099.GMRT325.SP2B.PBCOR.FITS'

            fits_dir = os.path.dirname(fits_file)

            fits_table = fits.open(fits_file)
            fits_header = fits_table[0].header

            data_keys = {}

            object = os.path.basename(fits_file).split('.')[0]
            # object = "A3376_E"

            # summary_file = glob.glob(fits_dir + '/spam_A3376-E*.summary')
            summary_file = glob.glob(fits_dir + '/spam_' + object + '*.summary')
            rms = "NA"
            for each_summary in summary_file:
                if 'DONE' in open(each_summary).read():
                    # print each_summary
                    lines = open(each_summary).readlines()
                    rms = lines[-1].split(' ')[-5]
                    # print rms
                else:
                    # print "Needs to be deleted"
                    if rms == "NA":
                        log_file = each_summary.replace('summary', 'log')
                        lines = open(log_file).readlines()
                        rms = lines[-2].split(' ')[0]
            if rms == "NA":
                rms = 2.11

            observation_no = fits_file.split('/')[4]

            columnKeys = {
                "project_id"
            }

            if observation_no == 'MIXCYCLE':
                mix_path = fits_file.split('/')[4]+'/'+fits_file.split('/')[5]
                mix_sql = "select observation_no from projectobsno where base_path like '%"+mix_path+"%'"
                mix_cycle_data = dbutils.select_gadpu_query(mix_sql)
                observation_no = mix_cycle_data[0][0]

            whereKeys = {
                "observation_no": observation_no
            }

            project_id = dbutils.select_from_table("projectobsno", columnKeys, whereKeys, 0)

            columnKeys = {
                "das_scangroup_id",
                "ltacomb_file"
            }
            whereKeys = {
                "project_id": project_id,
            }
            result = dbutils.select_from_table("ltadetails", columnKeys, whereKeys, 0)

            sql = "select ant_mask, band_mask, calcode, chan_width, corr_version, g.observation_no, " \
                  "date_obs, ddec, dec_2000, dec_date, dra, lsr_vel1, lsr_vel2, lta_time, " \
                  "net_sign1, net_sign2, net_sign3, net_sign4, num_chans, num_pols, onsrc_time, " \
                  "proj_code, qual, ra_2000, ra_date, rest_freq1, rest_freq2, sky_freq1, " \
                  "sky_freq2, source, sta_time from das.scangroup g inner join " \
                  "das.scans s on s.scangroup_id = g.scangroup_id " \
                  "where s.scangroup_id = " + str(result[1]) + " AND source like '" + object + "'"
            scangroup_data = dbutils.select_scangroup_query(sql)

            if scangroup_data:
                data_keys = {
                    "ANTMASK": scangroup_data[0],
                    "BANDMASK": scangroup_data[1],
                    "CALCODE": scangroup_data[2],
                    "CHANWIDT": scangroup_data[3],
                    "CORRVERS": scangroup_data[4],
                    "OBSNUM": scangroup_data[5],
                    "DATEOBS": str(scangroup_data[6]),
                    "DDEC": scangroup_data[7],
                    "DEC2000": scangroup_data[8],
                    "DECDATE": scangroup_data[9],
                    "DRA": scangroup_data[10],
                    "LSRVEL1": scangroup_data[11],
                    "LSRVEL2": scangroup_data[12],
                    "LTATIME": scangroup_data[13],
                    "NETSIGN1": scangroup_data[14],
                    "NETSIGN2": scangroup_data[15],
                    "NETSIGN3": scangroup_data[16],
                    "NETSIGN4": scangroup_data[17],
                    "NUMCHANS": scangroup_data[18],
                    "NUMPOLS": scangroup_data[19],
                    "ONSRCTIM": scangroup_data[20],
                    "PROJCODE": scangroup_data[21],
                    "QUAL": scangroup_data[22],
                    "RA2000": scangroup_data[23],
                    "RADATE": scangroup_data[24],
                    "RESTFRE1": scangroup_data[25],
                    "RESTFRE2": scangroup_data[26],
                    "SKYFREQ1": scangroup_data[27],
                    "SKYFREQ2": scangroup_data[28],
                    "STATIME": scangroup_data[30],
                    "RMS": float(rms)
                }

                # print(data_keys)
                filename = fits_file
                hdulist = fits.open(filename, mode='update')
                header = hdulist[0].header

                try:
                    histroy = str(fits_header["HISTORY"]).strip().split(' ')
                    nh = [x for x in histroy if x]
                    data_keys["BMAJ"] = float(nh[3])
                    data_keys["BMIN"] = float(nh[5])
                    data_keys["BPA"] = float(nh[7])

                    try:
                        del header['HISTORY']
                    except Exception as exh:
                        print(exh)
                except Exception as ex:
                    print(ex)

                if fits_header["BMAJ"]:
                    data_keys["BMAJ"] = float(fits_header["BMAJ"])
                    data_keys["BMIN"] = float(fits_header["BMIN "])
                    data_keys["BPA"] = float(fits_header["BPA"])

                for key, value in data_keys.iteritems():
                    print key, value
                    header.set(key, value)
                hdulist.flush()
                print(counter)

    def __init__(self):
        self.stage6() # POST_PROC

Pipeline()
