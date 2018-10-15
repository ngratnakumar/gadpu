#ifndef _SOL_INCLUDED_
#define _SOL_INCLUDED_

typedef struct complex_type{ float re,im;} Complex;
extern Complex cmplxZtmp;
extern float   floatZtmp;

#define MAX_RECS 1024
#define C 299792458.0 /* speed of light (m/s) */
#define MAX_FILT 32

typedef struct
{ float   phi0;   /* phi0 from phase ramp fitting      */
  float   delay;  /* delay from phase ramp fitting (m) */
  Complex gain;   /* gain averaged over the band       */
} SmpSolType;
typedef struct
{ int      block_len;                     /* number of records in one "solution block"            */
  float    min_phase_stability;           /* threshold for flagging sampler as bad                */
  float    rms_cut;                       /* flagging threshold (units of rms)                    */
  float    max_bad_cp;                    /* max. frac of bad closure phase for a "good" baseline */
  float    min_good;                      /* minimum fraction good data for a "good" sampler      */
  int      bp_start_chan;                 /* first channel for computing bandpass                 */
  int      bp_end_chan;                   /* last channel for computing bandpass                  */
  int      change_endian;                 /* flip from big to little endian?                      */
  char     samp_flag[MAX_SAMPS];          /* sampler flag (one per sampler)                       */
  char    *samp_rec_flag[MAX_SAMPS];      /* sampler flag (one per sampler per rec)               */
  char    *samp_chan_flag[MAX_CHANS];     /* sampler flag (oner per chan per samp)                */
  char   **samp_chanrec_flag[MAX_CHANS];  /* sampler flag (oner per chan per samp per rec)        */
  char    *base_flag[MAX_SAMPS];          /* baseline flag (one per baseline)                     */
  char   **base_chan_flag[MAX_CHANS];     /* baseline flag (oner per chan per baseline)           */
  Complex *samp_chan_gain[MAX_CHANS];     /* unnormalized per channel gain                        */
  Complex *samp_bp[MAX_CHANS];            /* normalized bandpass                                  */
  Complex  samp_gain[MAX_SAMPS];          /* gain for "channel0"                                  */
  Complex *samp_bpgain[MAX_CHANS];        /* combined bandpass&chan0 gain                         */
  short    nsol[MAX_SAMPS];               /* num good solutions (per samp)                        */
  short   *n_chan_sol[MAX_CHANS];         /* num good solutions (per chan per samp)               */
  SmpSolType smp_sol[MAX_RECS][MAX_SAMPS];/* broad band gain,delay per samp per record            */
  Complex **samp_rec_gain[MAX_CHANS];     /* gain per channel per record                          */
  int      save_rec_gain;                 /* save per record gain                                 */
  int      rec_off;                       /* offset at which to start saving next block's solns   */
  int      ref_samp[MAX_BANDS];           /* reference samplers                                   */
} SolParType;


#define CMPLX_MUL(a,b,c){\
        c->re = a->re*b->re-a->im*b->im;c->im = a->re*b->im+a->im*b->re;}
#define CMPLX_CONJ_MUL(a,b,c){\
        c->re = a->re*b->re + a->im*b->im; c->im = a->im*b->re - a->re*b->im;}
#define CMPLX_SQR(a) (a->re*a->re+a->im*a->im)
#define CMPLX_AMP(a) sqrt(CMPLX_SQR(a))
#define CMPLX_PHS(a) atan2(a->im,a->re)
#define CMPLX_DIV(a,b,c){\
        CMPLX_CONJ_MUL(a,b,(&cmplxZtmp));\
        floatZtmp=CMPLX_SQR(b);\
        c->re=cmplxZtmp.re/floatZtmp;c->im=cmplxZtmp.im/floatZtmp;}
#define CMPLX_ADD(a,b,c){c->re=a->re+b->re;c->im=a->im+b->im;}
#define CMPLX_SUB(a,b,c){c->re=a->re-b->re;c->im=a->im-b->im;}
        
#define RAD2DEG(x)  ((x)*180.0/M_PI)
#define DEG2RAD(x)  ((x)/180.0*M_PI)

enum {MAX_CALS=32,MAX_INTEG=32};


void stats1(float *x, int n, float *mean, float *rms);
void stats(float *x, int n, float *mean);
float median(float *x, int n);
int get_one_chan(char *dbuf, float *data, int chan, int integ, LtaInfo *linfo);
int put_one_chan(char *dbuf, float *data, int chan, int integ, LtaInfo *linfo);
int solve_one_chan(float flux,float *data, char **vis_flag,int chan, int integ,LtaInfo *linfo,SolParType *solpar);
int  compute_chan0(char *buf,LtaInfo *linfo,int start_chan,int end_chan);
int solve_all_chans(LtaInfo *linfo, int scan, FILE *fp, FILE *ofp, SolParType *solpar);
int get_normalized_bp(VisInfo *vinfo, SolParType *solpar);
int apply_gain_to_buf(char *buf,LtaInfo *linfo,Complex **gain);
int apply_bbgain_to_buf(char *buf,LtaInfo *linfo,Complex *bb_gain);
int apply_gain_to_scan(LtaInfo *linfo,int scan,FILE *fp, FILE *ofp,Complex **gain,int change_endian,
		       int start_chan,int end_chan);
int compute_total_gain(VisInfo *vinfo, Complex **bandpass, Complex *broadband_gain, 
		       Complex **total_gain);
int solve_chan0(LtaInfo *linfo, int scan, FILE *fp, FILE *ofp,SolParType *solpar);
int find_bad_samplers_chan(LtaInfo *linfo, float *data, int chan, int integ,SolParType *solpar);
int init_solpar(SolParType *solpar,LtaInfo *linfo);
int get_delay_gain(SolParType *solpar, LtaInfo *linfo,int scan);
int get_ref_samp(SolParType *solpar, LtaInfo *linfo, char *ref_ant);
#endif
