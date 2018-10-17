import os
import glob
from astropy.io import fits

from gadpu_pipeline import DBUtils

dbutils = DBUtils.DBUtils()
fits_images_list = glob.glob('/GARUDATA/IMAGING19/CYCLE19/*/*/FITS_IMAGE/*PBCOR*.FITS')
# fits_images_list = ['/GARUDATA/IMAGING19/CYCLE19/5164/19_085_27DEC10/FITS_IMAGE/1445+099.GMRT325.SP2B.PBCOR.FITS']

for fits_file in fits_images_list:
    # fits_file = '/GARUDATA/IMAGING19/CYCLE19/5164/19_085_27DEC10/FITS_IMAGE/1445+099.GMRT325.SP2B.PBCOR.FITS'

    fits_dir = os.path.dirname(fits_file)

    fits_table =  fits.open(fits_file)
    fits_header = fits_table[0].header

    data_keys = {}



    object = os.path.basename(fits_file).split('.')[0]

    summary_file = glob.glob(fits_dir+'/spam_'+object+'*.summary')
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
    if rms =="NA":
        rms = 2.11

    observation_no = fits_file.split('/')[4]

    columnKeys = {
        "project_id"
    }
    whereKeys = {
        "observation_no": observation_no
    }

    project_id = dbutils.select_from_table("projectobsno",columnKeys, whereKeys, 0)

    columnKeys = {
        "das_scangroup_id",
        "ltacomb_file"
    }
    whereKeys = {
        "project_id": project_id,
    }
    result = dbutils.select_from_table("ltadetails",columnKeys, whereKeys, 0)

    sql = "select ant_mask, band_mask, calcode, chan_width, corr_version, g.observation_no, " \
          "date_obs, ddec, dec_2000, dec_date, dra, lsr_vel1, lsr_vel2, lta_time, " \
          "net_sign1, net_sign2, net_sign3, net_sign4, num_chans, num_pols, onsrc_time, " \
          "proj_code, qual, ra_2000, ra_date, rest_freq1, rest_freq2, sky_freq1, " \
          "sky_freq2, source, sta_time from das.scangroup g inner join " \
          "das.scans s on s.scangroup_id = g.scangroup_id " \
          "where s.scangroup_id = "+str(result[1])+" AND source like '"+object+"'"
    scangroup_data = dbutils.select_scangroup_query(sql)

    if scangroup_data:
        data_keys = {
            "ANTMASK": scangroup_data[0],
            "BANDMASK": scangroup_data[1],
            "CALCODE":scangroup_data[2],
            "CHANWIDT": scangroup_data[3],
            "CORRVERS":scangroup_data[4],
            "OBSNUM":scangroup_data[5],
            "DATEOBS": str(scangroup_data[6]),
            "DDEC":scangroup_data[7],
            "DEC2000":scangroup_data[8],
            "DECDATE":scangroup_data[9],
            "DRA":scangroup_data[10],
            "LSRVEL1":scangroup_data[11],
            "LSRVEL2":scangroup_data[12],
            "LTATIME":scangroup_data[13],
            "NETSIGN1": scangroup_data[14],
            "NETSIGN2":scangroup_data[15],
            "NETSIGN3":scangroup_data[16],
            "NETSIGN4":scangroup_data[17],
            "NUMCHANS":scangroup_data[18],
            "NUMPOLS":scangroup_data[19],
            "ONSRCTIM": scangroup_data[20],
            "PROJCODE":scangroup_data[21],
            "QUAL":scangroup_data[22],
            "RA2000":scangroup_data[23],
            "RADATE":scangroup_data[24],
            "RESTFRE1":scangroup_data[25],
            "RESTFRE2":scangroup_data[26],
            "SKYFREQ1":scangroup_data[27],
            "SKYFREQ2":scangroup_data[28],
            # "SOURCE":scangroup_data[29],
            "STATIME":scangroup_data[30],
            "RMS": rms
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
            data_keys["BMIN"] = float(fits_header["BMIN"])
            data_keys["BPA"] = float(fits_header["BPA"])

        for key, value in data_keys.iteritems():
            # print key, value
            header.set(key, value)
        hdulist.flush()