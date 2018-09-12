import sys
import subprocess
import socket
import time
import gadpudbapi.project_model_v1 as project_model
import gadpudbapi.tableSch as tableSchema
import glob


def main():
    hostname = socket.gethostname()
    # hostname = "garuda-079"
    max_threads = 30
    thread_no = sys.argv[1]
    # thread_no = 11
    column_keys = [tableSchema.computenodeId, "threads_count", "reboot_flag", "status"]
    where_condition = {
        "node_name": hostname
    }

    db_model = project_model.ProjectModel()
    counter = 0


    node_details = db_model.select_from_table("computenode", column_keys, where_condition, 1)
    node_id = node_details[0]
    node_thread_count = check_thread_count(node_id)
    print check_thread_count(node_id)
    node_reboot_flag = node_details[2]
    node_status = node_details[3]
    print node_details
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
        exit(code=0)
    elif node_status:
        if "need_reboot" in node_status:
            where_condition = {
                'node_id': node_id,
                # 'status': "success"
            }
            db_model.delete_from_table("computethread", where_condition)

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
        # print "Starting Parsel Tounge == " + str(counter)
        print start_parsel_tounge(thread_no)
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
    thread_no = int(thread_no)+1
    time.sleep(10)


def start_parsel_tounge(spam_id):
    thread_no = spam_id
    thread_list = glob.glob('THREAD' + str(thread_no))
    while thread_list > 0:
        thread_list = glob.glob('THREAD'+str(thread_no))
        print thread_list
        if not thread_list:
            print "Running parsel_Tounge"
            cmd = ['start_parseltongue.sh', 'THREAD' + str(thread_no) + '/', str(thread_no),
                   '../proctarget' + str(thread_no) + '.py']
            start_process = subprocess.Popen(cmd)
            start_process.wait()
            break
        else:
            print "Waiting for the process to complete .."
        time.sleep(300)


def check_thread_count(node_id):
    db_model = project_model.ProjectModel()
    node_thread_count_data = ["count(*)"]
    node_thread_where_data = {
        "node_id": node_id
    }
    threads_count = db_model.select_from_table("computethread", node_thread_count_data, node_thread_where_data, 0)
    return threads_count[0]


if __name__ == '__main__':
    print "Started MAIN run ..."
    while 1:
        print "Checking ..."
        main()
        time.sleep(120)
