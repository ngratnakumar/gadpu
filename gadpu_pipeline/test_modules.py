import DBUtils

def initialize(cycle_id):
    cycle_id = str(cycle_id)

    sql_query = "select distinct o.observation_no, p.proposal_id, g.file_path from " \
                "gmrt.proposal p inner join das.observation o on p.proposal_id = o.proj_code " \
                "inner join das.scangroup g on g.observation_no = o.observation_no " \
                "inner join das.scans s on s.scangroup_id = g.scangroup_id " \
                "where cycle_id ='"+cycle_id+"' " \
                "and s.sky_freq1=s.sky_freq2 and s.sky_freq1 < 900000000 " \
                "and s.chan_width >= 62500 " \
                "and o.proj_code not like '16_279' " \
                "and o.proj_code not like '17_072' " \
                "and o.proj_code not like '18_031' " \
                "and o.proj_code not like '19_043' " \
                "and o.proj_code not like '20_083' " \
                "and o.proj_code not like '21_057';"

    dbutils = DBUtils.DBUtils()

    data = dbutils.select_query(sql_query)
    gadpudata = {}
    for each_row in data:
        gadpudata[each_row[0]] = {
                "proposal_id": each_row[1],
                "file_path": each_row[2]
            }

    return gadpudata


if __name__ == '__main__':
    initialize(19)
