#include <lta.h>
#include <newcorr.h>
#include <opt.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <unistd.h>
#include <slalib.h>

#define RAD2DEG(x)  ((x)*180.0/M_PI)
#define DEG2RAD(x)  ((x)/180.0*M_PI)

//static char rcsid[]="$Id: ltahdr.c,v 1.6 2005/12/02 10:00:15 das Exp $";

char *LtaFile=NULL;
int   Verbose;
char  Usage[256];
int   ShwAntCrd=0,ShwAllFrq=0,ShwSrcCrd=0,ShwBinStd=1,ShwAscStd=0;
int   ShwAscScn=0,ShwBinScn=0,ShwAscHdr=0,ShwBinHdr=0;

int ltahdr(int argc, char **argv)
{  FILE         *fp;
   LtaInfo      linfo;
   int          i;
   char         buf[DAS_HDRSIZE],date[24];
   char         ras[32],decs[32],ras1[32],decs1[32],lsts[32],object[12];
   CorrType     *corr=&linfo.corr;
   double       mjd,lst,el,eld,eldd,az,azd,azdd,pa,pad,padd,ha,g_lat,g_long;
   
   g_lat=DEG2RAD(19+5.0/60+26.35/3600); /* GMRT Latitude */
   g_long=DEG2RAD(74+2.0/60+59.9/3600); /* GMRT Longitude */

   if(LtaFile==NULL) { fprintf(stderr,Usage); return 1;}
   if((fp=fopen(LtaFile,"r"))==NULL)
   { fprintf(stderr,"Cannot open %s\n",LtaFile); return 1;}

   { int t=1;
     if (*((unsigned char *)&t) == 1) linfo.local_byte_order = LittleEndian;
     else linfo.local_byte_order=BigEndian;
   }
   if(get_lta_hdr(fp,&linfo)) return 1;
   if(make_scantab(fp,&linfo)) return 1;

   if(ShwBinHdr){corrgen(corr,buf);printf("%s",buf);}
   if(ShwAscHdr)print_lta_hdr(&linfo.lhdr);

   if(ShwAscStd||ShwBinStd)
   { printf("         VERSION: %s\n",linfo.lhdr.version);
     if(strstr(linfo.lhdr.version,"COR30x2") !=NULL || strstr(linfo.lhdr.version,"GWB-III") !=NULL)
     { float lta_version;
       sscanf(linfo.lhdr.version+11,"%f",&lta_version);
       if(lta_version > 1.00){
         char obs_mode[NAMELEN],host_name[NAMELEN];
         void get_obs_mode(CorrType *corr, char *obs_name);
         if(strstr(linfo.lhdr.version,"HST00")!=NULL)strcpy(host_name,"B");
         if(strstr(linfo.lhdr.version,"HST01")!=NULL)strcpy(host_name,"A");
         if(strstr(linfo.lhdr.version,"HST02")!=NULL)strcpy(host_name,"A+B");
         if(strstr(linfo.lhdr.version,"HST03")!=NULL)strcpy(host_name,"GSB");
         if(strstr(linfo.lhdr.version,"HST04")!=NULL)strcpy(host_name,"GWB");
         if(strstr(linfo.lhdr.version,"LSB1")!=NULL)strcpy(host_name,"OLD");
         get_obs_mode(corr,obs_mode);
	 printf("        OBS_MODE: %-16s",obs_mode);
	 printf("CORR_HOST: %-6s",host_name);
	 printf("BASELINES: %-6d",corr->daspar.baselines);
	 printf("CHANNELS: %-5d\n",corr->daspar.channels);
       }
     }
     printf("SCN OBJECT      RA(MEAN)     DEC(MEAN) " );
     printf("   DATE        IST       RF(MHz) CW(kHz)   Nrecs\n");
   }
   if(ShwAllFrq)
   { printf("SCN OBJECT      LO1(130)  LO1(175)");
     printf("  LO4(130) LO4(175)  RF(130)   RF(175)   CW   Nrecs\n");
     printf("                  MHz       MHz   ");
     printf("    MHz      MHz      MHz       MHz      kHz\n");
   }
   if(ShwSrcCrd)
   { printf("SCN OBJECT      RA(MEAN)     DEC(MEAN)   ");
     printf(" RA(APP)     DEC(APP)      DRA       DDEC      MJD_SRC\n");
   }
   if(ShwAntCrd)
   { printf("SCN OBJECT      DATE        IST   ");
     printf("    MJD       LST           HA   EL     AZ    PAR_A\n");
   }
   for(i=0;i<linfo.scans;i++)
   { ScanInfoType *scan=&linfo.stab[i].scan;
     ScanHdr      *shdr=&linfo.stab[i].shdr;
     SourceParType *src=&scan->source;
     if(ShwBinScn){printf("#Scan %d\n",i);print_scan(scan);}
     if(ShwAscScn){printf("#Scan %d\n",i);print_scan_hdr(shdr);}
//     strncpy(date,mjd2ist_date(scan->t/(24.0*3600.0)),23);
     strncpy(date,mjd2ist_date(corr->daspar.mjd_ref+scan->t/(24.0*3600.0)),23);
//     strncpy(date,mjd2ist_date(scan->t/(24.0*3600.0)),23);
//     fprintf(stdout, "===>>> MJD_REF=%f\n", corr->daspar.mjd_ref);
     strncpy(object,shdr->object,11);object[11]='\0';
     if(ShwBinStd)
     { convradec(RAD2DEG(scan->source.ra_mean),ras,0);
       convradec(RAD2DEG(scan->source.dec_mean),decs,1);
       printf("%3d %-11s %s %s %s %7.2f  %7.3f %4d\n",i,object,ras,decs,date,
	      scan->source.freq[0]/1.0e6,scan->source.ch_width/1.0e3,
	      linfo.stab[i].recs); 
     }
     if(ShwAscStd)
     { convradec(shdr->ra_date,ras,0);
       convradec(shdr->dec_date,decs,1);
       printf("%3d %-11s %s %s %s %7.2f  %7.3f %4d\n",i,object,ras,decs,
	      date, shdr->rf[0]/1.0e6,shdr->f_step/1.0e3,
	      linfo.stab[i].recs); 
     }
     if(ShwAllFrq)
     { printf("%3d %-11s %-9.3f %-9.3f %-7.3f  %-7.3f   %-9.3f %-9.3f %-6.2f %-4d\n",
	      i,object,src->first_lo[0]/1.0e6,src->first_lo[1]/1.0e6,
	      src->bb_lo[0]/1.0e6,src->bb_lo[1]/1.0e6,src->freq[0]/1.0e6,
	      src->freq[1]/1.0e6,src->ch_width/1.0e3,linfo.stab[i].recs);
     }
     if(ShwSrcCrd)
     { convradec(RAD2DEG(scan->source.ra_mean),ras,0);
       convradec(RAD2DEG(scan->source.dec_mean),decs,1);
       convradec(RAD2DEG(scan->source.ra_app),ras1,0);
       convradec(RAD2DEG(scan->source.dec_app),decs1,1);
       printf("%3d %-11s %s %s %s %s %9.3e %9.3e %10.3f\n",i,object,
	      ras,decs,ras1,decs1,src->dra,src->ddec,src->mjd0);
     }
     if(ShwAntCrd)
     { mjd=corr->daspar.mjd_ref+scan->t/(24.0*3600.0);
       lst=slaGmst(mjd)+g_long;
       ha=lst-src->ra_mean;
       slaAltaz(ha,src->dec_mean,g_lat,&az,&azd,&azdd,&el,&eld,&eldd,
		&pa,&pad,&padd);
       convradec(RAD2DEG(lst),lsts,0);
       strncpy(date,mjd2ist_date(mjd),23);
       printf("%3d %-11s %s %10.3f %s %5.1f %5.1f %6.1f %6.1f\n",i,object,date,
	      mjd,lsts,RAD2DEG(ha),RAD2DEG(el),RAD2DEG(az),RAD2DEG(pa));
     }
   }

   return 0;
}

int main(int argc, char **argv)
{
  optrega(&LtaFile,OPT_STRING,'i',"in", "Input LTA File");
  optrega(&ShwBinHdr,OPT_FLAG,'g',"global","Binary Global Hdr");
  optrega(&ShwAscHdr,OPT_FLAG,'G',"Global","Ascii  Global Hdr");
  optrega(&ShwBinScn,OPT_FLAG,'s',"scan",  "Binary Scan Hdr");
  optrega(&ShwAscScn,OPT_FLAG,'S',"Scan",  "Ascii  Scan Hdr");
  optrega(&ShwBinStd,OPT_FLAG,'b',"bin",   "Binary Std Info");
  optrega(&ShwAscStd,OPT_FLAG,'a',"ascii", "Ascii  Std Info");
  optrega(&ShwAllFrq,OPT_FLAG,'f',"freq",  "Freq   Settings");
  optrega(&ShwSrcCrd,OPT_FLAG,'c',"coord", "Source Coords");
  optrega(&ShwAntCrd,OPT_FLAG,'A',"antcrd","Antnna Coords");
  sprintf(Usage,"Usage: %s -i InputLtaFile [-a][-A][-b][-c][-f][-g][-G][-s][-S]\n",argv[0]);
  optUsage(Usage);
  optTitle("Prints Headers of LTA Files\n");
  optMain(ltahdr );
  opt(&argc,&argv);


  if(ShwBinHdr||ShwAscHdr||ShwBinScn||ShwAscScn||ShwAllFrq||
     ShwAntCrd||ShwSrcCrd) ShwAscStd=ShwBinStd=0;
  return ltahdr(argc,argv);
}



  
