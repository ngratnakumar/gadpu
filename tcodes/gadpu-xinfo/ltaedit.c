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

enum {KnownScanKeys=8,KnownGlobalKeys=1};
char *LtaFile=NULL,Usage[2048],*GlobalPair=NULL,*ScanPair=NULL;
int  *SelScan=NULL,SelScans=0;

char *ScanKeyNames[KnownScanKeys]={"PROJECT","CODE","FIRST_LO","BB_LO","FREQ","OBJECT", "CALCODE", "QUAL"};
char *ScanKeyFormats[KnownScanKeys]={"%s","%s","%f,%f (Hz)","%f,%f (Hz)","%f,%f (Hz)","%s", "%d", "%s"};

char *GlobalKeyNames[KnownGlobalKeys]={"SAMP_NUM"};
char *GlobalKeyFormats[KnownGlobalKeys]={"%s"};

static char qualcode[8]={'P','A','B','F','X','X','X','X'};
  
int edit_global_hdr(char *buf)
{  char keyval[256],keyword[256],*p,key_pair[256];
   int  found=0,j;
   char *fill_line(char *p); /* defined in hdrsubs.c */

   strncpy(key_pair,GlobalPair,255);key_pair[255]='\0';
   if((p=index(key_pair,':'))==NULL)
   {fprintf(stderr,"Missing delimiter ':' in %s\n",key_pair); return -1;}
   *p=' ';
   sscanf(key_pair,"%s",keyword);
   strncpy(keyval,p+1,254-(p-key_pair));

   if(!strcasecmp(keyword,"SAMP_NUM"))
   { found=1;
     if((p=strstr(buf,"SAMP_NUM"))==NULL)
     { fprintf(stderr,"Keyword SAMP_NUM not found in the header!\n"); return -1;}
       sprintf(p,"%-8s= %s","SAMP_NUM",keyval); p = fill_line(p);
   }

   if(!found)
   { fprintf(stderr,"Ignoring unknown keyword %s\n",keyword);
     fprintf(stderr,"Editable Keywords (and expected formats) are:");
     for(j=0;j<KnownGlobalKeys;j++)
       fprintf(stderr,"%-12s [%s]",GlobalKeyNames[j],GlobalKeyFormats[j]);
     fprintf(stderr,"\n");
     return 1;
   }

   return 0;
}
int edit_scan_hdr(ScanInfoType *scan)
{  char keyval[256],keyword[256],*p,key_pair[256];
   int  found=0,j,k; 
   float val[2];

   strncpy(key_pair,ScanPair,255);key_pair[255]='\0';
   if((p=index(key_pair,':'))==NULL)
   {fprintf(stderr,"Missing delimiter ':' in %s\n",key_pair); return -1;}
   *p=' ';
   if(sscanf(key_pair,"%s %s",keyword,keyval)!=2)
   {fprintf(stderr,"Bad/Missing value in %s\n",ScanPair); return -1;}     
   if(!strcasecmp(keyword,"PROJECT"))
   { found=1;
     strncpy(scan->proj.title,keyval,NAMELEN);
     if(strlen(keyval)>=NAMELEN)scan->proj.title[NAMELEN-1]='\0';
   }
   if(!strcasecmp(keyword,"CODE"))
   { found=1;
     strncpy(scan->proj.code,keyval,7);
     if(strlen(keyval)>=7)scan->proj.code[7]='\0';
   }
   if(!strcasecmp(keyword,"FIRST_LO"))
   { found=1;
     for(j=0;j<strlen(keyval);j++)if(keyval[j]==',')keyval[j]=' ';
     if(sscanf(keyval,"%f %f",&val[0],&val[1])!=2)
     {fprintf(stderr,"Bad Values in %s\n",ScanPair); return -1;}     
     scan->source.first_lo[0]=val[0];scan->source.first_lo[1]=val[1];
   }
   if(!strcasecmp(keyword,"BB_LO"))
   { found=1;
     for(j=0;j<strlen(keyval);j++)if(keyval[j]==',')keyval[j]=' ';
     if(sscanf(keyval,"%f %f",&val[0],&val[1])!=2)
     {fprintf(stderr,"Bad Values in %s\n",ScanPair); return -1;}     
     scan->source.bb_lo[0]=val[0];scan->source.bb_lo[1]=val[1];
   }
   if(!strcasecmp(keyword,"FREQ"))
   { found=1;
     for(j=0;j<strlen(keyval);j++)if(keyval[j]==',')keyval[j]=' ';
     if(sscanf(keyval,"%f %f",&val[0],&val[1])!=2)
     {fprintf(stderr,"Bad Values in %s\n",ScanPair); return -1;}     
     scan->source.freq[0]=val[0];scan->source.freq[1]=val[1];
   }
   if(!strcasecmp(keyword,"OBJECT"))
   { found=1;
     strncpy(scan->source.object,keyval,NAMELEN);
     if(strlen(keyval)>=NAMELEN)scan->source.object[NAMELEN-1]='\0';
   }
   if(!strcasecmp(keyword,"CALCODE"))
   { found=1;
     scan->source.calcode=(int)atoi(keyval);
   }
   if(!strcasecmp(keyword,"QUAL"))
   { found=1;
     scan->source.qual=0;
     for(j=0;j<strlen(keyval);j++) 
       for(k=0;k<4;k++)if(keyval[j]==qualcode[k]){scan->source.qual |= 1<<k;}
   }
   if(!found)
   { fprintf(stderr,"Ignoring unknown keyword %s\n",keyword);
     fprintf(stderr,"Editable Keywords (and expected formats) are:");
     for(j=0;j<KnownScanKeys;j++)
       fprintf(stderr,"%-12s [%s]",ScanKeyNames[j],ScanKeyFormats[j]);
     fprintf(stderr,"\n");
     return 1;
   }
	
   return 0;
}
int ltaedit (int argc, char **argv)
{  LtaInfo      linfo;
   FILE         *fp;
   int          i,edit_global=0,edit_scan=0,change_endian;
   char         *hdr_buf;

   if(LtaFile==NULL) {fprintf(stderr,"No Input file\n%s\n",Usage); return 1;}
   if((fp=fopen(LtaFile,"r+"))==NULL)
   { fprintf(stderr,"Cannot open File %s\n%s",LtaFile,Usage); return 1;}
   if(GlobalPair!=NULL)edit_global=1;
   if(ScanPair!=NULL)edit_scan=1;
   

   { int t=1;
     if (*((unsigned char *)&t) == 1) linfo.local_byte_order = LittleEndian;
     else linfo.local_byte_order=BigEndian;
   }

   if(get_lta_hdr(fp,&linfo)) return 1;
   if(make_scantab(fp,&linfo)) return 1;

   if(linfo.local_byte_order!=linfo.data_byte_order)change_endian=1;
   else change_endian=0;
	
   if(edit_global)
   { int arecs=linfo.lrecs-linfo.lbrecs;
     int  recl=linfo.recl;
     rewind(fp);
     if((hdr_buf=malloc(recl*arecs))==NULL)
     { fprintf(stderr,"malloc error!\n"); return -1;}
     if(fread(hdr_buf,recl,arecs,fp)!=arecs)
     { fprintf(stderr,"error reading global header!\n"); return -1;}
     if(edit_global_hdr(hdr_buf)) return -1;
     rewind(fp);
     if(fwrite(hdr_buf,recl,arecs,fp)!=arecs)
     { fprintf(stderr,"error writing global header!\n"); return -1;}
     free(hdr_buf);
   }

   for(i=0;i<linfo.scans;i++)
   { int j;
     ScanInfoType *scan=&linfo.stab[i].scan;
     if(SelScans)
     { for(j=0;j<SelScans;j++)if(i==SelScan[j])break;
       if(j==SelScans) continue;
     }
     if(edit_scan)if(edit_scan_hdr(scan)) return -1;
     rewind(fp); ltaseek(fp,linfo.stab[i].start_rec,linfo.recl);
     write_scan(scan,1,&linfo,i,linfo.stab[i].shdr.date_obs,fp);
   }

   fclose(fp);
   return 0;
}

int main(int argc, char **argv)
{ int j;
  optrega(&LtaFile,   OPT_STRING,'i',"infile",   "Input LTA File");
  optrega(&GlobalPair,OPT_STRING,'g',"globalpar","GLOBAL KEYWORD TO CHANGE");
  optrega(&ScanPair,  OPT_STRING,'s',"scanpar",  "SCAN KEYWORD TO CHANGE");
  optrega_array(&SelScans,&SelScan,OPT_INT, 'S',"scans", "SCANS TO EDIT");

  sprintf(Usage,"-i InputLtaFile [-g GKW:GKV][-s SKW:SKV][-S(selscans)]\n");
  sprintf(Usage+strlen(Usage),"No global keyword change enabled yet!\n");
  sprintf(Usage+strlen(Usage),"Editable Scan keywords (and expected formats):\n");
  for(j=0;j<KnownScanKeys;j++)
    sprintf(Usage+strlen(Usage),"%-12s [%s]\n",ScanKeyNames[j],ScanKeyFormats[j]);
  optUsage(Usage);
 
  optTitle("Edits the Global and/or Scan Headers of an LTA file\n");

  optMain(ltaedit);
  opt(&argc,&argv);

  return ltaedit(argc,argv);
}



  
