/*
    corrsub.c   :  Miscellaneous library routines for handling Corr
                   includes text generation from binary,
                   Endian Conversion, etc...

                   EXTENSIVELY REVISED VERSION /crs 99apr08

                   Requires newcorr.h  which is incompatible
                   with the old corr.h file.  Many structures
                   have also undergone extensive revision

   Think carefully before editing Keywords defined in this file !
*/
# include <stdio.h>
# include <string.h>
# include <ctype.h>
# include <limits.h>
# include <stdlib.h>
# include "newcorr.h"

enum {Len = 128, Hdrlen = 64*1024};
static char Buf[Hdrlen], *Bufp ;
static const char cont_char = '*', begin_char = '{', end_char = '}';

enum {Version, Ants, Chan_max, Pols, Samplers, Macs, F_step, Clock,
      Sta, StaTime, T_unit, Rf, First_lo, Bb_lo, Antmask, Bandmask,
      Mode, Seq, Object, Observer, Project, Code,
      Dra, Ddec, Ra_app, Dec_app, Ra_mean, Dec_mean, MJD, Date_obs,
      Lta, Samp_num, Chan_num, Mac_num, Integ, 
       Mac_mode, Fft_mode, Dpc_mux, Clk_sel, Net_sign,Vars};
static char *varname[Vars] =
     {"VERSION","ANTS","CHAN_MAX","POLS","F_ENABLE","X_ENABLE","F_STEP","CLOCK",
      "STA","STATIME","T_UNIT","RF","FIRST_LO","BB_LO","ANTMASK","BANDMASK",
      "MODE","SEQ","OBJECT","OBSERVER","PROJECT","CODE",
      "DRA/DT","DDEC/DT","RA-APP","DEC-APP","RA-MEAN","DEC-MEAN","MJD_SRC",
      "DATE-OBS","LTA","SAMP_NUM", "CHAN_NUM","MAC_NUM","INTEG",
      "MAC_MODE", "FFT_MODE", "DPC_MUX", "CLK_SEL", "NET_SIGN"};

int    str_cmp(const char *s1, const char *s2);
int    search_str(char *s1, char **s2, int n);
int    get_range(char *str, short *arr, int max);

int get_soft_baselines(CorrType *corr)
{ int          i,j;
  BaseParType *base=corr->baseline;

  corr->daspar.baselines=0;
  for(i=0;i<MAX_SAMPS;i++)
  { if(corr->sampler[i].dpc==MAX_SAMPS) continue;
    for(j=i;j<corr->daspar.samplers;j++)
    { if(corr->sampler[j].dpc==MAX_SAMPS) continue;
      memcpy(&base->samp[0],&corr->sampler[i],sizeof(SamplerType));
      memcpy(&base->samp[1],&corr->sampler[j],sizeof(SamplerType));
      base++;corr->daspar.baselines++;
    }
  }
  return corr->daspar.baselines ;
}

int get_soft_sampler(FILE *f, CorrType *corr)
{  
  int               ant_id, seq=0, id, band;
  unsigned long     antmask = 0 ;
  char              str1[16],str2[16], *p;
  AntennaParType   *antenna = corr->antenna;
  SamplerType      *samp    = corr->sampler;
  DasParType       *daspar  =&corr->daspar ;
  char             *band_name[MAX_BANDS];

  band_name[0] =  corr->bandname[0];
  band_name[1] =  corr->bandname[1];
  band_name[2] =  corr->bandname[2];
  band_name[3] =  corr->bandname[3];
  daspar->samplers = 0 ;
  daspar->bandmask = 0 ;

  for (id=0; id < MAX_SAMPS; id++)
  { samp[id].ant_id = MAX_ANTS;
    samp[id].band = MAX_BANDS ;
    samp[id].fft_id = MAX_FFTS;
    samp[id].dpc = MAX_SAMPS ;
  }

  seq = 0 ;
  while ((p = fgets(Bufp, Len, f)) != NULL)
  { Bufp += strlen(Bufp);
    if (*p == cont_char) continue;
    if (*p == end_char) break;

    if (strncmp(p, "SMP", 3) != 0) continue;
    sscanf(p+3,"%d", &id);
    if (id >= MAX_SAMPS)
    { 
      fprintf(stderr,"ignoring BAD sampler %d\n",id) ;
      continue ;
    }
    sscanf(p+10,"%s %s", str1, str2 );
    /* antenna_name, band_name  to which it is connected */
    fprintf(stdout, "%s %s\n", str1, str2);
    for (ant_id=0; ant_id < MAX_ANTS; ant_id++)
      if (str_cmp(str1, antenna[ant_id].name) == 0) break ;
    if (ant_id == MAX_ANTS ) continue ;
    band = search_str(str2, band_name, MAX_BANDS);
    if (band == MAX_BANDS)
    { fprintf(stderr,"IF_CON: Illegal band-name %s ignore\n",str2); continue ;}

    antmask = antmask | (1 << ant_id) ;
    daspar->bandmask |= (1 << band) ;

    samp = corr->sampler + daspar->samplers ;
    samp->ant_id = ant_id;
    samp->fft_id = daspar->samplers;
    samp->band   = band ;
    daspar->samplers++;
  }
  daspar->antmask = antmask ;
  return 0;
}

void get_soft_selection(FILE *f, CorrType *corr)
{ int          i,j,n, val;
  short        samp_num[MAX_SAMPS] ;
  char         str[128], *p;
  DasParType  *daspar = &corr->daspar ;
  CorrParType *corrpar = &corr->corrpar ;

  while (fgets(Bufp, Len, f))
  { p = Bufp;
    while (*Bufp) Bufp++;
    if (*p == cont_char) continue;
    if (*p == end_char) break;
    if (sscanf(p,"%s", str) == 0) continue;
    str[8] = 0;
    n = search_str(str, varname, Vars);
    if (n == Vars) continue;
    switch (n)
    {
      case Clk_sel  : sscanf(p+10,"%d", &val ) ;
                      corrpar->clksel = val ;  break ;

      case Lta      : sscanf(p+10,"%d", &daspar->lta  ); continue ;
      case Chan_num : daspar->channels = get_range(p+10, daspar->chan_num,MAX_CHANS);
					fprintf(stdout, "CHANNELS = %d\n", daspar->channels);
		      continue ;
      case Samp_num : daspar->samplers = get_range(p+10, samp_num,MAX_SAMPS);
  	              for(i=0;i<MAX_SAMPS;i++)
		      { SamplerType *samp=&corr->sampler[i];
		        for(j=0;j<daspar->samplers;j++)
			  if(samp_num[j]==i)break;
			if(j==daspar->samplers)samp->dpc=MAX_SAMPS;
		      }
		      continue ;
      default       : break ;
    }
  }
  get_soft_baselines(corr);
  return;
}
int scan_hdr_parse(char *scanfile, ScanInfoType *scan)
{ char *p, str[128],buf[128] ;
  FILE *fp ;
  SourceParType *source = &scan->source ;
  ProjectType *proj = &scan->proj ;
  static double rpd = 3.14159265359/180 ;
  int n, rf[2],first_lo[2], bb_lo[2] ;
  
  fp = fopen(scanfile,"rt") ;
  if (fp == NULL) 
      { printf(" No scan header !\n") ; return -1 ;}

  source->flux.i = 1.0 ;
  source->flux.q = source->flux.u = source->flux.v = 0 ;
  source->lsrvel[0] = source->lsrvel[1] =0.0 ;
  source->rest_freq[0] = source->rest_freq[1] =0.0 ;
  source->calcode = source->qual = source->id = 0 ;
  source->mode=0;

  while ( (p = fgets(buf,80,fp)) != NULL)
  { double val ;
    if ( (*p == end_char) || (p[1] == end_char) )break ;
    if ( strncmp(p,"END_OF",6) == 0 ) break;
    if (*p == cont_char) continue;
    if (sscanf(p,"%s", str) == 0) continue;
    str[8] = 0;
    n = search_str(str, varname, Vars);
    if (n == Vars) continue;
    switch (n)
    { case Object  : sscanf(p+10,"%s",  source->object  ) ;
                  /* printf("%d: found %s\n",id,scan->object); */  break ;
      case Observer: sscanf(p+10,"%s",  proj->observer) ;  break ;
      case Project : sscanf(p+10,"%s",  proj->title ) ;  break ;
      case Code    : sscanf(p+10,"%s",  proj->code    ) ;  break ;
      case Antmask : sscanf(p+10,"%x", &proj->antmask)  ;  
	             source->antmask=proj->antmask;  break ; 
      case Bandmask: sscanf(p+10,"%hx", &proj->bandmask) ;
	             source->bandmask=proj->bandmask; break ;
      case Seq     : sscanf(p+10,"%hu", &proj->seq    )  ;  break ;
 
      case Ra_app  : sscanf(p+10,"%lf",&val);
                     source->ra_app = rpd * val ; break;
      case Dec_app : sscanf(p+10,"%lf",&val);
                     source->dec_app = rpd * val; break;
      case Ra_mean : sscanf(p+10,"%lf",&val);
                     source->ra_mean = rpd * val ; break;
      case Dec_mean: sscanf(p+10,"%lf",&val);
                     source->dec_mean = rpd * val; break;
      case MJD     : sscanf(p+10,"%lf",&source->mjd0 ) ; break;
      case Dra     : sscanf(p+10,"%lf",&val);
                     source->dra  = rpd * val ; break;
      case Ddec    : sscanf(p+10,"%lf",&val);
                     source->ddec  = rpd*val ; break;

      case Rf      : sscanf(p+10,"%d %d",&rf[0],&rf[1]); 
	             source->freq[0]=rf[0];source->freq[1]=rf[1]; break ;
      case First_lo: sscanf(p+10,"%d %d", &first_lo[0],&first_lo[1] ); 
	             source->first_lo[0]=first_lo[0];source->first_lo[1]=first_lo[1]; break ;
      case Bb_lo   : sscanf(p+10,"%d %d", &bb_lo[0],&bb_lo[1]); 
                     source->bb_lo[0]=bb_lo[0];source->bb_lo[1]=bb_lo[1]; break ;

      default      : break ;
    }
  }

  return 0 ;
}
int softcorr_hdr(char *filename, CorrType *corr)
{ char *p, str[32];
  FILE *f;
  int i,new_section ;
  char *band_name[MAX_BANDS] ={ "USB-130", "USB-175", "LSB-130", "LSB-175" } ;
  int   get_antenna(FILE *f, AntennaParType *antenna);
  int   get_sampler(FILE *f, CorrType *corr);
  void  copy_text(FILE *f);

  for (i=0; i<MAX_BANDS; i++) strcpy(corr->bandname[i],band_name[i]) ;
  fprintf(stdout, "INSIDE softcorr_hdr\n");
  Bufp = Buf ;
  f = fopen(filename, "rt");
  if (f == NULL) return -1;
	fprintf(stdout, "In get corrsel.hdr\n");
  while (fgets(Bufp, Len, f))
  { if (*Bufp == cont_char) continue;
    while (isspace(*Bufp)) Bufp++;
    if (*Bufp == 0) continue;
    p = Bufp; Bufp += strlen(Bufp);
    if (*p == begin_char)
    { p++; sscanf(p,"%s",str);
      new_section=0;
      if (str_cmp(str,"Antenna.def") == 0)
      { new_section=1; get_antenna(f, corr->antenna);}
      if (str_cmp(str,"Sampler.def") == 0)
      { new_section=1; get_sampler(f, corr);fprintf(stdout, "CALLING get_sampler routine\n");}
      if (str_cmp(str,"Corrsel.def") == 0)
      { new_section=1; get_soft_selection(f, corr);fprintf(stdout,"SOFTCORR SELECTION\n");}
      if(!new_section) copy_text(f);
    }
    else if (strncmp(p,"END_OF",6) == 0) break;
  }
  fclose(f);
  return 0;
}
