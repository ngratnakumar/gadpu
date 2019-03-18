import os
import glob
from astropy.io import fits
import DBUtils


def cycle20_post_proc():
    data_structure = glob.glob("/GARUDATA/IMAGING20/IMAGES/*")
    data_structure.sort()
    for each_dir in data_structure:
        if '_lta' in each_dir:
            lta_file = os.path.basename(each_dir.replace('_lta','.lta'))
            if 'lta_' in lta_file:
                lta_file = lta_file.replace('lta_','lta.')
        fits_files = glob.glob(each_dir+'/*PBCOR*.FITS')
        for each_fits_files in fits_files:
            source_name = os.path.basename(each_fits_files).split('.')[0]
            summary_path = glob.glob(os.path.dirname(each_fits_files)+'/spam_*'+source_name+'*.summary')
            post_proc(each_fits_files, lta_file, source_name)



def cycle21_post_proc():
    data_structure = glob.glob("/GARUDATA/IMAGING21/IMAGES/*")
    data_structure.sort()
    for each_dir in data_structure:
        if '_lta' in each_dir:
            lta_file = os.path.basename(each_dir.replace('_lta','.lta'))
            if 'lta_' in lta_file:
                lta_file = lta_file.replace('lta_','lta.')
        fits_files = glob.glob(each_dir+'/*PBCOR*.FITS')
        for each_fits_files in fits_files:
            source_name = os.path.basename(each_fits_files).split('.')[0]
            summary_path = glob.glob(os.path.dirname(each_fits_files)+'/spam_*'+source_name+'*.summary')
            post_proc(each_fits_files, lta_file, source_name)

def cycle24_post_proc():
    garudata = "/GARUDATA/IMAGING24/CYCLE24"
    data_structure = glob.glob(garudata+'/*')
    GMRT_C24 = '/data2/CYCLE24/'
    print data_structure
    for each_data in data_structure:
        proposal_dir = each_data.split('/')[4]
        if not proposal_dir.isdigit():
            print(proposal_dir)
            merge_dir = glob.glob(garudata+'/*/'+proposal_dir)
            if merge_dir:
                print(merge_dir)
                if os.path.exists(merge_dir[0]):
                    print("cp -r "+garudata+'/'+proposal_dir+'/* '+merge_dir[0]+'/')

        # print(glob.glob(GMRT_C24+each_data.split('/')[-1]+'/*lta'))

def post_proc(fits_file, lta, object):
    print(fits_file, lta, object)
    dbutils = DBUtils.DBUtils()
    # fits_images_list = ['/GARUDATA/IMAGING20/IMAGES/20_086_29jun2011_lta/0054-035.SP2B.PBCOR_20_086_29jun2011_lta.FITS']
    # lta = '20_086_29jun2011.lta'
    # object = '0054-035'
    fits_dir = os.path.dirname(fits_file)
    fits_table = fits.open(fits_file)
    fits_header = fits_table[0].header
    data_keys = {}
    summary_file = glob.glob(fits_dir + '/spam_' + object + '*.summary')
    rms = "NA"
    for each_summary in summary_file:
        if 'DONE' in open(each_summary).read():
            lines = open(each_summary).readlines()
            rms = lines[-1].split(' ')[-5]
        else:
            if rms == "NA":
                log_file = each_summary.replace('summary', 'log')
                lines = open(log_file).readlines()
                rms = lines[-2].split(' ')[0]

    sql = "select distinct file_path, s.scangroup_id from das.scangroup sg inner join das.scans s on s.scangroup_id = sg.scangroup_id " \
          "where lta_file = '" + lta + "' OR lta_gsb_file = '"+ lta +"' AND source like '" + object + "'"
    file_path_data = dbutils.select_scangroup_query(sql)
    if file_path_data:
        observation_file = glob.glob(file_path_data[0]+'/*.obslog')[0]
        observation_number = str(observation_file.split('.')[0])
        scangroup_id = str(file_path_data[1])

    sql_sg = "select ant_mask, band_mask, calcode, chan_width, corr_version, g.observation_no, " \
          "date_obs, ddec, dec_2000, dec_date, dra, lsr_vel1, lsr_vel2, lta_time, " \
          "net_sign1, net_sign2, net_sign3, net_sign4, num_chans, num_pols, onsrc_time, " \
          "proj_code, qual, ra_2000, ra_date, rest_freq1, rest_freq2, sky_freq1, " \
          "sky_freq2, source, sta_time from das.scangroup g inner join " \
          "das.scans s on s.scangroup_id = g.scangroup_id " \
          "where s.scangroup_id = " + scangroup_id + " AND source like '" + object + "'"
    scangroup_data = dbutils.select_scangroup_query(sql_sg)

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

        filename = fits_file
        hdulist = fits.open(filename, mode='update')
        header = hdulist[0].header

        try:
            histroy = str(fits_header["HISTORY"]).strip().split(' ')
            nh = [x for x in histroy if x]
            in_line_his = [''.join(i) for i in histroy]
            in_line_his = list(filter(None, in_line_his))
            data_keys["BMAJ"] = float(in_line_his[in_line_his.index('BMAJ=')+1])
            data_keys["BMIN"] = float(in_line_his[in_line_his.index('BMIN=')+1])
            data_keys["BPA"] = float(in_line_his[in_line_his.index('BPA=')+1].split('\n')[0])

        except Exception as ex:
            print(ex)

        for key, value in data_keys.iteritems():
            print key, value
        #     header.set(key, value)
        # hdulist.flush()


if __name__ == '__main__':
    cycle24_post_proc()
