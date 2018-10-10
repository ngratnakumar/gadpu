import psycopg2
# LTA_FILE = "25_044_10nov2013.lta"

def get_naps_scangroup_details(LTA_FILE):
    if LTA_FILE is not None or LTA_FILE is not "":
        result = None
        queryResult = None
        try:
            conn = psycopg2.connect(
                database="napsgoadb",
                user="postgres",
                password="Postgre$.2010",
                host="192.168.118.48",
                port="5432"
            )
            sql = "SELECT " \
                  "sg.scangroup_id, sg.lta_file, s.proj_code, sg.observation_no " \
                  "FROM das.scangroup sg " \
                  "INNER JOIN das.scans s on sg.scangroup_id = s.scangroup_id " \
                  "WHERE lta_file like '%"+LTA_FILE+"%' OR ltb_file like '%"+LTA_FILE+"%' "\
		  "OR lta_gsb_file like '%"+LTA_FILE+"%' " \
                  "GROUP BY sg.scangroup_id,sg.lta_file,s.proj_code,sg.observation_no " \
                                                    "ORDER BY sg.scangroup_id;"
            cur = conn.cursor()
            cur.execute(sql)
            queryResult = cur.fetchone()
            result = {
                "proposal_id": queryResult[2],
                "ltacomb_file": queryResult[1],
                "das_scangroup_id": queryResult[0],
                "observation_no": int(queryResult[3])
            }
            conn.commit()
            conn.close()
        except Exception as e:
            print e
        if result is not None:
            return result
        else:
            return (None,"Something Went Wrong in Query/Database")
