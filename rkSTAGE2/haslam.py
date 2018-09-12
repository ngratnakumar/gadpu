import subprocess

UVFITS_DATA = '/GARUDATA/IMAGING24/CYCLE24/'
PRECAL_PROCESSING = 'precalibration.processing'
PRECAL_SUCCESS = 'precalibration.success'
PRECAL_FAILED = 'precalibration.failed'


def check_haslam_flag(project):
    '''
    # Definition: check_haslam_flag() - NO ARGS
    # Returns: list of obslog file paths
    # Description: This function checks for the all obslog files and 
    gets the is ALC set to OFF.
        If th ALC at 13. in obslog if FLAGGED as OFF, then we set apply_tsys to 
    spam.pre_calibrate_targets(apply_tsys=False), apply_tsys is True by default
    # Code: spam.pre_calibrate_targets(UVFITS_FILE_NAME, apply_tsys=False)
    -----------------------------------------------------------------------------------
    $fgrep '13.' /GARUDATA/IMAGING24/CYCLE24/*/*.obslog | grep 'OFF' | cut -d ':' -f 1 
    -----------------------------------------------------------------------------------
    # Reference: /export/spam/python/spam/gmrt.py:846,42

    '''
    cmd = "fgrep '13.' " + UVFITS_DATA + "*/*.obslog | grep 'OFF' | cut -d ':' -f 1"
    output = subprocess.check_output(cmd, shell=True)
    obs_list = output.split('\n')
    obs_list.remove('')
    for is_haslam_flagged in obs_list:
        return any(project in string for string in obs_list)


proj = "24_047_5AUG13"
print check_haslam_flag(proj)
# for is_haslam_flagged in check_haslam_flag():
#     if project in is_haslam_flagged:
#         print project
#     else:
#         print "NA"
