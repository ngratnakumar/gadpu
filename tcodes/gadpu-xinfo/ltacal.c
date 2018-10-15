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


static char qualcode[8]={'P','A','B','F','X','X','X','X'};
Complex cmplxZtmp;
float   floatZtmp;

char *LtaFile=NULL;
char  Usage[256];
float PhaseStabilityThreshold=0.90;


int ltacal(int argc, char **argv)
{ LtaInfo linfo;
  FILE    *fp,*ofp;
  int      i,j,k;
  char     outfile[256];
  char     qual[8];
  VisInfo  *vinfo=&linfo.lhdr.vinfo;
  SolParType solpar;

  if(LtaFile==NULL) { fprintf(stderr,Usage); return 1;}
  if((fp=fopen(LtaFile,"r"))==NULL)
  { fprintf(stderr,"Cannot open %s\n",LtaFile); return 1;}
  strcpy(outfile,"ltacal_out.lta");
  if((ofp=fopen(outfile,"w"))==NULL)
  { fprintf(stderr,"cannot open %s\n",outfile); return 1;}
  
  { int t=1;
    if (*((unsigned char *)&t) == 1) linfo.local_byte_order = LittleEndian;
    else linfo.local_byte_order=BigEndian;
  }
  if(get_lta_hdr(fp,&linfo)) return 1;
  if(make_scantab(fp,&linfo))return 1;
  rewind(fp);

   if(linfo.local_byte_order!=linfo.data_byte_order)solpar.change_endian=1;
   else solpar.change_endian=0;

   write_hdr(&linfo,ofp);

   if(init_solpar(&solpar,&linfo)) return 1;
   for(i=0;i<linfo.scans;i++)
  { SourceParType *src=&linfo.stab[i].scan.source;
    write_scan(&linfo.stab[i].scan,1,&linfo,i,linfo.stab[i].shdr.date_obs,ofp);
    rewind(fp); ltaseek(fp,linfo.stab[i].start_rec+linfo.srecs,linfo.recl);
    for(j=0,k=0;j<4;j++) 
      if(1<<j&src->qual){qual[k]=qualcode[j];k++;}
    qual[k]='\0';
    if(src->calcode)
    { if(index(qual,'B') !=NULL) /* bandpass calibrator */
      { fprintf(stderr,"%s : Compute Bandpass\n", linfo.stab[i].scan.source.object);
        solve_all_chans(&linfo,i,fp,ofp,&solpar);
	get_normalized_bp(vinfo,&solpar);
      }else{ /* Phase calibrator */
	fprintf(stderr,"%s : Compute Chan0 gain\n",linfo.stab[i].scan.source.object);
	solve_chan0(&linfo,i,fp,ofp,&solpar);
      }
    }else{ /* target source */
      fprintf(stderr,"%s : Apply last gain\n", linfo.stab[i].scan.source.object);
      compute_total_gain(vinfo, solpar.samp_bp,solpar.samp_gain,solpar.samp_bpgain);
      apply_gain_to_scan(&linfo,i,fp,ofp,solpar.samp_bpgain,solpar.change_endian,solpar.bp_start_chan,
			 solpar.bp_end_chan);
    } 
  }


  return 0;
}

int main(int argc, char **argv)
{
  optrega(&LtaFile,OPT_STRING,'i',"in", "Input LTA File");
  optMain(ltacal);
  opt(&argc,&argv);

  sprintf(Usage,"Usage: %s -i LtaFile [-b BPLtaFile] [-s scan] [-S BPscan]\n[-a antenna][-d delectedantenna]\n",argv[0]);
  
  optUsage(Usage);
  
  optTitle("Averages Selected Self Correlations");

  return ltacal(argc,argv);
}



  
    /*  for(chan=0;chan<vinfo->channels;chan++)
        { for(j=18;j<19;j++)
          { if(nsol[chan][j]>0)
            { float a,p;
              Complex *g=&samp_gain[chan][j];
              a=CMPLX_AMP((g));p=CMPLX_PHS((g));
              fprintf(stderr,"%s%s AMP %12.4e PHS %10.2f\n",vinfo->samp[vinfo->samp_num[j]].ant, 
                     vinfo->samp[vinfo->samp_num[j]].band,a,RAD2DEG(p));
            }
            else  fprintf(stderr,"%s%s Flagged\n",vinfo->samp[vinfo->samp_num[j]].ant,
                     vinfo->samp[vinfo->samp_num[j]].band);
	   }
        }
    */



/* from apply_gain (saved for cross checking) 

    { for(k=0;k<vinfo->baselines;k++)
      {  short *rev_num=vinfo->samp_rev_num;
          int s0=rev_num[vinfo->base[k].samp[0]],s1=rev_num[vinfo->base[k].samp[1]];
	  Complex *d=(Complex *)(dbuf+linfo->lhdr.data_off+k*vinfo->channels*2*sizeof(float));
	  Complex c,c1;
	  for(chan=0;chan<vinfo->channels;chan++)
	  { Complex *g0=&samp_gain[chan][s0];
	    Complex *g1=&samp_gain[chan][s1];
	    if(CMPLX_AMP(g0)>0.0 && CMPLX_AMP(g1)>0.0)
	    { CMPLX_CONJ_MUL(g0,g1,(&c));
	      CMPLX_DIV(d,(&c),(&c1));
	      d->re=c1.re;d->im=c1.im;
	    }else{
	      int f_off=k*vinfo->channels+chan;
	      char *flag=dbuf+linfo->lhdr.flg_dat_off+f_off/8;
	      *flag |= 1<<(f_off%8);
	    }
	    d++;
	  }
      }
*/
