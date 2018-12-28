import sys

from DBUtils import DBUtils
from gadpu_spam import Pipeline


def __prerequisites():
    """
    Fetch the basic proposal data depending on the CYCLE_ID
    This is basically stage0
    :return:
    """
    CYCLE_ID = Pipeline().pipeline_configuration()["cycle_id"]
    CYCLE_PATH = Pipeline().pipeline_configuration()["cycle_path"]
    CYCLE_ID = str(CYCLE_ID)

    sql_query = "select distinct o.observation_no, p.proposal_id, g.file_path, p.backend_type from " \
                "gmrt.proposal p inner join das.observation o on p.proposal_id = o.proj_code " \
                "inner join das.scangroup g on g.observation_no = o.observation_no " \
                "inner join das.scans s on s.scangroup_id = g.scangroup_id " \
                "inner join gmrt.sourceobservationtype so on p.proposal_id = so.proposal_id " \
                "where p.cycle_id ='" + CYCLE_ID + "' " \
                                                   "and so.obs_type not like 'pulsar%' " \
                                                   "and so.obs_type not like 'phased array'" \
                                                   "and s.sky_freq1=s.sky_freq2 " \
                                                   "and s.sky_freq1 < 900000000 " \
                                                   "and s.chan_width >= 62500 " \
                                                   "and o.proj_code not like '16_279' " \
                                                   "and o.proj_code not like '17_072' " \
                                                   "and o.proj_code not like '18_031' " \
                                                   "and o.proj_code not like '19_043' " \
                                                   "and o.proj_code not like '20_083' " \
                                                   "and o.proj_code not like '21_057';"

    dbutils = DBUtils()

    data = dbutils.select_query(sql_query)
    gadpudata = {}
    for each_row in data:
        gadpudata[each_row[0]] = {
            "proposal_id": each_row[1],
            "file_path": each_row[2],
            "backend_type": each_row[3],
            "cycle_id": CYCLE_ID
        }
    return (gadpudata, CYCLE_PATH)

def start_pipeline():
    stage = sys.argv[1]
    pipeline = Pipeline()
    stages = {
        # '1': pipeline.copying_and_ltacomb(__prerequisites()),
        '2': pipeline.running_gvfits(),
        '3': pipeline.pre_calibration_targets(),
        '4': pipeline.combining_lsb_usb(),
        '5': pipeline.process_targets(),
        '6': pipeline.updating_fits_header(),
        '7': pipeline.generate_jpeg_images()
    }
    print(stages[stage])

start_pipeline()