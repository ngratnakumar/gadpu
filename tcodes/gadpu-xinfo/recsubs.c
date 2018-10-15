# include <stdio.h>
# include<stdlib.h>
# include <string.h>
# include <ctype.h>
# include <sys/types.h>
# include <sys/ipc.h>
# include <sys/shm.h>
# include <unistd.h>

# include "newcorr.h"
# include "protocol.h"

DataInfoType *DataInfo ;
DataBufType *dBuf ;
DataTabType *dTab ;
ScanInfoType *ScanTab, ScanInfo ;
EventLogType *EventLog ;
ProjectType Project ;

static int IndProj = -1, NextRec=-1, ParOff[MAX_SAMPS] ;
static int MaxBlocks;


RecOffsetType *RecOff ;


static int ScanState=DAS_UNINIT ;


void blink(void) { usleep(40000) ; }

int gvgetMacName(char name[12], BaseParType *base, CorrType *corr)
{ int  mask, ant0 = base->samp[0].ant_id, ant1 = base->samp[1].ant_id ;
  int band0 = base->samp[0].band, band1 = base->samp[1].band ;
  char *p = name ;
  mask = (1<< (ant0&31) ) | (1 << (ant1&31)) ;
  if ((corr->daspar.antmask & mask) != mask) return -1 ;
  
  mask = (1<< (band0 & 3) ) | 1 << ((band1 & 3)) ;
  if((corr->daspar.bandmask & mask) != mask) return -2 ;

  strncpy(p,corr->antenna[ant0].name,3) ;
  strncpy(p+6,corr->antenna[ant1].name,3) ;
  p[3] = p[9] = '/' ;
  p[4]  = '0' + (band0&3) ;
  p[10] = '0' + (band1&3) ;
  p[5] = ':' ;
  p[11] = 0 ;

  return 0 ;
}

int mem_init(CorrType *corr, RecOffsetType *off)
{ int i, id_h, id_d ;
  
  for (i=0; i<20; i++)
  { id_h = shmget(DAS_H_KEY, sizeof(DataInfoType), 0) ;
    if(id_h >= 0)break ; else sleep(1) ;
  }
   
  id_d = shmget(DAS_D_KEY, sizeof(DataBufType), 0 ) ;
  if (id_h < 0 || id_d < 0 )
  { fprintf(stderr,"FATAL: mem_init()Error connecting shm (ID_H=%d, ID_D=%d)\n", id_h, id_d); return -1; }

  DataInfo = (DataInfoType *) shmat(id_h, 0,SHM_RDONLY) ;
  dBuf = (DataBufType*) shmat(id_d, 0,SHM_RDONLY) ;
  dTab = dBuf->dtab;

  ScanTab = DataInfo->scaninfo ;
  EventLog = DataInfo->event ;
  
  while ( (DataInfo->active == 1) || (DataInfo->status == DAS_UNINIT)) blink();
  while ( (DataInfo->active == 1) || (DataInfo->status != STARTPROJ)) blink();

  memcpy(corr, &DataInfo->corr, sizeof(CorrType)) ;

  RecOff = &DataInfo->offset ;
  memcpy (off, RecOff, sizeof(RecOffsetType)) ;

  MaxBlocks = DAS_BUFSIZE/dBuf->blocksize ;
  if(MaxBlocks>MaxDataBuf)MaxBlocks = MaxDataBuf;

  return 0 ;
}  

void mem_clear(void)
{
  shmdt((void *) DataInfo ) ;
  shmdt ((void *) dBuf ) ;
}

int das_available(void)
{ int status ;
  while (DataInfo->active) blink() ;
  status = DataInfo->status ;
  if (status == DAS_FINISH) return 0 ;
  return 1 ;
}


int wait_for_data(int rec )
{ 
  while ((dTab[rec].flag & BufReady) == 0)
  { blink() ;
    if (DataInfo->status == DAS_FINISH)
    {ScanState=DAS_FINISH; return -1;} 
  }
  return 0;
}

int get_data(RecBufType *rbuf, float *t)
{ int i, rec, found=0, seq = rbuf->seq, wordsize = rbuf->off.wordsize ;
  char *inbuf, *outbuf = rbuf->buf, *pbuf ;
  MacWordType *mac = rbuf->mac ;
  RecOffsetType *in = &DataInfo->offset ;
  RecOffsetType *out = &rbuf->off ;
  int *active ;

  if (NextRec < 0) { rbuf->status = DAS_UNINIT ; return -1 ; }
  rec = NextRec%MaxBlocks ;

  if (wait_for_data(rec) < 0)
  { rbuf->status = DAS_FINISH; return -2 ; }

  inbuf = dBuf->buf + dTab[rec].rec * dBuf->blocksize ;
  active = (int *) inbuf ;
  for (i=0; i<ActiveScans; i++)
    if(active[i] >=0 && active[i] < MAX_SCANS)  
      if (ScanTab[active[i]].proj.seq == seq) 
      { found = 1 ; *t=ScanTab[active[i]].t; break ; }
  if (!found) 
  {  for(i=MAX_SCANS-1;i>=0;i--)
     { if(ScanTab[i].t <0) continue;
       if(ScanTab[i].proj.seq==seq && ScanTab[i].status==DELETEPROJ)
       { found=1; *t=ScanTab[i].t; rbuf->status=DELETEPROJ; return -3;}
     }
  }
  if(!found)
  {  fprintf(stderr,"INFO: PRJ Inactive\n"); rbuf->status=STOPPROJ; return -3; }

  rbuf->status= STARTPROJ ;
  rbuf->seqnum = dTab[rec].seqnum ;

  memcpy(outbuf+out->t_off, inbuf+in->t_off, TimeSize) ;
  memcpy(outbuf+out->wt_off, inbuf+in->wt_off, WtSize) ;
  pbuf = outbuf + out->par_off ;
  for (i=0; i < out->par_words; i++, pbuf += sizeof(DataParType))
      memcpy(pbuf, inbuf+ParOff[i], sizeof(DataParType)) ;

  pbuf = outbuf + out->data_off ;
  for (i=0; i<out->data_words; i++, pbuf+=wordsize)
      memcpy(pbuf, inbuf+mac[i].off, wordsize) ;

  NextRec++ ;
  return 0 ;
}

int wait_for_proj(char *code, int rec)
{ int i,*active;
  ProjectType *proj ;

  ScanState = DAS_UNINIT ;

  if (DataInfo->status == DAS_FINISH)
  { ScanState = DAS_FINISH ; return DAS_FINISH ; }

  while ((DataInfo->status == STARTPROJ) && ((dTab[rec].flag & BufReady) == 0))
    blink() ;
  if  ( (DataInfo->status != STARTPROJ) && (dTab[rec].flag & BufReady) == 0)
  { fprintf(stderr,"* PRJ %8s Not Found\n", code) ;
    ScanState = DAS_UNINIT ;
    return DAS_UNINIT ;
  }

  active =(int *)  (dBuf->buf+dTab[rec].rec*dBuf->blocksize) ;
  for (i=0; i<ActiveScans; i++)
  { if (active[i] < 0)continue ;
    proj = &ScanTab[active[i]].proj ;
    if (strncmp(proj->code, code, 8) == 0) /* Ignore ScanTab JNC 11Oct00 */
      { ScanState=STARTPROJ; IndProj = active[i];return ScanState ;}
  }
  for (i=MAX_SCANS-1; i>=0; i--)
  { if (ScanTab[i].t < 0 ) continue ;
    proj = &ScanTab[i].proj ;
    if (strncmp(proj->code, code, 8) == 0)
      if(ScanTab[i].status == DELETEPROJ)
	{ ScanState = DAS_FINISH ; return DAS_FINISH ; }
  }

  return DAS_UNINIT ;
}

int proj_init(char *code, ScanInfoType *scan, CorrType *corr, MacWordType *mac)
{ /*  CAUTION:  Modified Corr is returned !!!!  */
  int i,j, err, samplers, baselines,data_off,wordsize ;
  unsigned int antmask,bandmask ;
  BaseParType *base ;
  SamplerType samp[MAX_SAMPS] ;
  ProjectType *proj = &scan->proj ;
  

  if(NextRec<0)NextRec=dBuf->cur_block;
  for(i=0;(err = wait_for_proj(code,NextRec%MaxBlocks))!= STARTPROJ;i++)
  { if(err == DAS_FINISH) return DAS_FINISH;
    if(i==0)fprintf(stderr,"* Waiting for ScanStart\n") ;
    NextRec++;
    blink();
  }

  memcpy(scan, ScanTab+IndProj, sizeof(ScanInfoType)) ;
  memcpy(corr, &DataInfo->corr, sizeof(CorrType)) ;
  
/*  Now revise corr to suit the subarray selected by the project */

  for (samplers=0,i=0; i<MAX_SAMPS; i++)
  { antmask = 1 << corr->sampler[i].ant_id ;
    bandmask = 1 << corr->sampler[i].band ;
    if ((antmask & proj->antmask) && (bandmask & (int)proj->bandmask))
    { 
      memcpy(samp+samplers, corr->sampler+i, sizeof(SamplerType)) ;
      ParOff[samplers]=DataInfo->offset.par_off +i*sizeof(DataParType);
      samplers++ ;
    }
  }

  data_off = DataInfo->offset.data_off ;
  wordsize = DataInfo->offset.wordsize ;

  for (base=corr->baseline,baselines=0,i=0; i<corr->daspar.baselines; i++,base++)
  { int d0=base->samp[0].dpc, d1=base->samp[1].dpc ;
    int k0=0, k1=0 ;
    MacWordType *word = mac + baselines ;
    for (j=0; j<samplers; j++)
    { if (samp[j].dpc == d0)k0 = 1 ;
      if (samp[j].dpc == d1)k1 = 1 ;
    }
    if ( (k0 == 0) || (k1 == 0))continue ;

    memcpy(&word->base, base, sizeof(BaseParType)) ; 
    if (gvgetMacName(word->name, base, corr) < 0) continue ;
    word->off = data_off + i * corr->daspar.channels * wordsize ;
        /*
         * byte offset wrt beginning of data block for the
         * first complex word related to this baseline
         */
    baselines++ ;
  }

  corr->daspar.baselines = baselines ;
  corr->daspar.samplers = samplers ;
  corr->daspar.antmask &= scan->proj.antmask ;
  corr->daspar.bandmask &= scan->proj.bandmask ;

  memcpy(corr->sampler, samp, samplers*sizeof(SamplerType)) ;
  for (i=0; i<baselines; i++)
     memcpy(corr->baseline+i, &mac[i].base, sizeof(BaseParType)) ;
  

  /* Doesn't seem to make sense JNC 10Oct00 
  
  NextRec = dBuf->cur_rec ;
  for (i=NextRec; i<NextRec; i++)
  { double t0 = scan->t, t ;
    int rec = i%MaxDataBuf ;
    char *buf = dBuf->buf + RecOff->t_off ;
    if (wait_for_data(rec) < 0) return -1 ;
    memcpy(&t, buf + dTab[rec].rec*dBuf->blocksize, sizeof(double)) ;
    if (t >= t0) { NextRec = i ; return 0 ;
     } 
  }
  */ 
  return STARTPROJ ;
}

