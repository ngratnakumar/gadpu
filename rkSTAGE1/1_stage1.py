from config import *

'''

***************************************************************
*** PLEASE CHECK for HIGH NOTE's before RUNNING THIS SCRIPT ***
***************************************************************

GADPU Stage1 - Step1 Code

1) Get LIST of VALID Data from Database
2) Move the INVALID DATA to INVALID_CYCLE_DIR
3) Check the Xs.lta files and their order, sort them
4) Move the FILTERED DATA to FILTERED_CYCLE_DIR and 
leave the remaining files in the base directory CYCLE_DIR
5) NOW, each and every PROPOSAL_DIR should have One And 
Only One LTA file with obslog and FLAG files
6) Run LTACOMB for every PROPOSAL_DIR and generates
the ltacomb_out.lta file
7) Rename the ltacomb_out.lta to the proposal_lta filename
8) Move the Uncombined to UNCOMB_DIR
9) The input COMBINED LTA files are moved to ORGINAL_DIR
'''


def checkLTACOMB(FILTERED_LTA_LIST):
    '''
    Parse through the given Source Directory and if any directory is having more
    than one lta file the do LTACOMB
    '''
    '''
    # ***-----HIGH NOTE------ Comment the failed_list data for regular run ***
     <---- remove # for regular run
        FILTERED_LTA_LIST = filterData()
        FAILED_FILTERED_LTA_LIST = []
        failed_list_file = open("/GARUDATA/rkSTAGE1/c24failed", "r")
        failed_list = failed_list_file.readlines()
        for proposal in failed_list:
            failed_proposal = proposal.strip().split('/')[4]
            if failed_proposal in FILTERED_LTA_LIST:
                FAILED_FILTERED_LTA_LIST.append(failed_proposal)

    # <--- remove # for regular run & also uncomment the below HIGH NOTE's for loop
    Also comment the below for loop having FAILED_FILTERED_LTA_LIST
        for each_dir in FAILED_FILTERED_LTA_LIST:
        # ***-----HIGH NOTE------ Uncomment the below for loop FILTERED_LTA_LIST for regular run ***
    '''
    for each_dir in FILTERED_LTA_LIST:
        lta_dir = LTA_DIR + each_dir
        lta_list = glob.glob(lta_dir + '/*.lta*')
        lta_list.sort()
        lta_count = len(lta_list)
        x2s_counter = 0
        x8s_counter = 0
        x16s_counter = 0
        lta_counter = 0
        if lta_count > 1:
            for lta_file_name in lta_list:
                new_lta_list = copy.deepcopy(lta_list)
                # if re.findall(r'_\d\ds.lta', lta_file_name):
                if re.findall(r'_2s.lta', lta_file_name):
                    x2s_counter += 1
                if re.findall(r'_8s.lta', lta_file_name):
                    x8s_counter += 1
                if re.findall(r'_16s.lta', lta_file_name):
                    x16s_counter += 1
                if re.findall(r'.lta', lta_file_name):
                    lta_counter += 1
            print str(lta_list)
            if x2s_counter < lta_counter:
                lta_counter -= x2s_counter
                x2s_counter = 0
                for lta_file_name in lta_list:
                    if re.findall(r'_2s.lta', lta_file_name):
                        src_file_name = lta_file_name
                        dest_file_name = lta_file_name.replace(CYCLE_NAME, FILTERED_CYCLE_NAME)
                        moveFiles(src_file_name, dest_file_name)
                        new_lta_list.remove(src_file_name)
                    if x8s_counter < lta_counter:
                        if re.findall(r'_8s.lta', lta_file_name):
                            src_file_name = lta_file_name
                            dest_file_name = lta_file_name.replace(CYCLE_NAME, FILTERED_CYCLE_NAME)
                            moveFiles(src_file_name, dest_file_name)
                            new_lta_list.remove(src_file_name)
            print x2s_counter, x8s_counter, x16s_counter, lta_counter
            if len(new_lta_list) > 1:
                runLTACOMB(lta_dir, new_lta_list)
        elif lta_count == 1:
            logging.info(lta_dir + " single LTA " + str(lta_list) + ": LTACOMB not required\n")
        else:
            logging.debug(lta_dir + " NO LTA files\n")


def runLTACOMB(lta_dir, lta_list):
    os.chdir(lta_dir)
    comb_list = ','.join(map(str, lta_list))
    lta_list.sort()
    comb_file_name = lta_list[0]
    cmd = ['ltacomb', '-i', comb_list]
    runCommand(cmd, None)
    print (cmd)
    if checkFileSize > 0:
        for lta_files in lta_list:
            dest_file = lta_files.replace(CYCLE_NAME, ORIGINAL_CYCLE_NAME)
            moveFiles(lta_files, dest_file)
        moveFiles("ltacomb_out.lta", comb_file_name)
    else:
        moveFiles(lta_dir, UNCOMB_CYCLE_NAME)
        logging.error(lta_dir + ": LTACOMB Failed\n")


def filterData():
    FILTERED_LTA_LIST = []
    try:
        conn = psycopg2.connect("dbname='" + DBNAME + "' user='" + DBUSER + "' host='" + DBHOST + "'")
        cur = conn.cursor()
        cur.execute(QUERY1)
        rows = cur.fetchall()
        for row in rows:
            FILTERED_LTA_DIR = str(row[5]).split('/')[3]
            FILTERED_LTA_LIST.append(FILTERED_LTA_DIR)
            FILTERED_LTA_LIST = list(set(FILTERED_LTA_LIST))
    except Exception, ex:
        print("Error Connecting NAPS Database")
        logging.error("Error Connecting NAPS Database" + str(ex))
    SOURCE_DIR_LIST = os.listdir(LTA_DIR)
    logging.info("SOURCE_DIR_LIST Count" + str(len(SOURCE_DIR_LIST)))
    logging.info(str(SOURCE_DIR_LIST))
    if SOURCE_DIR_LIST:
        for DELIST_THIS_DIR in SOURCE_DIR_LIST:
            logging.info(DELIST_THIS_DIR)
            if DELIST_THIS_DIR not in FILTERED_LTA_LIST:
                logging.info("INVALID NOT in filterData" + DELIST_THIS_DIR)
                src = LTA_DIR + DELIST_THIS_DIR
                dest = INVALID_DIR
                logging.info("INVALID " + src + " moving to " + dest)
                # moveFiles(src, dest)
    print(str(FILTERED_LTA_LIST))
    logging.info(str(FILTERED_LTA_LIST))
    return FILTERED_LTA_LIST


def __main__():
    print(len(filterData()))
    if not checkPreviousRunFlag():
        pass
        # checkLTACOMB(filterData())
        # cmd = ['touch', SCRIPT_PRE_RUN_FLAG_FILE]
        # runCommand(cmd, None)
    else:
        logging.info(SCRIPT_PRE_RUN_FLAG_FILE + " === Exists! === \n *** Aborting...\n")


if __name__ == "__main__":
    __main__()
