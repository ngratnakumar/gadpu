from config import *
import spam


def lta2uvfits():
    logging.info("Tested - Started LTA_to_UVFITS using SPAM")
    os.chdir(BASE_DIR)
    all_observations = os.listdir(BASE_DIR)
    all_observations.sort()
    for DIR_NAME in all_observations:
        working_dir = BASE_DIR + DIR_NAME + '/'
        logging.info("Changing the Working directory " + working_dir)
        print working_dir
        os.chdir(working_dir)
        lta_files = glob.glob('*.lta*')
        lta_files.sort()
        uvfits_files = glob.glob('*.UVFITS*')
        if len(uvfits_files) == 0:
            for i in range(len(lta_files)):
                lta_file_name = lta_files[i]
                uvfits_file_name = lta_files[i] + '.UVFITS'
                logging.info(str(i) + " * Checked LTA: " + lta_file_name + " | UVFITS: " + uvfits_file_name)
                try:
                    logging.info("Started LTA_TO_UVFITS: " + lta_file_name + " => " + uvfits_file_name)
                    spam.convert_lta_to_uvfits(lta_file_name, uvfits_file_name)
                except Exception, r:
                    os.chdir(working_dir)
                    logging.error("Error LTA_TO_UVFITS: " + lta_file_name + " => " + uvfits_file_name)
                    logging.error("Error LTA_TO_UVFITS: " + str(r))


def __main__():
    logging.info("====================================================")
    logging.info("Started STAGE 1 SPAM : LTA TO UVFITS")
    if not checkSpamPreviousRunFlag():
        lta2uvfits()
    else:
        logging.info(SCRIPT_PRE_RUN_SPAM_FLAG_FILE + " === FOR SPAM Exists! === \n *** Aborting...")
    logging.info("====================================================")


if __name__ == "__main__":
    __main__()
