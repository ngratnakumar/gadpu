#!/usr/bin/python

import psycopg2
from config import config

CREATE_TABLE_IN_DATABASE = True


def create_tables():
    """ create tables in the PostgreSQL database"""
    commands = (
        """
        CREATE TABLE pipeline (
            pipeline_id SERIAL PRIMARY KEY,
            name VARCHAR(30) NOT NULL,
            version INTEGER NOT NULL,
            process_type VARCHAR(30) NOT NULL,
            status VARCHAR(20) NOT NULL,
            comments TEXT NULL,
            generated TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
        """,
        """
        CREATE TABLE projectobsno (
            project_id SERIAL PRIMARY KEY,
            pipeline_id INTEGER NOT NULL,
            proposal_dir VARCHAR(255) NOT NULL,
            base_path VARCHAR(255) NOT NULL,
            observation_no BIGINT NOT NULL,
            status VARCHAR(20),
            counter INTEGER DEFAULT 0,
            comments TEXT NULL,
            FOREIGN KEY (pipeline_id)
            REFERENCES pipeline (pipeline_id)
            ON UPDATE CASCADE ON DELETE CASCADE            
        )
        """,
        """
        CREATE TABLE ltadetails (
            project_id INTEGER NOT NULL,
            das_scangroup_id INTEGER NOT NULL,
            lta_id SERIAL PRIMARY KEY,
            ltacomb_file VARCHAR(255) NOT NULL,
            status VARCHAR(20),
            start_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            ltacomb_size BIGINT NOT NULL,
            end_time TIMESTAMP,
            comments TEXT NULL,
            FOREIGN KEY (project_id)
            REFERENCES projectobsno (project_id)
            ON UPDATE CASCADE ON DELETE CASCADE
        )
        """,
        """
        CREATE TABLE calibrationinput(
            calibration_id SERIAL PRIMARY KEY,
            project_id INTEGER NOT NULL,
            lta_id INTEGER NOT NULL,
            uvfits_file VARCHAR(255) NOT NULL,
            uvfits_size BIGINT NOT NULL,
            status VARCHAR(20),
            start_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            end_time TIMESTAMP,
            comments TEXT NULL,
            FOREIGN KEY (lta_id)
            REFERENCES ltadetails (lta_id)
            ON UPDATE CASCADE ON DELETE CASCADE,
            FOREIGN KEY (project_id)
            REFERENCES projectobsno (project_id)
            ON UPDATE CASCADE ON DELETE CASCADE            
        )
        """,
        """
        CREATE TABLE imaginginput (
            imaging_id SERIAL PRIMARY KEY,
            project_id INTEGER NOT NULL,
            calibration_id INTEGER NOT NULL,
            calibrated_fits_file VARCHAR(255) NOT NULL,
            status VARCHAR(20),
            file_size BIGINT NOT NULL,
            start_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            end_time TIMESTAMP,
            comments TEXT NULL,
            FOREIGN KEY (calibration_id)
            REFERENCES calibrationinput (calibration_id)
            ON UPDATE CASCADE ON DELETE CASCADE,
            FOREIGN KEY (project_id)
            REFERENCES projectobsno (project_id)
            ON UPDATE CASCADE ON DELETE CASCADE
        )
        """,
        """
        CREATE TABLE dataproducts (
            product_id SERIAL PRIMARY KEY,
            project_id INTEGER NOT NULL,
            imaging_id INTEGER NOT NULL,
            file_name VARCHAR(255) NOT NULL,
            file_type VARCHAR(30) NOT NULL,
            file_size BIGINT NOT NULL,
            status VARCHAR(20) NOT NULL,
            generated TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            comments TEXT NULL,
            FOREIGN KEY (project_id)
            REFERENCES projectobsno (project_id)
            ON UPDATE CASCADE ON DELETE CASCADE,
            FOREIGN KEY (imaging_id)
            REFERENCES imaginginput (imaging_id)
            ON UPDATE CASCADE ON DELETE CASCADE
        )
        """,
        """
        CREATE TABLE computenode (
            node_id SERIAL PRIMARY KEY,
            node_name VARCHAR(255) NOT NULL,
            threads_count INTEGER DEFAULT 0,
            status VARCHAR(20),
            comments TEXT NULL,
            reboot_flag BOOLEAN DEFAULT FALSE
        )
        """,
        """
        CREATE TABLE computethread (
            thread_id SERIAL PRIMARY KEY,
            pipeline_id INTEGER NOT NULL,
            node_id INTEGER,
            thread_dir VARCHAR(255),
            status VARCHAR(20),
            file_name VARCHAR(255) NOT NULL,
            comments TEXT NULL,
            FOREIGN KEY (node_id)
            REFERENCES computenode (node_id)
            ON UPDATE CASCADE ON DELETE CASCADE,
            FOREIGN KEY (pipeline_id)
            REFERENCES pipeline (pipeline_id)
            ON UPDATE CASCADE ON DELETE CASCADE
        )
        """)
    conn = None
    try:
        # read the connection parameters
        params = config()
        # connect to the PostgreSQL server
        conn = psycopg2.connect(**params)
        cur = conn.cursor()
        # create table one by one
        for command in commands:
            cur.execute(command)
        # close communication with the PostgreSQL database server
        cur.close()
        # commit the changes
        conn.commit()
    except (Exception, psycopg2.DatabaseError) as error:
        print(error)
    finally:
        if conn is not None:
            conn.close()


if __name__ == '__main__':
    if CREATE_TABLE_IN_DATABASE:
        create_tables()
    else:
        print "===> CREATE_TABLE_IN_DATABASE is set to False"
