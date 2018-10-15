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


Complex cmplxZtmp;
float   floatZtmp;

char *LtaFile=NULL;
char  Usage[256];
float PhaseStabilityThreshold=0.90;
int   SelScans=0,*SelScan=NULL;
int   XtractFmt=1;
char  *RefAnt=NULL;
int   FilterLength=1;

int ltasol(int argc, char **argv)
{ LtaInfo linfo;
  FILE       *fp;
  int         i,j,k,l,m;
  VisInfo    *vinfo=&linfo.lhdr.vinfo;
  SolParType  solpar;
  char        object[16];
  CorrType   *corr=&linfo.corr;
  float       g_re[MAX_FILT],g_im[MAX_FILT],g_dly[MAX_FILT],g_cp0[MAX_FILT],g_sp0[MAX_FILT];
  SmpSolType  g_ave;
  FILE       *fp1,*fp2;
  int         write_hdr=1;

  if((fp1=fopen("gain.dat","w"))==NULL)
  { fprintf(stderr,"Error opening output file gain.dat\n");return 1;}
  if((fp2=fopen("delay.dat","w"))==NULL)
  { fprintf(stderr,"Error opening output file delay.dat\n");return 1;}

  if(FilterLength>MAX_FILT)
  {fprintf(stderr,"Cannot average solutions for more than %d records\n",MAX_FILT); return 1;}
       
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

   if(linfo.local_byte_order!=linfo.data_byte_order)solpar.change_endian=1;
   else solpar.change_endian=0;

   if(init_solpar(&solpar,&linfo)) return 1;
   solpar.save_rec_gain=1;
   for(i=0;i<linfo.scans;i++)
   { if(SelScans)
     { for(j=0;j<SelScans;j++)if(i==SelScan[j]) break;
       if(j==SelScans) continue;
     }
     strncpy(object,linfo.stab[i].shdr.object,11);object[11]='\0';
     fprintf(stderr,"SCAN %03d %-12s\n",i,object);
     rewind(fp); ltaseek(fp,linfo.stab[i].start_rec+linfo.srecs,linfo.recl);
     solve_all_chans(&linfo,i,fp,NULL,&solpar);
     get_delay_gain(&solpar,&linfo,i);
     if(RefAnt !=NULL)
     { SmpSolType ref_sol[MAX_RECS][MAX_BANDS];
       fprintf(stderr,"Adjusting solutions to reference %s\n",RefAnt);
       get_ref_samp(&solpar,&linfo,RefAnt);
       for(k=0;k<MAX_BANDS;k++)
       { int ref    =solpar.ref_samp[k];
	 if(ref<0) continue;
	 for(j=0;j<linfo.stab[i].recs;j++)
	   memcpy(&ref_sol[j][k],&solpar.smp_sol[j][ref],sizeof(SmpSolType));
	   
       }   
       for(k=0;k<linfo.stab[i].recs;k++)
       { for(j=0;j<vinfo->samplers;j++)
         { SmpSolType *smp_sol=&solpar.smp_sol[k][j];
	   int         band=linfo.corr.sampler[j].band;
  	   SmpSolType *rs=&ref_sol[k][band];
  	   float      a,p;
	   a = CMPLX_AMP((&smp_sol->gain));
	   p = CMPLX_PHS((&smp_sol->gain))-CMPLX_PHS((&rs->gain));
	   smp_sol->gain.re=a*cos(p); smp_sol->gain.im=a*sin(p);
	   smp_sol->delay -= rs->delay;
	   smp_sol->phi0  -= rs->phi0;
	   while(smp_sol->phi0>M_PI)  smp_sol->phi0 -=2.0*M_PI;
	   while(smp_sol->phi0<-M_PI) smp_sol->phi0 +=2.0*M_PI;
	 }
       }
     }else{
       fprintf(stderr,"Warning: No RefAnt, Floating Solutions\n");
     }
     if(XtractFmt)
     { if(write_hdr)
       { fprintf(fp1,"#LTAT    %f\n",16.0); fprintf(fp2,"#LTAT    %f\n",16.0);
         fprintf(fp1,"#NROWS   %d\n",linfo.stab[i].recs);fprintf(fp2,"#NROWS   %d\n",linfo.stab[i].recs);
         fprintf(fp1,"#NCOLS   %d\n",2*vinfo->samplers+1); fprintf(fp2,"#NCOLS   %d\n",2*vinfo->samplers+1);
         fprintf(fp1,"#CHAN1   %d\n",0);  fprintf(fp2,"#CHAN1   %d\n",0);
         fprintf(fp1,"#CHAN2   %d\n",0);  fprintf(fp2,"#CHAN2   %d\n",0);
         fprintf(fp1,"#CHANINC %d\n",1);  fprintf(fp2,"#CHANINC %d\n",1);
         fprintf(fp1,"#LABEL00  ist\n");  fprintf(fp2,"#LABEL00  ist\n");
         for(k=0;k<vinfo->samplers;k++)
         { SampInfo *sinfo=&vinfo->samp[vinfo->samp_num[k]];
	   int id=2*k+1;
   	   fprintf(fp1,"#LABEL%02d %s-%s:%s-%s a\n",id,sinfo->ant,sinfo->band,sinfo->ant,sinfo->band);
	   fprintf(fp1,"#LABEL%02d %s-%s:%s-%s p\n",id+1,sinfo->ant,sinfo->band,sinfo->ant,sinfo->band);
	   fprintf(fp2,"#LABEL%02d %s-%s:%s-%s delay\n",id,sinfo->ant,sinfo->band,sinfo->ant,sinfo->band);
	   fprintf(fp2,"#LABEL%02d %s-%s:%s-%s phs0\n",id+1,sinfo->ant,sinfo->band,sinfo->ant,sinfo->band);
           }
           fprintf(fp1,"#End\n");       fprintf(fp2,"#End\n");
           fprintf(fp1,"#OBJECT= %s RA= %s DEC= %s Lambda= %s DATE-OBS= %s %s %s %s\n","Cal","00:00:00","00:00:00",
		   "21.0","Day","Date","0""00:00:00","year");
	   fprintf(fp2,"#OBJECT= %s RA= %s DEC= %s Lambda= %s DATE-OBS= %s %s %s %s\n","Cal","00:00:00","00:00:00",
		   "21.0","Day","Date","0""00:00:00","year");
       }
       if(FilterLength>1) /*filter the solutions */
       { int nrecs=linfo.stab[i].recs;
	 for(j=0,k=0;j<=nrecs;) /* last block extends to scan end */
	 { if((j>0 && j%FilterLength==0 && j <(nrecs/FilterLength)*FilterLength)||(j==nrecs))
	   { float  tm=(linfo.stab[i].scan.t+j*corr->daspar.lta*corr->corrpar.statime/1e6)/(3600.0) ;
	     fprintf(fp1,"%10.5f ",tm);fprintf(fp2,"%10.5f ",tm);
	     for(l=0;l<vinfo->samplers;l++)
	     { for(m=0;m<k;m++)
	       { SmpSolType *smp_sol=&solpar.smp_sol[m+j-k][l];
		 g_re[m]=smp_sol->gain.re;g_im[m]=smp_sol->gain.im;
		 g_dly[m]=smp_sol->delay;
		 g_cp0[m]=cos(smp_sol->phi0);g_sp0[m]=sin(smp_sol->phi0);
	       }
	       g_ave.gain.re=nr_select(k/2,k,g_re-1); g_ave.gain.im=nr_select(k/2,k,g_im-1);
	       g_ave.delay  =nr_select(k/2,k,g_dly-1); 
	       g_ave.phi0=atan2(nr_select(k/2,k,g_sp0-1),nr_select(k/2,k,g_cp0-1));
	       fprintf(fp1,"%12.4e %6.1f ",CMPLX_AMP((&g_ave.gain)),RAD2DEG(CMPLX_PHS((&g_ave.gain))));
	       fprintf(fp2,"%8.2f %6.1f ", g_ave.delay,RAD2DEG(g_ave.phi0));
	     }
	     k=0;
	     fprintf(fp1,"\n"); fprintf(fp2,"\n");
	   }
	   if(j==nrecs) break; /* done with this scan */
	   j++;k++;
	 }
       }else{ /* don't filter the solutions */
	 for(k=0;k<linfo.stab[i].recs;k++)
	 { float  tm=(linfo.stab[i].scan.t+k*corr->daspar.lta*corr->corrpar.statime/1e6)/(3600.0) ;
	   fprintf(fp1,"%10.5f ",tm);fprintf(fp2,"%10.5f ",tm);
	   for(j=0;j<vinfo->samplers;j++)
	   { SmpSolType *smp_sol=&solpar.smp_sol[k][j];
	     fprintf(fp1,"%12.4e %6.1f ",CMPLX_AMP((&smp_sol->gain)),RAD2DEG(CMPLX_PHS((&smp_sol->gain))));
	     fprintf(fp2,"%8.2f %6.1f ", smp_sol->delay,RAD2DEG(smp_sol->phi0));
	   }
	   fprintf(fp1,"\n");fprintf(fp2,"\n");
	 }
       }
     }else{ /* not XTRACT format */
       for(k=0;k<vinfo->samplers;k++)
       { SampInfo *sinfo=&vinfo->samp[vinfo->samp_num[k]];
	 printf("smp=%3d fft=%3d (%s %s)\n",vinfo->samp_num[k],sinfo->fft_id,sinfo->ant,sinfo->band);
       }
       printf("\n");
       for(k=0;k<linfo.stab[i].recs;k++)
	 {  printf("%d ",k);
	   for(j=0;j<vinfo->samplers;j++)
	     {SmpSolType *smp_sol=&solpar.smp_sol[k][j];
	       printf("%12.4e %6.1f %8.2f %6.1f",CMPLX_AMP((&smp_sol->gain)),RAD2DEG(CMPLX_PHS((&smp_sol->gain))),
		      smp_sol->delay,RAD2DEG(smp_sol->phi0));
	       /*printf("%9.2f ",smp_sol->delay);*/
	     }
	   printf("\n");
	 }
     }
   }

   if(XtractFmt){fclose(fp1);fclose(fp2);}
   return 0;
}

int main(int argc, char **argv)
{
  optrega(&LtaFile,     OPT_STRING,'i',"in",      "Input LTA File");
  optrega(&XtractFmt,   OPT_FLAG,  'X',"xtract",  "XTRACT compatible output fmt");
  optrega(&RefAnt,      OPT_STRING,'r',"ref",      "Reference Antenna");
  optrega(&FilterLength,OPT_INT,   'a',"average",  "average solutions over specified nrecs");

  optrega_array(&SelScans,&SelScan,OPT_INT,'S', "scans","Scans to solve");

  optMain(ltasol);
  opt(&argc,&argv);
  sprintf(Usage,"Usage: %s -i LtaFile [-r refant][-S scan0,scan1,...][-X xtractfmt][-a nrec]\n",argv[0]);
  
  optUsage(Usage);
  
  optTitle("Computes antenna based gain solutions");

  return ltasol(argc,argv);
}

