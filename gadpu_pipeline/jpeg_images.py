import os
from DBUtils import DBUtils


dbutils = DBUtils()

sql_query = "select p.base_path, p.observation_no, d.file_name from projectobsno p inner " \
            "join dataproducts d on p.project_id = d.project_id where d.file_name like '%PBCOR%'"


jpeg_images_path = '/GARUDATA/jpeg_images/'

data = dbutils.select_gadpu_query(sql_query)
counter=0
for each_row in data:
    counter+=1
    base_path = each_row[0]
    observation_no = each_row[1]
    jpeg_img_file = each_row[2].replace('.FITS','.jpeg')
    source_name = jpeg_img_file.split('.')[0]
    jpeg_img_path = base_path+'/FITS_IMAGE/'+jpeg_img_file
    if os.path.exists(jpeg_img_path):
        sql_query = "select distinct scan_id, observation_no, source from das.scans where observation_no = "+str(observation_no)+" and  source like '"+source_name+"'"
        scans_data = dbutils.select_query(sql_query)
        # print(int(observation_no), source_name, jpeg_img_path)
        if scans_data:
            cycle_dir = base_path.split('/')[3]
            images_path = jpeg_images_path+cycle_dir+'/'+str(observation_no)
            if not os.path.exists(images_path):
                os.system("mkdir -p "+images_path)
            copying = os.system("cp "+jpeg_img_path+" "+images_path+"/")
            for each_scan in scans_data:
                scan_id = each_scan[0]
                scans_update_data = {
                    "set": {
                        "jpeg_img_path": str(images_path+'/'+jpeg_img_file).replace('GARUDATA','data')
                    },
                    "where": {
                        "scan_id": int(scan_id)
                    }
                }
                dbutils.update_naps_table(scans_update_data, "das.scans")
                print("------------"+str(counter)+"----------")