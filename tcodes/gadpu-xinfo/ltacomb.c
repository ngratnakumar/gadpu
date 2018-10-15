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

#define RAD2DEG(x)  ((x)*180.0/M_PI)
#define DEG2RAD(x)  ((x)/180.0*M_PI)

//static char rcsid[]="$Id: ltacomb.c,v 1.3 2005/11/22 10:12:08 das Exp $";

char **LtaFile=NULL;
int   NFiles=0;
char  Usage[256];

int ltacomb(int argc, char **argv)
{ LtaInfo linfo,linfo1;
  FILE    *fp,*ofp; 
  int     i,j,k,scanno,change_endian;
  char    *dbuf,object[16];
  double  dt;

  if(NFiles==0) { fprintf(stderr,Usage); return 1;}
  if((ofp=fopen("ltacomb_out.lta","w"))==NULL)
  { fprintf(stderr,"Cannot open file %s\n","ltacomb_out.lta"); return 1;}

  { int t=1;
    if (*((unsigned char *)&t) == 1) linfo.local_byte_order = LittleEndian;
    else linfo.local_byte_order=BigEndian;
  }
  scanno=0;
  dt=0;
  for(i=0;i<NFiles;i++)
  { if((fp=fopen(LtaFile[i],"r"))==NULL)
    { fprintf(stderr,"Cannot open %s\n",LtaFile[i]); return 1;}
    printf("INPUT   : %-s\n",LtaFile[i]);
    if(get_lta_hdr(fp,&linfo)) return 1;
    if(make_scantab(fp,&linfo))return 1;
    if(i==0)
    { if((dbuf=malloc(linfo.recl))==NULL)
      { fprintf(stderr,"malloc failure\n"); return 1;}
      write_hdr(&linfo,ofp); /* write global header */
      linfo1=linfo;
    }else 
    { if(cmp_global_hdr(&linfo,&linfo1,"abcsB")) 
      { fprintf(stderr,"LTA file headers do not match\n"); return 1;}
      dt=(linfo.corr.daspar.mjd_ref-linfo1.corr.daspar.mjd_ref)*86400.0;
      printf("Change in REF_TIME = %12.4f (sec)\n",dt);
    }
    if(linfo.data_byte_order!=linfo.local_byte_order)change_endian=1;
    else change_endian=0;
    for(j=0;j<linfo.scans;j++)
    { ScanInfoType *scan=&linfo.stab[j].scan;
      scan->t += dt;
      write_scan(scan,1,&linfo,j,linfo.stab[j].shdr.date_obs,ofp);
      rewind(fp);  /* go to start of data in this scan */
      ltaseek(fp,linfo.stab[j].start_rec+linfo.srecs,linfo.recl);
      for(k=0;k<linfo.stab[j].recs;k++)
      { double t;
	fread(dbuf,linfo.recl,1,fp);
        if(change_endian)flipData(dbuf,&linfo.lhdr);
	memcpy(&t,dbuf+linfo.lhdr.time_off,linfo.lhdr.time_size);
	t += dt;
	memcpy(dbuf+linfo.lhdr.time_off,&t,linfo.lhdr.time_size);
	update_signature(dbuf,scanno,k);
	fwrite(dbuf,linfo.recl,1,ofp);
      }
      strncpy(object,linfo.stab[j].shdr.object,11);object[11]='\0';
      printf("SCAN %-3d: %-12s %4d recs\n",scanno,object,
	     linfo.stab[j].recs);
      scanno++;
    }
    fclose(fp);
  }
  return 0;
}

int main(int argc, char **argv)
{
  optrega_array(&NFiles,&LtaFile,OPT_STRING,'i',"in", "Input LTA Files");
  sprintf(Usage,"Usage: %s -i InputLtaFile1[,File2[,File3...]\n",argv[0]);
  optUsage(Usage);
  optTitle("Combines LTA files\n");
  optMain(ltacomb);
  opt(&argc,&argv);

  return ltacomb(argc,argv);
}



  
