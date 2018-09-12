import project_model_v1 as project_model
import time
import datetime
import random

currentTimeInSec = time.time()
currentTimeInFormat = datetime.datetime.fromtimestamp(currentTimeInSec).strftime('%Y-%m-%d %H:%M:%S')
model = project_model.ProjectModel()

pipelineId = "pipeline_id"
pipelineData = {
    "name": "SPAM-testing",
    "version": 234,
    "process_type": "testing",
    "status": "testing",
    "comments": "Testing Datetimestamp auto generate",
}

calibrationinputId = "calibration_id"
calibrationinputData = {
    "project_id": 1,
    "lta_id": 1,
    "uvfits_file": "uvfitsfile.UVFITS",
    "uvfits_size": 2342342,
    "status": "processing",
    "start_time": currentTimeInFormat,
    "end_time": currentTimeInFormat,
    "comments": "calibrationinputData's comments"
}

computenodeId = "node_id"
computenodeData = {
    "node_name": "garuda-072",
    "threads_count": 4,
    "status": "processing",
    "comments": "computenode's comments",
    "reboot_flag": False
}

computethreadId = "thread_id"
computethreadData = {
    'pipeline_id': 23,
    'node_id': 3,
    'thread_dir': '/home/gadpu/THREAD12/',
    'status': 'processing',
    'file_name': 'lta_file.lta',
    'comments': "computethread's data"
}

dataproductsId = "product_id"
dataproductsData = {
    'project_id': 12,
    'imaging_id': 423,
    'file_name': 'product.ext',
    'file_type': 'ext',
    'file_size': 432455,
    'status': 'success',
    'generated': currentTimeInFormat,
    'comments': "data products table comments"
}

imaginginputId = "imaging_id"
imaginginputData = {
    'project_id': 34,
    'calibration_id': 23,
    'calibrated_fits_file': 'calibarated_uvfits_file.uvfits',
    'status': 'processing',
    'file_size': 234234892,
    'start_time': currentTimeInFormat,
    'end_time': currentTimeInFormat,
    'comments': "imaging table comments"
}

ltadetailsId = "lta_id"
ltadetailsData = {
    'project_id': 45,
    'das_scangroup_id': 234523,
    'ltacomb_file': 'lta_comb.lta',
    'status': 'processing',
    'start_time': currentTimeInFormat,
    'ltacomb_size': 3223343443,
    'end_time': currentTimeInFormat,
    'comments': "lta details table's comments"
}

projectobsnoId = "project_id"
projectobsnoData = {
    'pipeline_id': 1,
    'proposal_dir': '25_076_23MAY2013',
    'base_path': '/GARUDATA/IMAGING25/CYCLE25/',
    'observation_no': 3466,
    'status': 'processing',
    'counter': 1,
    'comments': "project observation table's comments"
}

#
# def dictValuePad(key):
#     return '%s'

# projectobsno - table


upData = {
    'set': {
        'status': 'testing',
        'comments': "checking the update table functionality",
        'counter': 24
    },
    
    'where': {
        'project_id': 2
    }
}

testData = {
    'pipeline_id': 56,
    'proposal_dir': '25_076_23MAY2013',
    'base_path': '/GARUDATA/IMAGING25/CYCLE25/',
    'observation_no': 3466,
    'status': 'processing',
    'counter': 1,
    'comments': "project observation table's comments"
}

# Insert Values into table
#print model.insert_into_table("pipeline", pipelineData, pipelineId)

# Update the table
# updateCount = model.update_table(upData, "projectobsno")
# print 'Updated => Rows : '+str(updateCount)


# selectData = projectobsnoData.keys()
# selectData.append(projectobsnoId)
# whereData = {
#     "status": "unprocessed",
# }
# # # Select from the table
# selectResult = model.select_from_table("projectobsno", selectData, whereData, None)
# print selectResult

# column_keys = [imaginginputId, projectobsnoId, "calibrated_fits_file"]
# where_con = {
#     "status": "unprocessed"
# }
# unprocessed_id_list = model.select_from_table("imaginginput", column_keys, where_con, 1)
# print unprocessed_id_list

# where_con = {
#     "status": "unprocessed"
# }
# to_be_processed = model.select_from_table("imaginginput", column_keys, where_con, 1)
# calibrated_fits_file = to_be_processed[2]
# imaging_id = to_be_processed[0]
# project_id = to_be_processed[1]
#
# print to_be_processed
# print imaging_id, project_id, calibrated_fits_file

# where_condition = {
#     'node_id': 23,
#     # 'status': "success"
# }
# print(model.delete_from_table("computethread", where_condition),where_condition.values())
column_keys = [imaginginputId, projectobsnoId, "calibrated_fits_file", "status"]
where_con = {
    "status": "unprocessed"
}
to_be_processed = model.select_from_table("imaginginput", column_keys, where_con, None)
print to_be_processed
for e in to_be_processed[2]:
	print e
