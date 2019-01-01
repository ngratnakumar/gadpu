import glob
import os

import getScangroup as gs

def copying(src, dst):
    try:
        print('cp -r '+src+'/* '+dst+'/')
        os.system('cp -r '+src+'/* '+dst+'/')
    except Exception as ex:
        print(ex)

def cycle20():
    cycle_area = '/GARUDATA/IMAGING20/IMAGES/'

    precalib_uvfits_path = '/GARUDATA/backup/partitions/FITS/data/SPLIT/'
    process_target_fits_path = glob.glob('/GARUDATA/IMAGING20/IMAGES/*')

    uvfits_list = []
    for each_uvfits_dir in precalib_uvfits_path:
        uvfits_list.append(each_uvfits_dir.split('/')[-1])

    uvfits_list.sort()
    uvfits_list = list(set(uvfits_list))

    fits_list = []
    for each_uvfits_dir in process_target_fits_path:
        fits_list.append(each_uvfits_dir.split('/')[-1])

    fits_list.sort()
    fits_list = list(set(fits_list))


    sorted_list=[]
    cycle_20_path = '/GARUDATA/IMAGING20/CYCLE20/'
    for each_dir in fits_list:
        # print('============' + each_dir+'============')
        dir_path = cycle_20_path+each_dir
        proj_code = each_dir
        fname = proj_code
        if not proj_code.isdigit():
            proj_code = dir_path.split('/')[-1]
            fname = proj_code
        else:
            print("--- "+each_dir)
            # print(glob.glob(cycle_area+each_dir+'/*'))
            link_loc = glob.glob("/GARUDATA/backup/partitions/FITS/data/SPLIT_LINK/"+each_dir+'/*')
            for each_file in link_loc:
                # print(each_file)
                if(os.path.islink(each_file)):
                    lta_file = os.readlink(each_file)
                    proj_code = lta_file.split('/')[4]
                    fname = proj_code

        proj = proj_code.replace('_lta','.lta')
        sorted_list.append(dir_path)
        nproj = proj.replace('lta_','lta.')
        proj = proj_code
        print(nproj)
        lta_data = gs.get_naps_scangroup_details(nproj)
        dir_name = nproj.upper().split('.')[0]
        observation_no = lta_data['observation_no']
        print(lta_data, dir_name, dir_path)

        #FOR PRECALIB
        new_precalib_path = cycle_20_path+str(observation_no)+'/'+dir_name+'/PRECALIB/'
        if not os.path.exists(new_precalib_path):
            os.makedirs(new_precalib_path)
            # print(new_precalib_path)
        else:
            print('PRECALIB -- ' +new_precalib_path)
            # print('EXISTS: '+new_precalib_path)
            # print(cycle_area+each_dir)
            # print(glob.glob(precalib_uvfits_path+each_dir+'/*'))
        copying(precalib_uvfits_path+fname, new_precalib_path)


        # FOR FITS_IMAGE

        new_fits_path = cycle_20_path+str(observation_no)+'/'+dir_name+'/FITS_IMAGE/'

        # # print(new_fits_path)
        if not os.path.exists(new_fits_path):
            os.makedirs(new_fits_path)
        else:
            print('FITS_IMAGE -- ' + new_fits_path)
        #     print('Exists: '+new_fits_path)
        #     print(cycle_area+each_dir)
        #     print(glob.glob(cycle_area+each_dir+'/*'))
        copying(cycle_area+each_dir, new_fits_path)
        print('----------------'+each_dir+'---------------')


def cycle21():
    cycle_area = '/GARUDATA/IMAGING21/IMAGES/'

    precalib_uvfits_path = '/GARUDATA/FITS/CYCLE21/SPLIT_CYCLE_21_2/'
    process_target_fits_path = glob.glob('/GARUDATA/IMAGING21/IMAGES/*')

    uvfits_list = []
    for each_uvfits_dir in precalib_uvfits_path:
        uvfits_list.append(each_uvfits_dir.split('/')[-1])

    uvfits_list.sort()
    uvfits_list = list(set(uvfits_list))

    fits_list = []
    for each_uvfits_dir in process_target_fits_path:
        fits_list.append(each_uvfits_dir.split('/')[-1])

    fits_list.sort()
    fits_list = list(set(fits_list))

    sorted_list = []
    cycle_20_path = '/GARUDATA/IMAGING21/CYCLE21/'
    for each_dir in fits_list:
        # print('============' + each_dir+'============')
        dir_path = cycle_20_path + each_dir
        proj_code = each_dir
        fname = proj_code
        if not proj_code.isdigit():
            proj_code = dir_path.split('/')[-1]
            fname = proj_code
        else:
            print("--- " + each_dir)
            # print(glob.glob(cycle_area+each_dir+'/*'))
            link_loc = glob.glob("/GARUDATA/FITS/CYCLE21/SPLIT_LINK_CYCLE_21/" + each_dir + '/*')
            for each_file in link_loc:
                # print(each_file)
                if (os.path.islink(each_file)):
                    lta_file = os.readlink(each_file)
                    print("-------->>> "+lta_file)
                    proj_code = lta_file.split('/')[6]
                    fname = proj_code
        print(dir_path)
        proj = proj_code.replace('_lta', '.lta')
        sorted_list.append(dir_path)
        nproj = proj.replace('lta_', 'lta.')
        proj = proj_code
        print(nproj)
        lta_data = gs.get_naps_scangroup_details(nproj)
        dir_name = nproj.upper().split('.')[0]
        observation_no = lta_data['observation_no']
        print(lta_data, dir_name, dir_path)

        # FOR PRECALIB
        new_precalib_path = cycle_20_path + str(observation_no) + '/' + dir_name + '/PRECALIB/'
        if not os.path.exists(new_precalib_path):
            os.makedirs(new_precalib_path)
            # print(new_precalib_path)
        else:
            print('PRECALIB -- ' + new_precalib_path)
            # print('EXISTS: '+new_precalib_path)
            # print(cycle_area+each_dir)
            # print(glob.glob(precalib_uvfits_path+each_dir+'/*'))
        copying(precalib_uvfits_path + fname, new_precalib_path)

        # FOR FITS_IMAGE

        new_fits_path = cycle_20_path + str(observation_no) + '/' + dir_name + '/FITS_IMAGE/'

        # # print(new_fits_path)
        if not os.path.exists(new_fits_path):
            os.makedirs(new_fits_path)
        else:
            print('FITS_IMAGE -- ' + new_fits_path)
        #     print('Exists: '+new_fits_path)
        #     print(cycle_area+each_dir)
        #     print(glob.glob(cycle_area+each_dir+'/*'))
        copying(cycle_area + each_dir, new_fits_path)
        print('----------------' + each_dir + '---------------')


def cycle22():
    cycle_area = '/GARUDATA/IMAGING22/IMAGES/'

    precalib_uvfits_path = '/GARUDATA/FITS/CYCLE22/SPLIT_CYCLE_22/'
    process_target_fits_path = glob.glob('/GARUDATA/IMAGING22/IMAGES/*')

    uvfits_list = []
    for each_uvfits_dir in precalib_uvfits_path:
        uvfits_list.append(each_uvfits_dir.split('/')[-1])

    uvfits_list.sort()
    uvfits_list = list(set(uvfits_list))

    fits_list = []
    for each_uvfits_dir in process_target_fits_path:
        fits_list.append(each_uvfits_dir.split('/')[-1])

    fits_list.sort()
    fits_list = list(set(fits_list))

    sorted_list = []
    cycle_20_path = '/GARUDATA/IMAGING22/CYCLE22/'
    for each_dir in fits_list:
        # print('============' + each_dir+'============')
        dir_path = cycle_20_path + each_dir
        proj_code = each_dir
        fname = proj_code
        if not proj_code.isdigit():
            proj_code = dir_path.split('/')[-1]
            fname = proj_code
        else:
            print("--- " + each_dir)
            # print(glob.glob(cycle_area+each_dir+'/*'))
            link_loc = glob.glob("/GARUDATA/FITS/CYCLE22/SPLIT_LINK_CYCLE_22/" + each_dir + '/*')
            for each_file in link_loc:
                print(each_file)
                if (os.path.islink(each_file)):
                    lta_file = os.readlink(each_file)
                    print("-------->>> "+lta_file)
                    proj_code = lta_file.split('/')[4]
                    fname = proj_code
        print(dir_path)
        proj = proj_code.replace('_lta', '.lta')
        sorted_list.append(dir_path)
        nproj = proj.replace('lta_', 'lta.')
        proj = proj_code
        print(nproj)
        lta_data = gs.get_naps_scangroup_details(nproj)
        dir_name = nproj.upper().split('.')[0]
        observation_no = lta_data['observation_no']
        print(lta_data, dir_name, dir_path)

        # FOR PRECALIB
        new_precalib_path = cycle_20_path + str(observation_no) + '/' + dir_name + '/PRECALIB/'
        if not os.path.exists(new_precalib_path):
            os.makedirs(new_precalib_path)
            # print(new_precalib_path)
        else:
            print('PRECALIB -- ' + new_precalib_path)
            # print('EXISTS: '+new_precalib_path)
            # print(cycle_area+each_dir)
            # print(glob.glob(precalib_uvfits_path+each_dir+'/*'))
        copying(precalib_uvfits_path + fname, new_precalib_path)

        # FOR FITS_IMAGE

        new_fits_path = cycle_20_path + str(observation_no) + '/' + dir_name + '/FITS_IMAGE/'

        # # print(new_fits_path)
        if not os.path.exists(new_fits_path):
            os.makedirs(new_fits_path)
        else:
            print('FITS_IMAGE -- ' + new_fits_path)
        #     print('Exists: '+new_fits_path)
        #     print(cycle_area+each_dir)
        #     print(glob.glob(cycle_area+each_dir+'/*'))
        copying(cycle_area + each_dir, new_fits_path)
        print('----------------' + each_dir + '---------------')


def cycle23():
    cycle_area = '/GARUDATA/IMAGING23/CYCLE_23_IMAGES/'

    precalib_uvfits_path = '/GARUDATA/backup/partitions/FITS/data/SPLIT_CYCLE_23/'
    process_target_fits_path = glob.glob('/GARUDATA/IMAGING23/CYCLE_23_IMAGES/*')

    uvfits_list = []
    for each_uvfits_dir in precalib_uvfits_path:
        uvfits_list.append(each_uvfits_dir.split('/')[-1])

    uvfits_list.sort()
    uvfits_list = list(set(uvfits_list))

    fits_list = []
    for each_uvfits_dir in process_target_fits_path:
        fits_list.append(each_uvfits_dir.split('/')[-1])

    fits_list.sort()
    fits_list = list(set(fits_list))

    sorted_list = []
    cycle_20_path = '/GARUDATA/IMAGING23/CYCLE23/'
    for each_dir in fits_list:
        # print('============' + each_dir+'============')
        dir_path = cycle_20_path + each_dir
        proj_code = each_dir
        fname = proj_code
        if not proj_code.isdigit():
            proj_code = dir_path.split('/')[-1]
            fname = proj_code
        else:
            print("--- " + each_dir)
            # print(glob.glob(cycle_area+each_dir+'/*'))
            link_loc = glob.glob("/GARUDATA/backup/partitions/FITS/data/SPLIT_LINK_CYCLE_23/" + each_dir + '/*')
            for each_file in link_loc:

                print("********"+each_file)
                if (os.path.islink(each_file)):
                    lta_file = os.readlink(each_file)
                    print("-------->>> "+lta_file)
                    proj_code = lta_file.split('/')[4]
                    fname = proj_code
        print("=============="+dir_path)
        proj = proj_code.replace('_lta', '.lta')
        sorted_list.append(dir_path)
        nproj = proj.replace('lta_', 'lta.')
        proj = proj_code
        print(nproj)
        print("***********")
        if 'comb' in nproj:
            nproj = nproj.replace('comb_', '')
        lta_data = gs.get_naps_scangroup_details(nproj)
        dir_name = nproj.upper().split('.')[0]
        observation_no = lta_data['observation_no']
        print(lta_data, dir_name, dir_path)

        # FOR PRECALIB
        new_precalib_path = cycle_20_path + str(observation_no) + '/' + dir_name + '/PRECALIB/'
        if not os.path.exists(new_precalib_path):
            os.makedirs(new_precalib_path)
            # print(new_precalib_path)
        else:
            print('PRECALIB -- ' + new_precalib_path)
            # print('EXISTS: '+new_precalib_path)
            # print(cycle_area+each_dir)
            # print(glob.glob(precalib_uvfits_path+each_dir+'/*'))
        copying(precalib_uvfits_path + fname, new_precalib_path)

        # FOR FITS_IMAGE

        new_fits_path = cycle_20_path + str(observation_no) + '/' + dir_name + '/FITS_IMAGE/'

        # # print(new_fits_path)
        if not os.path.exists(new_fits_path):
            os.makedirs(new_fits_path)
        else:
            print('FITS_IMAGE -- ' + new_fits_path)
        #     print('Exists: '+new_fits_path)
        #     print(cycle_area+each_dir)
        #     print(glob.glob(cycle_area+each_dir+'/*'))
        copying(cycle_area + each_dir, new_fits_path)
        # print('----------------' + each_dir + '---------------')

def cycle24():
    cycle_area = '/GARUDATA/IMAGING24/CYCLE24/'
    # images_path = cycle_area+'*/FITS_IMAGE/*PBCOR*FITS'
    pbcors = glob.glob(images_path)
    for each_pbcor in pbcors:
        dir_path = os.path.dirname(each_pbcor)
        pbcor_file = os.path.basename(each_pbcor)
        source = each_pbcor.split('/')[-1].split('.')[0]
        summary_file = glob.glob(dir_path+'/spam_'+source+'*.summary')
        freq = summary_file[0].split('/')[-1].split('_')[2]
        new_pbcor_file = pbcor_file.replace(source, source+'.'+freq)
        # print(dir_path, summary_file[0].split('/')[-1], source, freq)
        print(dir_path+'/'+pbcor_file, dir_path+'/'+new_pbcor_file)
        # os.system('mv '+dir_path+'/'+pbcor_file+' '+dir_path+'/'+new_pbcor_file)
        print('mv ' + dir_path + '/' + pbcor_file + ' ' + dir_path + '/' + new_pbcor_file)

if __name__ == '__main__':
    # cycle20()
    # cycle21()
    # cycle22()
    # cycle23()
    cycle24()
