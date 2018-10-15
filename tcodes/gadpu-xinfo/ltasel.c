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

//static char rcsid[]="$Id: ltasel.c,v 1.5 2007/03/21 12:34:15 das Exp $";

#define RAD2DEG(x)  ((x)*180.0/M_PI)
#define DEG2RAD(x)  ((x)/180.0*M_PI)

char *LtaFile=NULL,Usage[512];
int   StartChan,Channels=0,SelScans=0,*SelScan=NULL,AveChan[2],NewChan0=0;
int   *RecRange=NULL,RecLims=0;
int   SelfsOnly=0;

float MaxSize=-10.0;
int avechan(char *dbuf,LtaInfo *linfo)
{  float re,im,*v;
   int i,j,nc;
   LtaHdr *lhdr=&linfo->lhdr;
   VisInfo *vinfo=&lhdr->vinfo;
   int  channels =vinfo->channels,baselines=vinfo->baselines;


   if(AveChan[0]*AveChan[1]<=0)
   { AveChan[0] = channels/6; AveChan[1]=(channels*5)/6;}
   if((AveChan[0]>=AveChan[1]) || (AveChan[0]>=channels)|| (AveChan[1]>=channels))
   { fprintf(stderr,"Illegal Channel Average Range [%d - %d] (ValidRange== %d -%d)\n",
	     AveChan[0],AveChan[1],0,channels-1);
     return -1;
   }

   nc=AveChan[1]-AveChan[0]+1;
   for(j=0;j<baselines;j++)
   { v =(float *)(dbuf+lhdr->data_off);
     v += 2*j*channels;
     re=im=0.0;
     for(i=AveChan[0];i<=AveChan[1];i++)
     { re+=v[2*i]/nc;im+=v[2*i+1]/nc;}
     v[0]=re;v[1]=im;
   }
   return 0;
}
int next_file(LtaInfo *linfo, int inscan, ScanInfoType *scan, FILE *ofp)
{ static int ext=1;
  char   fname[80];
  
  sprintf(fname,"%s.%d","ltasel_out.lta",ext);
  fclose(ofp);
  if((ofp=fopen(fname,"w"))==NULL)
  { fprintf(stderr,"cannot open %s\n",fname); return -1;}
  write_hdr(linfo,ofp);
  write_scan(scan,1,linfo,0,linfo->stab[inscan].shdr.date_obs,ofp);
  printf("MaxFileSize reached -- opened new output file %s\n",fname);
  ext++;

  return 0;
} 
void chansel(char *dbuf,LtaInfo  *linfo, LtaInfo *linfo1, char *outbuf)
{ int i,j,k,chans,chans1,baselines;
  float *d,*d1;
  LtaHdr *lhdr=&linfo->lhdr,*lhdr1=&linfo1->lhdr;
  VisInfo *vinfo=&lhdr->vinfo,*vinfo1=&lhdr1->vinfo;

  chans =vinfo->channels;
  chans1=vinfo1->channels;
  baselines=vinfo->baselines;

  memcpy(outbuf+lhdr1->time_off,dbuf+lhdr->time_off,lhdr->time_size);
  memcpy(outbuf+lhdr1->wt_off,dbuf+lhdr->wt_off,lhdr->wt_size);

  for(i=0;i<baselines;i++)
  { d =(float *)(dbuf+lhdr->data_off+i*2*sizeof(float)*chans);
    d1=(float *)(outbuf+lhdr1->data_off+i*2*sizeof(float)*chans1);
    k =2*vinfo->chan_num[StartChan];
    for(j=0;j<2*chans1;j+=2,k+=2)
    { d1[j]=d[k];d1[j+1]=d[k+1];}
  }
}
int select_selfs(LtaInfo  *linfo, LtaInfo *linfo1, int *sel_base)
{ int          i,baselines;
  CorrType    *corr=&linfo->corr,*corr1=&linfo1->corr;
  BaseParType *base,*base1;

  base =corr->baseline;
  base1=corr1->baseline;
  for (baselines=0,i=0; i<corr->daspar.baselines; i++,base++)
  { int d0=base->samp[0].dpc, d1=base->samp[1].dpc ;

    if(d0 != d1) continue;
    if(d0==MAX_SAMPS)continue;
    memcpy(base1, base, sizeof(BaseParType)) ; 
    base1++; sel_base[baselines]=i;
    baselines++ ;
  }

  corr1->daspar.baselines = baselines ;
  return baselines;
}
void copy_selfs(char *inbuf, LtaInfo  *linfo, char *outbuf,LtaInfo *linfo1, int *sel_base,int selbases)
{ int   i;
  long  off,off1;
  CorrType *corr=&linfo->corr;

  memcpy(outbuf+linfo1->lhdr.time_off,inbuf+linfo->lhdr.time_off,linfo->lhdr.time_size);
  for(i=0;i<selbases;i++)
    { off  =linfo->lhdr.data_off+sel_base[i]*corr->daspar.channels*2*sizeof(float);
      off1 =linfo1->lhdr.data_off+ i*corr->daspar.channels*2*sizeof(float);
      memcpy(outbuf+off1,inbuf+off,corr->daspar.channels*2*sizeof(float));
  }

//  return 0;
}
	  
int ltasel(int argc, char **argv)
{  LtaInfo      linfo,linfo1,linfo2; 
   FILE         *fp,*ofp;
   char         outfile[256],object[16];
   char         *dbuf=NULL,*outbuf=NULL;
   int          i,j,k,change_endian,net_sign;
   int          nrec,maxrec,scanno,check_size;
   int          sel_base[MAX_BASE],selbases;

   strcpy(outfile,"ltasel_out.lta");
   if((ofp=fopen(outfile,"w"))==NULL)
   { fprintf(stderr,"cannot open %s\n",outfile); return 1;}


   if(LtaFile==NULL) {fprintf(stderr,"No Input file\n%s\n",Usage); return 1;}
   if((fp=fopen(LtaFile,"r"))==NULL)
   { fprintf(stderr,"Cannot open File %s\n%s",LtaFile,Usage); return 1;}


   { int t=1;
     if (*((unsigned char *)&t) == 1) linfo.local_byte_order = LittleEndian;
     else linfo.local_byte_order=BigEndian;
   }
   linfo1.local_byte_order=linfo.local_byte_order;
   if(get_lta_hdr(fp,&linfo)) return 1;
   if(make_scantab(fp,&linfo)) return 1;

   if(linfo.local_byte_order!=linfo.data_byte_order)change_endian=1;
   else change_endian=0;
	
   if((dbuf=malloc(linfo.recl))==NULL||(outbuf=malloc(linfo.recl))==NULL)
   {fprintf(stderr,"MallocError\n"); return 1;}

   /* remake the lta header */
   linfo1=linfo;
   if(Channels)
   { linfo1.corr.daspar.channels=Channels;
     for(i=0;i<Channels;i++) 
       linfo1.corr.daspar.chan_num[i]=linfo.corr.daspar.chan_num[StartChan+i]-
	                              linfo.corr.daspar.chan_num[StartChan];
   }
   update_hdr(&linfo1);
   linfo2=linfo1;
   if(SelfsOnly){selbases=select_selfs(&linfo1,&linfo2,sel_base);update_hdr(&linfo2);}
   write_hdr(&linfo2,ofp);
   nrec=linfo2.lrecs;

   if(MaxSize < -2.0) check_size=0;
   else
   { check_size=1;
     if(MaxSize < 0.0)
       maxrec=(2.0e9/linfo1.recl -1.0);
     else
       maxrec=(MaxSize*1.0e9/linfo1.recl -1.0);
   }

   for(scanno=0,i=0;i<linfo.scans;i++)
   { ScanInfoType *scan=&linfo.stab[i].scan,scan1;
     int chan0 =linfo2.corr.daspar.chan_num[0];
     SourceParType *source=&scan->source;
     if(SelScans)
     { for(j=0;j<SelScans;j++)if(i==SelScan[j]) break;
       if(j==SelScans) continue;
     }
     /* generate the new scan header */
     memcpy(&scan1,scan,sizeof(ScanInfoType));
     if(Channels)
     { net_sign=source->net_sign[0]; /* NEEDS GENERALIZATION ! */
       scan1.source.freq[0]=source->freq[0]+chan0*source->ch_width*net_sign;
       scan1.source.freq[1]=source->freq[1]+chan0*source->ch_width*net_sign;
     }
     write_scan(&scan1,1,&linfo2,scanno,linfo2.stab[i].shdr.date_obs,ofp);
     nrec+=linfo2.srecs;
     /* copy the selected parts of the data */
     rewind(fp); 
     ltaseek(fp,linfo.stab[i].start_rec+linfo.srecs,linfo.recl);
     for(j=0,k=0;j<linfo.stab[i].recs;j++)
     { if(RecLims && j < RecRange[0]) continue;
       if(RecLims && j > RecRange[1]) continue;
       fread(dbuf,linfo.recl,1,fp);
       if(change_endian)flipData(dbuf,&linfo.lhdr);
       if(SelScans||check_size||RecLims) update_signature(dbuf,scanno,k);
       memcpy(outbuf,dbuf,linfo.recl);
       if(NewChan0)avechan(outbuf,&linfo);
       if(Channels)chansel(dbuf,&linfo,&linfo1,outbuf);
       if(SelfsOnly)copy_selfs(outbuf,&linfo1,outbuf,&linfo2,sel_base,selbases);
       fwrite(outbuf,linfo2.recl,1,ofp);
       nrec++;k++;
       if(check_size && nrec==maxrec-1)
       { printf("%-12s IN_SCAN=%03d OUT_SCAN=%03d [%-3d records]\n",object,i,
		scanno,k);
         if(next_file(&linfo2,i,&scan1,ofp)) return -1;
         k=0;scanno=0;nrec=linfo2.lrecs+linfo2.srecs;
       }
     }
     strncpy(object,linfo.stab[i].shdr.object,11);object[11]='\0';
     if(SelScans||check_size)
       printf("%-12s IN_SCAN=%03d OUT_SCAN=%03d [%-3d records]\n",object,i,
	      scanno,k);
     else
       printf("SCAN %03d: %-12s [%-3d records]\n",i,object,linfo.stab[i].recs);
     scanno++;
   }

   free(dbuf);
   free(outbuf);
   fclose(ofp);
   return 0;
}

int main(int argc, char **argv)
{ int n,m;
  optrega(&LtaFile,   OPT_STRING,'i',"infile",   "Input LTA File");
  optrega(&StartChan, OPT_INT,   'o',"ochan",    "Output Starting Channel");
  optrega(&Channels,  OPT_INT,   'n',"nchans",   "Number of Channels in Output");
  optrega(&MaxSize,   OPT_FLOAT, 'm',"maxsize",  "Max output FileSize (GB)");
  optrega(&NewChan0,  OPT_FLAG,  'a',"chan0",    "Make a new Channel 0");
  optrega(&AveChan[0],OPT_INT,   's',"schan0",   "Start Channel for Average");
  optrega(&AveChan[1],OPT_INT,   'e',"echan0",   "End Channel for Average");
  optrega(&SelfsOnly, OPT_INT,   'A',"auto",     "Copies AutoCorrelations only");
  n=optrega_array(&SelScans,&SelScan,OPT_INT,'S', "scans","Scans to Copy");
  m=optrega_array(&RecLims,&RecRange,OPT_INT,'R', "recs", "Record range to copy");


  opthelp(&StartChan,"channo in the LtaFile not absolute Corr channo\n");
  opthelp(&NewChan0,"Replaces Input Channel 0 not Output Channel 0\n");
  opthelp(&AveChan[0],"Relative to Input Channel Numbers\n");
  opthelp(&AveChan[1],"Relative to Input Channel Numbers\n");
  opthelp_n(n,"comma separated list of scan numbers\n");
  opthelp_n(m,"comma separated start and stop record number\n");
  opthelp(&MaxSize,"if set to -1 it takes the default of 2.0GB");

  sprintf(Usage,"Usage: %s -i InputLtaFile -o OutStartChan -n OutNumChans",argv[0]);
  sprintf(Usage+strlen(Usage)," -S ScansToCopy -m MaxFileSize (GB)");
  sprintf(Usage+strlen(Usage)," -a AveChans -s StartAveChan -e EndAveChan\n");
  optUsage(Usage);
 
  optTitle("Selects subsets of a given LTA file\n");

  AveChan[0]=AveChan[1]=-1;
  optMain(ltasel);
  opt(&argc,&argv);

  return ltasel(argc,argv);
}



  
