# ifndef __OLDCORR_H__
# define __OLDCORR_H__

/*    Obsolete (oldcorr) types used in older versions of corr */

typedef struct
{ char  name[4];
  float bx, by, bz;  /* nanosec */
  int   rf[2], first_lo[2], bb_lo[2];   /* Hz  */
  float d0[MAX_BANDS], p0[MAX_BANDS];
  unsigned char samp_con[MAX_BANDS], array[MAX_BANDS] ;
} AntennaType;

typedef struct 
{ int    ants, samplers, macs, channels, pols, sta, statime /* in usec */;
  unsigned char dpcmux,clksel,fftmode,macmode ;  /* replaces old dummy int */
  float  f_step, clock; /* Hz  */
  double t_unit;        /* Sec */
} OldCorrParType ;

typedef struct
{ char   object[NAMELEN], date_obs[DATELEN];
  char   observer[NAMELEN], project[NAMELEN], code[8] ;
  double ra_date, dec_date, mjd_ref, dra, ddec;
  int    antmask, bandmask, flag, seq ;
         /* we use (daspar.antmask & scanpar.antmask) */
  float  integ, f_step ;
  int    rf[2], first_lo[2], bb_lo[2];  /*  Hz  */
} ScanParType;

#define MAX_MACS  (16*MAC_CARDS)
typedef struct
{ int    antmask, samplers, macs, baselines, channels, lta;
  short  bandmask, mode ;
  short  samp_num[MAX_SAMPS], mac_num [MAX_MACS ],
         base_num[MAX_BASE ], chan_num[MAX_CHANS];
  short  dummy[(2+MAX_SAMPS+MAX_MACS+MAX_BASE+MAX_CHANS) % 4]; /*double align*/
} OldDasParType ;

typedef struct { unsigned char ant[4], band[4]; } MacFftType;
typedef struct { unsigned char ant[2], band[2], samp[2], array[2]; } BaseType;
                 /* array[2] in BaseType is dummy. But samp[2] may be used */

#define MAX_ARRAYS 1

typedef struct
{ unsigned char flag[4];
  int           antmask;                   /* It's dummy, may be changed.  */
  char          version [NAMELEN   ];
  char          bandname[MAX_BANDS ][8];
  AntennaType   antenna [MAX_ANTS  ];      /* Ant pos, freq & other config */
  SamplerType   sampler [MAX_SAMPS ];      /* ant, band vs. ffts           */
  unsigned char mac     [MAX_MACS  ][4];   /* The 4 ffts, the mac scans    */
  BaseType      baseline[MAX_BASE  ];      /* Pair of ant, band            */
  OldCorrParType   corrpar;                   /* Max. enabled mac_params      */
  OldDasParType    daspar;                    /* Actually useful params       */
  ScanParType   scanpar [MAX_ARRAYS];      /* Observation dependent params */
} OldCorrType;

/*     ===========  end of  obsolete prototypes   ============    */

typedef struct
{ char object[NAMELEN];
  struct { float i,q,u,v ; } flux ;
  double mjd0 /* fractional mjd, to which ra, dec refer  */ ;
        /*
           mjd0 refers to the epoch at which ra_date,dec_date.
           Note that the timestamp in data is wrt to the global
           reference time contained in daspar->mjd_ref
        */
  double ra_date, dec_date, dra,ddec ; /* rad, rad/s */
  double freq[2], first_lo[2],bb_lo[2];   /* Hz */
  double rest_freq[2], lsrvel[2] ;  /* Hz, km/s  */
  double ch_width ;  /* Hz */
  int id, net_sign[MAX_BANDS], mode , dum1;
  unsigned int antmask; /* antennas to fringe stop */
  unsigned short bandmask, dum2; 
  short calcode, qual ;
} OldSourceParType ;

typedef struct
{ int status ;
  float t ;  /* program dependent meaning ! */
  ProjectType proj ;
  OldSourceParType source ;
} OldScanInfoType ;

#endif
