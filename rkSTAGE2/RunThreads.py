import threading
import Queue
import commands
import time
import sys


class RunThread(threading.Thread):
    def __init__(self, cmd, queue):
        threading.Thread.__init__(self)
        self.cmd = cmd
        self.queue = queue

    def run(self):
        (status, output) = commands.getstatusoutput(self.cmd)
        self.queue.put((self.cmd, output, status))


result_queue = Queue.Queue()

HOME_DIR = "/home/gadpu/"
cmd_file = HOME_DIR + "precalib"
parsel_tongue = "start_parseltongue.sh"

# cmds = ['/bin/bash ' + HOME_DIR + 'stage2.sh']
cmds = []

for cmd_val in range(11, 17):
    push_cmd = parsel_tongue + " " + HOME_DIR + "THREAD" + str(cmd_val) + "/ " + str(cmd_val) + " " + cmd_file + str(cmd_val) + ".py"
    cmds.append(push_cmd)


for cmd in cmds:
    thread = RunThread(cmd, result_queue)
    time.sleep(3)
    thread.start()

while threading.active_count() > 1 or not result_queue.empty():
    while not result_queue.empty():
        original_stdout = sys.stdout
        original_stderr = sys.stderr
        precal_log = open('stage2.log', 'a+')
        precal_log.write('\n\n******STAGE2 STARTED******\n\n')
        sys.stdout = precal_log
        sys.stderr = precal_log
        (cmd, output, status) = result_queue.get()
        print('%s:' % cmd)
        print(output)
        print('=' * 60)
        sys.stdout = original_stdout
        sys.stderr = original_stderr
    time.sleep(3)
