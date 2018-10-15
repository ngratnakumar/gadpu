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

//static char rcsid[]="$Id: ltabcal.c,v 1.2 2005/11/22 10:09:50 das Exp $";

char *LtaFile=NULL,Usage[256];
int   NCalList,*CalList=NULL;

int ltabcal(int argc, char **argv)
{  LtaInfo      linfo;
   FILE         *fp,*ofp;
   char         outfile[256],object[16];
   char         *dbuf=NULL,*calbuf=NULL;
   off_t        recl;
   int          i,j,change_endian,*calscans,*calindex,*madecal,ncal;
      
   if(LtaFile==NULL) { fprintf(stderr,Usage); return 1;}
   if((fp=fopen(LtaFile,"r"))==NULL)
   { fprintf(stderr,"Cannot open %s\n",LtaFile); return 1;}
   strcpy(outfile,"ltabcal_out.lta");
   if((ofp=fopen(outfile,"w"))==NULL)
   { fprintf(stderr,"cannot open %s\n",outfile); return 1;}
   if(NCalList%2)
   { fprintf(stderr,"Incomplete Calibration List\n"); return 1;}
   { int t=1;
     if (*((unsigned char *)&t) == 1) linfo.local_byte_order = LittleEndian;
     else linfo.local_byte_order=BigEndian;
   }
   if(get_lta_hdr(fp,&linfo)) return 1;
   if(make_scantab(fp,&linfo)) return 1;
   if(linfo.data_byte_order!=linfo.local_byte_order)change_endian=1;
   else change_endian=0;
   if(((calscans=(int*)malloc(linfo.scans*sizeof(int)))==NULL)||
      ((calindex=(int*)malloc(linfo.scans*sizeof(int)))==NULL)||
      ((madecal =(int*)malloc(linfo.scans*sizeof(int)))==NULL))
     { fprintf(stderr,"MallocError\n"); return 1;}
   for(i=0;i<linfo.scans;i++)
   { for(j=0;j<NCalList;j+=2)
     if(i==CalList[j]){calscans[i]=CalList[j+1];break;}
     if(j==NCalList)calscans[i]=-1;
   }
   for(ncal=0,i=0;i<linfo.scans;i++)
   { if(calscans[i]>=linfo.scans)
     {fprintf(stderr,"Illegal calibration scan %d\n",calscans[i]); return 1;}
     if(calscans[i]>-1)
     { for(j=0;j<i;j++)if(calscans[i]==calscans[j]) break;
       if(j==i)calindex[i]=ncal++; 
       else calindex[i]=calindex[j];
       madecal[i]=0;
     }
   }
   recl=linfo.recl;
   if(((calbuf=malloc(recl*ncal))==NULL)||((dbuf=malloc(recl))==NULL))
   {fprintf(stderr,"MallocError\n"); return 1;}

   rewind(fp); 
   for(i=0;i<linfo.lrecs;i++) /* copy the LTA Header */
   {fread(dbuf,recl,1,fp);fwrite(dbuf,recl,1,ofp);}

   for(i=0;i<linfo.scans;i++)
   { int recs=linfo.stab[i].recs;
     rewind(fp);ltaseek(fp,linfo.stab[i].start_rec,linfo.recl);
     for(j=0;j<linfo.srecs;j++) /* copy the scan header */
     { fread(dbuf,recl,1,fp);fwrite(dbuf,recl,1,ofp);}
     strncpy(object,linfo.stab[i].shdr.object,11);object[11]='\0';
     if(calscans[i]>-1 && !madecal[calindex[i]])
     { if(mk_median(change_endian,calscans[i],calbuf+(calindex[i]*recl),&linfo,fp)) return 1;
       rewind(fp);ltaseek(fp,linfo.stab[i].start_rec+linfo.srecs,linfo.recl);
       madecal[calindex[i]]=1;
     }
     for(j=0;j<recs;j++)
     { if(fread(dbuf,recl,1,fp) !=1) return 0;
       if(calscans[i]>-1)
       { if(change_endian)flipData(dbuf,&linfo.lhdr);
         if(do_bcal(dbuf,calbuf+(calindex[i]*recl),&linfo.lhdr)) return 1;
	 do_avspc(dbuf,&linfo.lhdr);
	 /*memcpy(dbuf+linfo.lhdr.data_off,calbuf+(calindex[i]*recl+linfo.lhdr.data_off),recl-linfo.lhdr.data_off);*/
	 if(change_endian)flipData(dbuf,&linfo.lhdr);
       }
       fwrite(dbuf,recl,1,ofp);
     }
     printf("SCAN %-3d: %-12s %-4drecords\n",i,object,recs);
   }

   free(dbuf); free(calbuf);
   fclose(ofp);

   return 0;
}

int main(int argc, char **argv)
{
  optrega(&LtaFile,OPT_STRING,'i',"in", "Input LTA File");
  optrega_array(&NCalList,&CalList,OPT_INT,'c',"CalList","Calibration info");

  sprintf(Usage,"Usage: %s -i InputLtaFile [-c s0,c0,s1,c1,...]\n",argv[0]);
  optUsage(Usage);
  
  optTitle("Baseline based calibration of LTA files\n");
  optMain(ltabcal);
  opt(&argc,&argv);


  return ltabcal(argc,argv);
}



  
