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

//static char rcsid[]="$Id: ltaint.c,v 1.4 2009/11/11 12:49:32 das Exp $";

char *LtaFile=NULL,Usage[256];

int ltaint(int argc, char **argv)
{  LtaInfo      linfo;
   FILE         *fp;
   char         *dbuf=NULL,*ltahdr=NULL;
   off_t        recl;
   float        integ;

   if(LtaFile==NULL) { fprintf(stderr,Usage); return 1;}
   if((fp=fopen(LtaFile,"r"))==NULL)
   { fprintf(stderr,"Cannot open %s\n",LtaFile); return 1;}
   { int t=1;
     if (*((unsigned char *)&t) == 1) linfo.local_byte_order = LittleEndian;
     else linfo.local_byte_order=BigEndian;
   }
   if(get_lta_hdr(fp,&linfo)) return 1;
   if(make_scantab(fp,&linfo)) return 1;

   recl=linfo.recl;
   if((dbuf=malloc(recl))==NULL)
   {fprintf(stderr,"MallocError\n"); return 1;}
   rewind(fp); /* copy the LTA Header */
   if((ltahdr=malloc(recl*linfo.lrecs))==NULL)
   {fprintf(stderr,"MallocError\n"); return 1;}
   fread(ltahdr,recl*linfo.lrecs,1,fp);
   free(ltahdr);

   integ = linfo.stab[0].shdr.integ;
   fprintf(stderr,"\nInteg Time : %6.3f secs\n\n",integ);

   free(dbuf);
   fclose(fp);

   return 0;
}

int main(int argc, char **argv)
{
  optrega(&LtaFile,OPT_STRING,'i',"in", "Input LTA File");
  sprintf(Usage,"Usage: %s -i InputLtaFile \n",argv[0]);
  optUsage(Usage);
  
  optTitle("Prints LTA interval of LTA files\n");
  optMain(ltaint);
  opt(&argc,&argv);


  return ltaint(argc,argv);
}



  
