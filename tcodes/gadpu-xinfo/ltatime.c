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

char *LtaFile=NULL,*LtaFile1=NULL;
char  Usage[256];
int   DeltaTime=0;

int ltatime(int argc, char **argv)
{  FILE         *fp,*fp1;
   LtaInfo      linfo,linfo1;
   int          i,k,recl,scans,recs;
   CorrType     *corr=&linfo.corr,*corr1=&linfo1.corr;
   char         date[24],date1[24],*dbuf;
   double       t0=0;

   if(LtaFile==NULL) { fprintf(stderr,Usage); return 1;}
   if((fp=fopen(LtaFile,"r"))==NULL)
   { fprintf(stderr,"Cannot open %s\n",LtaFile); return 1;}
   if(LtaFile1==NULL) { fprintf(stderr,Usage); return 1;}
   if((fp1=fopen(LtaFile1,"r"))==NULL)
   { fprintf(stderr,"Cannot open %s\n",LtaFile); return 1;}

   { int t=1;
     if (*((unsigned char *)&t) == 1) linfo.local_byte_order = LittleEndian;
     else linfo.local_byte_order=BigEndian;
   }
   linfo1.local_byte_order=linfo.local_byte_order;

   if(get_lta_hdr(fp,&linfo)) return 1;
   if(make_scantab(fp,&linfo)) return 1;
   if(get_lta_hdr(fp1,&linfo1)) return 1;
   if(make_scantab(fp1,&linfo1)) return 1;

   if(linfo.recl!=linfo1.recl)
   { fprintf(stderr,"files %s %s have different record lengths!\n",LtaFile,LtaFile1);return 1;}
   recl=linfo.recl;

   if(linfo.scans!=linfo1.scans)
     fprintf(stderr,"WARNING: files %s %s have different number of scans!\n",LtaFile,LtaFile1);
   scans=(linfo.scans>linfo1.scans)?linfo1.scans:linfo.scans;

   if((dbuf=malloc(recl))==NULL)
   {fprintf(stderr,"MallocError\n"); return 1;}
   
   for(i=0;i<scans;i++)
   { ScanInfoType *scan=&linfo.stab[i].scan;
     ScanInfoType *scan1=&linfo1.stab[i].scan;

     strncpy(date,mjd2ist_date(corr->daspar.mjd_ref+scan->t/(24.0*3600.0)),23);
     strncpy(date1,mjd2ist_date(corr1->daspar.mjd_ref+scan1->t/(24.0*3600.0)),23);
     printf("SCAN %04d START %s START1 %s [OFFSET= %12.4e (sec)]\n",i,date,date1,corr->daspar.mjd_ref+scan1->t
	    -corr1->daspar.mjd_ref-scan1->t);
     rewind(fp);rewind(fp1);  /* go to start of data in this scan */
     ltaseek(fp,linfo.stab[i].start_rec+linfo.srecs,linfo.recl);
     ltaseek(fp1,linfo1.stab[i].start_rec+linfo1.srecs,linfo1.recl);
     if(linfo1.stab[i].recs !=linfo.stab[i].recs)
       fprintf(stderr,"WARNING:SCAN %d has different num of recs in LTA and LTB!\n",i);
     recs=linfo1.stab[i].recs > linfo.stab[i].recs?linfo.stab[i].recs:linfo1.stab[i].recs;
     for(k=0;k<recs;k++)
     {  double t,t1;
	fread(dbuf,linfo.recl,1,fp);
	memcpy(&t,dbuf+linfo.lhdr.time_off,linfo.lhdr.time_size);
	fread(dbuf,linfo1.recl,1,fp1);
	memcpy(&t1,dbuf+linfo1.lhdr.time_off,linfo1.lhdr.time_size);
	strncpy(date,mjd2ist_date(corr->daspar.mjd_ref+t/(24.0*3600.0)),23);
	strncpy(date1,mjd2ist_date(corr1->daspar.mjd_ref+t1/(24.0*3600.0)),23);
	if(!DeltaTime)
	  printf("REC %6d T0=%s T1=%s OFFSET= %12.4e (sec)\n",i*1000+k,date,date1,t-t1);
	else
	  printf("REC %6d T0=%s T1=%s DELTA_T= %12.4e (sec)\n",i*1000+k,date,date1,t-t0);
	t0=t;
     }
   }
   free(dbuf);

   return 0;
}

int main(int argc, char **argv)
{
  optrega(&LtaFile,OPT_STRING,'i',"in", "Input LTA File");
  optrega(&LtaFile1,OPT_STRING,'I',"in1", "Input LTB File");
  optrega(&DeltaTime,OPT_FLAG,'d',"delta","print interval between records");

  sprintf(Usage,"Usage: %s -i InputLtaFile -I InputLtbFile [-d print RecordInterval]\n",argv[0]);
  optUsage(Usage);
  optTitle("Compare LTA/LTB time offsets\n");
  optMain(ltatime );
  opt(&argc,&argv);

  return ltatime(argc,argv);
}



  
