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

#define RAD2DEG(x)  ((x)*180.0/M_PI)
#define DEG2RAD(x)  ((x)/180.0*M_PI)

//static char rcsid[]="$Id: aveself.c,v 1.4 2005/11/22 10:27:05 das Exp $";

char *LtaFile=NULL;
char *BPFile=NULL;
int   SelScan,BPScan,Antenna=-1,Ignore=-1;
char  Usage[256];

int aveself(int argc, char **argv)
{ LtaInfo linfo,linfo1;
  FILE    *fp,*bfp; 
  float   x[MAX_CHANS],*iw,*bw;
  int     i,j,na;
  long    off;
  VisInfo *vinfo=&linfo.lhdr.vinfo;
  char    *dbuf,*dbuf1,*bpbuf;


  if(LtaFile==NULL) { fprintf(stderr,Usage); return 1;}
  if((fp=fopen(LtaFile,"r"))==NULL)
  { fprintf(stderr,"Cannot open %s\n",LtaFile); return 1;}
  
  { int t=1;
    if (*((unsigned char *)&t) == 1) linfo.local_byte_order = LittleEndian;
    else linfo.local_byte_order=BigEndian;
  }
  if(get_lta_hdr(fp,&linfo)) return 1;
  if(make_scantab(fp,&linfo))return 1;
  rewind(fp);
  if(SelScan<0 || SelScan >= linfo.scans)
  { fprintf(stderr,"Illegal SelScan [Legal Range 0 - %d]\n",linfo.scans-1);
    return 1;
  }

  if(BPFile!=NULL)
  { if((bfp=fopen(BPFile,"r"))==NULL)
    { fprintf(stderr,"Cannot open %s\n",BPFile); return 1;}
    if(get_lta_hdr(bfp,&linfo1)) return 1;
    if(make_scantab(bfp,&linfo1))return 1;
    linfo1.local_byte_order=linfo.local_byte_order;
    if(BPScan<0 || BPScan >= linfo1.scans)
    { fprintf(stderr,"Illegal BPScan [Legal Range 0 - %d]\n",linfo1.scans-1);
    return 1;
    }
    rewind(bfp);
    if((bpbuf=malloc(linfo1.recl))==NULL)
    { fprintf(stderr,"Malloc Error\n"); return -1;}
    ltaseek(bfp,linfo1.stab[BPScan].start_rec+linfo1.srecs,linfo1.recl);
    fread(bpbuf,1,linfo1.recl,bfp);
    if(linfo1.data_byte_order!=linfo1.local_byte_order)
      flipData(bpbuf,&linfo1.lhdr);
  }

  if((dbuf=malloc(linfo.recl))==NULL)
  { fprintf(stderr,"Malloc Error\n"); return -1;}
  if((dbuf1=malloc(linfo.recl))==NULL)
  { fprintf(stderr,"Malloc Error\n"); return -1;}

  ltaseek(fp,linfo.stab[SelScan].start_rec+linfo.srecs,linfo.recl);
  fread(dbuf,1,linfo.recl,fp);
  if(linfo.data_byte_order!=linfo.local_byte_order)
    flipData(dbuf,&linfo.lhdr);
  for(i=1;i<linfo.stab[SelScan].recs;i++)
  { fread(dbuf1,1,linfo.recl,fp);
    if(linfo.data_byte_order!=linfo.local_byte_order)
      flipData(dbuf1,&linfo.lhdr);
      do_average(dbuf,dbuf1,&linfo.lhdr);
  }
  for(i=0;i<vinfo->channels;i++)x[i]=0;
  na=0;
  for(j=0;j<vinfo->baselines;j++)
  { int a0=vinfo->base[j].ant[0],a1=vinfo->base[j].ant[1];
    if(a0 != a1) continue;
    if(Antenna>=0 && a0!=Antenna) continue;
    if(Ignore>=0 && a0 == Ignore) continue;
    na++;
    off=linfo.lhdr.data_off+2*sizeof(float)*vinfo->channels*j;
    iw=(float *)(dbuf+off);
    if(BPFile != NULL)
    { if(a0 != linfo1.lhdr.vinfo.base[j].ant[0])
      { fprintf(stderr,"Antenna number mismatch\n"); return -1;}
      bw=(float *)(bpbuf+off);
      for(i=0;i<vinfo->channels;i++){x[i]+=*iw/(*bw);iw+=2;bw+=2;}
    }else
      for(i=0;i<vinfo->channels;i++){x[i]+=*iw;iw+=2;bw+=2;}
  }
  for(i=0;i<vinfo->channels;i++)
    printf("%d %f\n",i,x[i]/na);

       
   return 0;
}

int main(int argc, char **argv)
{
  optrega(&LtaFile,OPT_STRING,'i',"in", "Input LTA File");
  optrega(&BPFile,OPT_STRING,'b',"bp", "BandPAss LTA File");
  optrega(&SelScan,OPT_INT,'s',"selscan","Scan to Show");
  optrega(&BPScan,OPT_INT,'S',"bpscan","Scan to bandpass");
  optrega(&Antenna,OPT_INT,'a',"antenna","Selected Antenna");
  optrega(&Ignore,OPT_INT,'d',"ignore","deSelected Antenna");

  sprintf(Usage,"Usage: %s -i LtaFile [-b BPLtaFile] [-s scan] [-S BPscan]\n[-a antenna][-d delectedantenna]\n",argv[0]);
  
  optUsage(Usage);
  
  optTitle("Averages Selected Self Correlations\n");
  optMain(aveself);
  opt(&argc,&argv);

  return aveself(argc,argv);
}



  
