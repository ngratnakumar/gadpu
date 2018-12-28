import psycopg2
import config
import time
from config import Config


class DBUtils:

    def insert_into_table(self, tableName, data, returnValue):

        isql = 'INSERT INTO ' + tableName + ' ({})'.format(
            ', '.join('{}'.format(k) for k in data)) + ' VALUES ({})'.format(
            ', '.join('%s'.format(x) for x in data)) + ' RETURNING ' + returnValue
        conn = None
        rVal = None
        error = None
        try:
            # read database configuration
            params = Config().config()
            # connect to the PostgreSQL database
            conn = psycopg2.connect(**params)
            # create a new cursor
            cur = conn.cursor()
            # execute the INSERT statement
            cur.execute(isql, data.values())
            # get the generated id back
            rVal = cur.fetchone()[0]
            # commit the changes to the database
            conn.commit()
            # close communication with the database
            cur.close()
        except (Exception, psycopg2.DatabaseError) as error:
            print(error)
        finally:
            if conn is not None:
                conn.close()
        if error is not None:
            return error
        else:
            return rVal

    def update_table(self, updateData, tableName):
        setData = updateData['set']
        whereData = updateData['where']
        sql = 'UPDATE '+tableName+' SET {}'.format(', '.join('{}=%s'.format(k) for k in setData)) + ' WHERE {}'.format(
            ' AND '.join('{}=%s'.format(x) for x in whereData))
        refData = []
        for setFieldValue in setData.values():
            refData.append(setFieldValue)
        for whereFieldValue in whereData.values():
            refData.append(whereFieldValue)

        conn = None
        updated_rows = ""
        try:
            # read database configuration
            params = Config().config()
            # connect to the PostgreSQL database
            conn = psycopg2.connect(**params)
            # create a new cursor
            cur = conn.cursor()
            # execute the UPDATE statement
            cur.execute(sql, refData)
            # get the updated row counts
            updated_rows = cur.rowcount
            # commit the changes to the database
            conn.commit()
            # close communication with the database
            cur.close()
        except (Exception, psycopg2.DatabaseError) as error:
            print(error)
        finally:
            if conn is not None:
                conn.close()
        return updated_rows

    def update_naps_table(self, updateData, tableName):
        setData = updateData['set']
        whereData = updateData['where']
        sql = 'UPDATE '+tableName+' SET {}'.format(', '.join('{}=%s'.format(k) for k in setData)) + ' WHERE {}'.format(
            ' AND '.join('{}=%s'.format(x) for x in whereData))
        refData = []
        for setFieldValue in setData.values():
            refData.append(setFieldValue)
        for whereFieldValue in whereData.values():
            refData.append(whereFieldValue)

        conn = None
        updated_rows = ""
        try:
            # read database configuration
            params = Config().naps_config()
            # connect to the PostgreSQL database
            conn = psycopg2.connect(**params)
            # create a new cursor
            cur = conn.cursor()
            # execute the UPDATE statement
            cur.execute(sql, refData)
            # get the updated row counts
            updated_rows = cur.rowcount
            # commit the changes to the database
            conn.commit()
            # close communication with the database
            cur.close()
        except (Exception, psycopg2.DatabaseError) as error:
            print(error)
        finally:
            if conn is not None:
                conn.close()
        return updated_rows

    def select_test_table(self, tableName, columnKeys, whereData, returnFieldsCount):
        valList =[]
        whrVal =[]
        for val in columnKeys:
            valList.append(val)
        if returnFieldsCount is None:
            sql = 'SELECT {}'.format(', '.join('{}'.format(y) for y in columnKeys)) + ' FROM ' + tableName
        else:
            for val in whereData.values():
                valList.append(val)
                whrVal.append((val))
            sql = 'SELECT {}'.format(', '.join('{}'.format(y) for y in columnKeys))+' FROM '+tableName+' WHERE {}'.format('AND '.join('{}=%s '.format(u) for u in whereData))

        # sql = 'SELECT '+tableName+' SET {}'.format(', '.join('{}=%s'.format(k) for k in setData)) + ' WHERE {}'.format(
        #     ', '.join('{}=%s'.format(x) for x in whereData))
        conn = None
        selected_rows = None
        result = []
        try:
            # read database configuration
            params = Config().config()
            # connect to the PostgreSQL database
            conn = psycopg2.connect(**params)
            # create a new cursor
            cur = conn.cursor()
            # execute the UPDATE statement
            cur.execute(sql, whrVal)
            # get the updated row counts
            if returnFieldsCount is None:
                selected_rows = cur.fetchall()
                for each in selected_rows:
                    result.append(dict(zip(columnKeys, each)))
            else:
                result = cur.fetchone()
            # commit the changes to the database
            conn.commit()
            # close communication with the database
            cur.close()
        except (Exception, psycopg2.DatabaseError) as error:
            print(error)
            result = error
        finally:
            if conn is not None:
                conn.close()
        return result

    def select_from_table(self, tableName, columnKeys, whereData, returnFieldsCount):
        valList =[]
        whrVal =[]
        for val in columnKeys:
            valList.append(val)
        if returnFieldsCount is None:
            for val in whereData.values():
                valList.append(val)
                whrVal.append((val))
            sql = 'SELECT {}'.format(', '.join('{}'.format(y) for y in columnKeys)) + ' FROM ' + tableName+' WHERE {}'.format('AND '.join('{}=%s '.format(u) for u in whereData))
        else:
            for val in whereData.values():
                valList.append(val)
                whrVal.append((val))
            sql = 'SELECT {}'.format(', '.join('{}'.format(y) for y in columnKeys))+' FROM '+tableName+' WHERE {}'.format('AND '.join('{}=%s '.format(u) for u in whereData))

        # sql = 'SELECT '+tableName+' SET {}'.format(', '.join('{}=%s'.format(k) for k in setData)) + ' WHERE {}'.format(
        #     ', '.join('{}=%s'.format(x) for x in whereData))
        conn = None
        selected_rows = None
        result = []
        try:
            # read database configuration
            params = Config().config()
            # connect to the PostgreSQL database
            conn = psycopg2.connect(**params)
            # create a new cursor
            cur = conn.cursor()
            # execute the UPDATE statement
            cur.execute(sql, whrVal)
            # get the updated row counts
            if returnFieldsCount is None:
                selected_rows = cur.fetchall()
                for each in selected_rows:
                    result.append(dict(zip(columnKeys, each)))
            else:
                result = cur.fetchone()
            # commit the changes to the database
            conn.commit()
            # close communication with the database
            cur.close()
        except (Exception, psycopg2.DatabaseError) as error:
            print(error)
            result = error
        finally:
            if conn is not None:
                conn.close()
        return result


    def delete_from_table(self, tableName, whereData):
        time.sleep(3)
        whrVal =[]
        for val in whereData.values():
            whrVal.append(val)
        sql = 'DELETE FROM ' + tableName + ' WHERE {}'.format('AND '.join('{}=%s '.format(u) for u in whereData))
        conn = None
        selected_rows = None
        result = []
        try:
            # read database configuration
            params = Config().config()
            # connect to the PostgreSQL database
            conn = psycopg2.connect(**params)
            # create a new cursor
            cur = conn.cursor()
            # execute the DELETE statement
            cur.execute(sql, whrVal)
            # get the deleted row counts
            result = ""
            # commit the changes to the database
            conn.commit()
            # close communication with the database
            cur.close()
        except (Exception, psycopg2.DatabaseError) as error:
            print(error)
            result = error
        finally:
            if conn is not None:
                conn.close()
        return 0

    def select_gadpu_query(self, sql):
        print("def gadpu select_query")

        nconfig = config.Config()
        conn = None
        selected_rows = None
        result = []
        try:
            # read database configuration
            params = nconfig.config()
            # connect to the PostgreSQL database
            conn = psycopg2.connect(**params)
            # create a new cursor
            cur = conn.cursor()
            # execute the UPDATE statement
            cur.execute(sql)
            # get the updated row counts
            result = cur.fetchall()
            # commit the changes to the database
            conn.commit()
            # close communication with the database
            cur.close()
        except (Exception, psycopg2.DatabaseError) as error:
            print(error)
            result = error
        finally:
            if conn is not None:
                conn.close()
        return result

    def select_query(self, sql):
        print("def select_query")
        nconfig=config.Config()
        conn = None
        selected_rows = None
        result = []
        try:
            # read database configuration
            params = nconfig.naps_config()
            # connect to the PostgreSQL database
            conn = psycopg2.connect(**params)
            # create a new cursor
            cur = conn.cursor()
            # execute the UPDATE statement
            cur.execute(sql)
            # get the updated row counts
            result = cur.fetchall()
            # commit the changes to the database
            conn.commit()
            # close communication with the database
            cur.close()
        except (Exception, psycopg2.DatabaseError) as error:
            print(error)
            result = error
        finally:
            if conn is not None:
                conn.close()
        return result

    def select_scangroup_query(self, sql):
        nconfig = config.Config()
        conn = None
        selected_rows = None
        result = []
        try:
            # read database configuration
            params = nconfig.naps_config()
            # connect to the PostgreSQL database
            conn = psycopg2.connect(**params)
            # create a new cursor
            cur = conn.cursor()
            # execute the UPDATE statement
            cur.execute(sql)
            # get the updated row counts
            result = cur.fetchone()
            # commit the changes to the database
            conn.commit()
            # close communication with the database
            cur.close()
        except (Exception, psycopg2.DatabaseError) as error:
            print(error)
            result = error
        finally:
            if conn is not None:
                conn.close()
        return result

    def get_all_observations_list(self):
        pass

    def is_gsb(self, observation_no):
        pass

    def is_ghb(self, observation_no):
        pass

    def is_dual_frequency(self, observation_no):
        pass

    def check_correlator_type(self):
        pass

