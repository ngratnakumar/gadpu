import os
import glob
from astropy.io import fits

from gadpu_pipeline import DBUtils

dbutils = DBUtils.DBUtils()

fits_file = '/GARUDATA/IMAGING19/CYCLE19/5164/19_085_27DEC10/FITS_IMAGE/1445+099.GMRT325.SP2B.PBCOR.FITS'

fits_table =  fits.open(fits_file)
fits_header = fits_table[0].header
object = os.path.basename(fits_file).split('.')[0]
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

sql = "select band_mask, calcode, chan_width, corr_version, g.observation_no, date_obs, ddec, dec_2000, dec_date, dra, lsr_vel1, lsr_vel2, lta_time, net_sign1, net_sign2, net_sign3, net_sign4, num_chans, num_pols, onsrc_time, proj_code, qual, ra_2000, ra_date, rest_freq1, rest_freq2, sky_freq1, sky_freq2, source, sta_time from das.scangroup g inner join das.scans s on s.scangroup_id = g.scangroup_id where s.scangroup_id = "+str(result[1])+" AND source like '"+object+"'"
scangroup_data = dbutils.select_scangroup_query(sql)
print(scangroup_data)

print(result)
print(fits_header)
print(object)
