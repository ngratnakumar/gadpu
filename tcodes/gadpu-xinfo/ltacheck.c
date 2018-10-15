#include <lta.h>
#include <sol.h>
#include <newcorr.h>
#include <opt.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/types.h>
#include <regex.h>
#include <slalib.h>

#define RAD2DEG(x)  ((x)*180.0/M_PI)
#define DEG2RAD(x)  ((x)/180.0*M_PI)
#define RAD2HR(x)   ((x)*12.0/M_PI)

enum{SelfType,CrossType};

char  *LtaFile=NULL,Usage[1024];
float RmsCut=3.0,BadFrac=0.9;
int   ChangeEndian=0;
float PhaseStabilityThreshold=0.9;
int   BPStartChan=-1,BPEndChan=-1;
Complex cmplxZtmp;
float   floatZtmp;
int    DeadSamps[MAX_SAMPS];
int    RedunCheck=0,ClosurePhaseCheck=1;
int   SelScans=0,*SelScan=NULL;

int is_self(BaselineType *base,int i)
{ BaselineType *b=base+i;
  if(b->ant[0]==b->ant[1] && b->band[0]==b->band[1]) return 1;
  else return 0;
}
int group_baselines(LtaInfo *linfo,int **bgrp)
{ VisInfo *vinfo=&linfo->lhdr.vinfo;
  int i,j,k,ngrp=0;
  
  for(i=0;i<vinfo->baselines;i++)
  { BaselineType *b1=&vinfo->base[i];
    for(j=0;j<ngrp;j++)
    { BaselineType *b0=&vinfo->base[bgrp[j][0]];
      if(b1->ant[0]  == b0->ant[0]  &&  b1->ant[1]  == b0->ant[1] && 
	 b1->band[0] == b0->band[0] &&  b1->band[1] == b0->band[1])
      { for(k=0;k<4;k++)if(bgrp[j][k]==MAX_BASE)break;
	if(k==4){fprintf(stderr,"Hardware baseline duplication limit crossed!\n"); return -1;}
	bgrp[j][k]=i;
	break;
      }
    }
    if(j==ngrp){ bgrp[j][0]=i;ngrp++;}
  }

  return ngrp;
}
int identify_triangles(LtaInfo *linfo,short ***triangles)
{ short   i,j;
  VisInfo *vinfo=&linfo->lhdr.vinfo; 
  int     baselines=vinfo->baselines; 
  short  *rev_num=vinfo->samp_rev_num;
  int     s0,s1,s2,b1,b2;
  for(i=0;i<baselines;i++)
  { BaselineType *base=&vinfo->base[i];
    s0=rev_num[base->samp[0]];s1=rev_num[base->samp[1]];
    if(s0==s1)continue;
    for(j=0,s2=0;s2<vinfo->samplers;s2++)
    { if(s2==s0 || s2==s1) continue;
      if((b1=vinfo->x_base_num[s1][s2])==MAX_BASE)continue;
      if((b2=vinfo->x_base_num[s2][s0])==MAX_BASE)continue;
      triangles[i][j][0]=b1; triangles[i][j][1]=b2;
      j++;
    }
  }
  return 0;
}
int get_digital_noise(int type,char *dbuf,LtaInfo *linfo,float *dnoise,int **bgrp,int groups)
{ int c,g,i,j,k;
  int channels=linfo->lhdr.vinfo.channels;
  float *data=(float*)(dbuf+linfo->lhdr.data_off);
  float  x[MAX_BASE*3]; /* 2 * 6 * MAX_BASE/4 */
  BaselineType *base=linfo->lhdr.vinfo.base;
  for(c=0;c<channels;c++)
  { for(k=0,g=0;g<groups;g++)
    { for(i=0;i<4 && bgrp[g][i]!=MAX_BASE;i++)
      { int b0=bgrp[g][i];
        float *d0=data+2*(b0*channels+c);
        if(type == CrossType && is_self(base,bgrp[g][i])) continue;
        if(type == SelfType  && !is_self(base,bgrp[g][i])) continue;
	for(j=i+1;j<4 && bgrp[g][j] !=MAX_BASE;j++)
	{ int b1=bgrp[g][j];
          float *d1=data+2*(b1*channels+c);
	  x[k]=(*d0-*d1); d0++;d1++; x[k+1]=(*d0-*d1);
	  if(x[k]<0) x[k]=-x[k];if(x[k+1]<0) x[k+1]=-x[k+1];
	  k+=2;
	}
      }
    }
    /* sort(k,x-1);
      dnoise[c]=0.5*(x[k/2]+x[k/2-1]); k guaranteed even 
    */
    dnoise[c]=nr_select(k/2,k,x-1);
  }

  return 0;
}
int find_bad_baselines(int type,char *dbuf,LtaInfo *linfo,float *dnoise,int **bgrp,int groups,int *bflag,
		       float rms_cut)
{ int c,g,i,j,bad,bad_chan;
  int channels=linfo->lhdr.vinfo.channels;
  float *data=(float*)(dbuf+linfo->lhdr.data_off);
  float re,im;
  BaselineType *base=linfo->lhdr.vinfo.base;

  for(g=0;g<groups;g++)
  { for(i=0;i<4 && bgrp[g][i]!=MAX_BASE;i++)
    { int b0=bgrp[g][i];
      if(type == CrossType && is_self(base,bgrp[g][i])) continue;
      if(type == SelfType  && !is_self(base,bgrp[g][i])) continue;
      for(bad_chan=0,c=0;c<channels;c++)
      { float *d0=data+2*(b0*channels+c);
        for(bad=0,j=0;j<4 && bgrp[g][j] !=MAX_BASE;j++)
	{ if(j==i) continue;
          else{ 
	    int b1=bgrp[g][j];
            float *d1=data+2*(b1*channels+c);
	    re=(*d0-*d1); d0++;d1++; im=(*d0-*d1);
	    if(re<0) re=-re;if(im<0) im=-im;
	    if(re>rms_cut*dnoise[c]||im>rms_cut*dnoise[c])bad++;
	  }
	}
	if(bad>2) bad_chan++;
      }
      bflag[bgrp[g][i]]+=bad_chan;
    }
  }

  return 0;
}
int find_dead_antennas(LtaInfo *linfo, int scan, FILE *fp, int integ,int *dead_samps)
{ char    *dbuf;
  float   *data;
  int     i,j,k;
  VisInfo *vinfo= &linfo->lhdr.vinfo;
  int     chan,nrecs=linfo->stab[scan].recs;
  SolParType solpar;
  
  if((dbuf=(malloc(linfo->recl*2*MAX_INTEG)))==NULL)
  {fprintf(stderr,"malloc failure\n"); return 1;}
  if((data=(float*)(malloc(vinfo->baselines*2*MAX_INTEG*2*sizeof(float))))==NULL)
  {fprintf(stderr,"malloc failure\n"); return 1;}
  
  init_solpar(&solpar,linfo);
  j=0;k=0;
  while(j<=nrecs)
  { char *buf=dbuf+k*linfo->recl; 
    if((j>0 && j%integ==0 && j <(nrecs/integ)*integ)||(j==nrecs))/* last block extends to scan end */
    { for(chan=0;chan<vinfo->channels;chan++)
      { get_one_chan(dbuf,data,chan,k,linfo);
        find_bad_samplers_chan(linfo,data,chan,k,&solpar);
	for(i=0;i<vinfo->samplers;i++)dead_samps[i] +=solpar.samp_chan_flag[chan][i];
      }
      if(j==nrecs) break; /* done with this scan */
      k=0; buf=dbuf; /* else process the next integ recs */
    }
    fread(buf,linfo->recl,1,fp);
    if(ChangeEndian)flipData(buf,&linfo->lhdr);
    j++;k++;
  }
  for(i=0;i<vinfo->samplers;i++)
    if(dead_samps[i]/(1.0*(nrecs/integ)*vinfo->channels) > 0.5) dead_samps[i]=1;
    else dead_samps[i]=0;
  return 0;
}
float get_closure_phase_rms(char *dbuf,LtaInfo *linfo,float *cp_rms,short ***triangles)
{ int           b0,b1,b2,sgn1,sgn2,j,k,c;
  VisInfo      *vinfo=&linfo->lhdr.vinfo;
  int           baselines=vinfo->baselines,samplers=vinfo->samplers;
   int          channels=vinfo->channels;
  BaselineType *base=vinfo->base;
  float        *d0,*d1,*d2,re0,im0,re1,im1,re2,im2,p0,p1,p2,re,im,p;
  float        *data=(float*)(dbuf+linfo->lhdr.data_off);
  float         re_mean[MAX_BASE][MAX_CHANS],im_mean[MAX_BASE][MAX_CHANS],cp_var[MAX_BASE][MAX_CHANS];
  float         r[MAX_BASE],x[MAX_BASE],y[MAX_BASE],z_re[MAX_CHANS],z_im[MAX_CHANS];
    

  for(b0=0;b0<baselines;b0++)
  { int s0=vinfo->samp_rev_num[base[b0].samp[0]];
    int s1=vinfo->samp_rev_num[base[b0].samp[1]];
    if(is_self(base,b0)) continue;
    if(DeadSamps[s0]||DeadSamps[s1]) continue;
    for(c=0;c<channels;c++)
    { re_mean[b0][c]=im_mean[b0][c]=cp_var[b0][c]=0.0;
      for(k=0,j=0;j<samplers;j++)
      { if((b1=triangles[b0][j][0])==MAX_BASE) break;
        if((b2=triangles[b0][j][1])==MAX_BASE) break;
	sgn1=b1>0? 1:-1;sgn2=b2>0? 1:-1;
	if(DeadSamps[base[sgn1*b1].samp[1]]) continue;
        { d0=data+2*(b0*channels+c);      re0=*d0;im0=*(d0+1);p0=atan2(im0,re0);
  	  d1=data+2*(sgn1*b1*channels+c); re1=*d1;im1=*(d1+1);p1=atan2(im1,re1);
	  d2=data+2*(sgn2*b2*channels+c); re2=*d2;im2=*(d2+1);p2=atan2(im2,re2);
	  p=p0+sgn1*p1+sgn2*p2;re=cos(p);im=sin(p);
	  x[k] =re; y[k]=im; 
	  r[k] =2*(1.0-re); /* assuming zero mean closure phase */
	  k++;
	}
      }
      re_mean[b0][c] = nr_select(k/2,k,x-1); 
      im_mean[b0][c] = nr_select(k/2,k,y-1);
      cp_var[b0][c]  = nr_select(k/2,k,r-1);
    }
  }
  
  for(c=0;c<channels;c++)  /* mean closure phase */
  { for(k=0,b0=0;b0<baselines;b0++)
    { int s0=vinfo->samp_rev_num[base[b0].samp[0]];
      int s1=vinfo->samp_rev_num[base[b0].samp[1]];
      if(is_self(base,b0)) continue;
      if(DeadSamps[s0]||DeadSamps[s1]) continue;
      x[b0] = re_mean[b0][c];
      y[b0] = im_mean[b0][c];
      k++;
    }
    z_re[c] = nr_select(k/2,k,x-1); z_im[c]=nr_select(k/2,k,y-1);
  }
  re = nr_select(channels/2,channels,z_re-1); im=nr_select(channels/2,channels,z_im-1);

  for(c=0;c<channels;c++) /* mean closure rms */
  { for(k=0,b0=0;b0<baselines;b0++)
    { int s0=vinfo->samp_rev_num[base[b0].samp[0]];
      int s1=vinfo->samp_rev_num[base[b0].samp[1]];
      if(is_self(base,b0)) continue;
      if(DeadSamps[s0]||DeadSamps[s1]) continue;
      x[b0] = cp_var[b0][c];
      k++;
    }
    cp_rms[c] = nr_select(k/2,k,x-1); /* Closure phase rms per channel */
  }
  return acos(re/sqrt(re*re+im*im)); /* typical closure phase */
}
float find_bad_closure_phase(char *dbuf,LtaInfo *linfo,float *cp_rms,short ***triangles, short **cpflag, float rms_cut)
{ int           b0,b1,b2,sgn1,sgn2,j,k,c;
  VisInfo      *vinfo=&linfo->lhdr.vinfo;
  int           baselines=vinfo->baselines,samplers=vinfo->samplers;
   int          channels=vinfo->channels;
  BaselineType *base=vinfo->base;
  float        *d0,*d1,*d2,re0,im0,re1,im1,re2,im2,p0,p1,p2,re,im,p;
  float        *data=(float*)(dbuf+linfo->lhdr.data_off);
  float         x[MAX_BASE],y[MAX_BASE];
    

  for(b0=0;b0<baselines;b0++)
  { int s0=vinfo->samp_rev_num[base[b0].samp[0]];
    int s1=vinfo->samp_rev_num[base[b0].samp[1]];
    int s2;
    if(is_self(base,b0)) continue;
    if(DeadSamps[s0]||DeadSamps[s1]) continue;
    for(c=0;c<channels;c++)
    { for(k=0,j=0;j<samplers;j++)
      { if((b1=triangles[b0][j][0])==MAX_BASE) break;
        if((b2=triangles[b0][j][1])==MAX_BASE) break;
	sgn1=b1>0? 1:-1;sgn2=b2>0? 1:-1;
	s2=vinfo->samp_rev_num[base[sgn1*b1].samp[1]];
	if(DeadSamps[s2]) continue; /* other 2 vertices already checked */
        { d0=data+2*(b0*channels+c);      re0=*d0;im0=*(d0+1);p0=atan2(im0,re0);
  	  d1=data+2*(sgn1*b1*channels+c); re1=*d1;im1=*(d1+1);p1=atan2(im1,re1);
	  d2=data+2*(sgn2*b2*channels+c); re2=*d2;im2=*(d2+1);p2=atan2(im2,re2);
	  p=p0+sgn1*p1+sgn2*p2;re=cos(p);im=sin(p);
	  x[k] =re; y[k]=im; 
	  k++;
	}
      }
      re = nr_select(k/2,k,x-1); im = nr_select(k/2,k,y-1); p=atan2(im,re);
      if(2.0*(1-cos(p))>rms_cut*rms_cut*cp_rms[c])cpflag[b0][c]++;
    }
  }
  
  return 0;
}
int ltacheck(int argc, char **argv)
{  LtaInfo      linfo;
   FILE         *fp;
   off_t        recl,baselines;
   char         *dbuf;
   int          i,j,k,change_endian,*bgrp[MAX_BASE],groups,bflag[MAX_BASE];
   short        *cpflag[MAX_BASE];
   float        dx_noise[MAX_CHANS],ds_noise[MAX_CHANS],badfrac;
   float        cp_mean,cp_rms[MAX_CHANS];
   short        **triangles[MAX_BASE];
   int          integ=8;
   char         object[16];

   if(LtaFile==NULL) { fprintf(stderr,Usage); return 1;}
   if((fp=fopen(LtaFile,"r"))==NULL)
   { fprintf(stderr,"Cannot open %s\n",LtaFile); return 1;}

   { int t=1;
     if (*((unsigned char *)&t) == 1) linfo.local_byte_order = LittleEndian;
     else linfo.local_byte_order=BigEndian;
   }
   if(get_lta_hdr(fp,&linfo)) return 1;
   if(make_scantab(fp,&linfo)) return 1;
   if(linfo.data_byte_order!=linfo.local_byte_order)change_endian=1;
   else change_endian=0;

   recl=linfo.recl;
   baselines=linfo.lhdr.vinfo.baselines;
   if((dbuf=malloc(recl))==NULL)
   {fprintf(stderr,"MallocError\n"); return 1;}

   for(i=0;i<MAX_BASE;i++)
   { if((bgrp[i]=(int*)malloc(4*sizeof(int)))==NULL)
     { fprintf(stderr,"Mallor error\n"); return 1;}
     if((cpflag[i]=(short*)malloc(MAX_CHANS*sizeof(short)))==NULL)
     { fprintf(stderr,"Mallor error\n"); return 1;}
     if((triangles[i]=(short**)malloc(MAX_SAMPS*sizeof(short**)))==NULL)
     { fprintf(stderr,"Mallor error\n"); return 1;}
     for(j=0;j<MAX_SAMPS;j++)
       if((triangles[i][j]=(short*)malloc(2*sizeof(short)))==NULL)
       { fprintf(stderr,"Mallor error\n"); return 1;}
   }
       
   for(i=0;i<MAX_BASE;i++)
     for(j=0;j<4;j++)bgrp[i][j]=MAX_BASE;
   if((groups=group_baselines(&linfo,bgrp))<0) return 1;
   /* for(i=0;i<groups;i++)
       printf("group %d : %d %d %d %d\n",i,bgrp[i][0],bgrp[i][1],bgrp[i][2],bgrp[i][3]);
   */

   for(i=0;i<MAX_BASE;i++)
     for(j=0;j<MAX_SAMPS;j++){triangles[i][j][0]=triangles[i][j][1]=MAX_BASE;}
   identify_triangles(&linfo,triangles);
   /* for(i=0;i<baselines;i++)
      for(j=0;j<MAX_SAMPS;j++)
      { int b0,b1;
        if((b0=triangles[i][j][0])==MAX_BASE)break;
        if((b1=triangles[i][j][1])==MAX_BASE)break;
	printf("%d %d %d\n",i,b0,b1);
	}
   */


   for(i=0;i<linfo.scans;i++)
   { int recs=linfo.stab[i].recs;
     int baselines=linfo.lhdr.vinfo.baselines;
     int channels=linfo.lhdr.vinfo.channels;
     { if(SelScans)
       { for(j=0;j<SelScans;j++)if(i==SelScan[j]) break;
	 if(j==SelScans) continue;
       }
     }
     strncpy(object,linfo.stab[i].shdr.object,11);object[11]='\0';
     fprintf(stderr,"SCAN %03d %-12s\n",i,object);
     for(j=0;j<linfo.lhdr.vinfo.samplers;j++)DeadSamps[j]=0;
     rewind(fp);ltaseek(fp,linfo.stab[i].start_rec+linfo.srecs,linfo.recl);
     find_dead_antennas(&linfo,i,fp,integ,DeadSamps);
     printf("Dead Samplers:\n");
     for(j=0;j<linfo.lhdr.vinfo.samplers;j++)
     { SampInfo *sinfo=&linfo.lhdr.vinfo.samp[linfo.lhdr.vinfo.samp_num[j]];
       if(DeadSamps[j])
	 printf("smp=%3d fft=%3d (%s %s)\n",linfo.lhdr.vinfo.samp_num[j],sinfo->fft_id,sinfo->ant,sinfo->band);
     }
     printf("\n");
     rewind(fp); 
     ltaseek(fp,linfo.stab[i].start_rec+linfo.srecs,linfo.recl);
     for(j=0;j<MAX_BASE;j++){for(k=0;k<channels;k++)cpflag[j][k]=0;bflag[j]=0;}
     for(j=0;j<recs;j++)
     { if(fread(dbuf,recl,1,fp) !=1) return 0;
       if(j==0)
       { if(RedunCheck)
	 { get_digital_noise(SelfType,dbuf,&linfo,ds_noise,bgrp,groups);
	   get_digital_noise(CrossType,dbuf,&linfo,dx_noise,bgrp,groups);
	 }
         if(ClosurePhaseCheck)
	 { cp_mean=get_closure_phase_rms(dbuf,&linfo,cp_rms,triangles);
   	   printf("Mean Closure Phase = %5.2f\n",RAD2DEG(cp_mean));
	 }
	 /*for(k=0;k<channels;k++)printf("Chan %d: Closure rms %5.1f\n",k,RAD2DEG(acos(1.0-cp_rms[k]/2)));*/
       }
       if(RedunCheck)
       { find_bad_baselines(SelfType,dbuf,&linfo,ds_noise,bgrp,groups,bflag,RmsCut);
         find_bad_baselines(CrossType,dbuf,&linfo,dx_noise,bgrp,groups,bflag,RmsCut);
       }
       if(ClosurePhaseCheck)
	 find_bad_closure_phase(dbuf,&linfo,cp_rms,triangles,cpflag,RmsCut);
     }
     if(RedunCheck)
     { printf("Wrong Baseline list:\n");
       for(k=0,j=0;j<baselines;j++)
       { if((badfrac=bflag[j]/(1.0*channels*recs))>BadFrac) 
         { BaseParType  *bpar=&linfo.corr.baseline[j];
           BaselineType *base=&linfo.lhdr.vinfo.base[j];
	   printf("base= %4d s0= %3d s1= %3d f0= %3d f1= %3d card= %2d chip= %2d (%s %s %s %s ) %5.1f%% data bad\n",
		j,base->samp[0],base->samp[1],bpar->samp[0].fft_id,bpar->samp[1].fft_id,
		bpar->card,bpar->chip,base->antname[0],base->bandname[0],base->antname[1],
		base->bandname[1],badfrac*100);
	   k++;
	 }
       }
       printf("%d/%d baselines have > %.0f%% wrong data \n",k,baselines,100*BadFrac);
     }
     if(ClosurePhaseCheck)
     { int c,isbad;
       printf("Large Closure Phase List:\n");
       for(k=0,j=0;j<baselines;j++)
       { for(isbad=0,c=1;c<channels;c++)
	 if((badfrac=cpflag[j][c]/(1.0*recs))>BadFrac) isbad++;
         if(isbad)
	 { BaseParType  *bpar=&linfo.corr.baseline[j];
           BaselineType *base=&linfo.lhdr.vinfo.base[j];
	   int l;
	   printf("base= %4d s0= %3d s1= %3d f0= %3d f1= %3d card= %2d chip= %2d (%s %s %s %s )\nChans[%3d/%3d]: ",
		  j,base->samp[0],base->samp[1],bpar->samp[0].fft_id,bpar->samp[1].fft_id,
		  bpar->card,bpar->chip,base->antname[0],base->bandname[0],base->antname[1],
		  base->bandname[1],isbad,channels-1);
	   for(l=0,c=1;c<channels;c++)
	   { if((badfrac=cpflag[j][c]/(1.0*recs))>BadFrac)
	     { printf("%3d ",c);l++;
	       if(l==15){printf("\n                ");l=0;}
	       k++;
	     }
	   }
	   printf("\n");
	 }
       }
       printf("%d/%d baselines have > %.0f%% data with closure phase errors \n",k,channels*baselines,100*BadFrac);
     }
   }

   return 0;
}

int main(int argc, char **argv)
{

  optrega(&LtaFile,     OPT_STRING,'i',"in",     "Input LTA File");
  optrega(&RmsCut,      OPT_FLOAT, 'c',"cut",    "Threshold for flagging (sigma)");
  optrega(&BadFrac,     OPT_FLOAT, 'b',"bad",    "Minimim bad data fraction");
  optrega(&RedunCheck,  OPT_INT,   'r',"red",    "Do redundant data checks (copy modes only)");
  optrega(&ClosurePhaseCheck,  OPT_INT,   'C',"phase",    "Do closurephase checks");
  optrega_array(&SelScans,&SelScan,OPT_INT,'S', "scans","Scans to solve");

  optMain(ltacheck);
  opt(&argc,&argv);

  optUsage(Usage);
  
  optTitle("Do data quality checks");

  return ltacheck(argc,argv);
}



  
