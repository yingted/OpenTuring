#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>


#define CTRLZ	26

/* Result codes */

extern int gFileManagerIgnoreFileOperation;

#define ResultCode_Ok		0
#define ResultCode_NoRead	1
#define ResultCode_NoWrite	2
#define ResultCode_NonAscii	3
#define ResultCode_NoSpace	4
#define ResultCode_IsOpen	5

/*
** The following string definitions give the default locations for the
** support files.  They are declared as full 256-byte strings to allow
** them to be changed using adb (or equivalent).
**
** These paths must be changed to conform to the actual installation
** of the support files.
*/

// extern void vaOkMsgBox(char *str, ...);

void NullFile (char **pmTextHandle, long *pmTextSize);

static int IsLegalNoPrint(c)
    int c;
{
    return (c == '\n' || c == '\r' || c == '\t');
}

void ReadOOTFile (char *pmFileName, char **pmTextHandle, long *pmTextSize, 
		  short *pmResult)
{
    struct stat statBuf;
    FILE* fd = NULL;
    char *src, *dst, *last;

    if (gFileManagerIgnoreFileOperation)
    {
    	NullFile (pmTextHandle, pmTextSize);
	*pmResult = ResultCode_Ok;
    	return;
    }
    *pmTextHandle = NULL;

    if (stat (pmFileName, &statBuf) != 0) 
    {
	*pmResult = ResultCode_NoRead;
	return;
    }

    if (!(fd=fopen(pmFileName, "rb")))
    {
	*pmResult = ResultCode_NoRead;
	return;
    }

    if ((*pmTextHandle = malloc (statBuf.st_size+1)) == NULL)
    {
	fclose(fd);
	*pmResult = ResultCode_NoSpace;
	return;
    }

    if (fread (*pmTextHandle, 1, statBuf.st_size, fd) != statBuf.st_size)
    {
	fclose(fd);
	free (*pmTextHandle);
	*pmTextHandle = NULL;
	*pmResult = ResultCode_NoRead;
	return;
    }
    
    fclose(fd);

    /* Find first carriage return or line feed */

    *pmResult = ResultCode_Ok;

    last = *pmTextHandle + statBuf.st_size;
    dst = src = *pmTextHandle;

    while (src < last) {
	if ( isprint(*src) || IsLegalNoPrint(*src) ) {
	    *dst = *src;
	    dst++;
	}
	else if ( *src != CTRLZ)
	    *pmResult = ResultCode_NonAscii;
	src++;
    }
    *dst++ = '\0';

    *pmTextSize = dst - *pmTextHandle;
    if ((src = realloc (*pmTextHandle, *pmTextSize) ) != NULL )
    {
        *pmTextHandle = src;
    }
}


void NullFile (char **pmTextHandle, long *pmTextSize)
{
    if (gFileManagerIgnoreFileOperation)
    {
    	*pmTextSize = 0;
    	// We hope to cause a Bus error if this handle is ever actually used.
    	*pmTextHandle = (char *) 1; 
    }
    else
    {
    	*pmTextSize = 1;
    	if ((*pmTextHandle = malloc(1)) != NULL)
    	{
	    **pmTextHandle = '\0';
	}
    }
}


// Rewritten by Tom West (Oct 2000)
void SaveFile (char *pmfileName, char *pmTextHandle, long pmTextSize, 
	       short *pmResult)
{
    FILE* fd = NULL;

    if (gFileManagerIgnoreFileOperation)
    {
    	*pmResult = 0;
    	return;
    }

    fd = fopen(pmfileName, "a");
    
    if (!fd)
    {
	*pmResult = ResultCode_NoWrite;
	return;
    }

    if (fwrite(pmTextHandle, sizeof(char), pmTextSize-1, fd) != pmTextSize-1)
    {
	fclose(fd);
	*pmResult = ResultCode_NoWrite;
	return;
    }

    if (fclose(fd))
    {
	*pmResult = ResultCode_NoWrite;
	return;
    }

    *pmResult = ResultCode_Ok;
}

/*
int
CreateFile(fileName)
    char *fileName;
{
    int fd;

    if((fd=open(fileName, O_RDWR|O_CREAT|O_EXCL|O_BINARY,
	        S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) < 0)
	return(ResultCode_NoWrite);

    (void) close(fd);
    return(0);
}
*/

void
RemoveFile(char* fileName)
{
    (void) unlink(fileName);
}


void FreeFile (char **textHandle)
{
    if (!gFileManagerIgnoreFileOperation)
    {	
    	if(*textHandle)
    	{
	    free(*textHandle);
	}
    	*textHandle = 0;
    }
}



void
HomeDirectory(char* user, char* home)
{
    home[0] = '\0';
}


// Rewritten by Tom West (Oct 2000)
void CurrentDirectory (char pmDirectory [256])
{
    getcwd(pmDirectory, 256);
    int len = strlen(pmDirectory);
    if (pmDirectory[len-1] != '/') {
        pmDirectory[len] = '/';
        pmDirectory[len+1] = '\0';
    }
}


void
TempDirectory(char path[256])
{
    char *pnt;
    int len;

    pnt = getenv("TMPDIR");

    if (pnt == NULL)
	pnt = getenv("TMP");

    if (pnt == NULL)
	pnt = getenv("TEMP");

    if (pnt == NULL || strlen(pnt) > 255)
	strcpy(path, "/tmp");
    else
	(void) strcpy(path, pnt);

    len = strlen(path);
    if (path[len-1] != '/') {
	path[len] = '/';
	path[len+1] = '\0';
    }
}


void IncludeDirectory (char pmDirectory [256])
{
    pmDirectory[0] = '\0';
}

void
UserName(user)
    char *user;
{
    user[0] = '\0';
}


char
Exists(path)
    char *path;
{
    struct stat statBuf;

    if(!path || *path == '\0')
	return 0;
    else
	return (stat(path,&statBuf) == 0);
}


// Rewritten by Tom West (Oct 2000)
char CanRead (char *pmPathName)
{

    if ((pmPathName == NULL) || (!pmPathName [0]))
    {
    	return 0;
    }
    return access(pmPathName, R_OK) != -1;
}


// Rewritten by Tom West (Oct 2000)
char CanReadWrite (char *pmPathName)
{
    if ((pmPathName == NULL) || (pmPathName [0] == 0))
    {
    	return 0;
    }	
    return access(pmPathName, R_OK|W_OK) != -1;
}


// Rewritten by Tom West (Oct 2000)
char CanCD (char *pmPathName)
{
    struct stat sb;
    if (stat(pmPathName, &sb) == -1) return 0;
    return S_ISDIR(sb.st_mode);
}


// Rewritten by Tom West (Oct 2000)
char IsRegularFile (char *pmPathName)
{
    struct stat sb;
    if (stat(pmPathName, &sb) == -1) return 0;
    return S_ISREG(sb.st_mode);
}


// Rewritten by Tom West (Oct 2000)
char IsSamePath (char *pmPathName0, char *pmPathName1)
{
    return (char) (strcmp(pmPathName0, pmPathName1) == 0);
}


static void ConvertPath(char* path)
{
}


/*   FullPath - Converts path to minimal absolute path
 *
 */

void FullPath(char path[256], char fpath[256])
{
    realpath(path, fpath);
}


struct FID {
    char n[256];
    int32_t t;
};

void
GetFID(char* file, struct FID* fid_p )
{
    struct stat statBuf;

    if( file == NULL || (*file) == '\0' || stat( file, &statBuf ) < 0 ) {
	fid_p->n[0] = '\0';
	fid_p->t = 0;
    }
    else {
        realpath(file, fid_p->n);
    	fid_p->t = statBuf.st_mtime;
    }
}

