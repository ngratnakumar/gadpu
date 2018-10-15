#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <lta.h>
#include <newcorr.h>
#include <unistd.h>


//static char rcsid[]="$Id: xinfo.c,v 1.5 2004/07/19 05:58:01 das Exp $";

#define WRITE_FIELD(name,type,fmt,val)  { fprintf(ifp,"%s %s \n",type, name);\
                                        fprintf(dfp,fmt,val); }

#define CLASSIC_LTA 1
#define CLASSIC_LTB 2
#define GSB_LTA 3

int scansCounter=0;
int resetScanSize=0;

int xinfo_observation(CorrType *corr, ScanInfoType *scan,char *obsno, FILE *dfp, char *logfile)
{
    fprintf(dfp, "\n {\"observation_details\": {\"observation_no\": \"%d\", \"proj_code\": \"%s\", \"logfile\": \"%s\" },", atoi(obsno), scan->proj.code, logfile);
    return 0;
}	
                        
double getFileSize ( FILE * hFile )
{
    off_t lCurPos, lEndPos;
        // Check for valid file pointer
    if ( hFile == NULL )
    {
        return -1;
    }
        // Store current position
    lCurPos = ftello ( hFile );
        // Move to the end and get position
    fseeko ( hFile, 0, SEEK_END );
    lEndPos = ftello ( hFile );
        // Restore the file pointer
    fseeko ( hFile, lCurPos,SEEK_SET );
    //double fileSize=lEndPos/pow(10,6);
    double fileSize=lEndPos/(1024.0*1024.0);
    return fileSize;

}


int xinfo_global_hdr(CorrType *corr, ScanInfoType *scan,char *obsno, FILE *dfp,
                     char *ltaltbFileWithPath, int isLTA, FILE *filePointer)
{ 

    char  ltaltbFileName[200];
    double fileSize=0;
    char path[1000];  
    int lastSlashIndex = -1;
    strcpy(path,"");

    fileSize=getFileSize(filePointer);  //get file size of lta/b file

    for(lastSlashIndex = strlen(ltaltbFileWithPath)-1; lastSlashIndex >=0 ; lastSlashIndex--) //reverse loop through ltaltbFileWithPath having path+filename 
    {
        if(ltaltbFileWithPath[lastSlashIndex] == '/') //pointer at last slash found 
            break;
    }


    if(lastSlashIndex == -1) //if no slash found, ltaltbFileWithPath is itself the ltaltbFileName
    {
        strcpy(path, "");
        strcpy(ltaltbFileName, ltaltbFileWithPath);
    }
    else
    {
        strncpy(path, ltaltbFileWithPath, lastSlashIndex+1);  //copy ltaltbFileWithPath till the slash before ltafilename , 3rd paramter is the size of charactesr to copy
        path[lastSlashIndex+1] = '\0';
    //    ltaltbFileName = ltaltbFileWithPath + lastSlashIndex + 1;
        strcpy(ltaltbFileName, (ltaltbFileWithPath + lastSlashIndex + 1) );
        //ltaltbFileName[strlen(ltaltbFileWithPath)] = '\0';
        
    }

    //code to extract the filename without extension
    char *pointerToFileExtension;
    if(isLTA == GSB_LTA)
        pointerToFileExtension = strstr(ltaltbFileName,"_gsb.lta");
    else if(isLTA == CLASSIC_LTA)
        pointerToFileExtension = strstr(ltaltbFileName,".lta");
    else if(isLTA == CLASSIC_LTB)
        pointerToFileExtension = strstr(ltaltbFileName,".ltb");

    char ltafileWithoutExtension[ (pointerToFileExtension - ltaltbFileName) + 1 ];
    //if abc.lta, pointerToFileExtension will point to "." and ltaltbFileName
    //to start of file.so subtracting it will give "abc"
    strncpy(ltafileWithoutExtension, ltaltbFileName, (pointerToFileExtension-ltaltbFileName));
    ltafileWithoutExtension[(pointerToFileExtension-ltaltbFileName)]='\0';

    //extract extension including . or _ before the extension
    //we need to extract full extension as files have names like abc.lta.1
    //in this case full extension is .lta.1
    char fullExtension[20];
    if(isLTA == CLASSIC_LTA)
    	strcpy(fullExtension, strstr(ltaltbFileName,".lta"));
    else if(isLTA == CLASSIC_LTB)
    	strcpy(fullExtension, strstr(ltaltbFileName,".ltb"));
    else if(isLTA == GSB_LTA)
        strcpy(fullExtension, strstr(ltaltbFileName,"_gsb.lta"));
    

    char dataFileColToBeUpdated[20];
    char fileSizeCol[20];
    char col1[20];
    char col2[20];
    char file1[200];
    char file2[200];
    
    //if its a lta file
    if(isLTA == CLASSIC_LTA){

        //we have to check in the query further down if a scangroup
        //tuple exists for this lta having gsb or ltb file corresponding
        //to this lta file.
        strcpy(dataFileColToBeUpdated, "lta_file");
        strcpy(fileSizeCol, "lta_file_size");
        strcpy(col1, "ltb_file");
        strcpy(col2, "lta_gsb_file");
        
        // assuming file name is within 200 chars
        char extension1[20];
        char extension2[20];

        //make ltb file name - for abc.lta.1 the ltb file name is abc.ltb.1        
        strcpy(file1, ltafileWithoutExtension);
        strcpy(extension1, fullExtension);
        strncpy(strstr(extension1,"lta"), "ltb",3);
        strcat(file1, extension1);

        //make gsb_lta file name - for abc.lta.1 the gsb-lta file name is abc_gsb.lta.1
        strcpy(file2, ltafileWithoutExtension);
        strcpy(extension2, "_gsb");
        strcat(extension2, fullExtension);
        strcat(file2, extension2);

    } else if(isLTA == CLASSIC_LTB) {

        strcpy(dataFileColToBeUpdated, "ltb_file");
        strcpy(fileSizeCol, "ltb_file_size");
        strcpy(col1, "lta_file");
        strcpy(col2, "lta_gsb_file");

        // assuming file name is within 200 chars
        char extension1[20];
        char extension2[20];

        //make lta file name - for abc.ltb.1 the lta file name is abc.lta.1
        strcpy(file1, ltafileWithoutExtension);
        strcpy(extension1, fullExtension);
        strncpy(strstr(extension1,"ltb"), "lta",3);
        strcat(file1, extension1);

        //make gsb_lta file name - for abc.ltb.1 the gsb-lta file name is abc_gsb.lta.1
        strcpy(file2, ltafileWithoutExtension);
        strcpy(extension2, "_gsb");
        strcat(extension2, extension1);
        strcat(file2, extension2);
        
    } else if(isLTA == GSB_LTA) {
        
        strcpy(dataFileColToBeUpdated, "lta_gsb_file");
        strcpy(fileSizeCol, "lta_gsb_file_size");
        strcpy(col1, "lta_file");
        strcpy(col2, "ltb_file");

        // assuming file name is within 200 chars
        char extension1[20];
        char extension2[20];

        //make lta file name - for abc_gsb.lta.1 the lta file name is abc.lta.1
        strcpy(file1, ltafileWithoutExtension);
        strcpy(extension1, fullExtension+strlen("_gsb"));
        strcat(file1, extension1);

        //make ltb file name - for abc_gsb.lta.1 the ltb file name is abc.ltb.1
        strcpy(file2, ltafileWithoutExtension);
        strcpy(extension2, extension1);
        strncpy(strstr(extension2, "lta"), "ltb", 3);
        strcat(file2, extension2);        
    }

    if(resetScanSize == 1){
	    	    fprintf(dfp, ",\n \"scangroup\": {\"corr_version\": \"%s\", \"sta_time\": \"%d\", \"num_pols\": \"%d\", \"num_chans\": \"%d\", \"lta_time\": \"%d\", \"fileSize_val\": \"%lf\" }, \"scans\": ["\
    , corr->version, corr->corrpar.statime, corr->corrpar.pols,
            corr->daspar.channels, corr->daspar.lta, fileSize);
	    	resetScanSize=0;
	    } else {
	    	    fprintf(dfp, "\n \"scangroup\": { \"corr_version\": \"%s\", \"sta_time\": \"%d\", \"num_pols\": \"%d\", \"num_chans\": \"%d\", \"lta_time\": \"%d\", \"fileSize_val\": \"%lf\" }, \"scans\": ["\
    , corr->version, corr->corrpar.statime, corr->corrpar.pols,
            corr->daspar.channels, corr->daspar.lta, fileSize);
	    }
    return 0;
}   

int xinfo_scan_hdr(CorrType *corr, ScanInfoType *scan, int datarecs, 
                ScanHdr *shdr, char *ltapath, char *obsno, int lscannum,FILE *dfp, int isLTA, int scansSize)
{
    double time_on_src=corr->daspar.lta*(((float)corr->corrpar.statime)/1.0e6)*datarecs;
    char  *ltafile;
    double epoch,epoch1,ra,dec;
    void sla_preces_(char *system,double *epoch, double *epoch1,
                    double *ra, double *dec,int len);
    int lastSlashIndex = -1;
    char path[1000];
    strcpy(path, "");

    // printf("%d\n", scansSize);

    //reverse loop through ltaltbFileWithPath having path+filename
    for(lastSlashIndex = strlen(ltapath)-1; lastSlashIndex >=0 ; lastSlashIndex--)
    {
        if(ltapath[lastSlashIndex] == '/') //pointer at last slash found 
            break;
    }

    //if no slash found, ltaltbFileWithPath is itself the ltaltbFileName
    if(lastSlashIndex == -1) 
    {
    
        ltafile = ltapath;
        strcpy(path, "");
    }
    else
    {
        strncpy(path, ltapath, lastSlashIndex+1);  //copy ltaltbFileWithPath till the slash before ltafilename , 3rd paramter is the size of charactesr to copy
        path[lastSlashIndex+1] = '\0';
        ltafile = ltapath + lastSlashIndex + 1;
         
    }

    epoch=2000.0 + (corr->daspar.mjd_ref - 51544.5)/365.25 ;
    epoch1=2000.0;
    ra=scan->source.ra_mean;dec=scan->source.dec_mean;
    sla_preces_("FK5",&epoch,&epoch1,&ra,&dec,3);  /* J2000 co-ordinates */

    char dataFileColumnInDb[20];
    if(isLTA == CLASSIC_LTA){
        strcpy(dataFileColumnInDb, "lta_file");
    } else if(isLTA == CLASSIC_LTB) {
        strcpy(dataFileColumnInDb, "ltb_file");
    } else if(isLTA == GSB_LTA) {
        strcpy(dataFileColumnInDb, "lta_gsb_file");
    }



    if(scansCounter == 1) {
		fprintf(dfp,"\n {\"source\": \"%s\", \"ra_2000\": %lf, \"dec_2000\": %lf, \"date_obs\": \"%s\", \"ant_mask\": \"%d\", \"band_mask\": \"%d\", \"calcode\": \"%d\", \"qual\": \"%d\", \"ra_date\": \"%f\", \"dec_date\": \"%f\", \"dra\": \"%f\",\"ddec\": \"%f\", \"sky_freq1\": \"%f\", \"sky_freq2\": \"%f\", \"rest_freq1\": \"%f\", \"rest_freq2\": \"%f\", \"lsr_vel1\": \"%f\", \"lsr_vel2\": \"%f\", \"chan_width\": \"%f\", \"net_sign1\": \"%d\", \"net_sign2\": \"%d\", \"net_sign3\": \"%d\", \"net_sign4\": \"%d\", \"onsrc_time\": \"%lf\" ,\"source_position\": \"spoint(%lf,%lf)\"}]",scan->source.object, ra, dec, shdr->date_obs, scan->source.antmask, scan->source.bandmask, scan->source.calcode, scan->source.qual,  scan->source.ra_mean, scan->source.dec_mean, scan->source.dra, scan->source.ddec, scan->source.freq[0], scan->source.freq[1], scan->source.rest_freq[0], scan->source.rest_freq[1], scan->source.lsrvel[0], scan->source.lsrvel[1], scan->source.ch_width, scan->source.net_sign[0], scan->source.net_sign[1], scan->source.net_sign[2], scan->source.net_sign[3], time_on_src,ra,dec);
    		resetScanSize++;
    } else {
    	fprintf(dfp,"\n {\"source\": \"%s\", \"ra_2000\": %lf, \"dec_2000\": %lf, \"date_obs\": \"%s\", \"ant_mask\": \"%d\", \"band_mask\": \"%d\", \"calcode\": \"%d\", \"qual\": \"%d\", \"ra_date\": \"%f\", \"dec_date\": \"%f\", \"dra\": \"%f\",\"ddec\": \"%f\", \"sky_freq1\": \"%f\", \"sky_freq2\": \"%f\", \"rest_freq1\": \"%f\", \"rest_freq2\": \"%f\", \"lsr_vel1\": \"%f\", \"lsr_vel2\": \"%f\", \"chan_width\": \"%f\", \"net_sign1\": \"%d\", \"net_sign2\": \"%d\", \"net_sign3\": \"%d\", \"net_sign4\": \"%d\", \"onsrc_time\": \"%lf\" ,\"source_position\": \"spoint(%lf,%lf)\"},",scan->source.object, ra, dec, shdr->date_obs, scan->source.antmask, scan->source.bandmask, scan->source.calcode, scan->source.qual,  scan->source.ra_mean, scan->source.dec_mean, scan->source.dra, scan->source.ddec, scan->source.freq[0], scan->source.freq[1], scan->source.rest_freq[0], scan->source.rest_freq[1], scan->source.lsrvel[0], scan->source.lsrvel[1], scan->source.ch_width, scan->source.net_sign[0], scan->source.net_sign[1], scan->source.net_sign[2], scan->source.net_sign[3], time_on_src,ra,dec);
        	scansCounter--;

    }
    return 0;
}   

void printUsage() {
    fprintf(stderr,"USAGE: xinfo -l lta[,lta1,...] "
                    "-p lta/ltb-filenames-with-full-path-to-be-stored-in-db "
                    "-o output-file -n obsno -f observation-logfile\n");
}


int main(int argc, char **argv)
{

//initial declarations ..
LtaInfo      linfo;
int          i,c;
FILE         *fp, *fp1=stdout;
char         datafiles[2048], logfile[1024],obsno[1024],obslogfile[1024],datafilesOriginalPaths[2048] ;
char         *p, *dp;

datafiles[0]=datafilesOriginalPaths[0]=logfile[0]=obsno[0]=obslogfile[0]='\0';

//option parsing ..
//Note option -l is used for passing list of lta file paths (absolute or relative
//to working directory.
//Option -p is used for passing list of file paths corresponding to the list
//passed using -l option. Values passed to -p are stored in database. 
while((c=getopt(argc,argv,"l:p:o:n:f:"))!=-1){
        switch(c){
            case 'l': strncpy(datafiles,optarg,2047); break;
            case 'p': strncpy(datafilesOriginalPaths,optarg,2047); break;
            case 'o': strncpy(logfile,optarg,1023);
            if((fp1 = fopen(logfile,"w")) == NULL){ perror(logfile); exit(1); }
            break;
            case 'f': strncpy(obslogfile,optarg,1023);
            break;
            case 'n': strncpy(obsno,optarg,4);break;
            default : printUsage();
            return 1;
        }
}

// check if flags / arguments applied to the program are proper
if(!strlen(datafiles)){ 
    printUsage();
    return 1;
}
if(!strlen(datafilesOriginalPaths)){ 
    printUsage();
    return 1;
}
if(!strlen(obslogfile)){ 
    printUsage();
    return 1;
}

int observationtableDone=0;
p=datafiles;
dp=datafilesOriginalPaths;

//loop through the passed data files. 
while(p!=NULL && strlen(p)){ 
        char *q=strchr(p,',');
        char *qdp=strchr(dp,',');
        if(q==NULL){
            q=p;p=NULL;
        }
        else{ 
            char *t=q; *q='\0';q=p;p=++t;
        }
        //fprintf(stderr, "working on %s", q);
        if(qdp==NULL){
            qdp=dp;dp=NULL;
        }
        else{ 
            char *tdp=qdp; *qdp='\0';qdp=dp;dp=++tdp;
        }
       
        if((fp=fopen(q,"r"))==NULL){ 
            fprintf(stderr,"xinfo: Cannot open %s\n",q);
            return 1;
        }
                { 
            int t=1;
            if (*((unsigned char *)&t) == 1) linfo.local_byte_order = LittleEndian;
            else linfo.local_byte_order=BigEndian;
        }

        //if header info cannot be read raise error and close the pl/sql function
        if(get_lta_hdr(fp,&linfo)){ 
            fprintf(stderr,"Error reading LtaHdr from %s\n",q);
            return 1;
        }

        //parse info on scans from the file .. in case of error raise error
        //and close the pl/sql function
        if(make_scantab(fp,&linfo)) return 1;

        //determine whether the file is GSB produced one or classic LTA or
        //LTB files.
        int dataFileType = -1;

        if (strstr(q,"_gsb.lta")!= NULL ) {
            dataFileType = GSB_LTA;
        }
        else if (strstr(q,".lta")!= NULL ) {
            dataFileType = CLASSIC_LTA;
        }
        else if (strstr(q,".ltb")!= NULL ) {
            dataFileType = CLASSIC_LTB;
        }
        else {
            fprintf(stderr,"Encountered unknow data file type (not classic lta, ltb or gsb-lta :: %s\n",q);
            return 1;
        }

        int n=0;
        int m=0;
        int scansSize=linfo.scans;
        scansCounter=scansSize;

        //loop through scans .. 
        for(i=0;i<linfo.scans;i++){

            ScanInfoType *scan=&linfo.stab[i].scan;
            ScanHdr      *shdr=&linfo.stab[i].shdr;
            int           recs=linfo.stab[i].recs;


            //if it is the first scan of the current data file
            //update das.observation, das.scangroup table also
            if(i==0 ){

                //several data files could belong to same observation
                //So in the current run das.observation has to be populated only
                //once for all datafiles passed.
                if(observationtableDone==0 ){
                    //note that this would only update das.observation table
                    //if an entry is not already present
                    if(xinfo_observation(&linfo.corr,scan,obsno,fp1,obslogfile)==0){
                        observationtableDone=1;
                    }
                    else{
                        fprintf(stderr,"xinfo: Error creating observation table info\n");
                        return 1;
                    }
                }

                //Populate scangroup table for lta/ltb file info and common params
                if(xinfo_global_hdr(&linfo.corr,scan,obsno,fp1,qdp,dataFileType,fp) ){
                    fprintf(stderr,"xinfo: Error creating global info for %s\n", q);
                    return 1;
                }
            }
            if(xinfo_scan_hdr(&linfo.corr,scan,recs,shdr,qdp,obsno,i,fp1,dataFileType,scansSize)){
                fprintf(stderr,"xinfo: Error creating scan info\n");
                return 1;
            }
        }

        free(linfo.stab);
        fclose(fp);
}
    fprintf(fp1, "}");
    return 0;
}
