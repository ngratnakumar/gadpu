#include<stdio.h>
#include<stdlib.h>
#include<math.h>

/* 
   Function to convert mean co-ordinates on a given date to J2000 co-ordinates.
   Written to allow conversion of the DATE co-ordinates in existing GMRT
   Archival database entries to J2000 co-ordinates.

   JNC 03/Sep/2003
*/

int main(int argc, char **argv)
{ char   date_obs[81],mname[81];
  double ra,dec,epoch1,epoch,ra0,dec0,mjd;
  int    i,c,day,month,year,ierr;
  char  *month_names[12]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug",
			 "Sep","Oct","Nov","Dec"};
  extern char *optarg;

  void sla_preces_(char *system,double *epoch, double *epoch1,
		   double *ra, double *dec);
  void sla_cldj_(int *year, int *month,int *day,double *mjd, int *err);

  ra=-1.0;dec=-(M_PI/2+0.1);i=ierr=0;
  while((c=getopt(argc,argv,"D:r:d:"))!=-1)
  { switch(c)
    { case 'D': i++;strncpy(date_obs,optarg,80); break;
      case 'd': dec0=dec=atof(optarg);break;
      case 'r': ra0=ra=atof(optarg);break;
      default : fprintf(stderr,"USAGE:app2j2000 -D ObsDate -r ra -d dec \n");
	        return 1;
    }
  }
  if(ra<0.0 || ra>2*M_PI)
  { ierr=1; fprintf(stderr,"ra [%7.3f] out of range [0-2PI]\n",ra);}
  if(fabs(dec)>M_PI/2.0)
  { ierr=2; fprintf(stderr,"dec [%7.3f] out of range [-PI/2-PI/2]\n",dec);}
  if(i<1 || ierr)
  { fprintf(stderr,"USAGE:app2j2000 -D ObsDate -r ra -d dec \n");
  return 1;
  }
    
  /* compute MJD */
  if(sscanf(date_obs,"%*s %s %d %*s %d\n",&mname,&day,&year)!=3)
  { fprintf(stderr,"cannot parse %s\n"); return 1;}
  for(i=0;i<12;i++)if(!strcasecmp(mname,month_names[i]))break;
  if(i<12)month=i+1;
  else{fprintf(stderr,"Illegal Month Name %s\n"); return 1;}
  sla_cldj_(&year,&month,&day,&mjd,&ierr);
  if(ierr){ fprintf(stderr,"Error computing MJD"); return 1;}

  /* compute J2000 Co-ordinates */
  epoch=2000.0 + (mjd - 51544.5)/365.25 ;
  epoch1=2000.0;
  sla_preces_("FK5",&epoch,&epoch1,&ra,&dec);  /* J2000 co-ordinates */

  printf("RA_DATE= %lf DEC_DATE= %lf RA_2000= %lf DEC_2000= %lf\n",
	 ra0,dec0,ra,dec);

  return 0;
}
