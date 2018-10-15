#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <newcorr.h>
#include <protocol.h>
#include <lta.h>

//static char rcsid[]="$Id: hdrsubs.c,v 1.5 2005/11/22 10:08:29 das Exp $";


static char com_char = '*', begin_char = '{', end_char = '}';
enum { HdrType, ScanType, DataType, RecTypes };
char *InitName="Init", *CorrName="AddCorr", *ScanName="ExtraScan";
enum { RECL, HDR_RECS, REC_FORM, OBS_MODE, VERSION, InitKeys };
enum { KEYLEN=8+1, LineLen = 80};   /* Jan 1999/crs  */
enum { ANTENNAS, SAMPLERS, BASELINE, CHANNELS, CorrKeys };
enum { BAD_RECS, BAD_ANTS, BAD_SAMP, BAD_BASE, BAD_CHAN, ScanKeys };
enum { MACHINE, D_TYPE, D_SIZE, D_ALIGN, INT_REP, FL_REP, BYTE_SEQ, MacKeys };

char *MacKey [MacKeys ] =
 { "MACHINE", "D_TYPE", "D_SIZE", "D_ALIGN", "INT_REP", "FL_REP", "BYTE_SEQ" };
char *RecCode[RecTypes] = {"HDR ", "SCAN", "DATA"};
char *InitKey[InitKeys] =
  { "RECL", "HDR_RECS", "REC_FORM", "OBS_MODE", "VERSION" };
char *CorrKey[CorrKeys] = { "ANTENNAS", "SAMPLERS", "BASELINE", "CHANNELS" };
char *ScanKey[ScanKeys] = {"BAD_RECS", "BAD_ANTS", "BAD_SAMP", "BAD_BASE", "BAD_CHAN" };

MacWordType Mac[MAX_BASE] ;

int corrgen(CorrType *corr, char *p) ;
int put_scanpar (int id, int seq, ScanInfoType *scan, char *p) ;
int print_range(char *str, short *val, int n ) ;

typedef enum
{ Char, Short, Int, Long, Float, Double, BaseTypes } BaseDataType;
char *DataTypeName[] =
{"char", "short", "int", "long", "float", "double" };

typedef struct
{ int type_size[BaseTypes], type_align[BaseTypes];
  int twos_comp, float_ieee, big_endian;
} MachineSpecType;
MachineSpecType MachineSpec;

enum{MIN_CHAN=16};
void GetMachineSpec(MachineSpecType *mac)
{
  struct { char x; char y;        } a1;
  struct { char x; short y;       } a2;
  struct { char x; int   y;       } a3;
  struct { char x; long y;        } a4;
  struct { char x; float y;       } a5;
  struct { char x; double y;      } a6;
  /* struct { char x; char* y;       } a7; */

  mac->type_size[Char]       = sizeof(char);
  mac->type_size[Short]      = sizeof(short);
  mac->type_size[Int]        = sizeof(int);
  mac->type_size[Long]       = sizeof(long);
  mac->type_size[Float]      = sizeof(float);
  mac->type_size[Double]     = sizeof(double);

# define align(a) (int)((char*)&(a.y) - (char*)&(a.x))
  mac->type_align[Char]       = align(a1);
  mac->type_align[Short]      = align(a2);
  mac->type_align[Int]        = align(a3);
  mac->type_align[Long]       = align(a4);
  mac->type_align[Float]      = align(a5);
  mac->type_align[Double]     = align(a6);

  mac->twos_comp = ((unsigned short)(-1) == 65535U);
  { unsigned short big_end=1;
    mac->big_endian= *(unsigned char *)&big_end == 0;
  }
  { int i, j;
    float f[3] = { 1.2345, -1.2345, -1.2345e-42 };
    unsigned char big_rep[3][4] =
    { {0x3f, 0x9e, 0x04, 0x19},
      {0xbf, 0x9e, 0x04, 0x19},
      {0x80, 0x00, 0x03, 0x71}
    };
    unsigned char *f_rep = (unsigned char*)f;
    for (i=0; i < 3; i++)
    { if (mac->big_endian)
      { for (j=0; j < 4; j++) if (big_rep[i][j] != f_rep[i*4+j]) break; }
      else
      { for (j=0; j < 4 ; j++) if (big_rep[i][j] != f_rep[i*4+3-j]) break; }
      if (j != 4) break;
      /* for (j=0; j < 4; j++) printf("%02x ", *(f_rep+i*4+j)); */
    }
    if (i == 0)      mac->float_ieee = 0;
    else if (i == 3) mac->float_ieee = 1;
    else             mac->float_ieee = 2;
  }
}
char *fill_line(char *p)
{ int i, l;
  l = strlen(p);
  for(i=0;i<l;i++)
    if(p[i] =='\n') /* Delete Illegal NewLine Char */
      p[i]=' '; 
  if (l < LineLen)
    for (i=l; i < LineLen-1; i++) p[i] = ' ';
  p[LineLen-1] = '\n';
  return p+LineLen;
}
char *getln(char *line, char *buf)
{ char *p=buf;
  buf = strchr(buf,'\n');
  if (buf == NULL) return NULL;
  strncpy(line, p, buf-p);
  line[buf-p] = 0;
  return buf+1;
}
void copy_text_ln(char *from, char *to, char **into)
{ char *p=from, *bp = *into;
  while (p < to)
  { p = getln(bp, p);
    if (*bp == begin_char || *bp == end_char)
    { int l = strlen(bp)+1;
      while (l >= 0) { bp[l+1] = bp[l]; l--; }
      *bp = com_char;
    }
    else if (isspace(*bp)) continue;
    bp = fill_line(bp);
  }
  *into = bp;
}
void write_hdr(LtaInfo *linfo, FILE *ofp)
{ BaseDataType type;
  int i, l, corr_recs ,hdr_recs_off;
  char *bp, *HBuf, *bufp ;
  LtaHdr   *lhdr=&linfo->lhdr;
  VisInfo  *vinfo=&lhdr->vinfo;
  CorrType *corr=&linfo->corr,corr1;
  char     version[NAMELEN];

  GetMachineSpec(&MachineSpec);

  if((bp=HBuf=malloc(2*DAS_BUFSIZE))==NULL)
  { fprintf(stderr,"Malloc Failure\n"); exit(-1);}
  bufp = HBuf + 2*(DAS_BUFSIZE - 100000) ;  /* scratch for  corr.hdr */
  sprintf(bp,"%4s%-12d",RecCode[HdrType], linfo->recl); bp = fill_line(bp) ;

  sprintf(bp,"*{ %s.def",InitName); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d",InitKey[RECL    ],linfo->recl    ); bp = fill_line(bp);
  hdr_recs_off = bp-HBuf;
  sprintf(bp,"%-8s= %d",InitKey[HDR_RECS],linfo->lrecs); bp = fill_line(bp);
  sprintf(bp,"%-8s= %s",InitKey[REC_FORM],lhdr->rec_form); bp = fill_line(bp);
  sprintf(bp,"%-8s= %s",InitKey[OBS_MODE],lhdr->obs_mode); bp = fill_line(bp);

  if(linfo->keep_version) strcpy(version,linfo->lhdr.old_version);
  else strcpy(version,linfo->lhdr.version);

  sprintf(bp,"%-8s= %s",InitKey[VERSION ],version); bp = fill_line(bp);
  l = sprintf(bp,"%-8s= ",MacKey[D_TYPE  ]);
  for (type=Char; type < BaseTypes; type++)
    l += sprintf(bp+l," %s", DataTypeName[type]); bp = fill_line(bp);
  l = sprintf(bp,"%-8s= ",MacKey[D_SIZE  ]);
  for (type=Char; type < BaseTypes; type++)
    l += sprintf(bp+l," %d", MachineSpec.type_size[type]); bp = fill_line(bp);
  l = sprintf(bp,"%-8s= ",MacKey[D_ALIGN ]);
  for (type=Char; type < BaseTypes; type++)
    l += sprintf(bp+l," %d", MachineSpec.type_align[type]); bp = fill_line(bp);
  sprintf(bp,"%-8s= %s Twos Comp",MacKey[INT_REP ],
          MachineSpec.twos_comp ? "" : "NOT "); bp = fill_line(bp);
  sprintf(bp,"%-8s= %s IEEE",MacKey[FL_REP  ],
          MachineSpec.float_ieee  == 0 ? "NOT " :
         (MachineSpec.float_ieee == 1 ? "" : "Partial ")); bp = fill_line(bp);
  sprintf(bp,"%-8s= %s Endian",MacKey[BYTE_SEQ],
          MachineSpec.big_endian ? " Big" : " Little"); bp = fill_line(bp);
  sprintf(bp,"*} %s",InitName); bp = fill_line(bp);

  sprintf(bp,"*{ %s.def",CorrName); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d",CorrKey[ANTENNAS],vinfo->antennas); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d",CorrKey[SAMPLERS],vinfo->samplers); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d",CorrKey[BASELINE],vinfo->baselines);bp = fill_line(bp);
  sprintf(bp,"%-8s= %d",CorrKey[CHANNELS],vinfo->channels); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d","FLG_OFF ",lhdr->flg_off     ); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d","FLG_SIZE",lhdr->flg_size    ); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d","FLGRECOF",lhdr->flg_rec_off ); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d","FLGRECSZ",lhdr->flg_rec_size); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d","FLGANTOF",lhdr->flg_ant_off ); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d","FLGANTSZ",lhdr->flg_ant_size); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d","FLGSMPOF",lhdr->flg_smp_off ); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d","FLGSMPSZ",lhdr->flg_smp_size); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d","FLGBASOF",lhdr->flg_bas_off ); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d","FLGBASSZ",lhdr->flg_bas_size); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d","FLGDATOF",lhdr->flg_dat_off ); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d","FLGDATSZ",lhdr->flg_dat_size); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d","TIME_OFF",lhdr->time_off    ); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d","TIMESIZE",lhdr->time_size   ); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d","WT_OFF  ",lhdr->wt_off      ); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d","WT_SIZE ",lhdr->wt_size     ); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d","PAR_OFF ",lhdr->par_off     ); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d","PAR_SIZE",lhdr->par_size    ); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d","DATA_OFF",lhdr->data_off    ); bp = fill_line(bp);
  sprintf(bp,"%-8s= %d","DATASIZE",lhdr->data_size   ); bp = fill_line(bp);
  sprintf(bp,"%-8s= %s","DATAFMT ",lhdr->data_fmt    ); bp = fill_line(bp);
  sprintf(bp,"*} %s",CorrName); bp =fill_line(bp);

  memcpy(&corr1,&linfo->corr,sizeof(CorrType));
  if(linfo->keep_version) strcpy(corr1.version,linfo->lhdr.vinfo.corrlite.old_version);
  corrgen(&corr1, bufp) ;

  { char *endp = strstr(bufp, "END_OF_")-1;
    if (endp) copy_text_ln(bufp, endp, &bp);
  }
  sprintf(bp,"%s","END_OF_HEADER"); bp = fill_line(bp);
  l = (bp-HBuf)/linfo->recl; if ((bp-HBuf) % linfo->recl) l++;
  for (i=bp-HBuf; i < l*linfo->recl;) HBuf[i++] = 0; HBuf[i-1] = '\n';
//  if(strstr(linfo->lhdr.version,"COR30x2 ")==NULL && strstr(linfo->lhdr.version,"HST03") ==NULL && strstr(linfo->lhdr.version,"HST04") == NULL)
  if(linfo->keep_version && strstr(linfo->lhdr.old_version,"COR30x2")==NULL && strstr(linfo->lhdr.old_version,"HST03")==NULL && strstr(linfo->lhdr.old_version,"HST04")==NULL)
  { corr_ref_to_corr2(linfo,&corr1);
#ifdef DEBUG_MODE
     fprintf(stdout, "^^^^^^^^^^  REF - CORR2 ^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
#endif
     memcpy(HBuf+l*linfo->recl, &corr1, Corr2Size);
     corr_recs = (Corr2Size+linfo->recl-1)/linfo->recl;
  } else if(linfo->keep_version && strstr(linfo->lhdr.old_version,"COR30x2")!=NULL && strstr(linfo->lhdr.old_version,"HST03")==NULL)
  { corr_ref_to_corr3(linfo,&corr1); 
#ifdef DEBUG_MODE
     fprintf(stdout, "^^^^^^^^^^^^^  REF - CORR3 ^^^^^^^^^^^^^^^^^^^^^^^^\n");
#endif
     memcpy(HBuf+l*linfo->recl, &corr1, Corr3Size);
     corr_recs = (Corr3Size+linfo->recl-1)/linfo->recl;
  } else if(strstr(linfo->lhdr.old_version,"COR30x2")!=NULL && strstr(linfo->lhdr.old_version,"HST03")==NULL) // logic from ltasub.c
  { corr_ref_to_corr3(linfo,&corr1); 
#ifdef DEBUG_MODE
     fprintf(stdout, "^^^^^^^^^^^^^  REF - CORR3 ^^^^^^^^^^^^^^^^^^^^^^^^\n");
#endif
     memcpy(HBuf+l*linfo->recl, &corr1, Corr3Size);
     corr_recs = (Corr3Size+linfo->recl-1)/linfo->recl;
  } else if(linfo->keep_version && strstr(linfo->lhdr.old_version,"COR30x2")!=NULL && strstr(linfo->lhdr.old_version,"HST03")!=NULL)
  { corr_ref_to_corr4(linfo,&corr1);
#ifdef DEBUG_MODE
     fprintf(stdout, "^^^^^^^^^^^^^^  REF - CORR4/GSB ^^^^^^^^^^^^^^^^^^^^^^\n");
#endif
     memcpy(HBuf+l*linfo->recl, &corr1, Corr4Size);
     corr_recs = (Corr4Size+linfo->recl-1)/linfo->recl;
//     fprintf(stderr, "REC LEN = %d  CorrSize =%ld, Corr4Size=%d l = %d corr_recs=%d\n", linfo->recl, sizeof(CorrType), Corr4Size, l, corr_recs);
  } else if(linfo->keep_version && strstr(linfo->lhdr.old_version,"GWB-III")!=NULL && strstr(linfo->lhdr.old_version,"HST04")!=NULL)
  { // linfo->corr.daspar.dut1 = 0.7777; 
    corr_ref_to_corr5(linfo,&corr1);
#ifdef DEBUG_MODE
     fprintf(stdout, "!!!!!!!!!!!!!!  CORR REF -> CORR5  !!!!!!!!!!!!!!!!!!!!!!\n");
#endif

     memcpy(HBuf+l*linfo->recl, &corr1, Corr5Size);
     corr_recs = (Corr5Size+linfo->recl-1)/linfo->recl;
//     fprintf(stderr, "REC LEN = %d  CorrSize =%ld, Corr5Size=%d l = %d corr_recs = %d\n", linfo->recl, sizeof(CorrType), Corr5Size, l, corr_recs);
#ifdef DEBUG_MODE
  fprintf(stdout, "===========================================================================\n");
  fprintf(stdout, "===========================================================================\n");
  fprintf(stdout, "<<<< ==== >>>>>>>>>  ANTENNA   = %s\n", corr1.antenna[1].name);
  fprintf(stdout, "<<<< ==== >>>>>>>>>  ANT DLY   = %lf\n", corr1.antenna[1].d0[0]);
  fprintf(stdout, "<<<< ==== >>>>>>>>>  ANT BX    = %lf\n", corr1.antenna[1].bx);
  fprintf(stdout, "<<<<===== >>>>>>>>>  ANT BY    = %lf\n", corr1.antenna[1].by);
  fprintf(stdout, "<<<<===== >>>>>>>>>  ANT BZ    = %lf\n", corr1.antenna[1].bz);

  fprintf(stdout, "\n<<<<======= >>>>>>>>>  SAMP ANT  = %d\n", corr1.sampler[2].ant_id);
  fprintf(stdout, "<<<<======= >>>>>>>>>  SAMP BAND = %d\n", corr1.sampler[2].band);
  fprintf(stdout, "<<<<======= >>>>>>>>>  SAMP FFT  = %d\n", corr1.sampler[2].fft_id);
  fprintf(stdout, "<<<<======= >>>>>>>>>  SAMP DPC  = %d\n", corr1.sampler[2].dpc);

  fprintf(stdout, "\n<<<<===== >>>>>>>>>  MACS      = %d\n", corr1.corrpar.macs);
  fprintf(stdout, "<<<<<==== >>>>>>>>>  CHANNELS  = %d\n", corr1.corrpar.channels);
  fprintf(stdout, "<<<<<==== >>>>>>>>>  POLS      = %d\n", corr1.corrpar.pols);
  fprintf(stdout, "<<<<<==== >>>>>>>>>  STA       = %d\n", corr1.corrpar.sta);
  fprintf(stdout, "<<<<<==== >>>>>>>>>  STATIME   = %d\n", corr1.corrpar.statime);
  fprintf(stdout, "<<<<<==== >>>>>>>>>  F_STEP    = %lf\n", corr1.corrpar.f_step);
  fprintf(stdout, "<<<<<==== >>>>>>>>>  CLOCK     = %lf\n", corr1.corrpar.clock);

  fprintf(stdout, "\n<<<<<<=== >>>>>>>>>  ANTMASK   = 0X%x\n", corr1.daspar.antmask);
  fprintf(stdout, "<<<<<===== >>>>>>>>>  SAMPLERS  = %d\n", corr1.daspar.samplers);
  fprintf(stdout, "<<<<<===== >>>>>>>>>  BASELINES = %d\n", corr1.daspar.baselines);
  fprintf(stdout, "<<<<<===== >>>>>>>>>  CHANNELS  = %d\n", corr1.daspar.channels);
  fprintf(stdout, "<<<<<===== >>>>>>>>>  LTA       = %d\n", corr1.daspar.lta);
  fprintf(stdout, "<<<<<===== >>>>>>>>>  BANDMASK  = %d\n", corr1.daspar.bandmask);
  fprintf(stdout, "<<<<<===== >>>>>>>>>  MODE      = %d\n", corr1.daspar.mode);
  fprintf(stdout, "#######== >>>>>>>>>  CHAN[127  ]= %d\n", corr1.daspar.chan_num[127]);
  fprintf(stdout, "#######== >>>>>>>>>  CHAN[130  ]= %d\n", corr1.daspar.chan_num[130]);
  fprintf(stdout, "#######== >>>>>>>>>  CHAN[1000 ]= %d\n", corr1.daspar.chan_num[1000]);
  fprintf(stdout, "<<<<<==== >>>>>>>>>  MJD_REF   = %lf\n", corr1.daspar.mjd_ref);
  fprintf(stdout, "<<<<<==== >>>>>>>>>  T_UNIT    = %f\n", corr1.daspar.t_unit);
  fprintf(stdout, "<<<<<==== >>>>>>>>>  DUT1      = %15.12lf \n", (corr1.daspar.dut1)*(12.0*3600/M_PI));
  fprintf(stdout, "===========================================================================\n");
  fprintf(stdout, "===========================================================================\n");
  fprintf(stdout, "========= >>>>>>>>>  ANTENNA   = %s\n", linfo->corr.antenna[1].name);
  fprintf(stdout, "========= >>>>>>>>>  ANT DLY   = %lf\n", linfo->corr.antenna[1].d0[0]);
  fprintf(stdout, "========= >>>>>>>>>  ANT BX    = %lf\n", linfo->corr.antenna[1].bx);
  fprintf(stdout, "========= >>>>>>>>>  ANT BY    = %lf\n", linfo->corr.antenna[1].by);
  fprintf(stdout, "========= >>>>>>>>>  ANT BZ    = %lf\n", linfo->corr.antenna[1].bz);

  fprintf(stdout, "\n=========== >>>>>>>>>  SAMP ANT  = %d\n", linfo->corr.sampler[2].ant_id);
  fprintf(stdout, "=========== >>>>>>>>>  SAMP BAND = %d\n", linfo->corr.sampler[2].band);
  fprintf(stdout, "=========== >>>>>>>>>  SAMP FFT  = %d\n", linfo->corr.sampler[2].fft_id);
  fprintf(stdout, "=========== >>>>>>>>>  SAMP DPC  = %d\n", linfo->corr.sampler[2].dpc);

  fprintf(stdout, "\n========= >>>>>>>>>  MACS      = %d\n", linfo->corr.corrpar.macs);
  fprintf(stdout, "========= >>>>>>>>>  CHANNELS  = %d\n", linfo->corr.corrpar.channels);
  fprintf(stdout, "========= >>>>>>>>>  POLS      = %d\n", linfo->corr.corrpar.pols);
  fprintf(stdout, "========= >>>>>>>>>  STA       = %d\n", linfo->corr.corrpar.sta);
  fprintf(stdout, "========= >>>>>>>>>  STATIME   = %d\n", linfo->corr.corrpar.statime);
  fprintf(stdout, "========= >>>>>>>>>  F_STEP    = %lf\n", linfo->corr.corrpar.f_step);
  fprintf(stdout, "========= >>>>>>>>>  CLOCK     = %lf\n", linfo->corr.corrpar.clock);

  fprintf(stdout, "\n========= >>>>>>>>>  ANTMASK   = 0X%x\n", linfo->corr.daspar.antmask);
  fprintf(stdout, "========= >>>>>>>>>  SAMPLERS  = %d\n", linfo->corr.daspar.samplers);
  fprintf(stdout, "========= >>>>>>>>>  BASELINES = %d\n", linfo->corr.daspar.baselines);
  fprintf(stdout, "========= >>>>>>>>>  CHANNELS  = %d\n", linfo->corr.daspar.channels);
  fprintf(stdout, "========= >>>>>>>>>  LTA       = %d\n", linfo->corr.daspar.lta);
  fprintf(stdout, "========= >>>>>>>>>  BANDMASK  = %d\n", linfo->corr.daspar.bandmask);
  fprintf(stdout, "========= >>>>>>>>>  MODE      = %d\n", linfo->corr.daspar.mode);
  fprintf(stdout, "#######== >>>>>>>>>  CHAN[127  ]= %d\n", linfo->corr.daspar.chan_num[127]);
  fprintf(stdout, "#######== >>>>>>>>>  CHAN[130  ]= %d\n", linfo->corr.daspar.chan_num[130]);
  fprintf(stdout, "#######== >>>>>>>>>  CHAN[1000 ]= %d\n", linfo->corr.daspar.chan_num[1000]);
  fprintf(stdout, "========= >>>>>>>>>  MJD_REF   = %lf\n", linfo->corr.daspar.mjd_ref);
  fprintf(stdout, "========= >>>>>>>>>  T_UNIT    = %f\n", linfo->corr.daspar.t_unit);
  fprintf(stdout, "========= >>>>>>>>>  DUT1      = %15.12lf \n", (linfo->corr.daspar.dut1)*(12.0*3600/M_PI));
  fprintf(stdout, "===========================================================================\n");
  fprintf(stdout, "===========================================================================\n");
#endif

  }else{
#ifdef DEBUG_MODE
      fprintf(stdout, "^^^^^^^^^^^^^^  REF - CORR REF  ^^^^^^^^^^^^^^^^^^^\n");
#endif
      memcpy(HBuf+l*linfo->recl, corr, sizeof(CorrType));
      corr_recs = (sizeof(CorrType)+linfo->recl-1)/linfo->recl;  /* Jan 1999/crs */
  }    
  l += corr_recs; 
  if (l != linfo->lrecs)
  i = sprintf(HBuf+hdr_recs_off+10,"%d",l); HBuf[hdr_recs_off+10+i] = ' ';
  i = sprintf(HBuf+16," %-7d %-7d",l,corr_recs) ; HBuf[16+i] = ' ' ;
  fwrite(HBuf, linfo->recl, l, ofp);
  linfo->lrecs=l;
  linfo->lbrecs=corr_recs;
//  fprintf(stderr, "????????????????????? L = %d, corr_recs = %d lrecs = %d lbrecs = %d\n", l, corr_recs, linfo->lrecs, linfo->lbrecs);
  free(HBuf);
}
void write_scan(ScanInfoType *scan, int lta2, LtaInfo *linfo, 
		int scan_num,char *date_obs,FILE *ofp)
{ int i, l, corr_recs=0,hdr_recs_off;
  char *bp, *HBuf, *endp, *bufp ;
  CorrType  *corr=&linfo->corr;
  float integ = corr->daspar.lta * corr->corrpar.statime/1e6 ;

  bp = HBuf = malloc(DAS_BUFSIZE) ;
  sprintf(bp,"%4s%04d%72s",RecCode[ScanType],scan_num,"\n"); bp += LineLen ;
  hdr_recs_off = bp-HBuf;
  sprintf(bp,"%-8s= %d",InitKey[HDR_RECS],linfo->srecs); bp = fill_line(bp);
  
  bufp = HBuf+DAS_HDRSIZE - 50000 ;  /* scratch for scan.hdr */
  endp = bufp + put_scanpar(0,scan->proj.seq, scan, bufp)  ;
  *endp = 0 ;
  copy_text_ln(bufp, endp, &bp);
  sprintf(bp,"*{ %s.def",ScanName); bp = fill_line(bp);

  sprintf(bp,"%-8.8s= %f ", "INTEG", integ   ); bp = fill_line(bp) ;
//  fprintf(stdout,"%-8.8s= %f\n", "INTEG", integ   );
  sprintf(bp,"DATE-OBS= %s ", date_obs); bp = fill_line(bp) ;
  sprintf(bp,"MJD_REF = %f ",  corr->daspar.mjd_ref); bp = fill_line(bp) ;


  sprintf(bp,"%-8s= %s", ScanKey[BAD_RECS],""); bp = fill_line(bp);
  sprintf(bp,"%-8s= %s", ScanKey[BAD_ANTS],""); bp = fill_line(bp);
  sprintf(bp,"%-8s= %s", ScanKey[BAD_SAMP],""); bp = fill_line(bp);
  sprintf(bp,"%-8s= %s", ScanKey[BAD_BASE],""); bp = fill_line(bp);
  sprintf(bp,"%-8s= %s", ScanKey[BAD_CHAN],""); bp = fill_line(bp);
  sprintf(bp,"*} %s",ScanName); bp = fill_line(bp);
  sprintf(bp,"END_OF_HEADER");  bp = fill_line(bp);

  l = (bp-HBuf)/linfo->recl; if ((bp-HBuf) % linfo->recl) l++;
  for (i=bp-HBuf; i < l*linfo->recl;) HBuf[i++] = 0; HBuf[i-1] = '\n';
  if(linfo->keep_version)
  { ScanHdr *shdr=&linfo->stab[0].shdr;
    int t=shdr->year*1000+shdr->month*100+shdr->day; 
    int t0=2001*1000+8*100+15; /* hdr changed on 15/Aug/2001 */
    if(strstr(linfo->lhdr.old_version,"LTA")==NULL && t<t0) /* old: have to rely on data_obs*/
    { ScanInfoType scan1;
      scan2_to_scan1(scan,&scan1);
      memcpy(HBuf+l*linfo->recl, scan, sizeof(ScanInfoType)-2*sizeof(double));
      corr_recs = (sizeof(ScanInfoType)-2*sizeof(double)+linfo->recl-1)/linfo->recl;
    }
    else  /* no change in scan structure */
    { memcpy(HBuf+l*linfo->recl, scan, sizeof(ScanInfoType));
      corr_recs = (sizeof(ScanInfoType)+linfo->recl-1)/linfo->recl;
    }
  }else{ /* stick to new scan structure */
    memcpy(HBuf+l*linfo->recl, scan, sizeof(ScanInfoType));
    corr_recs = (sizeof(ScanInfoType)+linfo->recl-1)/linfo->recl;
  }

  l += corr_recs;
  if (l != linfo->srecs)
  { i = sprintf(HBuf+hdr_recs_off+10,"%d",l); HBuf[hdr_recs_off+10+i] = ' ';}
  i = sprintf(HBuf+16," %-7d %-7d",l,corr_recs) ; HBuf[16+i] = ' ' ; 
  fwrite(HBuf, linfo->recl, l, ofp);
  free(HBuf);
  linfo->srecs=l;
  linfo->sbrecs=corr_recs;
}

void update_hdr(LtaInfo *linfo)
{  unsigned val=linfo->corr.daspar.antmask;
   VisInfo  *vinfo= &linfo->lhdr.vinfo;
   CorrType *corr = &linfo->corr;
   LtaHdr   *lhdr = &linfo->lhdr;
   int off=0,antennas=0,i;

   while (val) {antennas++; val &= val-1; }
   vinfo->antennas = antennas;
   vinfo->samplers = corr->daspar.samplers;
   vinfo->baselines= corr->daspar.baselines;
   vinfo->channels = corr->daspar.channels;
   for(i=0;i<vinfo->channels;i++)
     vinfo->chan_num[i] = corr->daspar.chan_num[i];

   lhdr->par_size   = corr->daspar.samplers*sizeof(DataParType);
   lhdr->data_size  = 2*sizeof(float)*(vinfo->baselines*vinfo->channels); 
//   fprintf(stdout, "ANTENNAS=%d SAMPS=%d BASE=%d CHANS=%d\n", vinfo->antennas,vinfo->samplers,vinfo->baselines, vinfo->channels);
   off       = LineLen ;      
   lhdr->flg_off     = off;
   lhdr->flg_rec_off = off; 
   lhdr->flg_rec_size= ((1       +31)/32)*4; off += lhdr->flg_rec_size;
   lhdr->flg_ant_off=off;
   lhdr->flg_ant_size = ((vinfo->antennas*1 +31)/32)*4; off += lhdr->flg_ant_size;
   lhdr->flg_smp_off = off;  
   lhdr->flg_smp_size = ((vinfo->samplers*1 +31)/32)*4; off += lhdr->flg_smp_size;
   lhdr->flg_bas_off = off; 
   lhdr->flg_bas_size = ((vinfo->baselines*1 +31)/32)*4; off += lhdr->flg_bas_size;
   lhdr->flg_dat_off = off; 
   lhdr->flg_dat_size = ((vinfo->baselines*vinfo->channels*4+31)/32)*4; off+= lhdr->flg_dat_size;
   lhdr->flg_size   = off - lhdr->flg_off;
   lhdr->time_off   = off; 
   lhdr->time_size=TimeSize; off += lhdr->time_size;
   lhdr->wt_off= off; 
   lhdr->wt_size=WtSize; off += lhdr->wt_size;
   lhdr->par_off = off;
  if(strstr(lhdr->version,"COR30x2 ")==NULL && strstr(lhdr->version,"HST03") ==NULL){
   lhdr->par_size=vinfo->samplers*sizeof(DataParType); off+=lhdr->par_size;
  } else if(strstr(lhdr->version,"COR30x2 ")!=NULL && strstr(lhdr->version,"HST03") ==NULL){
   lhdr->par_size=vinfo->samplers*sizeof(DataParType); off+=lhdr->par_size;
  } else if(strstr(lhdr->version,"COR30x2 ")!=NULL && strstr(lhdr->version,"HST02") !=NULL){
   lhdr->par_size=vinfo->samplers*sizeof(DataParType); off+=lhdr->par_size;
  } else if(strstr(lhdr->version,"COR30x2 ")!=NULL && strstr(lhdr->version,"HST03") !=NULL){
   lhdr->par_size=0;// off+=lhdr->par_size;
  }

//   lhdr->par_size=vinfo->samplers*sizeof(DataParType); off+=lhdr->par_size;
//   lhdr->par_size=0;// off+=lhdr->par_size;
   lhdr->data_off   = off;
   lhdr->recl=lhdr->data_off+lhdr->data_size;;
   linfo->recl=lhdr->recl;
   linfo->lrecs=1; /* default -- rechecked in write_hdr, write_scan */
   linfo->srecs=1;
}
