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

//static char rcsid[]="$Id: relta.c,v 1.4 2005/11/22 10:19:32 das Exp $";

char *LtaFile=NULL,Usage[256];
int   Lta2, ChInt;

int relta(int argc, char **argv)
{  LtaInfo      linfo;
   FILE         *fp,*ofp;
   char         outfile[256],object[16];
//   char         *dbuf=NULL,*ltahdr=NULL,*scanhdr=NULL,*ltabuf=NULL;
   char         *dbuf=NULL,*scanhdr=NULL,*ltabuf=NULL;
   off_t        recl;
//   float        integ;
   int          i,j,k,lta,change_endian;

   if(LtaFile==NULL) { fprintf(stderr,Usage); return 1;}
   if((fp=fopen(LtaFile,"r"))==NULL)
   { fprintf(stderr,"Cannot open %s\n",LtaFile); return 1;}
   strcpy(outfile,"relta_out.lta");
   if((ofp=fopen(outfile,"w"))==NULL)
   { fprintf(stderr,"cannot open %s\n",outfile); return 1;}
   if(Lta2 <2)
   { fprintf(stderr,"Lta2=%d invalid (<2)\n%s\n",Lta2,Usage); return 1;}
   //if(ChInt <2)
   //{ fprintf(stderr,"NChan=%d invalid (<2)\n%s\n",ChInt,Usage); return 1;}

   { int t=1;
     if (*((unsigned char *)&t) == 1) linfo.local_byte_order = LittleEndian;
     else linfo.local_byte_order=BigEndian;
   }
   if(get_lta_hdr(fp,&linfo)) return 1;
   if(make_scantab(fp,&linfo)) return 1;
   if(linfo.data_byte_order!=linfo.local_byte_order)change_endian=1;
   else change_endian=0;

   recl=linfo.recl;
   if((dbuf=malloc(recl))==NULL||(ltabuf=malloc(recl))==NULL)
   {fprintf(stderr,"MallocError\n"); return 1;}

   lta=linfo.lhdr.vinfo.lta*Lta2;
   linfo.corr.daspar.lta=lta;
   write_hdr(&linfo,ofp);

   if((scanhdr=malloc(recl*linfo.srecs))==NULL)
   { fprintf(stderr,"MallocError\n"); return 1;}
   for(i=0;i<linfo.scans;i++)
   {  ScanInfoType *scan=&linfo.stab[i].scan;
      char         *date_obs=linfo.stab[i].shdr.date_obs;
      int recs = linfo.stab[i].recs;

//	fprintf(stdout, "SCAN # %d INTEG = %f\t", i, linfo.stab[i].shdr.integ);

     linfo.stab[i].shdr.integ*=Lta2; 
//	fprintf(stdout, "SCAN # %d INTEG = %f\n", i, linfo.stab[i].shdr.integ);
     write_scan(scan,1,&linfo,i,date_obs,ofp);

//     edit_ascii_hdr(scanhdr, "INTEG",&integ,'f',"%f ");
//     fwrite(scanhdr,recl*linfo.srecs,1,ofp);
     strncpy(object,linfo.stab[i].shdr.object,11);object[11]='\0';
//     fprintf(stderr,"Working on scan %-3d [%-12s]   ",i,object);
     rewind(fp);ltaseek(fp,linfo.stab[i].start_rec+linfo.srecs,recl);
     for(j=0;j<recs;j++)
     { if(fread(dbuf,recl,1,fp) !=1) return 0;
       if(change_endian)flipData(dbuf,&linfo.lhdr);
       if(j%Lta2==0)
         memcpy(ltabuf,dbuf,recl);
       else
	 do_average(ltabuf,dbuf,&linfo.lhdr);
       if((j+1)%Lta2==0)
       { sprintf(ltabuf,"%4s%04d.%05d  ","DATA",i,j/Lta2);
	 ltabuf[16]=' ';
	 if(change_endian)flipData(ltabuf,&linfo.lhdr);
	 fwrite(ltabuf,recl,1,ofp);
       }
     }
     if((k=recs%Lta2)!=0)
     { sprintf(ltabuf,"%4s%04d.%05d  ","DATA",i,j/Lta2);
       ltabuf[16]=' ';
       if(change_endian)flipData(ltabuf,&linfo.lhdr);
       fwrite(ltabuf,recl,1,ofp);
     }
     printf("SCAN %-3d: %-12s %4d+%2d/%-3d records\n",
	    i,object,recs/Lta2,recs%Lta2,Lta2); 
   }

   free(dbuf);
   free(scanhdr);

   fclose(ofp);
   return 0;
}

int main(int argc, char **argv)
{
  optrega(&LtaFile,OPT_STRING,'i',"in", "Input LTA File");
  optrega(&Lta2,OPT_INT,'l',"Lta2","Num of Recs to Average");
  optrega(&ChInt,OPT_INT,'c',"NCHAN","Num of Chans to Average");

  sprintf(Usage,"Usage: %s -i InputLtaFile -l Lta2 -c NChan\n",argv[0]);
  optUsage(Usage);
  
  optTitle("Averages Records in LTA files\n");
  optMain(relta);
  opt(&argc,&argv);

  return relta(argc,argv);
}



  
