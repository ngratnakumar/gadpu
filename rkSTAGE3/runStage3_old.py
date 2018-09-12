import sys
import subprocess
import socket
import time
import gadpudbapi.project_model_v1 as project_model
import gadpudbapi.tableSch as tableSchema

hostname = socket.gethostname()
max_threads = 6
thread_no = sys.argv[1]
column_keys = [tableSchema.computenodeId, "threads_count", "reboot_flag"]
where_condition = {
    "node_name": hostname
}

db_model = project_model.ProjectModel()
counter = 0

node_details = db_model.select_from_table("computenode", column_keys, where_condition, 1)
node_id = node_details[0]
node_thread_count = node_details[1]
node_reboot_flag = node_details[2]
comment = "Processing will not start until system is rebooted"
if node_reboot_flag:
    node_update_data = {
        "set": {
            "threads_count": 0,
            "status": "need_reboot",
            "reboot_flag": False,
            "comments": comment
        },
        "where": {
            "node_id": node_id
        }
    }
    db_model.update_table(node_update_data, "computenode")
    print comment
    counter = 1
if node_thread_count <= max_threads and node_thread_count >= 0:
    node_update_data = {
        "set": {
            "threads_count": node_thread_count + 1,
            "status": "processing",
            "comments": "started processing"
        },
        "where": {
            "node_id": node_id
        }
    }
    db_model.update_table(node_update_data, "computenode")
    print "Starting Parsel Tounge == " + str(counter)
    cmd = ['start_parseltongue.sh', 'THREAD' + str(thread_no) + '/', str(thread_no),
           '../proctarget' + str(thread_no) + '.py']
    print cmd
    start_process = subprocess.Popen(cmd)
    start_process.wait()
    time.sleep(60)
    counter += 1
elif node_thread_count > max_threads:
    # Set reboot_flag to True
    print "Node thread count " + str(node_thread_count) + " | Setting reboot_flag to True for " + hostname
    node_update_data = {
        "set": {
            "threads_count": 0,
            "status": "rebooting",
            "comments": "max threads limit " + str(max_threads) + " reached, now rebooting",
            "reboot_flag": True
        },
        "where": {
            "node_id": node_id
        }
    }
    db_model.update_table(node_update_data, "computenode")
thread_no = thread_no+1
time.sleep(2)
