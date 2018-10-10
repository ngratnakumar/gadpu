import glob
import getScangroup as gS
import os
import project_model_v1 as project_model
import time
import datetime
import tableSch as tableSchema
import random
import socket

currentTimeInSec = time.time()


class FileUtilities:

    def __init__(self):
        pass

    @staticmethod
    def calculate_file_size(file_path):
        return ((os.path.getsize(file_path))/1024)/1024



def seeding_controller(cycle_id):
    seed_projectobsno(cycle_id)
    #seed_calibrationinput()
    seed_imaginginput()
    #test_stage3()


def test_stage3():
    db_model = project_model.ProjectModel()
    # Get random imaging_id & project_id
    column_keys = [tableSchema.imaginginputId,tableSchema.projectobsnoId, "calibrated_fits_file"]
    where_con = {
        "status": "unprocessed"
    }
    unprocessed_id_list = db_model.select_from_table("imaginginput",column_keys,where_con,None)
    to_be_processed = random.choice(unprocessed_id_list)
    calibrated_fits_file = to_be_processed["calibrated_fits_file"]
    # Using the above project_id, fetch base_path
    column_keys = ["base_path"]
    where_con = {
        "project_id": to_be_processed[tableSchema.projectobsnoId]
    }
    base_path = db_model.select_from_table("projectobsno", column_keys, where_con, 0)
    # Update status for imaginginput for selected imaging_id
    current_date_timestamp = datetime.datetime.fromtimestamp(currentTimeInSec).strftime('%Y-%m-%d %H:%M:%S')
    update_data = {
        "set": {
            "status": "processing",
            "start_time": current_date_timestamp
        },

        "where": {
            "imaging_id": to_be_processed[tableSchema.imaginginputId],
            "project_id": to_be_processed[tableSchema.projectobsnoId]
        }
    }
    db_model.update_table(update_data,"imaginginput")
    # Insert new thread to computethread
    hostname = socket.gethostname()
    column_keys = [tableSchema.computenodeId, "threads_count"]
    where_condition = {
        "node_name": hostname
    }
    node_details = db_model.select_from_table("computenode", column_keys, where_condition, 0)
    node_id = node_details[tableSchema.computenodeId]
    threads_count = node_details["threads_count"]
    computethread_data = {
        'pipeline_id': 1,
        'node_id': node_id,
        'thread_dir': '/home/gadpu/THREAD12/',
        'status': 'processing',
        'file_name': calibrated_fits_file
    }
    db_model.insert_into_table("computethread", computethread_data, tableSchema.computethreadId)
    # Update computenode with the above generated node_id & increment threads_count
    node_update_data = {
        "set": {
            "threads_count": threads_count+1,
            "status": "processing"
        },
        "where": {
            "node_id": node_id
        }
    }
    db_model.update_table(node_update_data, "computenode")


def seed_projectobsno(cycle_id):
    cycle_location = "/GARUDATA/IMAGING" + cycle_id + "/CYCLE" + cycle_id + "/*/"
    project_dir_list = glob.glob(cycle_location+"*")
    db_model = project_model.ProjectModel()
    for eachDir in project_dir_list:
        lta_file_list = glob.glob(eachDir+"/*.lta")
        if len(lta_file_list) >= 1:
            lta_file = lta_file_list[0].split('/')[-1]
            try:
                current_date_timestamp = datetime.datetime.fromtimestamp(currentTimeInSec).strftime('%Y-%m-%d %H:%M:%S')
                lta_details = gS.get_naps_scangroup_details(lta_file)
                utils = FileUtilities()
                lta_details["ltacomb_size"] = int(utils.calculate_file_size(lta_file_list[0]))
                lta_details["status"] = "unprocessed"
                lta_details["file_path"] = eachDir
                lta_details["start_time"] = current_date_timestamp
                lta_details["proposal_dir"] = eachDir.split('/')[-1]
                lta_details["pipeline_id"] = 1
                lta_details["comments"] = "cycle18"
                lta_details["counter"] = 0
                lta_details["ltacomb_file"] = lta_file

                projectobsno_data = {}
                for key in tableSchema.projectobsnoData.iterkeys():
                    if key in lta_details.iterkeys():
                        projectobsno_data[key] = lta_details[key]

                ltadetails_data = {}
                for key in tableSchema.ltadetailsData.iterkeys():
                    if key in lta_details.iterkeys():
                        ltadetails_data[key] = lta_details[key]
                #print ltadetails_data
                print("ltadetails_data")
                print(ltadetails_data)

                project_id = db_model.insert_into_table("projectobsno", projectobsno_data, tableSchema.projectobsnoId)
                ltadetails_data["project_id"] = project_id

                lta_id = db_model.insert_into_table("ltadetails", ltadetails_data, tableSchema.ltadetailsId)
                print lta_id
                print("projectobsno")
                print(projectobsno_data)
            except Exception as e:
                print e



def seed_ltadetails():
    return "NULL"


def seed_calibrationinput():
    calibrationinput_data = {}
    utils = FileUtilities()
    db_model = project_model.ProjectModel()
    column_keys = [k for k in tableSchema.ltadetailsData.viewkeys()]
    column_keys.append("project_id")
    column_keys.append("lta_id")
    lta_result = db_model.select_test_table("ltadetails", column_keys, None, None)
    for lta_row in lta_result:
        project_id = lta_row["project_id"]
        calibrationinput_data["project_id"] = project_id
        calibrationinput_data["lta_id"] = lta_row["lta_id"]
        where_con = {"project_id": project_id, "comments": "cycle 18"}
        column_keys = [k for k in tableSchema.projectobsnoData.viewkeys()]
        column_keys.append("project_id")
        proj_result = db_model.select_from_table("projectobsno", column_keys, where_con, None)
        if proj_result:
            proj_result = proj_result[0]
            #print(proj_result)
            uvfits_file_list = glob.glob(proj_result["file_path"]+"/*.UVFITS")
            if uvfits_file_list:
                current_date_timestamp = datetime.datetime.fromtimestamp(currentTimeInSec).strftime('%Y-%m-%d %H:%M:%S')
                uvfits_file_path = uvfits_file_list[0]
                uvfits_file = uvfits_file_list[0].split('/')[-1]
                calibrationinput_data["uvfits_file"] = uvfits_file
                calibrationinput_data["uvfits_size"] = utils.calculate_file_size(uvfits_file_path)
                calibrationinput_data["status"] = "unprocessed"
                calibrationinput_data["start_time"] = current_date_timestamp
                calibrationinput_data["comments"] = "cycle18"
                calibrationinput_result = db_model.insert_into_table("calibrationinput", calibrationinput_data, tableSchema.calibrationinputId)
                print calibrationinput_data


def seed_imaginginput():
    utils = FileUtilities()
    db_model = project_model.ProjectModel()
    column_keys = [k for k in tableSchema.calibrationinputData.viewkeys()]
    column_keys.append("project_id")
    column_keys.append("calibration_id")
    calibration_result = db_model.select_test_table("calibrationinput", column_keys, None, None)
    print(len(calibration_result))
    for calibration_row in calibration_result:
        project_id = calibration_row["project_id"]
        calibration_id = calibration_row["calibration_id"]
        column_keys = [k for k in tableSchema.projectobsnoData.viewkeys()]
        where_con = {"project_id": project_id, "comments": "cycle 18"}
        projectobsno_result = db_model.select_from_table("projectobsno", column_keys, where_con, None)
        # print(projectobsno_result)
        if projectobsno_result:
            projectobsno_result = projectobsno_result[0]
            base_path = projectobsno_result["file_path"]
            proposal_dir = projectobsno_result["proposal_dir"]
            precalibrated_base_path = base_path+"/PRECALIB/"
            calibrated_fits_list = glob.glob(precalibrated_base_path+"*.UVFITS")
            imaginginput_data = {}
            # print(len(calibrated_fits_list))
            for each_fits in calibrated_fits_list:
                # print each_fits
                current_date_timestamp = datetime.datetime.fromtimestamp(currentTimeInSec).strftime('%Y-%m-%d %H:%M:%S')
                imaginginput_data["calibration_id"] = calibration_id
                imaginginput_data["project_id"] = project_id
                fits_file = each_fits.split('/')[-1]
                imaginginput_data["calibrated_fits_file"] = fits_file
                imaginginput_data["status"] = "unprocessed"
                imaginginput_data["file_size"] = utils.calculate_file_size(each_fits)
                imaginginput_data["start_time"] = current_date_timestamp
                imaginginput_data["comments"] = "cycle18"
                try:
                    if 'lta' not in fits_file:
                        print(imaginginput_data)
                        imaging_id = db_model.insert_into_table("imaginginput", imaginginput_data, tableSchema.imaginginputId)
                        print("imaging_id")
                except Exception as e:
                    print e


def seed_dataproducts(self):
    return "NULL"


if __name__ == '__main__':
    print("Start Seeding")
    CYCLE_ID = "18"
    seeding_controller(CYCLE_ID)
