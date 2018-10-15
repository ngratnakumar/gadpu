#include <lta.h>
#include <newcorr.h>
#include <diffstop.h>
#include <opt.h>

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <unistd.h>

#define RAD2DEG(x)  ((x)*180.0/M_PI)
#define DEG2RAD(x)  ((x)/180.0*M_PI)

//static char rcsid[]="$Id: diffstop.c,v 1.5 2009/02/16 07:32:54 das Exp $";

static char *LtaFile=NULL,*AntFile=NULL,Usage[256];
static double TimeError=0.0;
double StaTime; /* unused -- but required to compile with astro_cal.c */

void calModelPar(double tm, CorrType *corr, ModelParType *mpar,
                 SourceParType *source, int *antmask);


int  diff_fstop(char *dbuf,LtaHdr *lhdr,ScanInfoType *scan,CorrType *corr,CorrType *corr1)
{ double       tm,dt,re,im,cp,sp,dp,dd,df,dp0;
  ModelParType mpar[MAX_SAMPS],mpar1[MAX_SAMPS];
  float        *d_in,*d_out;
  int          i,j,antmask[MAX_BANDS];

  StaTime=corr->corrpar.statime;
  dt=corr->daspar.lta*StaTime/1.0e6;
  if((d_out=(float *)malloc(corr->daspar.channels*2*sizeof(float)))==NULL)
  { fprintf(stderr,"diff_fstop: malloc error\n");return -1;}
  memcpy(&tm,dbuf+lhdr->time_off,sizeof(double));
  tm+=dt/2.0; /* middle of lta */
  for(i=0;i<MAX_BANDS;i++)
  { if((1<<i)&corr->daspar.bandmask)antmask[i]=corr->daspar.antmask;
    else antmask[i]=0;
  }
  calModelPar(tm,corr,mpar,&scan->source, antmask);
  calModelPar(tm,corr1,mpar1,&scan->source, antmask);
 
  for(i=0;i<corr->daspar.baselines;i++)
  { int s0=corr->baseline[i].samp[0].dpc ;
    int s1=corr->baseline[i].samp[1].dpc ;
    long off=lhdr->data_off+i*corr->daspar.channels*2*sizeof(float);
    int  k0,k1;
    for(k0=0;k0<corr->daspar.samplers;k0++)
      if(corr->sampler[k0].dpc==s0)break;
    for(k1=0;k1<corr->daspar.samplers;k1++)
      if(corr->sampler[k1].dpc==s1)break;
    if(k0==corr->daspar.samplers||k1==corr->daspar.samplers)
    { fprintf(stderr,"Cannot match sampler sequence for baseline %d\n",i); return -1;}
    if((j=corr->baseline[i].samp[0].band/2) !=corr->baseline[i].samp[1].band/2)
    {fprintf(stderr,"Cannot correct USBxLSB!\n"); return -1;}
    else df=scan->source.ch_width*scan->source.net_sign[j];

    dp0  = (mpar[k0].phase-mpar[k1].phase)-(mpar1[k0].phase-mpar1[k1].phase);
    dd   = (mpar[k0].delay-mpar[k1].delay)-(mpar1[k0].delay-mpar1[k1].delay);
    dp0 *= scan->source.net_sign[j];

    d_in=(float *)(dbuf+off);
    for(j=0;j<corr->daspar.channels;j++)
    { dp = dp0+2.0*M_PI*j*df*dd;
      cp=cos(dp); sp=sin(dp);
      re=*d_in;im=*(d_in+1);
      d_out[2*j]=re*cp-im*sp;
      d_out[2*j+1]=re*sp+im*cp;
      d_in+=2;
    }
    memcpy(dbuf+off,d_out,corr->daspar.channels*2*sizeof(float));
  }
  free(d_out);
  return 0;
}
int diffstop(int argc, char **argv)
{  LtaInfo      linfo,linfo1;
   FILE         *fp,*ofp,*antfile;
   char         outfile[256],object[16];
   char         *dbuf=NULL;
   int          i,j,recl,change_endian;


   strcpy(outfile,"diffstop_out.lta");
   if((ofp=fopen(outfile,"w"))==NULL)
   { fprintf(stderr,"cannot open %s\n",outfile); return 1;}

   { int t=1;
     if (*((unsigned char *)&t) == 1) linfo.local_byte_order = LittleEndian;
     else linfo.local_byte_order=BigEndian;
   }
   
   if(LtaFile==NULL) {fprintf(stderr,"No Input file\n%s\n",Usage); return 1;}
   if((fp=fopen(LtaFile,"r"))==NULL)
   { fprintf(stderr,"Cannot open File %s\n%s",LtaFile,Usage); return 1;}
   if(get_lta_hdr(fp,&linfo))
   { fprintf(stderr,"Error reading %s\n",LtaFile); return 1;}
   if(linfo.local_byte_order!=linfo.data_byte_order)change_endian=1;
   else change_endian=0;
   if(make_scantab(fp,&linfo))
   { fprintf(stderr,"Error reading %s\n",LtaFile); return 1;}

   /* update parameters */
   linfo1=linfo;
   linfo1.corr.daspar.mjd_ref += TimeError/86400.0;

   if(AntFile!=NULL) {
   	if((antfile=fopen(AntFile,"r"))==NULL)
   	{ fprintf(stderr,"Cannot open File %s\n%s",AntFile,Usage); return 1;}
			get_antenna(antfile, linfo1.corr.antenna);
		}

   recl=linfo.recl;
   if((dbuf=malloc(recl))==NULL)
   {fprintf(stderr,"MallocError\n"); return 1;}

   write_hdr(&linfo1,ofp); /* generate the LTA Header */
   
   for(i=0;i<linfo.scans;i++)
   { ScanInfoType *scan=&linfo.stab[i].scan;
     char         *date_obs=linfo.stab[i].shdr.date_obs;
     ScanHdr      *shdr=&linfo.stab[i].shdr;
     write_scan(scan,1,&linfo1,i,date_obs,ofp);
     strncpy(object,shdr->object,11);object[11]='\0';
     fprintf(stderr,"Working on scan %-3d [%-12s] ",i,object);
     rewind(fp);ltaseek(fp,linfo.stab[i].start_rec+linfo.srecs,recl);
     for(j=0;j<linfo.stab[i].recs;j++)
     { fread(dbuf,recl,1,fp);
       if(change_endian)flipData(dbuf,&linfo.lhdr);
       diff_fstop(dbuf,&linfo.lhdr,scan,&linfo.corr,&linfo1.corr);
       fwrite(dbuf,recl,1,ofp);
     }
     fprintf(stderr,"%4d records corrected\n",linfo.stab[i].recs);

   }

   free(dbuf);
   fclose(ofp);
   return 0;
}

int main(int argc, char **argv)
{
  optrega(&LtaFile,OPT_STRING,'i',"in", "Input LTA File");
  optrega(&AntFile,OPT_STRING,'a',"ant", "Input antsys File");
  optrega(&TimeError,OPT_DOUBLE,'t',"TimeError","TimeError (secods)");

  sprintf(Usage,"Usage: %s -i InputLtaFile -t TimerError (s) -a antsys.file\n",argv[0]);
  optUsage(Usage);
  optTitle("Does differential fringestop\n");

  optMain(diffstop);
  opt(&argc,&argv);

  
  return diffstop(argc,argv);
}



  
