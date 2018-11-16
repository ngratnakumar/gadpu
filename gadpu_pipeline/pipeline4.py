from FileUtils import FileUtils
from DBUtils import DBUtils
from SpamUtils import SpamUtils
import time
import datetime
import glob
import os
from astropy.io import fits
import random
import spam


class Pipeline:

    def stage1(self, gdata):
        print("Started Stage1: ")
        spamutils = SpamUtils()
        fileutils = FileUtils()

        data = gdata[0]
        path = gdata[1]

        for each_obs in data:
            print("=========================================")
            print(each_obs)
            print("=========================================")
            proposal_id = data[each_obs]['proposal_id']
            file_path = data[each_obs]['file_path']
            backend_type = data[each_obs]['backend_type']
            cycle_id = data[each_obs]['cycle_id']
            print("********"+file_path)
            project_path = path + "/".join(file_path.split('/')[-3:])
            print("---------"+project_path)
            lta, ltb, gsb = None, None, None
            isghb = False
            status = "cycle16"

            files_list = glob.glob(file_path + '*.lt*')
            files_list.sort()

            print("Processing .... " + str(project_path))
            print(files_list)

            if len(files_list) == 1:
                lta = files_list
            elif any("gsb" in lt_list for lt_list in files_list):
                gsb = [x for x in files_list if "gsb" in x][0]
                cgsb = fileutils.check_for_multiples(gsb)
                if cgsb:
                    print("LTACOMB: "+str(cgsb)+" "+project_path+""+os.path.basename(gsb))
                    status = spamutils.run_ltacomb(cgsb, project_path)
            elif any("ltb" in lt_list for lt_list in files_list):
                # print(files_list)
                isghb = True
                lta = [x for x in files_list if "lta" in x][0]
                ltb = [x for x in files_list if "ltb" in x][0]
                clta = fileutils.check_for_multiples(lta)
                cltb = fileutils.check_for_multiples(ltb)
                if clta:
                    print("LTACOMB: "+str(clta)+" "+project_path+""+os.path.basename(lta))
                    status = spamutils.run_ltacomb(clta, project_path)
                if cltb:
                    print("LTACOMB: "+str(cltb)+" "+project_path+""+os.path.basename(ltb))
                    status = spamutils.run_ltacomb(cltb, project_path)

            print(isghb, lta, ltb, gsb)

            if isghb:
                fileutils.copy_files(lta, project_path)
                fileutils.copy_files(ltb, project_path)
                fileutils.insert_details([lta, ltb], project_path, isghb, cycle_id, status)
            else:
                if gsb:
                    fileutils.copy_files(gsb, project_path)
                    fileutils.insert_details(gsb, project_path, isghb, cycle_id, status)
                if lta:
                    fileutils.copy_files(lta, project_path)
                    fileutils.insert_details(lta, project_path, isghb, cycle_id, status)

    def stage2(self):
        dbutils = DBUtils()
        spamutils = SpamUtils()
        fileutils = FileUtils()

        currentTimeInSec = time.time()
        current_date_timestamp = datetime.datetime.fromtimestamp(currentTimeInSec).strftime('%Y-%m-%d %H:%M:%S')

        columnKeys = {"project_id", "ltacomb_file", "lta_id"}
        whereKeys = {"comments": "cycle16"}

        lta_details = dbutils.select_from_table("ltadetails", columnKeys, whereKeys, None)

        print(lta_details)

        for each_lta in lta_details:
            print(each_lta)
            project_id = each_lta["project_id"]
            # project_id = each_lta[0]
            lta_file = each_lta["ltacomb_file"]
            # lta_file = each_lta[1]
            # lta_id = each_lta[2]
            lta_id = each_lta["lta_id"]
            columnKeys = {"base_path"}
            whereKeys = {"project_id": project_id}
            lta_path_details = dbutils.select_test_table("projectobsno", columnKeys, whereKeys, 0)
            print(lta_path_details)
            base_path = lta_path_details[0]
            print(base_path)
            uvfits_file = lta_file + '.UVFITS'
            base_lta = base_path + '/' + lta_file
            if os.path.exists(base_lta):
                base_uvfits = base_path + '/' + uvfits_file
                gvfits_status = spamutils.run_gvfits(base_lta, base_uvfits)
                if os.path.exists(base_uvfits):
                    status = "success"
                else:
                    status = "failed"

                calibration_data = {
                    "project_id": project_id,
                    "lta_id": lta_id,
                    "uvfits_file": uvfits_file,
                    "status": status,
                    "comments": gvfits_status,
                    "uvfits_size": fileutils.calculalate_file_sizse_in_MB(base_uvfits),
                    "start_time": current_date_timestamp
                }

                dbutils.insert_into_table("calibrationinput", calibration_data, "calibration_id")

            else:
                project_update_data = {
                    "set": {
                        "status": "failed",
                        "comments": "ltacomb failed"
                    },
                    "where": {
                        "project_id": project_id
                    }
                }
                lta_details_update_data = {
                    "set": {
                        "status": "failed",
                        "comments": "ltacomb failed"
                    },
                    "where": {
                        "lta_id": lta_id
                    }
                }
                dbutils.update_table(project_update_data, "projectobsno")
                dbutils.update_table(lta_details_update_data, "ltadetails")

    def stage3(self):
        spam.set_aips_userid(33)
        dbutils = DBUtils()
        fileutils = FileUtils()

        while True:
            columnKeys = {"calibration_id"}
            whereData = {"comments": "c16", "status": "copying"}
            uncalibrated_uvfits = dbutils.select_from_table("calibrationinput", columnKeys, whereData, 0)
            if not uncalibrated_uvfits:
                break
            print("Waiting for bandwidth ... ")
            time.sleep(50)

        columnKeys = {"calibration_id", "project_id", "uvfits_file"}
        whereData = {"comments": "c16", "status": "unprocessed"}
        uncalibrated_uvfits = dbutils.select_from_table("calibrationinput", columnKeys, whereData, 0)

        if not uncalibrated_uvfits:
            print("All for the data is processed ... please check the DB for pre_calib")
            spam.exit()

        calibration_id = uncalibrated_uvfits[0]
        project_id = uncalibrated_uvfits[1]
        uvfits_file = uncalibrated_uvfits[2]

        columnKeys = {"base_path", "observation_no"}
        whereData = {"project_id": project_id, "cycle_id": 16}
        project_details = dbutils.select_from_table("projectobsno", columnKeys, whereData, 0)



        base_path = project_details[1]
        observation_no = project_details[0]

        current_date_timestamp = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')

        projectobsno_update_data = {
            "set": {
                "status": "processing",
                "comments": "running precalibrate_target, calibration_id = " + str(calibration_id),
            },
            "where": {
                "project_id": project_id
            }
        }

        calibration_update_data = {
            "set": {
                "status": "copying",
                "start_time": current_date_timestamp
            },
            "where": {
                "calibration_id": calibration_id
            }
        }

        dbutils.update_table(projectobsno_update_data, "projectobsno")
        dbutils.update_table(calibration_update_data, "calibrationinput")

        UVFITS_FILE_NAME = uvfits_file
        UVFITS_BASE_DIR = base_path
        is_fits_dir = os.getcwd().split('/')
        print(is_fits_dir)
        SPAM_WORKING_DIR = os.getcwd()
        print(SPAM_WORKING_DIR)
        # for num in range(1, 3):
        #     SPAM_THREAD_DIR += "/" + is_fits_dir[num]
        # if 'fits' not in is_fits_dir:
        #     SPAM_THREAD_DIR = os.getcwd()
        SPAM_WORKING_DIR = os.getcwd() + "/fits/"
        print(SPAM_WORKING_DIR, UVFITS_BASE_DIR, UVFITS_FILE_NAME)
        UVFITS_FILE_PATH = UVFITS_BASE_DIR + "/" + UVFITS_FILE_NAME
        print(UVFITS_FILE_PATH)
        print(SPAM_WORKING_DIR)
        fileutils.copy_files(UVFITS_FILE_PATH, SPAM_WORKING_DIR)
        print("Copying done ==> Moving to pre_cal_target")
        current_date_timestamp = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')
        calibration_update_data = {
            "set": {
                "status": "processing",
                "start_time": current_date_timestamp
            },
            "where": {
                "calibration_id": calibration_id
            }
        }
        dbutils.update_table(calibration_update_data, "calibrationinput")

        fileutils.run_spam_precalibration_stage(UVFITS_BASE_DIR, SPAM_WORKING_DIR, UVFITS_FILE_NAME, observation_no)
        current_time_in_sec = time.time()
        current_date_timestamp = datetime.datetime.fromtimestamp(current_time_in_sec).strftime('%Y-%m-%d %H:%M:%S')

        check_status_file = glob.glob(base_path + "/PRECALIB/failed_log.txt")
        comments = "failed"
        if check_status_file:
            status = "failed"
            comments = str(open(check_status_file[0], 'r').read())
        else:
            status = "success"
            comments = "precalibrate_target done, calibration_id = " + str(calibration_id)

        projectobsno_update_data = {
            "set": {
                "status": status,
                "comments": comments
            },
            "where": {
                "project_id": project_id
            }
        }

        calibration_update_data = {
            "set": {
                "status": status,
                "end_time": current_date_timestamp,
                "comments": comments
            },
            "where": {
                "calibration_id": calibration_id
            }
        }

        dbutils.update_table(projectobsno_update_data, "projectobsno")
        dbutils.update_table(calibration_update_data, "calibrationinput")

    def stage4(self):
        spam.set_aips_userid(11)
        dbutils = DBUtils()
        fileutils = FileUtils()
        status = "failed"
        comments = "combine usb lsb failed"
        # fileutils = FileUtils()
        # query conditions for projectobsno
        columnKeys = {"project_id", "base_path", "observation_no"}
        whereKeys = {"isghb": True, "cycle_id": 16, "status": "success"}

        project_data = dbutils.select_from_table("projectobsno", columnKeys, whereKeys, 0)
        print(project_data)

        project_id = project_data[1]
        base_path = project_data[2]
        obsno = project_data[0]

        start_time = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')
        # print(project_id, base_path, obsno)

        # query conditions for calibrationinput
        columnKeys = {"calibration_id", "uvfits_file"}
        whereKeys = {"project_id": project_id, "status": "success"}
        calibration_data = dbutils.select_from_table("calibrationinput", columnKeys, whereKeys, None)
        print(calibration_data)

        if not calibration_data:
            print("All the data is processed ... OR \n ==> please check the DB for combinelsbusb")
            spam.exit()

        print(len(calibration_data))
        if len(calibration_data) < 2:
            status = "success"
            comments = "single file combinelsbusb not required"
            usb_lsb_file = glob.glob(base_path+"/PRECALIB/*GMRT*.UVFITS")
            if calibration_data:
                projectobsno_update_data = {
                    "set": {
                        "status": status,
                        "comments": comments
                    },
                    "where": {
                        "project_id": project_id
                    }
                }
                print("Updating the projectobsno ... ")
                dbutils.update_table(projectobsno_update_data, "projectobsno")
                if calibration_data:
                    calibration_update_data = {
                        "set": {
                            "status": status,
                            "comments": comments
                        },
                        "where": {
                            "calibration_id": calibration_data[0]["calibration_id"]
                        }
                    }
                    dbutils.update_table(calibration_update_data, "calibrationinput")
            else:
                projectobsno_update_data = {
                    "set": {
                        "status": "failed",
                        "comments": "Failed Error: Something went wrong"
                    },
                    "where": {
                        "project_id": project_id
                    }
                }
                print("Updating the projectobsno ... ")
                dbutils.update_table(projectobsno_update_data, "projectobsno")
        else:
            print("Values > 2")
            print("*************"+str(os.getcwd()))
            for each_uvfits in calibration_data:
                precalib_files = glob.glob(base_path+"/PRECALIB/*")
                lsb_list = glob.glob(base_path + "/PRECALIB/*_LL_*.UVFITS")
                usb_list = glob.glob(base_path + "/PRECALIB/*_RR_*.UVFITS")

                if len(lsb_list) == 0 or len(usb_list) == 0:
                    print(len(lsb_list), len(usb_list))
                    lsb_list = glob.glob(base_path + "/PRECALIB/*LSB*.UVFITS")
                    usb_list = glob.glob(base_path + "/PRECALIB/*USB*.UVFITS")

                projectobsno_update_data = {
                    "set": {
                        "status": "processing",
                        "comments": "combining_lsb_usb"
                    },
                    "where": {
                        "project_id": project_id
                     }
                }
                dbutils.update_table(projectobsno_update_data, "projectobsno")
                calibration_id = each_uvfits["calibration_id"]
                uvfits_file = each_uvfits["uvfits_file"]
                calibration_update_data = {
                    "set": {
                        "status": "processing",
                        "comments": "combining_lsb_usb",
                        "start_time": start_time
                    },
                    "where": {
                        "calibration_id": calibration_id
                    }
                }
                dbutils.update_table(calibration_update_data, "calibrationinput")
                print("lsb_list : "+str(len(lsb_list)))
                print("usb_list : "+str(len(usb_list)))
                status = "failed"
                comments ="combining lsb usb"
                if len(lsb_list) == len(usb_list):
                    print(">>>>>>COMBINE_LSB_USB<<<<<<<")
                    usb_list.sort()
                    lsb_list.sort()
                    print(usb_list)
                    print(lsb_list)
                    to_spam = list(zip(usb_list, lsb_list))
                    file_size = 0
                    print(to_spam)
                    for each_pair in to_spam:
                        print("-------------------------")
                        comb = each_pair[0].replace('USB', 'COMB')
                        data = each_pair, comb
                        print("++++++++++++++++"+comb)
                        currentTimeInSec = time.time()
                        fits_comb = comb.split('/')[-1]
                        check_comb_file = glob.glob("fits/"+fits_comb)
                        if not check_comb_file:
                            status, comments = fileutils.run_spam_combine_usb_lsb(data)
                            print("__________________________________________")
                            print(glob.glob("fits/*"))
                            print("__________________________________________")
                            end_time = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')
                            if not comments:
                                comments = "done combining usb lsb"
                            if glob.glob(comb):
                                file_size = fileutils.calculalate_file_sizse_in_MB(comb)
                            imagininput_data = {
                                "project_id": project_id,
                                "calibration_id": calibration_id,
                                "calibrated_fits_file": os.path.basename(comb),
                                "file_size": file_size,
                                "start_time": start_time,
                                "end_time": end_time,
                                "comments": "c16 "+comments,
                            }
                            dbutils.insert_into_table("imaginginput", imagininput_data, "imaging_id")
                            print("-------------------------")
                end_time = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')
                calibration_update_data = {
                    "set": {
                        "status": status,
                        "comments": comments,
                        "start_time": start_time,
                        "end_time": end_time
                    },
                    "where": {
                        "calibration_id": calibration_id
                    }
                }
                dbutils.update_table(calibration_update_data, "calibrationinput")

                projectobsno_update_data = {
                    "set": {
                        "status": status,
                        "comments": comments
                    },
                    "where": {
                        "project_id": project_id
                     }
                }
                dbutils.update_table(projectobsno_update_data, "projectobsno")

    def stage5(self):
        pass


    def stage6(self):
        dbutils = DBUtils.DBUtils()
        fits_images_list = glob.glob('/GARUDATA/IMAGING17/CYCLE17/*/*/FITS_IMAGE/*PBCOR*.FITS')
        # fits_images_list = ['/GARUDATA/IMAGING19/CYCLE19/5164/19_085_27DEC10/FITS_IMAGE/1445+099.GMRT325.SP2B.PBCOR.FITS']
        counter = 1
        for fits_file in fits_images_list:
            counter += 1
            # fits_file = '/GARUDATA/IMAGING19/CYCLE19/5164/19_085_27DEC10/FITS_IMAGE/1445+099.GMRT325.SP2B.PBCOR.FITS'

            fits_dir = os.path.dirname(fits_file)

            fits_table = fits.open(fits_file)
            fits_header = fits_table[0].header

            data_keys = {}

            object = os.path.basename(fits_file).split('.')[0]

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

    def __prerequisites(self):
        CYCLE_ID = 16
        CYCLE_PATH = '/GARUDATA/IMAGING16/CYCLE16/'
        CYCLE_ID = str(CYCLE_ID)

        sql_query = "select distinct o.observation_no, p.proposal_id, g.file_path, p.backend_type from " \
                    "gmrt.proposal p inner join das.observation o on p.proposal_id = o.proj_code " \
                    "inner join das.scangroup g on g.observation_no = o.observation_no " \
                    "inner join das.scans s on s.scangroup_id = g.scangroup_id " \
                    "inner join gmrt.sourceobservationtype so on p.proposal_id = so.proposal_id " \
                    "where p.cycle_id ='" + CYCLE_ID + "' " \
                                                       "and so.obs_type not like 'pulsar%' " \
                                                       "and so.obs_type not like 'phased array'" \
                                                       "and s.sky_freq1=s.sky_freq2 " \
                                                       "and s.sky_freq1 < 900000000 " \
                                                       "and s.chan_width >= 62500 " \
                                                       "and o.proj_code not like '16_279' " \
                                                       "and o.proj_code not like '17_072' " \
                                                       "and o.proj_code not like '18_031' " \
                                                       "and o.proj_code not like '19_043' " \
                                                       "and o.proj_code not like '20_083' " \
                                                       "and o.proj_code not like '21_057';"

        dbutils = DBUtils()

        data = dbutils.select_query(sql_query)
        gadpudata = {}
        for each_row in data:
            gadpudata[each_row[0]] = {
                "proposal_id": each_row[1],
                "file_path": each_row[2],
                "backend_type": each_row[3],
                "cycle_id": CYCLE_ID
            }
        return (gadpudata, CYCLE_PATH)

    def __init__(self):
        # self.stage1(self.__prerequisites()) # LTACOMB
        # self.stage2() # GVFITS
        # self.stage3() # PRE_CALIB
        self.stage4() # COMBINE_LSB_USB
        # self.stage5() # PROCESS_TARGET
        # self.stage6() # POST_PROC

Pipeline()
