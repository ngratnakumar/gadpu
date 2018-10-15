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
#include <cpgplot.h>

#define RAD2DEG(x)  ((x)*180.0/M_PI)
#define DEG2RAD(x)  ((x)/180.0*M_PI)

enum {MAX_CALS=32};

char *LtaFile=NULL,*ScanFile=NULL,*CalFile=NULL;
char  Usage[256];
int   PlotAll=0,WriteCal=0,AveAnt=0,AveSpc=0,UseRecs=0;
float MaxRms=2.0;

typedef struct { char name[16]; float flux;} CalParType;
typedef struct self_info_type{ char name[32]; int index;} SelfInfoType;

typedef struct { 
  float spc[MAX_SCANS*MAX_SAMPS*MAX_CHANS];
  int   weight[MAX_SCANS*MAX_SAMPS];
  int   channels,selfs,sscans,cscans;
  float diode_val[MAX_SAMPS*MAX_SCANS];
  float ctime[MAX_SCANS],stime[MAX_SCANS];
  CalParType calpar[MAX_CALS];
  SelfInfoType selfinfo[MAX_SAMPS] ;
}  SelfDataType;

void stats(float *x, int n, float *mean, float *rms)
{ float xsum,xsum2;
  int i;
  if(n<0){fprintf(stderr,"illegal array len %d in stats()\n",n);exit(0);}
  if(n==1){ *mean=x[0];*rms=0.0; return;}
  xsum=xsum2=0.0;
  for(i=0;i<n;i++)
  { xsum  += x[i];
    xsum2 += x[i]*x[i];
  }
  *mean = xsum/n;
  *rms  = sqrt(xsum2/n -(*mean)*(*mean));
  return;
}
int get_scan_type(char *scan_type, FILE *fp)
{ char line[1024];
  int i=0; 
  while(fgets(line,1023,fp) !=NULL)
  { if(line[0]=='#') continue;
    if(sscanf(line,"%*s %*s %c",scan_type+i)!=1)
    { fprintf(stderr,"cannot parse %s\n",line); return -1;}
    i++;
    if(i==MAX_SCANS)
    {fprintf(stderr,"MAX_SCANS (%d) exceeded!\n",MAX_SCANS);return -1;}
  } 
  return i;
}
int get_cal_par(CalParType *cal_par, FILE *fp)
{ char line[1024];
  int i=0; 
  while(fgets(line,1023,fp) !=NULL)
  { if(line[0]=='#') continue;
    if(sscanf(line,"%s %f",cal_par[i].name,&cal_par[i].flux)!=2)
    { fprintf(stderr,"cannot parse %s\n",line); return -1;}
    if(i==MAX_SCANS)
    {fprintf(stderr,"MAX_CALS (%d) exceeded!\n",MAX_CALS);return -1;}
     i++;
  } 
  return i;
}
int get_self_info(LtaInfo *linfo, struct self_info_type *self_info)
{  VisInfo *vinfo=&linfo->lhdr.vinfo;
   int  i,selfs; 
   selfs=0;
   for(i=0;i<vinfo->baselines;i++)
   { int a0=vinfo->base[i].ant[0],a1=vinfo->base[i].ant[1];
     if(a0 != a1) continue;
     else
     { self_info[selfs].index=i;
       strcpy(self_info[selfs].name,vinfo->base[i].antname[0]);
       strcat(self_info[selfs].name,vinfo->base[i].bandname[0]);
       selfs++;
     }
   }
   return selfs;
}
float median(float *x, int n)
{ int i;
  float *y; 
  if(n<=0) 
  {fprintf(stderr,"Illegal array length in median()\n"); return 0.0;}
  if(n==1) return x[0];
  if(n==2) return 0.5*(x[0]+x[1]);
  if((y=(float*)malloc(n*sizeof(float)))==NULL)
  {fprintf(stderr,"malloc failure\n");return -1;}
  for(i=0;i<n;i++)y[i]=x[i];
  sort(n,y-1);
  if(n%2) return y[n/2];
  else return 0.5*(y[n/2]+y[n/2+1]);
}
int get_data(FILE *fp,SelfDataType *self_data,LtaInfo *linfo, float *data, int scan,int nrecs)
{ char   *buf;
  float  *dbuf,*iw,*ow,*x;
  int     recs=linfo->stab[scan].recs,srec,erec,trecs;
  int     selfs=self_data->selfs,channels=self_data->channels;
  int     blocksize=channels*selfs;
  int     i,j,k;
  
  /* which records of the scan are to be used?*/
  if(nrecs>0){srec=0;erec=nrecs;}
  if(nrecs<0){srec=recs+nrecs;erec=recs;}
  if(nrecs==0){srec=0;erec=recs;}
  if(srec<0)srec=0;if(erec>recs)erec=recs;
  trecs=erec-srec;

  if((buf=malloc(linfo->recl))==NULL)
  { fprintf(stderr,"malloc failure\n"); return -1;}
  if((dbuf=(float*)malloc(sizeof(float)*blocksize*recs))==NULL)
  { fprintf(stderr,"malloc failure\n"); return -1;}
  if((x=(float*)malloc(sizeof(float)*recs))==NULL)
  { fprintf(stderr,"malloc failure\n"); return -1;}
  rewind(fp);
  ltaseek(fp,linfo->stab[scan].start_rec+linfo->srecs,linfo->recl);
  for(i=0;i<recs;i++)
  { fread(buf,1,linfo->recl,fp);
    if(linfo->data_byte_order!=linfo->local_byte_order)
    flipData(buf,&linfo->lhdr);
    ow=dbuf+i*blocksize;
    for(j=0;j<selfs;j++)
    { iw = (float *)(buf+linfo->lhdr.data_off+self_data->selfinfo[j].index*channels*sizeof(float)*2);
      for(k=0;k<channels;k++){ow[k] = *iw; iw+=2;}
      ow += channels;
    }
  }
  /* take the median of the selected records  */
  for(i=0;i<selfs;i++)
  { ow=data+i*channels;
    iw=dbuf+i*channels;
    for(j=0;j<channels;j++)
    { for(k=srec;k<erec;k++)x[k-srec]=iw[k*blocksize+j];
      ow[j] = median(x,trecs);
    }
  }
  free(x); free(buf);free(dbuf);
  return 0;
}

int fnd_cal(CalParType *calpar,int cals,char *object)
{ int i;
  for(i=0;i<cals;i++) 
    if(!strncasecmp(calpar[i].name,object,strlen(calpar[i].name))) break;
  if(i==cals)
    fprintf(stderr,"Couldn't find the flux for %s\n",object); 
  return i;
       
}
int get_diode_val(SelfDataType *self_data,float *on,float *off, float *off1, float *noise, int cscans, int cal)
{ int bchan,echan;
  float x[MAX_CHANS],*xon,*xoff,*xoff1,*xnoise;
  int   selfs=self_data->selfs, channels=self_data->channels;
  float *dval=self_data->diode_val+cscans*selfs; 
  float flux=self_data->calpar[cal].flux;
  int   i,j;
  float src_defl[MAX_SAMPS],diode_defl[MAX_SAMPS];

  bchan = floor(channels*0.1);
  echan = floor(channels*0.9);

  for(i=0;i<selfs;i++)
  { xon = on+i*channels;
    xoff= off+i*channels; 
    xoff1= off1+i*channels; 
    xnoise = noise+i*channels; 
    for(j=0;j<channels;j++)
      x[j]= xnoise[j]-xoff1[j];
    diode_defl[i]=median(x+bchan,echan-bchan);
    for(j=0;j<channels;j++)
      x[j]= xon[j]-xoff[j];
    src_defl[i]=median(x+bchan,echan-bchan);
    dval[i]=diode_defl[i]/src_defl[i]*flux;
  }

  return 0;
}
int get_spc(SelfDataType *self_data, float *on, float *off, float *off1, float *noise, int sscans)
{ float x[MAX_CHANS],norm,*xon,*xoff,*xoff1,*xnoise,diode_defl[MAX_SAMPS];
  int   channels=self_data->channels,selfs=self_data->selfs;
  float *s=&self_data->spc[sscans*selfs*channels];
  int   i,j,bchan,echan;


  bchan = floor(channels*0.1);
  echan = floor(channels*0.9);

  for(i=0;i<selfs;i++)
  { xon = on+i*channels;
    xoff= off+i*channels; 
    xoff1= off1+i*channels; 
    xnoise= noise+i*channels; 
    norm=median(xoff+bchan,echan-bchan);
    for(j=0;j<channels;j++)
      x[j]= xnoise[j]-xoff1[j];
    diode_defl[i]=median(x+bchan,echan-bchan);
    for(j=0;j<channels;j++)
      s[j]= (norm/diode_defl[i])*(xon[j]-xoff[j])/xoff[j];
    s += channels;
  }

  for(i=0;i<selfs;i++)
    if(diode_defl[i]<0.0)
      self_data->weight[sscans*selfs+i] -=2;

  return 0;
}

int  get_weights(SelfDataType *self_data)
{ float x[MAX_SCANS],mean,rms,y[MAX_SAMPS];
  float z[MAX_SAMPS],m,s; 
  int i,j,k;
  int selfs=self_data->selfs,cscans=self_data->cscans;
 
  for(i=0;i<selfs;i++)
  { for(j=0,k=0;j<cscans;j++)
    if((x[k]=self_data->diode_val[j*selfs+i])>0.0)k++;
    if(k>0){stats(x,k,&mean,&rms); y[i]=rms/mean;}
    else{ y[i]=mean=-10.0;}
    self_data->diode_val[cscans*selfs+i]=mean;
  }
  m=median(y,selfs);
  for(i=0;i<selfs;i++)z[i]=fabs(y[i]-m);
  s=median(z,selfs);
  for(i=0;i<selfs;i++)
    if(y[i]>0.35)
      for(j=0;j<self_data->sscans;j++)self_data->weight[j*selfs+i] -=1.0;
  return 0;
}
void ave_ant(SelfDataType *self_data)
{  int sscans=self_data->sscans,selfs=self_data->selfs;
   int channels=self_data->channels,*weight=self_data->weight; 
   int cscans=self_data->cscans;
   float *diode_val=self_data->diode_val,*spc=self_data->spc;
   float x[MAX_CHANS],y[MAX_SAMPS*MAX_CHANS],w[MAX_SAMPS],mean,rms;
   int i,j,k,bchan,echan;

   for(i=0;i<selfs;i++)w[i]=0.0;
   for(i=0;i<selfs*channels;i++)y[i]=0.0;
   bchan=0.1*channels;echan=0.9*channels;

   for(i=0;i<selfs;i++)
   { for(j=0;j<sscans;j++)
     { if(weight[j*selfs+i]>0)
       { for(k=0;k<channels;k++) 
	    x[k]=spc[(j*selfs+i)*channels+k]*diode_val[cscans*selfs+i];
         stats(x+bchan,echan-bchan,&mean,&rms);
	 for(k=0;k<channels;k++) 
	   y[i*channels+k] += x[k];
	   w[i] += 1.0;
       }
     }
   }
   for(i=0;i<channels;i++)
   { printf("%d ",i);
     for(j=0;j<selfs;j++)
       if(w[j]>0.0) printf("%f ",y[j*channels+i]/w[j]); 
       else printf("0.0 "); 
     printf("\n");
   } 
   return;
}
void plot_all(SelfDataType *self_data)
{  int    sscans=self_data->sscans,selfs=self_data->selfs;
   int    channels=self_data->channels;
   int    cscans=self_data->cscans;
   float *dval=self_data->diode_val+cscans*selfs,*spc=self_data->spc;
   float x[MAX_CHANS],z[MAX_CHANS],xmax,xmin,u,v,mean,rms;
   int   i,j,k,bchan,echan;
   char  xlab[128],ylab[128],zlab[128],ans[128];

   bchan=0.1*channels;echan=0.9*channels;
   for(i=0;i<channels;i++)z[i]=i;
   strcpy(xlab,"channels");strcpy(ylab,"flux");
   cpgopen("/xserve");
   for(i=0;i<selfs;i++)
   { for(j=0;j<sscans;j++)
     { for(k=0;k<channels;k++) 
          x[k]=spc[(j*selfs+i)*channels+k]*dval[i];
       xmin=xmax=x[bchan];
       for(k=bchan;k<echan;k++){ if(x[k]>xmax)xmax=x[k];if(x[k]<xmin)xmin=x[k];}
       xmin=xmin -(xmax-xmin)*0.1;
       xmax=xmax +(xmax-xmin)*0.1;
       stats(x+bchan,echan-bchan,&mean,&rms);
       if(rms>MaxRms)self_data->weight[j*selfs+i]-=4;
       if(self_data->weight[j*selfs+i]<=0)continue;
       cpgask(0);
       cpgeras();
       cpgsvp(0.2,0.8,0.2,0.8);
       cpgswin(0.0,channels,xmin,xmax);
       cpgbox("BCNTS",0.0,0.0,"BCNTS",0.0,0.0);
       cpgline(channels,z,x);
       sprintf(zlab,"%s scan=%d",self_data->selfinfo[i].name,j);
       cpglab(xlab,ylab,zlab);
       cpgupdt();
       cpgsvp(0.1,0.9,0.1,0.9);
       cpgswin(0.0,1.0,0.0,1.0);
       sprintf(zlab,"weight = %3d rms=%8.2f accept? [y/n]",
	       self_data->weight[j*selfs+i],rms);
       cpgsch(1.5);
       u=0;v=-0.1;cpgtext(u,v,zlab);
       cpgsch(1.0);
       cpgcurs(&u,&v,ans);
       if(ans[0] == 'n')self_data->weight[j*selfs+i] -= 8;
     }
   }
   cpgclos();
}
void ave_spc(SelfDataType *self_data)
{  int sscans=self_data->sscans,selfs=self_data->selfs;
   int channels=self_data->channels,*weight=self_data->weight; 
   int cscans=self_data->cscans;
   float *dval=self_data->diode_val+cscans*selfs,*spc=self_data->spc;
   float x[MAX_CHANS],s[MAX_CHANS],wt,mean,rms;
   int i,j,k,bchan,echan,flagged=0;

   bchan=0.1*channels;echan=0.9*channels;wt=0.0;
   for(i=0;i<channels;i++)s[i]=0.0;
   for(i=0;i<selfs;i++)
   { for(j=0;j<sscans;j++)
     { if(weight[j*selfs+i]<=0)continue;
       for(k=0;k<channels;k++) 
	 x[k]=spc[(j*selfs+i)*channels+k]*dval[i];
       stats(x+bchan,echan-bchan,&mean,&rms);
       if(rms<MaxRms)
       { for(k=0;k<channels;k++)s[k] += x[k];
         wt += 1.0;
       }
     }
   }
   if(wt<=0.0)
     fprintf(stderr,"No remaining unflagged data!\n");
   else
   { fprintf(stderr,"%d spectra of %d total spectra accepted\n",(int)floor(wt),selfs*sscans);
     for(k=0;k<channels;k++) 
       printf("%d %f\n",k,s[k]/wt);
   }
   return;

}
void write_cal(SelfDataType *self_data)
{  int selfs=self_data->selfs;
   int cscans=self_data->cscans;
   float *diode_val=self_data->diode_val;
   float x[MAX_SCANS],mean,rms;
   int i,j;

   for(i=0;i<selfs;i++)
   { for(j=0;j<cscans;j++)x[j]=diode_val[j*selfs+i];
     stats(x,cscans,&mean,&rms);
     printf("%s ",self_data->selfinfo[i].name);
     for(j=0;j<cscans;j++)printf("%9.2f ",diode_val[j*selfs+i]);
     printf("%9.2f %9.2f\n",mean,rms/fabs(mean));
   } 
   return;
}
int sdspc(int argc, char **argv)
{ LtaInfo linfo;
  FILE    *fp,*sfp,*cfp;
  int      scans,cal,cals;
  float    on[MAX_SAMPS*MAX_CHANS],off[MAX_SAMPS*MAX_CHANS],off1[MAX_SAMPS*MAX_CHANS],
           noise[MAX_SAMPS*MAX_CHANS];
  char     scan_type[MAX_SCANS];
  SelfDataType self_data;
  int     i,p1,p2,p3;

  
  if((sfp=(fopen(ScanFile,"r")))==NULL)
  { fprintf(stderr,"cannot open %s\n",ScanFile);return -1;}
  if((scans=get_scan_type(scan_type,sfp))<0)
  { fprintf(stderr,"no scantypes found\n"); return -1;}

  if((cfp=(fopen(CalFile,"r")))==NULL)
  { fprintf(stderr,"cannot open %s\n",CalFile);return -1;}
  if((cals=get_cal_par(self_data.calpar,cfp))<0)
  { fprintf(stderr,"no calibrator pars found\n"); return -1;}

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

  if(scans<linfo.scans)
  { fprintf(stderr,"%s has fewer ScanTypes than scans in %s\n",ScanFile,LtaFile);
    return -1;
  }
  if((self_data.selfs=get_self_info(&linfo,self_data.selfinfo))==0) 
  { fprintf(stderr,"no self correlations found!\n"); return -1;}

  self_data.channels=linfo.lhdr.vinfo.channels;
  for(i=0;i<self_data.selfs*MAX_SCANS;i++)self_data.weight[i]=1;
  self_data.cscans=p1=p2=p3=0;
  for(i=0;i<linfo.scans;i++)
  { ScanInfoType *scan=&linfo.stab[i].scan;
    switch(scan_type[i])
    { case '*': continue;
      case 'C': if(get_data(fp,&self_data,&linfo,on,i,0-UseRecs)<0)return -1;
	        if((cal=fnd_cal(self_data.calpar,cals,scan->source.object))==cals) return 1;
		p1++;
	        break;
      case 'c': get_data(fp,&self_data,&linfo,off,i,UseRecs);
	        get_data(fp,&self_data,&linfo,off1,i,0-UseRecs);
        	p2++;
	        break;
      case 'n': get_data(fp,&self_data,&linfo,noise,i,UseRecs); 
	        self_data.ctime[self_data.cscans]=scan->t;
		p3++;
	        break;
      case 'S': continue;
      case 's': continue;
      case 'N': continue;
       default: fprintf(stderr,"Unkown ScanType %c\n",scan_type[i]); return -1;
    }
    if(p1*p2*p3>0)
    { get_diode_val(&self_data,on,off,off1,noise,self_data.cscans,cal);
      self_data.cscans++; p1=p2=p3=0;
    }
  }
  if(WriteCal)write_cal(&self_data);
  if(!(AveAnt||AveSpc||PlotAll)) return 0;


  self_data.sscans=p1=p2=p3=0;
  for(i=0;i<linfo.scans;i++)
  { ScanInfoType *scan=&linfo.stab[i].scan;
    switch(scan_type[i])
    { case '*':continue;
      case 'S': if((get_data(fp,&self_data,&linfo,on,i,0))<0)return -1;
	        p1++;break;
      case 's': if((get_data(fp,&self_data,&linfo,off,i,0))<0)return -1;
	         if((get_data(fp,&self_data,&linfo,off1,i,0-UseRecs))<0)return -1;
	        p2++;break;
      case 'N': if((get_data(fp,&self_data,&linfo,noise,i,UseRecs))<0)return -1;
		self_data.stime[self_data.sscans]=scan->t;
	        p3++;break;
      case 'C': continue;
      case 'c': continue;
      case 'n': continue;
      default: fprintf(stderr,"Unkown ScanType %c\n",scan_type[i]); return -1;
    }
    if(p1*p2*p3>0)
    { get_spc(&self_data,on,off,off1,noise,self_data.sscans);
      self_data.sscans++; p1=p2=p3=0;
    }
  }
  get_weights(&self_data);
  if(AveAnt)ave_ant(&self_data);
  if(PlotAll)plot_all(&self_data);
  if(AveSpc)ave_spc(&self_data);


  return 0;
}

int main(int argc, char **argv)
{
  optrega(&LtaFile, OPT_STRING,'i',"in", "Input LTA File");
  optrega(&ScanFile,OPT_STRING,'s',"scan", "Scan Type File");
  optrega(&CalFile, OPT_STRING,'c',"cal", "Cal Par File");
  optrega(&PlotAll, OPT_FLAG,  'p',"plot","Plot All Spectra");
  optrega(&WriteCal,OPT_FLAG,  'C',"showcal","Show Calibration");
  optrega(&AveAnt,  OPT_FLAG,  'a',"ant","Spectrum per antenna");
  optrega(&AveSpc,  OPT_FLAG,  'S',"spc","final average spectrum");
  optrega(&MaxRms,  OPT_FLOAT, 'r',"rms","maximum allowable rms");
  optrega(&UseRecs, OPT_INT,   'R',"recs","num. of recs to use per scan");

  sprintf(Usage,"Usage: For some help type %s --help \n",argv[0]);
  
  optUsage(Usage);
  
  optTitle("Averages Selected Self Correlations\n");
  optMain(sdspc);
  opt(&argc,&argv);

  return sdspc(argc,argv);
}



  
