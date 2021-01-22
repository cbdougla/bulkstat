/* 
   bulkstat: stat of a list of files

   This is a program to take input of a list of files and generate a pipe
   delimited file containing the filename, size, access time, modification time 
   and status change time.

   It seems to work equally well for local and network mounted filesystems.    
   I've tested it with CIFS, and NFS and used it extensively with NCPFS.  

*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <openssl/md5.h>
#include <getopt.h>
#include <time.h>
#include <sys/stat.h>
#include <libgen.h>

#define MAXCHUNK 512
#define MAXCHUNKMD5 8192
#define MAXPATH 1024

int do_md5sum(char *checksum, char *filename);

int main(int argc, char *argv[])
{ extern char *optarg;
  extern int optind, opterr;

  int x=0, chunksize=0, pos=0, opt_d=0, opt_D=0, opt_u=0, opt_g=0, opt_n=0, filecount=0, err, totalfiles=0;
  int opt_s=0, opt_H=0, opt_p=0, opt_P=0, opt_m=0, maxchunk=MAXCHUNK;

  FILE *indata=NULL, *outdata=NULL, *errout, *pipeFILE;
  struct stat *fileinfo, tmpstat;
  struct tm *at, *ct, *mt, *today;
  time_t temptime;
  
  char pathname[MAXPATH], *dname, *fname, *buf, pathstripped[MAXPATH], temp2[MAXPATH];
  char c, *infilename, *outfilename, command[MAXPATH], stamp[11], d, *checksum;

  long long lines=0, linestmp=0;

  infilename=NULL;
  outfilename=NULL;
  d='|';
 
  errout=fdopen(STDERR_FILENO,"w");
  opterr=1;

  fileinfo=malloc(sizeof(struct stat));
  checksum=calloc(33,sizeof(char));
  at=malloc(sizeof(struct tm));
  ct=malloc(sizeof(struct tm));
  mt=malloc(sizeof(struct tm));


  while ((c=getopt(argc,argv,"b:d:ghHpPuvmDsi:o:n:")) != EOF)
    switch (c)
    { case 'b':
	maxchunk=atoi(optarg);
        if (opt_D > 0)
	  fprintf(errout,"Setting MAXCHUNK to %d\n",maxchunk);
	break;
      case 'd': 
        opt_d=1; 
        d=optarg[0];
	break;
      case 'D': 
	  opt_D++;
	break;
      case 'H': 
        opt_H=1; 
	break;
      case 'g': 
        opt_g=1; 
	break;
      case 'i':
	infilename=calloc(50, sizeof(char));
	strcpy(infilename,optarg);
        break;
      case 'm': 
        opt_m=1; 
	break;
      case 'n': 
	if ( optarg == NULL)
	{ printf("Invalid option.  Must specify number of lins with -n\n");
	  printf("\n");
	  exit(EINVAL);
	}
          opt_n=atoi(optarg); 
	break;
      case 'o':
	outfilename=calloc(50, sizeof(char));
	strcpy(outfilename,optarg);
        break;
      case 's': 
        opt_s=1; 
	break;
      case 'u': 
        opt_u=1; 
	break;
      case 'v': 
	printf("\n");
        printf("bulkstat version 1.5\n");
	printf("by Collin Douglas\n");
	printf("\n");
	exit(0);
	break;
      case 'h':
	printf("\n");
        printf("Usage:  bulkstat [-i <infile>] [-o <outfile>] [-d <delimiter>] [-n <numlines>] [-bDhv]\n");
	printf("\tb: Set max buffer chunk size (default %d bytes)\n",maxchunk);
	printf("\td: Set output delimiter to argument instead of '|'\n");
	printf("\tg: Output group ID of file owner\n");
	printf("\tD: Print debugging info. (Set to a number for more info)\n");
	printf("\tH: Print header at top of list\n");
	printf("\tm: Calculate MD5 checksum of file.\n");
	printf("\t   NOTE:  This will slow things down significantly\n");
	printf("\tn: Rolling count of number of lines processes\n");
	printf("\tp: Output separate pathname into dirname and filename fields\n");
	printf("\tP: Output all three dirname, filename, and pathname fields\n");
        printf("\ts: Include current date field in output file\n");
	printf("\tu: Output user ID of file owner\n");
	printf("\tv: Print version and exit\n\n");
	printf("\th: This help screen\n\n");
	printf("Program will use STDIN and/or STDOUT if no file options are given\n\n");
	printf("A typical example is:  find . -type f | bulkstat > bulkstat.csv\n\n");
	exit(0);
	break;
      case 'p': 
        opt_p=1; 
	break;
      case 'P': 
        opt_P=1; 
	break;
      default:
        printf("%s\n",strerror(errno));
        exit(errno);
        break;
    }
 
  if (opt_D >= 1)
    fprintf(errout,"MAXCHUNK: %d\n",maxchunk);

  if (infilename == NULL)
  { indata=fdopen(STDIN_FILENO,"r");
    if (opt_D>=1)
      fprintf(errout,"Using stdin for input\n");
  }
  else
  { indata=fopen(infilename,"r");
    if (opt_D>=1)
      fprintf(errout,"Using %s for input\n",infilename);
  }

  if (outfilename == NULL)
  { outdata=fdopen(STDOUT_FILENO,"w");
    if (opt_D>=1)
      fprintf(errout,"Using stdout for output\n");
  }
  else
  { outdata=fopen(outfilename,"w");
    if (opt_D>=1)
      fprintf(errout,"Using %s for outfile\n",outfilename);
  }

  if (indata == NULL)
  { printf("error opening input file: %s\n",strerror(errno));
    exit(errno);
  }

  if (outdata == NULL)
  { printf("error opening output file: %s\n",strerror(errno));
    exit(errno);
  }

  buf=calloc(maxchunk, sizeof(char));
  pos=0;

  temptime=time(NULL);

  if (opt_D > 1)
    fprintf(errout,"temptime: %ld\n",temptime);
  
  today=localtime(&temptime);

  sprintf(stamp,"%04d-%02d-%02d",(today->tm_year)+1900, (today->tm_mon)+1, (today->tm_mday));


  if(opt_D >= 1)
    fprintf(errout,"stamp: %s\n",stamp);

  if (opt_H == 1)
  { if (opt_p == 0)
      fprintf(outdata,"Path%cBytes%cAccess Date%cMod Date%cCreate Date",d,d,d,d);
    else
      fprintf(outdata,"Directory%cFile%cBytes%cAccess Date%cMod Date%cCreate Date",d,d,d,d,d);
    if (opt_m == 1)
      fprintf(outdata,"%cChecksum",d);
    if (opt_s == 1)
      fprintf(outdata,"%cReport Date",d);
    if (opt_u == 1)
      fprintf(outdata,"%cUID",d);
    if (opt_g == 1)
      fprintf(outdata,"%cGID",d);
    fprintf(outdata,"\n");
  }

/****************************************************
 *
 * Here's where all the work is done
 *
 * Here we read the file (or stdin) into memory a chunk at a time
 * x keeps track of where in the chunk buffer we are.
 * pathname[pos] is built character by character from data in the 
 * buffer until it reaches
 * a \n char at which point it changes the \n in the pathname
 * array to a \0 and copies the string into a temp variable.
 * That temp variable that now contains the string of the file
 * is passed to the stat system call.
 *
 ****************************************************/

  while ( (chunksize=fread((buf),sizeof(char),maxchunk,indata)) > 0)
  { if (opt_D >= 1)					/* debug option */
      fprintf(errout,"Read chunk: %d bytes -- ",chunksize);

    for(filecount=0, x=0; x < chunksize; x++, pos++)				
    { pathname[pos]=buf[x];
      if( (buf[x] == '\n') && (pos > 0))
      { lines++;
        linestmp++;	/* insignificant variables used just for printing line numbers */
        if (opt_n)
	{ if (linestmp == opt_n)			/* print line number option */
	  { printf("\t Processed %lld lines\n", lines);
	    linestmp=0;
	  }
	}
        pathname[pos] = '\0';

	sprintf(pathstripped,"%s",pathname);			/* strip the extra \n so it prints correctly */

        if ( opt_D > 0)
          fprintf(errout,"pathname: %s\n",pathname);

 
	err=lstat(pathname, fileinfo);

        if (opt_D > 2)
        { fprintf(errout,"st_mtime: %ld\n",fileinfo->st_mtime);
          fprintf(errout,"st_atime: %ld\n",fileinfo->st_atime);
          fprintf(errout,"st_ctime: %ld\n",fileinfo->st_ctime);
          fprintf(errout,"st_size: %ld\n",fileinfo->st_size);
        }

	if ( (err == 0) || (errno == 75) )
	{ 
          localtime_r(&(fileinfo->st_mtime),mt);	/* Last modification */
	  localtime_r(&(fileinfo->st_atime),at);	/* Last access */
	  localtime_r(&(fileinfo->st_ctime),ct);	/* Creation time */
 	  strcpy(temp2, pathstripped);
          fname=basename(temp2);			/* filename */
          strcpy(temp2, pathstripped);
          dname=dirname(temp2);				/* directory name */

	//debug option
        if (opt_D > 1)
        { fprintf(errout,"mt->tm_year: %04d, mt->tm_mon: %02d, mt->tm_mday: %02d\n", 
	    (mt->tm_year)+1900, (mt->tm_mon)+1, (mt->tm_mday));
          fprintf(errout,"at->tm_year: %04d, at->tm_mon: %02d, at->tm_mday: %02d\n", 
	    (at->tm_year)+1900, (at->tm_mon)+1, (at->tm_mday));
          fprintf(errout,"ct->tm_year: %04d, ct->tm_mon: %02d, ct->tm_mday: %02d\n", 
	    (ct->tm_year)+1900, (ct->tm_mon)+1, (ct->tm_mday));
	 }

          if ( (opt_p == 1) || (opt_P == 1) )
            fprintf(outdata,"%s%c%s%c",dname, d, fname, d);
          if ( (opt_p == 0) || (opt_P == 1 ) )
            fprintf(outdata,"%s%c",pathstripped,d);

          fprintf(outdata,"%ld%c%04d-%02d-%02d%c%04d-%02d-%02d%c%04d-%02d-%02d",  
  	                  fileinfo->st_size, d,
	                  (at->tm_year)+1900, (at->tm_mon)+1, (at->tm_mday), d,
	                  (mt->tm_year)+1900, (mt->tm_mon)+1, (mt->tm_mday), d,
	                  (ct->tm_year)+1900, (ct->tm_mon)+1, (ct->tm_mday), d);

	  //checksum option
	  if (opt_m == 1)
	  { strcpy(checksum,"error");

	    do_md5sum(checksum, pathstripped);
	    fprintf(outdata,"%c%s",d,checksum);

	  }

	  //current date option
          if (opt_s == 1)
            fprintf(outdata,"%c%s",d,stamp);

	  //userID option
	  if (opt_u == 1)
	    fprintf(outdata,"%c%d",d,(fileinfo->st_uid));

	  //groupID option
	  if (opt_g == 1)
	    fprintf(outdata,"%c%d",d,(fileinfo->st_gid));

	  fprintf(outdata,"\n");
	}

	else
	  fprintf(errout,"%s: %s\n",pathname,strerror(errno));
        pos=-1;
	filecount++;
	totalfiles++;
      }
    }
    buf[0]=buf[chunksize];
    if (opt_D >= 1)
      fprintf(errout,"%d pathnames processed\n",filecount);
  }

  if (opt_D >= 1)
    fprintf(errout,"%d files in input stream\n",totalfiles);

  if (infilename != NULL)
    free(infilename);
  if (outfilename != NULL)
    free(outfilename);

  free(buf);
  free(checksum);
  free(fileinfo);
  fclose(indata);
  fclose(outdata);
  fclose(errout);
}

// modified from a stackoverflow post
// which was taken from https://www.unix.com/programming/134079-computing-md5sum-c.html
//
int do_md5sum(char *checksum, char *filename)
{
    int n, x;
    MD5_CTX c;
    char buf[MAXCHUNKMD5], tempname[MAXPATH];
    char blah[3], *r;
    size_t bytes;
    unsigned char out[MD5_DIGEST_LENGTH];
    FILE *tempfile;


    tempfile=fopen(filename,"r");

    if ( tempfile == NULL )
    { sprintf(checksum,"%s\n","error");
      return(-1);
    }
    

    MD5_Init(&c);
    bytes=fread(buf, sizeof(char), MAXCHUNKMD5, tempfile);
    while(bytes > 0)
    {
        MD5_Update(&c, buf, bytes);
        bytes=fread(buf, sizeof(char), MAXCHUNKMD5, tempfile);
    }

    MD5_Final(out, &c);

    r=checksum;
    for(n=0; n<MD5_DIGEST_LENGTH; n++)
    { 
      sprintf(blah,"%02x",out[n]);
      strncpy(r,blah,2);
      r++;
      r++;
    }
    r='\0';
    fclose(tempfile);
    return(0);        
}
