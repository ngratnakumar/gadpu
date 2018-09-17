#!/usr/bin/python
# from configparser import ConfigParser
import time
import ConfigParser
import datetime

class Config:

    def config(self, filename='/home/ratnakumar/gadpu/gadpu_pipeline/database.ini', section='postgresql'):
        # create a parser
        parser = ConfigParser.ConfigParser()
        # read config file
        parser.read(filename)

        # get section, default to postgresql
        db = {}
        if parser.has_section(section):
            params = parser.items(section)
            for param in params:
                db[param[0]] = param[1]
        else:
            raise Exception('Section {0} not found in the {1} file'.format(section, filename))

        return db

    def naps_config(self, filename='/home/ratnakumar/gadpu/gadpu_pipeline/database.ini', section='napsgoadb'):
        # create a parser
        parser = ConfigParser.ConfigParser()
        # read config file
        parser.read(filename)

        # get section, default to postgresql
        db = {}
        if parser.has_section(section):
            params = parser.items(section)
            for param in params:
                db[param[0]] = param[1]
        else:
            raise Exception('Section {0} not found in the {1} file'.format(section, filename))

        return db

    def table_schema(self):

        currentTimeInSec = time.time()
        currentTimeInFormat = datetime.datetime.fromtimestamp(currentTimeInSec).strftime('%Y-%m-%d %H:%M:%S')

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
            "comments": "calibrationinput Data's comments"
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
