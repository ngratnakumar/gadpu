import os
import glob
from DBUtils import DBUtils
import getScangroup as gS
import time
import datetime
from config import Config
import tableSch as tableSchema

currentTimeInSec = time.time()

class FileUtils:

    def check_network_connectivity(self):
        pass


    def calculalate_file_sizse_in_MB(self, filename):
        return ((os.path.getsize(filename))/1024)/1024


    def tail(f, lines=1, _buffer=4098):
        """Tail a file and get X lines from the end"""
        # place holder for the lines found
        lines_found = []

        # block counter will be multiplied by buffer
        # to get the block size from the end
        block_counter = -1

        # loop until we find X lines
        while len(lines_found) < lines:
            try:
                f.seek(block_counter * _buffer, os.SEEK_END)
            except IOError:  # either file is too small, or too many lines requested
                f.seek(0)
                lines_found = f.readlines()
                break

            lines_found = f.readlines()
            block_counter -= 1

        return lines_found[-lines:]


    def copy_files(self, src, dest):
        try:
            print("Copying "+src+"* to "+dest)
            if not os.path.exists(dest):
                os.system('mkdir -p '+dest)

            if os.path.isfile(src):
                os.system('cp '+src+' '+dest)
            else:
                os.system('cp -r '+src+'* '+dest)
        except Exception as ex:
            print(ex)


    def move_files(self, src, dest):
        try:
            print("Moving "+src+" to "+dest)
            if not os.path.exists(dest):
                os.system('mkdir -p '+dest)
            os.system('mv '+src+' '+dest)
        except Exception as ex:
            print(ex)

    def delete_file_dir(self, path):
        try:
            print("Deleting "+path)
            if os.path.isdir(path):
                os.system('rm -r '+path)
            if os.path.isfile(path):
                os.system('rm '+path)
        except Exception as ex:
            print(ex)

    def check_for_multiples(self, filename):
        files_list = glob.glob(filename+"*")
        files_list.sort()
        if len(files_list) > 1:
            return files_list

    def insert_details(self, data, project_path, isghb, cycle_id, status):
        print(data)
        dbutils = DBUtils()
        for each_rec in data:
            lta_file = os.path.basename(each_rec)
            try:
                current_date_timestamp = datetime.datetime.fromtimestamp(currentTimeInSec).strftime('%Y-%m-%d %H:%M:%S')
                lta_details = gS.get_naps_scangroup_details(lta_file)
                utils = self
                lta_details["ltacomb_size"] = int(utils.calculalate_file_sizse_in_MB(each_rec))
                lta_details["status"] = "unprocessed"
                lta_details["file_path"] = project_path
                lta_details["start_time"] = current_date_timestamp
                lta_details["proposal_dir"] = project_path.split('/')[-2]
                lta_details["pipeline_id"] = 1
                lta_details["comments"] = status
                lta_details["counter"] = 0
                lta_details["ltacomb_file"] = lta_file
                lta_details["isghb"] = isghb
                lta_details["cycle_id"] = cycle_id

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

                project_id = dbutils.insert_into_table("projectobsno", projectobsno_data, tableSchema.projectobsnoId)
                ltadetails_data["project_id"] = project_id

                lta_id = dbutils.insert_into_table("ltadetails", ltadetails_data, tableSchema.ltadetailsId)
                print lta_id
                print("projectobsno")
                print(projectobsno_data)
            except Exception as e:
                print e

