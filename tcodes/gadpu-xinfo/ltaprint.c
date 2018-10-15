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
#include <sys/types.h>
#include <regex.h>
#include <slalib.h>

#define RAD2DEG(x)  ((x)*180.0/M_PI)
#define DEG2RAD(x)  ((x)/180.0*M_PI)
#define RAD2HR(x)   ((x)*12.0/M_PI)

//static char rcsid[]="$Id: ltaprint.c,v 1.4 2007/03/21 12:33:25 das Exp $";

char *LtaFile=NULL,Usage[1024],*Object=NULL;
int   BaseNum=-1;
int   *SelChan,*SelRec,*SelScan;
int   NSelRec=-1,NSelScan=-1,NSelChan=-1;
char  **SelAnt=NULL,**SelBase=NULL;
int   NSelAnt=-1,NSelBase=-1;
int   ShowSelf=0,ShowCross=0,ShowCoPol=0,ShowXPol=0,PrintType=0,XaxisType=0;
int   HonourFlags=0;
int   ShowModel=0;
int select__baselines(LtaInfo *linfo,int *sel_base,char *sel_base_name[])
{ VisInfo *vinfo=&linfo->lhdr.vinfo;
  regex_t ant_regex,base_regex;
  int i,j=0,m,n,s1,s2,card,chip,col,row;
  char name1[64],name2[64];
  CorrType *corr=&linfo->corr;

	fprintf(stdout, "NSELANT = %d\n", NSelAnt);
  for(n=0;n<NSelAnt;n++)
  { for(i=0;i<strlen(SelAnt[n]);i++)SelAnt[n][i]=toupper(SelAnt[n][i]);
    regcomp(&ant_regex,SelAnt[n],REG_EXTENDED|REG_NOSUB);
    for(m=0;m<NSelBase;m++)
    { for(i=0;i<strlen(SelBase[m]);i++)SelBase[m][i]=toupper(SelBase[m][i]);
      regcomp(&base_regex,SelBase[m],REG_EXTENDED|REG_NOSUB);
      for(i=0;i<vinfo->baselines;i++)
      { BaselineType *base=&vinfo->base[i];
        BaseParType  *bpar=&corr->baseline[i];
	card=bpar->card;chip=bpar->chip;
	col=bpar->samp[0].fft_id; row=bpar->samp[1].fft_id;
        if( (base->ant[0] == base->ant[1]) && ! ShowSelf) continue;
        if( (base->ant[0] != base->ant[1]) && ! ShowCross) continue;
	if( (base->band[0] == base->band[1]) && ! ShowCoPol) continue;
	if( (base->band[0] != base->band[1]) && ! ShowXPol) continue;
        sprintf(name1,"%s-%s",base->antname[0],base->bandname[0]);
	s1=base->samp[0];
        if(regexec(&ant_regex,name1,0,0,0)==0)
	{ sprintf(name2,"%s-%s",base->antname[1],base->bandname[1]);
	  s2=base->samp[1];
	  if(regexec(&base_regex,name2,0,0,0)==0)
	  {sprintf(sel_base_name[j],"%s:%s [b=%d s0=%d s1=%d cd=%d ch=%d f0=%d f1=%d] ",name1,name2,i,s1,s2,card,chip,col,row);sel_base[j]=i;j++;}
	}
        if(regexec(&base_regex,name1,0,0,0)==0)
        { sprintf(name2,"%s-%s",base->antname[1],base->bandname[1]);
	  s2=base->samp[1];
	  if(regexec(&ant_regex,name2,0,0,0)==0)
	  {sprintf(sel_base_name[j],"%s:%s [b=%d s0=%d s1=%d cd=%d ch=%d f0=%d f1=%d]", name1,name2,i,s1,s2,card,chip,col,row);sel_base[j]=i;j++;}
	}
      }
    }
  }
  /* eliminate duplicates */
  for(i=0,n=0;n<j;n++)
  { for(m=0;m<i;m++)
      if(sel_base[m]==sel_base[n]) break;
     if(m==i)  /* no duplicates */
     { sel_base[i]=sel_base[n]; 
       sel_base_name[i]=sel_base_name[n];
       i++;
     }
  }
  return i;
}
int ltaprint(int argc, char **argv)
{  LtaInfo      linfo;
   FILE         *fp;
   char         object[16];
   char         *dbuf=NULL,*sel_base_name[MAX_BASE];
   off_t        recl;
   float        re,im;
   int          i,j,k,l,change_endian,sel_base[MAX_BASE],n_sel_base;
   unsigned long off;
   CorrType     *corr=&linfo.corr;
   double       tm;
   double       mjd,lst,el,eld,eldd,az,azd,azdd,pa,pad,padd,ha;
   int          time_plot=0,chan_plot=0,freq_plot=0,par_plot=0,az_plot=0,el_plot=0,ha_plot=0,lst_plot=0,model_plot=0;
   double       gmrt_lat,gmrt_long;
   char         ras[32],decs[32],date[128];
   int          nc=0,f_off,f_ind;
   char         *flag;
   unsigned char flag_mask[8];

   for(i=0;i<8;i++)flag_mask[i]=1<<i;

   gmrt_lat =DEG2RAD((19+05/60.0+26.35/3600.0));/* GMRT Latitude */
   gmrt_long=DEG2RAD(74+2.0/60+59.9/3600); /* GMRT Longitude */

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
   if((dbuf=malloc(recl))==NULL)
   {fprintf(stderr,"MallocError\n"); return 1;}

   

   if(NSelChan <0){SelChan=(int *)malloc(2*sizeof(int));SelChan[0]=0;SelChan[1]=1;}
   if(NSelChan==1)
   { if(realloc(SelChan,2*sizeof(int))==NULL){printf("malloc failure!\n");return -1;}
     SelChan[1]=SelChan[0]+1;
   }
   if(NSelRec== 1)
   { if(realloc(SelRec,2*sizeof(int))==NULL){printf("malloc failure!\n"); return -1;}
     SelRec[1]=SelRec[0]+1;
   }
 
   if((SelChan[0] <0) || (SelChan[0] > corr->daspar.channels) || (SelChan[1] <0) || 
       (SelChan[1] > corr->daspar.channels) || (SelChan[0]>SelChan[1]))
   { fprintf(stderr,"Invalid channel range %d - %d\n",SelChan[0],SelChan[1]);
     fprintf(stderr,"The valid range is %d - %d\n",corr->daspar.chan_num[0],
	     corr->daspar.chan_num[corr->daspar.channels-1]);
     return -1;
   }

   switch (XaxisType)
   { case 0: time_plot=1; 
             printf("# Col %-6d RecNum\n",nc++);printf("# Col %-6d IST(Hrs)\n",nc++); 
	     break;
     case 1: par_plot=1; time_plot=1; 
             printf("# Col %-6d RecNum\n",nc++);printf("# Col %-6d IST(Hrs)\n",nc++);
	     printf("# Col %-6d ParAng(Deg)\n",nc++);
	     break;
     case 2: el_plot=1;time_plot=1;
             printf("# Col %-6d RecNum\n",nc++);printf("# Col %-6d IST(Hrs)\n",nc++);
             printf("# Col %-6d ElAng(Deg)\n",nc++);
	     break;
     case 3: az_plot=1;time_plot=1;
             printf("# Col %-6d RecNum\n",nc++); printf("# Col %-6d IST(Hrs)\n",nc++);
             printf("# Col %-6d AzAng(Deg)\n",nc++);
	     break;
     case 4: ha_plot=1;time_plot=1;
             printf("# Col %-6d RecNum\n",nc++); printf("# Col %-6d IST(Hrs)\n",nc++);
             printf("# Col %-6d HA(Hrs)\n",nc++);
	     break;
     case 5: lst_plot=1;time_plot=1;
             printf("# Col %-6d RecNum\n",nc++); printf("# Col %-6d IST(Hrs)\n",nc++);
             printf("# Col %-6d LST(Hrs)\n",nc++);
	     break;
     case 6: chan_plot=1; 
             printf("# Col %-6d ChanNum\n",nc++); 
	     break;
     case 7: freq_plot=1; chan_plot=1; 
             printf("# Col %-6d ChanNum\n",nc++); printf("# Col %-6d Freq(MHz)\n",nc++); 
	     break;
    default: time_plot=1;
   }
   if(ShowModel){time_plot=0;model_plot=1;}

   for(i=0;i<MAX_BASE;i++)sel_base_name[i]=malloc(128);
   if(BaseNum<0)
     n_sel_base=select__baselines(&linfo,sel_base,sel_base_name);
   else
   { n_sel_base=1;sel_base[0]=BaseNum;
     sprintf(sel_base_name[0],"BaseLine%d",BaseNum);
   }


   printf("#* Selected %d baselines\n",n_sel_base);
   if(PrintType==0)
   { for(i=0;i<n_sel_base;i++)
     { if(time_plot==1)
       { for(j=SelChan[0];j<SelChan[1];j++)
         { printf("# Col %-6d %sCh%d(R)\n",nc++,sel_base_name[i],j);
	   printf("# Col %-6d %sCh%d(I)\n",nc++,sel_base_name[i],j);
	 }
       }
       if(chan_plot==1)
       { printf("# Col %-6d %s(R)\n",nc++,sel_base_name[i]);
	 printf("# Col %-6d %s(I)\n",nc++,sel_base_name[i]);
       }
     }
   }else{
     for(i=0;i<n_sel_base;i++)
     { if(time_plot==1)
       { for(j=SelChan[0];j<SelChan[1];j++)
         { printf("# Col %-6d %sCh%d(A)\n",nc++,sel_base_name[i],j);
	   printf("# Col %-6d %sCh%d(P)\n",nc++,sel_base_name[i],j);
	 }
       }
       if(chan_plot==1)
       { printf("# Col %-6d %s(A)\n",nc++,sel_base_name[i]);
	 printf("# Col %-6d %s(P)\n",nc++,sel_base_name[i]);
       }

     }
   }
   printf("#*\n");
   
 
   for(i=0;i<linfo.scans;i++)
   { int recs=linfo.stab[i].recs;
     SourceParType *src=&linfo.stab[i].scan.source;
     regex_t object_regex;

     if(NSelScan>0)
     {  for(j=0;j<NSelScan;j++) if(i==SelScan[j]) break;
        if(j==NSelScan) continue;
     }
     
     strncpy(object,linfo.stab[i].shdr.object,11);object[11]='\0';
     if(Object !=NULL)
     { regcomp(&object_regex,Object,REG_EXTENDED|REG_NOSUB);
       if(regexec(&object_regex,object,0,0,0)!=0) continue;
     }
     rewind(fp); 
     ltaseek(fp,linfo.stab[i].start_rec+linfo.srecs,linfo.recl);
     strncpy(date,mjd2ist_date(corr->daspar.mjd_ref+linfo.stab[i].scan.t/(24.0*3600.0)),23);
     convradec(RAD2DEG(src->ra_mean),ras,0);
     convradec(RAD2DEG(src->dec_mean),decs,1);
     printf("#* SCAN: %-3d      OBJECT: %-11s RA: %s DEC: %s\n",i,object,ras,decs);
     printf("#* FREQ: %7.2f   CH_WD: %7.3f   DATE: %s  \n",src->freq[0]/1.0e6,src->ch_width/1.0e3,date);
     fprintf(stderr,"# SCAN: %3d OBJECT: %-11s\n",i,object);
     for(j=0;j<recs;j++)
     { if(fread(dbuf,recl,1,fp) !=1) return 0;
       if(NSelRec>0)
       {  if(j<SelRec[0]) continue;
  	  if(NSelRec>1 && j >= SelRec[1]) continue;
       }
       if(change_endian)flipData(dbuf,&linfo.lhdr);
       memcpy(&tm,dbuf+linfo.lhdr.time_off,linfo.lhdr.time_size);
       if(model_plot)
       { int s;
	 ModelParType *mpar=(ModelParType *)(dbuf+linfo.lhdr.par_off);
         printf("%5d %12f ",j,tm/3600.0);
	 for(s=0;s<linfo.lhdr.vinfo.samplers;s++)
	 { printf("smp=%d : %f %f %10.4e %10.4e\n",s,mpar->phase,mpar->dp,mpar->delay,mpar->dd);
   	   mpar++;
	 }
       }
       if(time_plot)
       { printf("%5d %12f ",j,tm/3600.0);
	 if(par_plot||el_plot||az_plot||ha_plot||lst_plot)
	 { mjd=corr->daspar.mjd_ref+tm/(24.0*3600.0);
	   lst=slaGmst(mjd)+gmrt_long;
	   while(lst>2.0*M_PI) lst -= 2.0*M_PI;
	   while(lst<0.0     ) lst += 2.0*M_PI;
	   ha=lst-src->ra_mean;
	   slaAltaz(ha,src->dec_mean,gmrt_lat,&az,&azd,&azdd,&el,&eld,&eldd,
		    &pa,&pad,&padd);
	   if(ha>M_PI)ha -=2.0*M_PI;
	   if(src->dec_mean > gmrt_lat)
	   { if(ha<0.0)pa+=M_PI;
	     else pa-=M_PI;
	   }
	   if(par_plot) printf("%7.2f ",RAD2DEG(pa));
	   if(el_plot)  printf("%7.2f ",RAD2DEG(el));
	   if(az_plot)  printf("%7.2f ",RAD2DEG(az));
	   if(ha_plot)  printf("%12.6f ",RAD2HR(ha));
	   if(lst_plot) printf("%12.6f ",RAD2HR(lst));
	 }
         for(k=0;k<n_sel_base;k++)
	 { for(l=SelChan[0];l<SelChan[1];l++)
	   { off=linfo.lhdr.data_off+(sel_base[k]*corr->daspar.channels+l)*2*sizeof(float);
             re=*((float*)(dbuf+off));
  	     im=*((float*)(dbuf+off+sizeof(float)));
	     if(HonourFlags)
	     { f_off=sel_base[k]*corr->daspar.channels+l;
	        f_ind=f_off/8; flag=dbuf+linfo.lhdr.flg_dat_off+f_ind;
		if (*flag & flag_mask[f_off%8]) re=im=0.0;
	     }
	     if(PrintType==0)
	       printf("%12.4e %12.4e ",re,im);
	     else
	       printf("%12.4e %7.2f ",pow((re*re+im*im),0.5),atan2(im,re)/M_PI*180.0);
	   }
	 }
	 printf("\n");
       }
       if(chan_plot)
       { BaselineType *base=&linfo.lhdr.vinfo.base[sel_base[0]];
         float freq;
	 int band=base->band[0];
	 printf("#* REC : %-5d       IST: %-12f\n",j,tm/3600.0);
	 for(l=SelChan[0];l<SelChan[1];l++)
	 { printf("%4d ",l);
	   freq=src->freq[band/2]+l*src->net_sign[band]*src->ch_width;
  	   if(freq_plot) printf("%12.5f ",freq/1.0e6);
	   for(k=0;k<n_sel_base;k++)
	   { off=linfo.lhdr.data_off+(sel_base[k]*corr->daspar.channels+l)*2*sizeof(float);
	     re=*((float*)(dbuf+off));
	     off +=4;
	     im=*((float*)(dbuf+off));
	     if(HonourFlags)
	     { f_off=sel_base[k]*corr->daspar.channels+l;
	        f_ind=f_off/8; flag=dbuf+linfo.lhdr.flg_dat_off+f_ind;
		if (*flag & flag_mask[f_off%8]) re=im=0.0;
	     }
	     if(PrintType==0)
	       printf("%12.4e %12.4e ",re,im);
	     else
	       printf("%12.4e %7.2f ",pow((re*re+im*im),0.5),atan2(im,re)/M_PI*180.0);
	   }
	   printf("\n");
	 }
       }
     }
   }

   free(dbuf);

   return 0;
}

int main(int argc, char **argv)
{

  optrega(&LtaFile,     OPT_STRING,'i',"in",     "Input LTA File");
  optrega(&Object,      OPT_STRING,'o',"object", "Object (Source Name)");
  optrega(&BaseNum,     OPT_INT,   'B',"basenum","baseline number");
  optrega(&PrintType,   OPT_INT,   'p',"print",  "print type");
  optrega(&HonourFlags, OPT_INT,   'f',"flags",  "honour flags? [0/1]");
  optrega(&XaxisType,   OPT_INT,   'x',"xaxis",  "xaxis type");
  optrega(&ShowSelf,    OPT_FLAG,  'S',"self",   "Show self correlations");
  optrega(&ShowCross,   OPT_FLAG,  'X',"cross",  "Show cross correlations");
  optrega(&ShowCoPol,   OPT_FLAG,  'P',"copol",  "Show copolar correlations");
  optrega(&ShowXPol,    OPT_FLAG,  'Q',"xpol",   "Show crosspolar correlations");
  optrega(&ShowModel,   OPT_FLAG,  'M',"model",  "Show model parameters");

  optrega_array(&NSelAnt, &SelAnt, OPT_STRING,'a',"ants", "Antenna List");
  optrega_array(&NSelBase,&SelBase,OPT_STRING,'b',"base", "Baseline List");
  optrega_array(&NSelChan,&SelChan,OPT_INT,   'c',"chan", "Channel Range");
  optrega_array(&NSelScan,&SelScan,OPT_INT,   's',"scan", "Scan List");
  optrega_array(&NSelRec, &SelRec, OPT_INT,   'r',"rec",  "Record Range");

  optMain(ltaprint);
  opt(&argc,&argv);

  sprintf(Usage,"Usage: %s -i InputLtaFile [-a ant1,ant2,..][-b base1,base2,...][-c ch1,chN]\n",argv[0]);
  sprintf(Usage+strlen(Usage),"                [-s scan1,scan2,...][-h HonourFlags][-r rec1,recN][-o Object]\n");
  sprintf(Usage+strlen(Usage),"                [-p PrintType][-x Xaxis][-B BaseNum][-S|-X|-P|-Q]\n");
  sprintf(Usage+strlen(Usage),"                Antenna, Baselines and Object are parsed as regex\n");
  sprintf(Usage+strlen(Usage),"                S == Self; X == Cross; P == CoPol; Q == CrossPol\n");
  sprintf(Usage+strlen(Usage),"                PrintType 0 == (Re,Im); 1 == (Amp,Phs)\n");
  sprintf(Usage+strlen(Usage),"                Xaxis 0 == IST Time\n");
  sprintf(Usage+strlen(Usage),"                      1 == ParAng(Deg)\n");
  sprintf(Usage+strlen(Usage),"                      2 == ElAng(Deg)\n");
  sprintf(Usage+strlen(Usage),"                      3 == AzAng(Deg)\n");
  sprintf(Usage+strlen(Usage),"                      4 == HourAng(Hrs)\n");
  sprintf(Usage+strlen(Usage),"                      5 == LST(Hrs)\n");
  sprintf(Usage+strlen(Usage),"                      6 == ChanNum\n");
  sprintf(Usage+strlen(Usage),"                      7 == Freq(MHz)\nn");

  optUsage(Usage);
  
  optTitle("Print selected data");

  return ltaprint(argc,argv);
}



  
