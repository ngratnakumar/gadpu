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

#define MAX_TIME_ERROR 1.0e-2

int time_align(char *dbuf, char *dbuf1, LtaInfo *linfo, LtaInfo *linfo1)
{  CorrType *corr=&linfo->corr;
   double lta=corr->daspar.lta*corr->corrpar.statime/1.0e6;
   double tm,tm1; 
   int    off;

   memcpy(&tm,dbuf+linfo->lhdr.time_off,linfo->lhdr.time_size);
   memcpy(&tm1,dbuf1+linfo1->lhdr.time_off,linfo1->lhdr.time_size);

   off=rint((tm-tm1)/lta);
   if(fabs(tm1-tm +off*lta) > MAX_TIME_ERROR) 
     fprintf(stderr,"WARNING: LTA and LTB time offset [%.3f] == non integer record lenght\n",tm-tm1);
   return off;
}
int ltamerge(int argc, char **argv)
{  FILE         *fp,*fp1,*ofp;
   LtaInfo      linfo,linfo1,linfo2;
   int          i,j,k,recl;
   int          data_off,data_off1,data_off2,data_size,data_size1,data_size2;
   CorrType     *corr=&linfo.corr,*corr1=&linfo1.corr,*corr2=&linfo2.corr;
   char         *dbuf,*dbuf1,*outbuf,mode[64],mode1[64];
   CorrParType  *corrpar=&corr->corrpar,*corrpar1=&corr1->corrpar;
   int           get_dpc_mode_name(unsigned char mode, char *name);

   if(LtaFile==NULL) { fprintf(stderr,Usage); return 1;}
   if((fp=fopen(LtaFile,"r"))==NULL)
   { fprintf(stderr,"Cannot open %s\n",LtaFile); return 1;}
   if(LtaFile1==NULL) { fprintf(stderr,Usage); return 1;}
   if((fp1=fopen(LtaFile1,"r"))==NULL)
   { fprintf(stderr,"Cannot open %s\n",LtaFile1); return 1;}
   if((ofp=fopen("ltamerge_out.lta","w"))==NULL)
   { fprintf(stderr,"Cannot open output file\n"); return 1;}

   { int t=1;
     if (*((unsigned char *)&t) == 1) linfo.local_byte_order = LittleEndian;
     else linfo.local_byte_order=BigEndian;
   }
   linfo1.local_byte_order=linfo.local_byte_order;

   if(get_lta_hdr(fp,&linfo)) return 1;
   if(make_scantab(fp,&linfo)) return 1;
   if(get_lta_hdr(fp1,&linfo1)) return 1;
   if(make_scantab(fp1,&linfo1)) return 1;

   if(!strstr(linfo.lhdr.version,"HST00") || !strstr(linfo1.lhdr.version,"HST01"))
   { fprintf(stderr,"LTB file must be specified with -i switch and LTA file with -I\n");
     return -1;
   }
   if(linfo.recl!=linfo1.recl)
   { fprintf(stderr,"files %s %s have different record lengths!\n",LtaFile,LtaFile1);return 1;}
   recl=linfo.recl;

   if(cmp_global_hdr(&linfo,&linfo1,"ac")) 
   { fprintf(stderr,"Cannot Merge: Global header Mismatch!\n"); return 1;}
   memcpy(&linfo2,&linfo,sizeof(LtaInfo));
   if(linfo.scans!=linfo1.scans)
     fprintf(stderr,"WARNING: files %s %s have different number of scans!\n",LtaFile,LtaFile1);
   linfo2.scans=(linfo.scans>linfo1.scans)?linfo1.scans:linfo.scans;
   if((linfo2.stab=(ScanHdrTab*)malloc(sizeof(ScanHdrTab)*linfo2.scans))==NULL)
   {fprintf(stderr,"Malloc error!\n"); return -1;}
   get_dpc_mode_name(corrpar->dpcmux,mode);   
   get_dpc_mode_name(corrpar1->dpcmux,mode1);
   if(strcmp(mode,mode1))
   {fprintf(stderr,"LTA/LTB files have different [%s %s] correlator modes!\n",mode1,mode);return 1;}
   if(!strcmp(mode,"IndianPolar"))
   { if(cmp_global_hdr(&linfo,&linfo1,"cpS")) return 1; /* same num baselines,samplers &same channels*/
     corr2->daspar.samplers=corr->daspar.samplers+corr1->daspar.samplers;
     for(i=corr->daspar.samplers,j=0;i<corr2->daspar.samplers;i++,j++)
	memcpy(&corr2->sampler[i],&corr1->sampler[j],sizeof(SamplerType));
   }else{
     if(cmp_global_hdr(&linfo,&linfo1,"cps")) return 1; /* same num baselines &same channels,samplers*/
     /* corr2 is already a copy of corr -- nothing more to do */
   }
   corr2->daspar.baselines=corr->daspar.baselines+corr1->daspar.baselines;
   for(i=corr->daspar.baselines,j=0;i<corr2->daspar.baselines;i++,j++)
     memcpy(&corr2->baseline[i],&corr1->baseline[j],sizeof(BaseParType));

   linfo2.keep_version=0;
   sprintf(linfo2.lhdr.version+24,"HST02 ");
   strcpy(linfo2.lhdr.vinfo.corrlite.version,linfo2.lhdr.version);
   strcpy(linfo2.corr.version,linfo2.lhdr.version);
   update_hdr(&linfo2);
   write_hdr(&linfo2,ofp); /* write global header */
//   fprintf(stdout, "CORR VERSION = %s\n", linfo2.corr.version); 
   if( (dbuf=malloc(recl))==NULL || (dbuf1=malloc(recl))==NULL || (outbuf=malloc(linfo2.recl))==NULL )
   {fprintf(stderr,"MallocError\n"); return 1;}


   data_off =linfo.lhdr.data_off;    data_size =linfo.lhdr.data_size;
   data_off1=linfo1.lhdr.data_off;   data_size1=linfo1.lhdr.data_size;
   data_off2=linfo2.lhdr.data_off;   data_size2=linfo2.lhdr.data_size;

   for(i=0;i<linfo2.scans;i++)
   { ScanInfoType *scan2=&linfo2.stab[i].scan;
     int rec=0,rec1=0,rec2=0,nskip;
     rewind(fp);rewind(fp1);  /* go to start of data in this scan */
     ltaseek(fp,linfo.stab[i].start_rec+linfo.srecs,linfo.recl);
     ltaseek(fp1,linfo1.stab[i].start_rec+linfo1.srecs,linfo1.recl);
     memcpy(&linfo2.stab[i],&linfo.stab[i],sizeof(ScanHdrTab));
     if(linfo1.stab[i].recs !=linfo.stab[i].recs)
       fprintf(stderr,"WARNING:SCAN %d has different num of recs in LTB and LTA (%d,%d)!\n",
	       i,linfo.stab[i].recs,linfo1.stab[i].recs);
     linfo2.stab[i].recs=(linfo1.stab[i].recs > linfo.stab[i].recs)?linfo.stab[i].recs:linfo1.stab[i].recs;
     write_scan(scan2,1,&linfo2,i,linfo2.stab[i].shdr.date_obs,ofp);
     for(k=0;k<linfo2.stab[i].recs;k++)
     { if(rec<linfo.stab[i].recs)fread(dbuf,linfo.recl,1,fp);
       else{fprintf(stderr,"Illegal record [%d] seek (maxrec=%d)\n",rec,linfo.stab[i].recs); return 1;}
       if(rec1<linfo1.stab[i].recs)fread(dbuf1,linfo1.recl,1,fp1);
       else{fprintf(stderr,"Illegal record1 [%d] seek (maxrec=%d)\n",rec1,linfo1.stab[i].recs); return 1;}
       nskip=time_align(dbuf,dbuf1,&linfo,&linfo1);
       if(nskip<0)
       { nskip=-nskip;
         fprintf(stderr,"LTB input record %d: skip %d records\n",rec,nskip); 
	 rec+=nskip; 
	 if(rec>linfo.stab[i].recs) {fprintf(stderr,"End of Scan %d reached\n",i); break;}
	 fseek(fp,linfo.recl*(nskip-1),SEEK_CUR); 
	 fseek(fp1,-linfo1.recl,SEEK_CUR); 
	 continue;
       }
       else
       { if(nskip>0)
	 { fprintf(stderr,"LTA input record %d: skip %d records\n",rec1,nskip); 
	   rec1+=nskip; continue;
	   if(rec1>linfo1.stab[i].recs) {fprintf(stderr,"End of Scan %d reached\n",i); break;}
	   fseek(fp1,linfo1.recl*(nskip-1),SEEK_CUR); 
	   fseek(fp,-linfo.recl,SEEK_CUR); 
	 }
       }
       update_signature(outbuf,i,rec2); /* record signature, scannum=i recnum=rec2  */
       memcpy(outbuf+linfo2.lhdr.time_off,dbuf+linfo.lhdr.time_off,linfo.lhdr.time_size);
       memcpy(outbuf+linfo2.lhdr.wt_off,dbuf+linfo.lhdr.wt_off,linfo.lhdr.wt_size);
       /* have still to copy flags and pars */
       memcpy(outbuf+data_off2,dbuf+data_off,data_size);
       memcpy(outbuf+data_off2+data_size,dbuf1+data_off1,data_size1);
       fwrite(outbuf,linfo2.recl,1,ofp);
       rec++;rec1++;rec2++;
     }
   }
//   fprintf(stdout, "%s\n%s\n", linfo2.lhdr.version, linfo2.corr.version);

   free(dbuf); free(dbuf1);free(outbuf);

   return 0;
}

int main(int argc, char **argv)
{
  optrega(&LtaFile,OPT_STRING,'i',"in", "Input LTB File");
  optrega(&LtaFile1,OPT_STRING,'I',"in1", "Input LTA File");

  sprintf(Usage,"Usage: %s -i InputLtbFile -I InputLtaFile\n",argv[0]);
  optUsage(Usage);
  optTitle("Merge lta and ltb files of a given observation\n");
  optMain(ltamerge );
  opt(&argc,&argv);

  return ltamerge(argc,argv);
}



  
