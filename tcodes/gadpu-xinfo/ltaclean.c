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

//static char rcsid[]="$Id: ltaclean.c,v 1.5 2005/11/22 10:11:20 das Exp $";

char *LtaFile=NULL;
char  Usage[256];
int   MaxScan=64;
int   EatRec=0;
int   Verbose=0;
int ltaclean(int argc, char **argv)
{  LtaInfo      linfo; 
   FILE         *fp,*ofp;
   int          change_endian,scannum=0,recnum=0;
   char         object[12],outfile[256];
   char         *dbuf=NULL;
   int          found_scan,found_data,sync_lost=0;
   long         recl,off;
   char         seq[24];

   strcpy(outfile,"ltaclean_out.lta");
   if((ofp=fopen(outfile,"w"))==NULL)
   { fprintf(stderr,"cannot open %s\n",outfile); return 1;}

   { int t=1;
     if (*((unsigned char *)&t) == 1) linfo.local_byte_order = LittleEndian;
     else linfo.local_byte_order=BigEndian;
   }
   
   if(LtaFile==NULL) { fprintf(stderr,Usage); return 1;}
   if((fp=fopen(LtaFile,"r"))==NULL)
   { fprintf(stderr,"Cannot open %s\n",LtaFile); return 1;}
   if(get_lta_hdr(fp,&linfo))
   { fprintf(stderr,"Error reading %s\n",LtaFile); return 1;}
   if(linfo.local_byte_order!=linfo.data_byte_order)change_endian=1;
   else change_endian=0;

   recl=linfo.recl;
   if((dbuf=malloc(recl))==NULL)
   {fprintf(stderr,"MallocError\n"); return 1;}
   
   write_hdr(&linfo,ofp); /* generate GLOBAL header */
   rewind(fp);ltaseek(fp,linfo.lrecs,recl);
   scannum=0; object[11]='\0'; 
   if((linfo.stab=(ScanHdrTab *)malloc(sizeof(ScanHdrTab)*MaxScan))==NULL)
   { fprintf(stderr,"Malloc Failure\n"); return -1;}
   while(1)
   { ScanInfoType *scan=&linfo.stab[scannum].scan;
     char         *date_obs=linfo.stab[scannum].shdr.date_obs;
     ScanHdr      *shdr=&linfo.stab[scannum].shdr;
     found_scan=1;
     if(get_scan_hdr(fp,&linfo,scannum))
     { sync_lost=1;found_scan=found_data=0;
       fprintf(stderr,"Sync lost, resyncing...\n");
       while(1)
       { if(fread(dbuf,recl,1,fp)!=1)
         { fprintf(stderr,"File Read Error\n"); return 1;}
	 if(Verbose){strncpy(seq,dbuf,14);seq[14]='\0'; printf("%s\n",seq);}
         if((off=locate("DATA",4,dbuf,recl))<0)
         { if((off=locate("SCAN",4,dbuf,recl))<0) continue;
   	   else { found_scan=1; fseek(fp,off-recl,SEEK_CUR); break;}
	 }
  	 else { found_data=1; fseek(fp,off-recl,SEEK_CUR); break;}
       }
     }	
     if(found_scan)
     { if(sync_lost){get_scan_hdr(fp,&linfo,scannum);}
       write_scan(scan,1,&linfo,scannum,date_obs,ofp);/* generate SCAN Hdr */
       strncpy(object,shdr->object,11);object[11]='\0';
     }
     if(sync_lost && found_data)scannum--;
     else recnum=0;
     if(fread(dbuf,recl,1,fp) !=1) return 0;
     if(Verbose){strncpy(seq,dbuf,14);seq[14]='\0'; printf("%s\n",seq);}
     while(!strncmp(dbuf,"DATA",4))
     { if(change_endian)flipData(dbuf,&linfo.lhdr);
       update_signature(dbuf,scannum,recnum-EatRec);
       if(recnum>=EatRec)fwrite(dbuf,recl,1,ofp);
       if(Verbose){strncpy(seq,dbuf,14);seq[14]='\0'; printf("%s\n",seq);}
       if(fread(dbuf,recl,1,fp) !=1)
       { printf("SCAN %03d: %-12s [%4d recs]\n",scannum,object,recnum); return 0;}
       recnum++; 
     }
     if(sync_lost && found_data) printf(" RECOVERED:");
     printf("SCAN %03d: %-12s [%4d recs]\n",scannum,object,recnum);
     fseek(fp,-1*recl,SEEK_CUR);
     scannum++; sync_lost=0;
     if(scannum==MaxScan)
     { fprintf(stderr,"MaxScan [%d] reached!\n",MaxScan);
       fprintf(stderr,"Please try again with a larger MaxScan\n");
       return -1;
     }
   }
   free(dbuf);
   fclose(ofp);

   return 0;
}

int main(int argc, char **argv)
{
  optrega(&LtaFile,OPT_STRING,'i',"in", "Input LTA File");
  optrega(&MaxScan,OPT_INT,'m',"maxscans", "Max no of scans");
  optrega(&EatRec,OPT_INT,'e',"eat", "Records to eat");
  optrega(&Verbose,OPT_INT,'v',"verbose", "verbose");

  sprintf(Usage,"Usage: %s -i InputLtaFile [-m (maxscans)][-e (eat)][-v (verbose)]\n",argv[0]);
  optUsage(Usage);
  
  optTitle("\"Cleans\" up format errors LTA Files\n");
  optMain(ltaclean);
  opt(&argc,&argv);


  return ltaclean(argc,argv);
}



  
