from astropy.io import fits

pbcor_name = '/GARUDATA/IMAGING20/CYCLE20/5503/20_085_12AUG2011_8S/FITS_IMAGE/3C236S.SP2B.PBCOR_20_085_12aug2011_8s_lta.FITS'
pbcor_name = '/GARUDATA/IMAGING21/CYCLE21/5612/21_031_06NOV2011/FITS_IMAGE/JP11NOV06.SP2B.PBCOR_21_031_06nov2011_lta.FITS'
pbcor_name = '/GARUDATA/IMAGING22/CYCLE22/5997/22_056_22JUN2012/FITS_IMAGE/EN1DEEP05.SP2B.PBCOR_208.FITS'
pbcor_name = '/GARUDATA/IMAGING23/CYCLE23/6399/23_059_27FEB2013_16S/FITS_IMAGE/J0826+2637.SP2B.PBCOR_23_059_27feb2013_16s_lta.FITS'
pbcor_name = '/GARUDATA/IMAGING24/CYCLE24/24_028_15SEP13/FITS_IMAGE/J2217+5733.GMRT325.SP2B.PBCOR.FITS'

fits_table = fits.open(pbcor_name)
fits_header = fits_table[0].header

data_keys = []

history = str(fits_header["HISTORY"]).strip().split(' ')
nh = [x for x in history if x]

bmin, bmaj, bpa = 0, 0, 0
for x in nh:
    if 'BMAJ' in x:
        bmaj = float(nh[nh.index(x)+1].strip().replace('\n',' ').split(' ')[0])
    if 'BMIN' in x:
        bmin = float(nh[nh.index(x)+1].strip().replace('\n',' ').split(' ')[0])
    if 'BPA' in x:
        bpa = float(nh[nh.index(x)+1].strip().replace('\n',' ').split(' ')[0])
print(bmin, bmaj, bpa)
