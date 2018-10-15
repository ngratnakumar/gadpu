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

int init_solpar(SolParType *solpar,LtaInfo *linfo)
{ VisInfo *vinfo=&linfo->lhdr.vinfo;
  int i,j,k;
  
  solpar->block_len=8;
  solpar->min_phase_stability=0.60;
  solpar->rms_cut=3.0;
  solpar->max_bad_cp=0.90;
  solpar->min_good=0.6;

  solpar->bp_start_chan=0.2*vinfo->channels;
  solpar->bp_end_chan=0.8*vinfo->channels;

  /* sampler based flags */
  for(i=0;i<MAX_SAMPS;i++)solpar->samp_flag[i]=0;
  for(i=0;i<MAX_CHANS;i++)
    if((solpar->samp_chan_flag[i]=(char*)malloc(sizeof(char)*MAX_SAMPS))==NULL)
      {fprintf(stderr,"Malloc error\n");return 1;}
  for(i=0;i<MAX_CHANS;i++)
    for(j=0;j<MAX_SAMPS;j++)solpar->samp_chan_flag[i][j]=0;


  /* baseline based flags */
  for(i=0;i<MAX_SAMPS;i++)
    if((solpar->base_flag[i]=(char*)malloc(sizeof(char)*MAX_SAMPS))==NULL)
      {fprintf(stderr,"Malloc error\n");return 1;}
  for(i=0;i<MAX_CHANS;i++)
  { if((solpar->base_chan_flag[i]=(char**)malloc(sizeof(char*)*MAX_SAMPS))==NULL)
      {fprintf(stderr,"Malloc error\n");return 1;}
    for(j=0;j<MAX_SAMPS;j++)
      if((solpar->base_chan_flag[i][j]=(char*)malloc(sizeof(char)*MAX_SAMPS))==NULL)
	{fprintf(stderr,"Malloc error\n");return 1;}
  }
  for(i=0;i<MAX_SAMPS;i++)
    for(j=0;j<MAX_SAMPS;j++)solpar->base_flag[i][j]=0.0;
  for(i=0;i<MAX_CHANS;i++)
    for(j=0;j<MAX_SAMPS;j++)
      for(k=0;k<MAX_SAMPS;k++)solpar->base_chan_flag[i][j][k]=0;

  /* gains (sampler based)*/
  for(i=0;i<MAX_CHANS;i++)
  { if((solpar->samp_chan_gain[i]=(Complex *)malloc(MAX_SAMPS*sizeof(Complex)))==NULL)
    { fprintf(stderr,"Malloc error\n"); return 1;}
    if((solpar->samp_bp[i]=(Complex *)malloc(MAX_SAMPS*sizeof(Complex)))==NULL)
    { fprintf(stderr,"Malloc error\n"); return 1;}
    if((solpar->samp_bpgain[i]=(Complex *)malloc(MAX_SAMPS*sizeof(Complex)))==NULL)
    { fprintf(stderr,"Malloc error\n"); return 1;}
  }
  for(i=0;i<MAX_SAMPS;i++)
  { solpar->samp_gain[i].re=1.0; solpar->samp_gain[i].im=0.0;
    for(j=0;j<MAX_CHANS;j++)
    { solpar->samp_chan_gain[j][i].re= 1.0; solpar->samp_chan_gain[j][i].im =0.0;
      solpar->samp_bp[j][i].re       = 1.0; solpar->samp_bp[j][i].im        =0.0;
      solpar->samp_bpgain[j][i].re    =1.0; solpar->samp_bpgain[j][i].im    =0.0;
    }
  }

/* record based gains and flags*/
  solpar->save_rec_gain=0; /* don't save record based gains by default */
  solpar->rec_off=0; 
  for(i=0;i<MAX_CHANS;i++)
  { solpar->samp_rec_gain[i]=NULL; solpar->samp_chanrec_flag[i]=NULL;}
  for(i=0;i<MAX_SAMPS;i++)solpar->samp_rec_flag[i]=NULL;
   
  


  for(i=0;i<MAX_CHANS;i++)
    if((solpar->n_chan_sol[i]=(short*)malloc(sizeof(short)*MAX_SAMPS))==NULL)
      {fprintf(stderr,"Malloc error\n");return 1;}
  for(i=0;i<MAX_SAMPS;i++)solpar->nsol[i]=0;
  for(i=0;i<MAX_CHANS;i++)
    for(j=0;j<MAX_SAMPS;j++)solpar->n_chan_sol[i][j]=0;

  return 0;
}
  
void stats1(float *x, int n, float *mean, float *rms)
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
void stats(float *x, int n, float *mean)
{ int i;
  if(n<0){fprintf(stderr,"illegal array len %d in stats()\n",n);exit(0);}
  if(n==1){ *mean=x[0];return;}
  *mean=0;
  for(i=0;i<n;i++)*mean += x[i]/n;
  return;
}
float median(float *x, int n)
{ int i;
  float *y,m; 
  if(n<=0) 
  {fprintf(stderr,"Illegal array length in median()\n"); return 0.0;}
  if(n==1) return x[0];
  if(n==2) return 0.5*(x[0]+x[1]);
  if((y=(float*)malloc(n*sizeof(float)))==NULL)
  {fprintf(stderr,"malloc failure\n");return -1;}
  for(i=0;i<n;i++)y[i]=x[i];
  nr_select(n/2,n,y-1);
  m= y[n/2];
  free(y);
  return m;
}
int get_ref_samp(SolParType *solpar,LtaInfo *linfo, char *ref_ant)
{ int     i,j;
  VisInfo *vinfo=&linfo->lhdr.vinfo;

  for(i=0;i<MAX_BANDS;i++)solpar->ref_samp[i]=-1;
  for(j=0,i=0;i<vinfo->samplers;i++)
  { SampInfo *sinfo=&vinfo->samp[vinfo->samp_num[i]];
    int band =linfo->corr.sampler[i].band;
    if(!strcasecmp(sinfo->ant,ref_ant)){solpar->ref_samp[band]=i;j++;}
  }
  if(j>0) return 0;
  else{ fprintf(stderr,"No matching sampler found for Antenna %s\n",ref_ant);return 1;}
}
int get_one_chan(char *dbuf, float *data, int chan, int integ, LtaInfo *linfo)
{ float   *iw,*ow;
  int     i,j;
  VisInfo *vinfo=&linfo->lhdr.vinfo;
  

  for(i=0;i<integ;i++)
  { char *buf=dbuf+i*linfo->recl;
    int   chan_off=chan*2*sizeof(float);
    for(j=0;j<vinfo->baselines;j++)
    { int base_off=j*vinfo->channels*2*sizeof(float);
      iw = (float *)(buf+linfo->lhdr.data_off+base_off+chan_off);
      ow = data+(j*integ+i)*2;
      *ow=*iw; *(ow+1)=*(iw+1);
    }
  }

  return 0;
}
int put_one_chan(char *dbuf, float *data, int chan, int integ, LtaInfo *linfo)
{ float   *iw,*ow;
  int     i,j;
  VisInfo *vinfo=&linfo->lhdr.vinfo;
  

  for(i=0;i<integ;i++)
  { char *buf=dbuf+i*linfo->recl;
    int   chan_off=chan*2*sizeof(float);
    for(j=0;j<vinfo->baselines;j++)
    { int base_off=j*vinfo->channels*2*sizeof(float);
      iw = (float *)(buf+linfo->lhdr.data_off+base_off+chan_off);
      ow = data+(j*integ+i)*2;
      *iw=*ow; *(iw+1)=*(ow+1); /* reverse of get_one_chan() */
    }
  }

  return 0;
}
int find_bad_samplers_chan(LtaInfo *linfo, float *data, int chan, int integ,SolParType *solpar)
{ VisInfo *vinfo=&linfo->lhdr.vinfo; 
  float    re,im,p,m;
  int      i,j,k,nbase,good,sgn;
  Complex *d;
  char   **base_flag=solpar->base_chan_flag[chan];
  char    *samp_flag=solpar->samp_chan_flag[chan];
  FILE    *fp=NULL;
  if(chan==33)fp=fopen("phastab.dat","w");
  for(i=0;i<vinfo->samplers;i++)
  { short *base_num=vinfo->x_base_num[i];
    if(vinfo->samp_num[i]==MAX_SAMPS) continue; /* for the copy modes */
    good=nbase=0;
    for(j=0;j<vinfo->samplers;j++)
    { if(vinfo->samp_num[j]==MAX_SAMPS) continue; /* for the copy modes */
      if(base_num[j]==MAX_BASE) continue;
      sgn=base_num[j]>0? 1:-1;
      d=((Complex *)data)+sgn*base_num[j]*integ;
      re=im=0.0;
      for(k=0;k<integ;k++)
      { p=CMPLX_PHS(d);
        re +=cos(p)/integ; im+=sin(p)/integ;
	d++;
      }
      m=sqrt(re*re+im*im);
      if(chan==33)fprintf(fp,"%d %f\n",i,m);
      if(m>solpar->min_phase_stability)
      { good++; base_flag[i][j]=base_flag[j][i]=0;}
      else{ base_flag[i][j]=base_flag[j][i]=1;}
      nbase++;
    }
    if(good > solpar->min_good*nbase)samp_flag[i]=0;
    else samp_flag[i]=1;
  }
  if(fp!=NULL)fclose(fp);
  return 0;
}
int find_bad_samplers(LtaInfo *linfo,SolParType *solpar)
{ /* check all channels to identify bad samplers */
  int    i,j,k;
  int    channels=linfo->lhdr.vinfo.channels;
  int    samplers=linfo->lhdr.vinfo.samplers;
  char  *samp_flag=solpar->samp_flag;
  char **samp_chan_flag=solpar->samp_chan_flag;

  for(i=0;i<samplers;i++)
  { for(k=0,j=0;j<channels;j++)
      if(samp_chan_flag[j][i])k++;
    if((1.0*k)/(1.0*channels)>1.0-solpar->min_good)
      samp_flag[i]=1;
    else
      samp_flag[i]=0;
  }
  if(solpar->save_rec_gain)
  { char  **samp_rec_flag=solpar->samp_rec_flag;
    char ***samp_chanrec_flag=solpar->samp_chanrec_flag;
    int   off=solpar->rec_off/solpar->block_len;
    for(i=0;i<samplers;i++)
    { for(k=0,j=0;j<channels;j++)
      if(samp_chanrec_flag[j][i][off])k++;
      if((1.0*k)/(1.0*channels)>1.0-solpar->min_good)
	samp_rec_flag[i][off]=1;
      else
	samp_rec_flag[i][off]=0;
    }
  }
  return 0;
}
int solve_one_chan(float flux,float *data, char **vis_flag, int chan, int integ, LtaInfo *linfo, SolParType *solpar)
{ int      i,j,k,l,sgn,b;
  float    m0,re,im,norm,a,p;
  VisInfo *vinfo=&linfo->lhdr.vinfo; 
  Complex  gain[MAX_SAMPS][MAX_INTEG],g_ave[MAX_SAMPS],*d,*g,*g0,*g1;
  Complex *samp_gain=solpar->samp_chan_gain[chan];
  short   *nsol=solpar->n_chan_sol[chan];
  char    *samp_flag=solpar->samp_chan_flag[chan];
  char   **base_flag=solpar->base_chan_flag[chan];

  find_bad_samplers_chan(linfo, data, chan, integ,solpar);
  
  for(i=0;i<vinfo->samplers;i++)
  { if(samp_flag[i]) continue;
    for(l=0;l<integ;l++)
    { g=&gain[i][l];
      a=re=im=g->re=g->im=0.0;
      for(j=0,k=0;j<vinfo->samplers;j++)
      { if(samp_flag[j]) continue;
        if((b=vinfo->x_base_num[i][j])==MAX_BASE)continue;
	if(base_flag[i][j]) continue;
        sgn=b>0? 1:-1;
	d=((Complex*)data)+(sgn*b*integ+l);
	a += CMPLX_AMP(d); p = atan2(sgn*d->im,d->re);
	re += cos(p); im+=sin(p);
	k++;
      }
      if(k>0){a /=k; p=atan2(im,re); g->re=a*cos(p); g->im=a*sin(p);}
    }
  }

  /*  fprintf(stderr,"Flagged ");
      for(i=0;i<vinfo->samplers;i++)
      if(samp_flag[i])fprintf(stderr,"%s%s ",vinfo->samp[vinfo->samp_num[i]].ant,
            vinfo->samp[vinfo->samp_num[i]].band);
      fprintf(stderr,"\n");
  */
  m0=0.0;k=0;
  for(i=0;i<vinfo->samplers;i++)
  { g_ave[i].re=g_ave[i].im=0.0;
    if(samp_flag[i]) continue;
    for(j=0;j<integ;j++)
    { g=&gain[i][j];
      if((a = sqrt(g->re*g->re+g->im*g->im))>0.0)
      { m0 +=a; g_ave[i].re += g->re; g_ave[i].im += g->im; 
        k++; nsol[i]++;
      }
    }
  }
  if(flux<=0.0)flux=1.0;
  if(k>0)norm=sqrt(flux*m0/k);
  else norm=1.0;

  for(i=0;i<vinfo->samplers;i++)
  { if(samp_flag[i]) continue;
    samp_gain[i].re += g_ave[i].re/norm; samp_gain[i].im += g_ave[i].im/norm;
    for(j=0;j<integ;j++)
    { g=&gain[i][j]; g->re /=norm; g->im /=norm;}
  }

  if(solpar->save_rec_gain)
  { int off=solpar->rec_off;
    int f_off=solpar->rec_off/solpar->block_len;
    for(i=0;i<vinfo->samplers;i++)
    { for(j=0;j<integ;j++)
      { solpar->samp_rec_gain[chan][i][off+j].re=gain[i][j].re;
        solpar->samp_rec_gain[chan][i][off+j].im=gain[i][j].im;
      }
      solpar->samp_chanrec_flag[chan][i][f_off]=samp_flag[i];
    }
  }

  for(l=0;l<integ;l++)
  { for(i=0;i<vinfo->baselines;i++)
    { Complex c,c1;
      int flag_off=(i*vinfo->channels+chan)/8;
      unsigned char flag_mask=1<<((i*vinfo->channels+chan)%8);
      short *rev_num=vinfo->samp_rev_num;
      int s0=rev_num[vinfo->base[i].samp[0]],s1=rev_num[vinfo->base[i].samp[1]];
      if(s0==s1) continue;
      if(samp_flag[s0] || samp_flag[s1]||base_flag[s0][s1])
      { vis_flag[l][flag_off] |= flag_mask; continue;}
      g0=&gain[s0][l]; g1=&gain[s1][l];
      d=((Complex*)data)+(i*integ+l);
      CMPLX_CONJ_MUL(g0,g1,(&c));
      CMPLX_DIV(d,(&c),(&c1));
      d->re=c1.re;d->im=c1.im;
    }
  }


  return 0;
}

int  compute_chan0(char *buf,LtaInfo *linfo,int start_chan,int end_chan)
{ Complex *d,c0;
  int      j,k,l,f_off;
  VisInfo *vinfo=&linfo->lhdr.vinfo;
  char    *flag;

  
  if(start_chan<0) start_chan=(int)floor(vinfo->channels*0.2);
  if(end_chan <0) end_chan  =(int)floor(vinfo->channels*0.8);
  if(start_chan>=vinfo->channels|| end_chan >=vinfo->channels)
  { fprintf(stderr,"Bad channel range [%d - %d]in compute_chan0()\n",start_chan,end_chan); return 1; }

  for(j=0;j<vinfo->baselines;j++)
  { d  = (Complex *)(buf+linfo->lhdr.data_off+j*vinfo->channels*2*sizeof(float));
    d += start_chan;
    c0.re=c0.im=0.0;k=0;
    for(l=0,k=start_chan;k<=end_chan;k++)
    { f_off=j*vinfo->channels+k;
      flag =buf+linfo->lhdr.flg_dat_off+f_off/8;
      if(*flag & (1<<(f_off%8))) continue; 
      c0.re +=d->re;c0.im +=d->im;
      l++;d++;
    }
    f_off=j*vinfo->channels;
    flag =buf+linfo->lhdr.flg_dat_off+f_off/8;
    if(l>0)
    { d=(Complex *)(buf+linfo->lhdr.data_off+j*vinfo->channels*2*sizeof(float));
      d->re=c0.re/l; d->im=c0.im/l;
      *flag &= ~(1<<(f_off%8)); /* unflag chan0 */
    }else{
      *flag |= 1<<(f_off%8); /* flag chan0 */
    }
  }
  return 0;
}

int solve_all_chans(LtaInfo *linfo, int scan, FILE *fp, FILE *ofp, SolParType *solpar)
{ char    *dbuf,*vis_flag[2*MAX_INTEG];
  float   *data;
  int     i,j,k,l,ns;
  VisInfo *vinfo= &linfo->lhdr.vinfo;
  int     chan,smp,nrecs=linfo->stab[scan].recs;
  float   i_flux = linfo->stab[scan].scan.source.flux.i;
  int     integ=solpar->block_len;

  if((dbuf=(malloc(linfo->recl*2*MAX_INTEG)))==NULL)
  {fprintf(stderr,"malloc failure\n"); return 1;}
  if((data=(float*)(malloc(vinfo->baselines*2*MAX_INTEG*2*sizeof(float))))==NULL)
  {fprintf(stderr,"malloc failure\n"); return 1;}


  for(chan=0;chan<vinfo->channels;chan++) /*initalize gains */
    for(smp=0;smp<vinfo->samplers;smp++)
    { solpar->n_chan_sol[chan][smp]=0; 
      solpar->samp_chan_gain[chan][smp].re=solpar->samp_chan_gain[chan][smp].im=0.0;
    }

  if(solpar->save_rec_gain)
  { for(i=0;i<MAX_CHANS;i++)
    { if(solpar->samp_rec_gain[i] !=NULL)
      { for(j=0;j<MAX_SAMPS;j++)free(solpar->samp_rec_gain[i][j]);free(solpar->samp_rec_gain[i]);}
      if(solpar->samp_chanrec_flag[i] !=NULL)
      { for(j=0;j<MAX_SAMPS;j++)free(solpar->samp_chanrec_flag[i][j]);free(solpar->samp_chanrec_flag[i]);}
      if((solpar->samp_rec_gain[i]=(Complex**)malloc(sizeof(Complex*)*MAX_SAMPS))==NULL)
      { fprintf(stderr,"malloc error\n"); return 1;}
      if((solpar->samp_chanrec_flag[i]=(char**)malloc(sizeof(char*)*MAX_SAMPS))==NULL)
      { fprintf(stderr,"malloc error\n"); return 1;}
      for(j=0;j<MAX_SAMPS;j++)
      { if((solpar->samp_rec_gain[i][j]=(Complex*)malloc(sizeof(Complex)*nrecs))==NULL)
	{ fprintf(stderr,"malloc error\n"); return 1;}
	if((solpar->samp_chanrec_flag[i][j]=(char*)malloc(sizeof(char)*(nrecs/solpar->block_len)))==NULL)
	{ fprintf(stderr,"malloc error\n"); return 1;}
      }
    }
    for(i=0;i<MAX_SAMPS;i++)
    {  if(solpar->samp_rec_flag[i]!=NULL)free(solpar->samp_rec_flag[i]);
       if((solpar->samp_rec_flag[i]=(char*)malloc(sizeof(char)*(nrecs/solpar->block_len)))==NULL)
       { fprintf(stderr,"malloc error\n"); return 1;}
    }
  }

  j=0;k=0;solpar->rec_off=0;
  while(j<=nrecs)
  { char *buf=dbuf+k*linfo->recl; 
    if((j>0 && j%integ==0 && j <(nrecs/integ)*integ)||(j==nrecs))/* last block extends to scan end */
    { for(l=0;l<k;l++)vis_flag[l]=dbuf+l*linfo->recl+linfo->lhdr.flg_dat_off;
      for(chan=0;chan<vinfo->channels;chan++)
      { get_one_chan(dbuf,data,chan,k,linfo);
        solve_one_chan(i_flux,data,vis_flag,chan,k,linfo,solpar);
	put_one_chan(dbuf,data,chan,k,linfo);
      }
      find_bad_samplers(linfo,solpar);
      if(ofp!=NULL)
      { for(l=0;l<k;l++)
	{ compute_chan0(dbuf+l*linfo->recl,linfo,solpar->bp_start_chan,solpar->bp_end_chan);
	  fwrite(dbuf+l*linfo->recl,linfo->recl,1,ofp);
	}
      }
      if(j==nrecs) break; /* done with this scan */
      k=0; buf=dbuf; /* else process the next integ recs */
      solpar->rec_off=j;
    }
    fread(buf,linfo->recl,1,fp);
    if(solpar->change_endian)flipData(buf,&linfo->lhdr);
    j++;k++;
  }

  for(j=0;j<vinfo->channels;j++)
    for(k=0;k<vinfo->samplers;k++)
    { if((ns=solpar->n_chan_sol[j][k]) >0)
      solpar->samp_chan_gain[j][k].re /= ns;solpar->samp_chan_gain[j][k].im /= ns;
    }
 
  free(dbuf);free(data);

  return 0;
}
int delay_fit(float *x, float *p, int nd, float *delay, float *p0)
{ float p1[MAX_CHANS];
  int   i;
  float phi0,phi1,dp,ddp,theta;
  float X,Y,XY,X2,D;
  float cp,sp;

  sp=cp=0;
  for(dp=0.0,i=1;i<nd;i++)
  {  sp += sin(p[i]-p[i-1])/(x[i]-x[i-1]);
     cp += cos(p[i]-p[i-1])/(x[i]-x[i-1]);
  }
  dp = atan2(sp,cp);

  for(phi0=0.0,i=0;i<nd;i++)
  { phi0+=sin(p[i]-x[i]*dp);}
  phi0=asin(phi0/nd);
  for(D=0.0,i=0;i<nd;i++) D += cos(p[i]-phi0-x[i]*dp);
  if(D<0.0)
  { phi0+=M_PI; 
    if(phi0>2.0*M_PI)phi0 -= M_PI;
    for(D=0.0,i=0;i<nd;i++) D += cos(p[i]-phi0-x[i]*dp);
  }

  for(i=0;i<nd;i++)
  { float z;
    theta=phi0+x[i]*dp;
    while(theta>M_PI)theta-=2.0*M_PI;  /* asumme that angle is limited to -PI to PI */
    while(theta<-M_PI)theta+=2.0*M_PI;
    z=p[i]-theta;
    if(fabs(z+2.0*M_PI)<fabs(z))z += 2.0*M_PI;
    if(fabs(z-2.0*M_PI)<fabs(z))z -= 2.0*M_PI;
    p1[i]=z;
  }

  X=Y=X2=XY=0.0;
  for(i=0;i<nd;i++)
  { X += x[i]; Y += p1[i];
    XY += x[i]*p1[i]; X2 += x[i]*x[i];
  }
  ddp = (X*Y-nd*XY)/(X*X-nd*X2);
  phi1= (Y-ddp*X)/nd;

  phi0 +=phi1;
  dp   +=ddp;

  *p0=phi0; *delay=dp;

  /*
    for(i=0;i<nd;i++)
    { float z;
      theta=phi0+x[i]*dp;
      while(theta>2.0*M_PI)theta-=2.0*M_PI;
      z=p[i]-theta;
      if(fabs(z+2.0*M_PI)<fabs(z))z += 2.0*M_PI;
      if(fabs(z-2.0*M_PI)<fabs(z))z -= 2.0*M_PI;
      printf("%f %f %f\n",x[i],p[i],theta);
    }
  */

  return 0;
}
int get_delay_gain(SolParType *solpar, LtaInfo *linfo, int scan)
{ float    p[MAX_CHANS],x[MAX_CHANS];
  float    re,im,re0,im0; 
  int      i,j,k,l;
  VisInfo *vinfo=&linfo->lhdr.vinfo;
  int      nrecs=linfo->stab[scan].recs;
  float    cw=linfo->stab[scan].scan.source.ch_width;
  CorrType *corr=&linfo->corr;

  for(i=0;i<vinfo->samplers;i++)
  { for(j=0;j<nrecs;j++)
    { SmpSolType *smp_sol=&solpar->smp_sol[j][i];
      smp_sol->gain.re=smp_sol->gain.im=0.0;
      smp_sol->delay=smp_sol->phi0=0.0;
    }
  }

  for(i=0;i<vinfo->samplers;i++)
  { int band=corr->sampler[i].band;
    int net_sign=linfo->stab[scan].scan.source.net_sign[band];
    for(j=0;j<nrecs;j++)
    { SmpSolType *smp_sol=&solpar->smp_sol[j][i];
      int         off=j/solpar->block_len;
      if(off==nrecs/solpar->block_len)off--; /* last block goes to end of scan */
      if(solpar->samp_rec_flag[i][off])continue;
      re0=im0=0.0;
      for(l=0,k=0;k<vinfo->channels;k++)
      { if(solpar->samp_chanrec_flag[k][i][off])continue;
        re=solpar->samp_rec_gain[k][i][j].re;im=solpar->samp_rec_gain[k][i][j].im;
	x[l]=k;p[l]=atan2(im,re);
	re0 += re;im0 += im;l++;
      }
      smp_sol->gain.re=re0/l;   /* l>0 guaranteed */
      smp_sol->gain.im=im0/l;
      delay_fit(x,p,l,&smp_sol->delay,&smp_sol->phi0);
      smp_sol->delay=-1.0*(net_sign*smp_sol->delay/cw*C/(2.0*M_PI));
      /*{ char   outf[64];
	FILE   *fp;
	int    c;
	sprintf(outf,"smp%drec%d.dat",i,j);
	fp=fopen(outf,"w");
	for(c=0;c<l;c++)fprintf(fp,"%f %f\n",x[c],p[c]);
	fclose(fp);
	}*/
    }
  }
  return 0;
}

int get_normalized_bp(VisInfo *vinfo, SolParType *solpar)
{ int   i,j,k;
  float m0,norm; 
  int   start_chan,end_chan;

  if(solpar->bp_start_chan<0)start_chan=(int)floor(vinfo->channels*0.2);
  else start_chan=solpar->bp_start_chan;
  if(solpar->bp_end_chan<0)end_chan=(int)floor(vinfo->channels*0.8);
  else end_chan=solpar->bp_end_chan;
  if(start_chan>=vinfo->channels|| end_chan >=vinfo->channels)
  { fprintf(stderr,"Bad channel range [%d - %d]in get_normalized_bp()\n",start_chan,end_chan); return 1; }

  for(i=0;i<vinfo->samplers;i++)
  { m0=0.0,k=0;
    for(j=start_chan;j<=end_chan;j++)
      if(solpar->n_chan_sol[j][i]>0){ m0 +=CMPLX_AMP((&solpar->samp_chan_gain[j][i]));k++;}
    if(k>0)
    { norm=m0/k;
      for(j=0;j<vinfo->channels;j++)
      { solpar->samp_bp[j][i].re=solpar->samp_chan_gain[j][i].re/norm;
        solpar->samp_bp[j][i].im=solpar->samp_chan_gain[j][i].im/norm;
      }
    }else{
      for(j=0;j<vinfo->channels;j++)
      { solpar->samp_bp[j][i].re=0.0;
        solpar->samp_bp[j][i].im=0.0;
      }
      solpar->samp_flag[i]++; /* flag the sampler */
    }
  }
 
 return 0;
}
	
int apply_gain_to_buf(char *buf,LtaInfo *linfo,Complex **gain)
{ VisInfo *vinfo= &linfo->lhdr.vinfo;
  int      k,chan;

  for(k=0;k<vinfo->baselines;k++)
  { short *rev_num=vinfo->samp_rev_num;
    int s0=rev_num[vinfo->base[k].samp[0]],s1=rev_num[vinfo->base[k].samp[1]];
    Complex *d=(Complex *)(buf+linfo->lhdr.data_off+k*vinfo->channels*2*sizeof(float));
    Complex c,c1;
    for(chan=0;chan<vinfo->channels;chan++)
    { Complex *g0=&gain[chan][s0];
      Complex *g1=&gain[chan][s1];
      if(CMPLX_AMP(g0)>0.0 && CMPLX_AMP(g1)>0.0)
      { CMPLX_CONJ_MUL(g0,g1,(&c));
	CMPLX_DIV(d,(&c),(&c1));
	d->re=c1.re;d->im=c1.im;
      }else{
	int f_off=k*vinfo->channels+chan;
	char *flag=buf+linfo->lhdr.flg_dat_off+f_off/8;
	*flag |= 1<<(f_off%8);
      }
      d++;
    }
  }

  return 0;
}

int apply_bbgain_to_buf(char *buf,LtaInfo *linfo,Complex *bb_gain)
{ VisInfo *vinfo= &linfo->lhdr.vinfo;
  int      k,chan;

  for(k=0;k<vinfo->baselines;k++)
  { short *rev_num=vinfo->samp_rev_num;
    int s0=rev_num[vinfo->base[k].samp[0]],s1=rev_num[vinfo->base[k].samp[1]];
    Complex *g0=&bb_gain[s0], *g1=&bb_gain[s1];
    Complex *d=(Complex *)(buf+linfo->lhdr.data_off+k*vinfo->channels*2*sizeof(float));
    Complex c,c1;
    if(CMPLX_AMP(g0)>0.0 && CMPLX_AMP(g1)>0.0)
    { CMPLX_CONJ_MUL(g0,g1,(&c));
      for(chan=0;chan<vinfo->channels;chan++)
      { CMPLX_DIV(d,(&c),(&c1));
         d->re=c1.re;d->im=c1.im;
	 d++;
      }
    }else{
      for(chan=0;chan<vinfo->channels;chan++)
      { int f_off=k*vinfo->channels+chan;
	char *flag=buf+linfo->lhdr.flg_dat_off+f_off/8;
	*flag |= 1<<(f_off%8);
      }
    }
  }

  return 0;
}

int apply_gain_to_scan(LtaInfo *linfo,int scan,FILE *fp, FILE *ofp,Complex **gain,
		       int change_endian,int start_chan,int end_chan)
{ char    *dbuf;
  int      j,nrecs=linfo->stab[scan].recs;

  if((dbuf=(malloc(linfo->recl)))==NULL)
  {fprintf(stderr,"malloc failure\n"); return 1;}
  
  for(j=0;j<nrecs;j++)
  { if(fread(dbuf,linfo->recl,1,fp) !=1)
    { fprintf(stderr,"read error!\n"); return 1;}
    if(change_endian)flipData(dbuf,&linfo->lhdr);
    if(apply_gain_to_buf(dbuf,linfo,gain)) return 1;
    if(compute_chan0(dbuf,linfo,start_chan,end_chan)) return 1;
    if(fwrite(dbuf,linfo->recl,1,ofp) !=1)
    { fprintf(stderr,"write error!\n"); return 1;}
  }
  
  free(dbuf);
  return 0;
}

int compute_total_gain(VisInfo *vinfo, Complex **bandpass, Complex *broadband_gain, 
			Complex **total_gain)
{ int i,j;
  Complex *tg,*bp,*bb;
  int channels=vinfo->channels,samplers=vinfo->samplers;

  for(i=0;i<channels;i++) 
  { for(j=0;j<samplers;j++)
    { tg=&total_gain[i][j];
      bp=&bandpass[i][j];
      bb=&broadband_gain[j];
      CMPLX_MUL(bp,bb,tg);
    }
  }
  return 0;
}

int solve_chan0(LtaInfo *linfo, int scan, FILE *fp, FILE *ofp, SolParType *solpar)
{ char    *dbuf,*vis_flag[2*MAX_INTEG];
  float   *data;
  int      j,k,l,ns;
  VisInfo *vinfo= &linfo->lhdr.vinfo;
  int      smp,nrecs=linfo->stab[scan].recs;
  float    i_flux = linfo->stab[scan].scan.source.flux.i;
  int      integ=solpar->block_len;
  
  if((dbuf=(malloc(linfo->recl*2*MAX_INTEG)))==NULL)
  {fprintf(stderr,"malloc failure\n"); return 1;}
  if((data=(float*)(malloc(vinfo->baselines*2*MAX_INTEG*2*sizeof(float))))==NULL)
  {fprintf(stderr,"malloc failure\n"); return 1;}


  for(smp=0;smp<vinfo->samplers;smp++) /* initialize gains */
  { solpar->nsol[smp]=0; solpar->samp_gain[smp].re=solpar->samp_gain[smp].im=0.0;}

  j=0;k=0;
  while(j<=nrecs)
  { char *buf=dbuf+k*linfo->recl; 
    if((j>0 && j%integ==0 && j <(nrecs/integ)*integ)||(j==nrecs))/* last block extends to scan end */
    { for(l=0;l<k;l++)vis_flag[l]=dbuf+l*linfo->recl+linfo->lhdr.flg_dat_off;
      for(l=0;l<vinfo->samplers;l++)
      { solpar->samp_chan_gain[0][l].re=solpar->samp_chan_gain[0][l].im=0.0;solpar->n_chan_sol[0][l]=0;}
      for(l=0;l<k;l++)
      { apply_gain_to_buf(dbuf+l*linfo->recl,linfo,solpar->samp_bp); /* apply bandpass */
        compute_chan0(dbuf+l*linfo->recl,linfo,solpar->bp_start_chan,solpar->bp_end_chan);
      }
      get_one_chan(dbuf,data,0,k,linfo);
      solve_one_chan(i_flux,data,vis_flag,0,k,linfo,solpar);
      for(l=0;l<vinfo->samplers;l++)
      { if((ns=solpar->n_chan_sol[0][l])>0)
        { solpar->samp_gain[l].re += solpar->samp_chan_gain[0][l].re; 
	  solpar->samp_gain[l].im += solpar->samp_chan_gain[0][l].im; 
	  solpar->nsol[l] += ns;
   	  solpar->samp_chan_gain[0][l].re /=ns; 
	  solpar->samp_chan_gain[0][l].im /=ns;
	}
      }
      if(ofp != NULL)
      { for(l=0;l<k;l++)
        {apply_bbgain_to_buf(dbuf+l*linfo->recl,linfo,solpar->samp_chan_gain[0]);
         if(fwrite(dbuf+l*linfo->recl,linfo->recl,1,ofp)!=1)
	 {fprintf(stderr,"Write Error\n");return 1;}
	}
      }
      if(j==nrecs) break; /* done with this scan */
      k=0; buf=dbuf; /* else process the next integ recs */
    }
    fread(buf,linfo->recl,1,fp);
    if(solpar->change_endian)flipData(buf,&linfo->lhdr);
    j++;k++;
  }

  for(k=0;k<vinfo->samplers;k++)
  { if((ns=solpar->nsol[k]) >0)
    solpar->samp_gain[k].re /= ns;solpar->samp_gain[k].im /= ns;
  }
  
  free(dbuf);free(data);

  return 0;
}


