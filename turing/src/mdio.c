/**********/
/* mdio.c */
/**********/
#include <windows.h>

/****************/
/* Self include */
/****************/
#include "mdio.h"

/******************/
/* Other includes */
/******************/
#include "mioerr.h"

#include "mio.h"

#include "edint.h"

/*******************/
/* System includes */
/*******************/
// #include <crtdbg.h>
// #include "psapi.h"
#include <stdio.h>

/**********/
/* Macros */
/**********/

/*************/
/* Constants */
/*************/
// Defined in winuser.h for W2K and above
#define GR_GDIOBJECTS     0       /* Count of GDI objects */
#define GR_USEROBJECTS    1       /* Count of USER objects */

/********************/
/* Global variables */
/********************/

/*********/
/* Types */
/*********/

typedef BOOL (*GetProcessMemoryInfoProc) (HANDLE Process,
		PPROCESS_MEMORY_COUNTERS ppsmemCounters, DWORD cb);
typedef DWORD (*GetGuiResourcesProc) (HANDLE hProcess, DWORD uiFlags);


/**********************/
/* External variables */
/**********************/
extern void	Ed_CreateMemoryLogFile (void);

/********************/
/* Static constants */
/********************/

/********************/
/* Static variables */
/********************/
static GetProcessMemoryInfoProc	stGetProcessMemoryInfoProc = NULL;
static GetGuiResourcesProc	stGetGuiResourcesProc = NULL;

/******************************/
/* Static callback procedures */
/******************************/

/*********************/
/* Static procedures */
/*********************/

/***********************/
/* External procedures */
/***********************/
extern void TL_TLI_TLISS ();extern void TL_TLI_TLIPS ();


void	MDIO_ProcessMSWindowsError (DWORD pmMSWindowsErrorCode);
/************************************************************************/
/* MDIO_Init								*/
/************************************************************************/
void	MDIO_Init (void)
{
} // MDIO_Init


/************************************************************************/
/* MDIO_Beep								*/
/************************************************************************/
void	MDIO_Beep (void)
{
    //MessageBeep (MB_ICONEXCLAMATION);
} // MDIO_Beep


/************************************************************************/
/* MDIO_DiskFile_Close							*/
/************************************************************************/
BOOL	MDIO_DiskFile_Close (void *pmFilePtr)
{
    FILE 	*myPtr = (FILE *) pmFilePtr;
    return fclose(myPtr);
} // MDIO_DiskFile_Close


/************************************************************************/
/* MDIO_GetResourceCount						*/
/************************************************************************/
void	MDIO_GetSystemInfo (int *pmResourceCount, DWORD *pmMemUsed)
{
} // MDIO_GetSystemInfo


/************************************************************************/
/* MDIO_MemoryLeakTest							*/
/*									*/
/* Output any memory allocations that occurred between the last time	*/
/* this was called and this time.  Note, there is an allocation of 12	*/
/* bytes to start with the data "Memory Test" to ensure that we are	*/
/* getting something.							*/
/************************************************************************/
/*void	MDIO_MemoryLeakTest (void)
{
    #define MEM_STRING	"Memory Test"

    _CrtMemState	myMemCheckpoint, myDiffMem;
    char		*myPtr;

    static BOOL		myStMemCheckOccurred = FALSE;
    static _CrtMemState	myStMemCheckpoint;
    static int		myStLastResourceUsed, myStCurrentResourceUsed;
    static DWORD	myStLastMemUsed, myStCurrentMemUsed;

    if (myStMemCheckOccurred)
    {        	    
	_CrtMemDumpAllObjectsSince (&myStMemCheckpoint);
	_CrtMemCheckpoint (&myMemCheckpoint);    
	_CrtMemDifference (&myDiffMem, &myStMemCheckpoint, &myMemCheckpoint);
        _CrtMemDumpStatistics (&myDiffMem);
	myStMemCheckpoint = myMemCheckpoint;
    }
    else
    {
	Ed_CreateMemoryLogFile ();
	_CrtMemCheckpoint (&myStMemCheckpoint);    
	myStMemCheckOccurred = TRUE;
    }

    // We allocate a few bytes to make it clear that this routine works
    myPtr = malloc (strlen (MEM_STRING) + 1);
    strcpy (myPtr, MEM_STRING);

    _CrtMemDumpAllObjectsSince (&myStMemCheckpoint);

    // If we need to break at a memory allocation, do it here.
//    _crtBreakAlloc = 4849;

    // This call also allows us (in a debugger, to see the resource usage
    myStLastResourceUsed = myStCurrentResourceUsed;
    myStLastMemUsed = myStCurrentMemUsed;
    MDIO_GetSystemInfo (&myStCurrentResourceUsed, &myStCurrentMemUsed);
} // MDIO_MemoryLeakTest
*/

/************************************************************************/
/* MDIO_OutputSystemInfo						*/
/*									*/
/* Output the resource and memory usage of the program			*/
/************************************************************************/
void	MDIO_OutputSystemInfo (const char *pmMessage)
{
    int 	myResCount;
    DWORD	myMemUsed;
    char	myMsg [1024];
    	
    MDIO_GetSystemInfo (&myResCount, &myMemUsed);
    MDIO_sprintf (myMsg, "%s: R:%d M:%u\n", pmMessage, myResCount, myMemUsed);
    
    TL_TLI_TLISS (0, (short) 2);
    TL_TLI_TLIPS (0, myMsg, (short) 0);
} // MDIO_OutputSystemInfo


/************************************************************************/
/* MDIO_ProcessMSWindowsError						*/
/************************************************************************/
void	MDIO_ProcessMSWindowsError (DWORD pmMSWindowsErrorCode)
{
#if 0
    LPVOID myMessageBuffer;

    switch (pmMSWindowsErrorCode)
    {
    	case ERROR_FILE_NOT_FOUND:
	    SET_ERRNO(E_FSYS_FILE_NOT_FOUND);
    	    break;
    	case ERROR_PATH_NOT_FOUND:
	    SET_ERRNO(E_FSYS_PATH_NOT_FOUND);
    	    break;
    	case ERROR_TOO_MANY_OPEN_FILES:
	    SET_ERRNO(E_FSYS_TOO_MANY_OPEN_FILES);
    	    break;
    	case ERROR_ACCESS_DENIED:
	    SET_ERRNO(E_FSYS_ACCESS_DENIED);
    	    break;
    	case ERROR_INVALID_HANDLE:
	    SET_ERRNO(E_FSYS_HANDLE_INVALID);
    	    break;
    	case ERROR_NOT_ENOUGH_MEMORY:
    	case ERROR_OUTOFMEMORY:
	    SET_ERRNO(E_FSYS_INSUFFICIENT_MEMORY);
    	    break;
    	case ERROR_BAD_ENVIRONMENT:
	    SET_ERRNO(E_FSYS_ENVIRONMENT_INVALID);
    	    break;
    	case ERROR_BAD_FORMAT:
	    SET_ERRNO(E_FSYS_FORMAT_INVALID);
    	    break;
    	case ERROR_INVALID_ACCESS:
	    SET_ERRNO(E_FSYS_ACCESS_CODE_INVALID);
    	    break;
    	case ERROR_INVALID_DATA:
	    SET_ERRNO(E_FSYS_DATA_INVALID);
    	    break;
    	case ERROR_INVALID_DRIVE:
	    SET_ERRNO(E_FSYS_DISK_DRIVE_INVALID);
    	    break;
    	case ERROR_NOT_SAME_DEVICE:
	    SET_ERRNO(E_FSYS_NOT_SAME_DEVICE);
    	    break;
    	case ERROR_WRITE_PROTECT:
	    SET_ERRNO(E_FSYS_WRITE_PROTECTED_DISK);
    	    break;
    	case ERROR_NOT_READY:
	    SET_ERRNO(E_FSYS_DRIVE_NOT_READY);
    	    break;
    	case ERROR_CRC:
	    SET_ERRNO(E_FSYS_DATA_ERROR_CRC);
    	    break;
    	case ERROR_READ_FAULT:
	    SET_ERRNO(E_FSYS_READ_ERROR);
    	    break;
    	case ERROR_WRITE_FAULT:
	    SET_ERRNO(E_FSYS_WRITE_ERROR);
    	    break;
    	case ERROR_SECTOR_NOT_FOUND:
	    SET_ERRNO(E_FSYS_SECTOR_NOT_FOUND);
    	    break;
    	case ERROR_GEN_FAILURE:
	    SET_ERRNO(E_FSYS_GENERAL_FAULT);
    	    break;
    	case ERROR_SHARING_VIOLATION:
    	    SET_ERRNO(E_FSYS_SHARING_VIOLATION);
    	    break;
    	default:
	    {
		SET_ERRMSG (E_FSYS_UNKNOWN_ERROR, 
	        	    "Unknown file system error (Windows error = %d)", 
	    		    pmMSWindowsErrorCode);
	    }
    	    break;
    } // switch
#endif
} // MDIO_ProcessMSWindowsError
/*

7 The storage control blocks were destroyed.  		ERROR_ARENA_TRASHED 
9 The storage control block address is invalid.  	ERROR_INVALID_BLOCK 
16 The directory cannot be removed.  			ERROR_CURRENT_DIRECTORY 
18 There are no more files.  				ERROR_NO_MORE_FILES 
20 The system cannot find the device specified.  	ERROR_BAD_UNIT 
22 The device does not recognize the command.  		ERROR_BAD_COMMAND 
24 The program issued a command but the command length is incorrect.  ERROR_BAD_LENGTH 
25 The drive cannot locate a specific area or track on the disk.  ERROR_SEEK 
26 The specified disk or diskette cannot be accessed.  	ERROR_NOT_DOS_DISK 
28 The printer is out of paper.  			ERROR_OUT_OF_PAPER 
32 The process cannot access the file because it is being used by another process.  		ERROR_SHARING_VIOLATION 
33 The process cannot access the file because another process has locked a portion of the file. ERROR_LOCK_VIOLATION 
34 The wrong diskette is in the drive. Insert %2 (Volume Serial Number: %3) into drive %1.  	ERROR_WRONG_DISK 
36 Too many files opened for sharing.  			ERROR_SHARING_BUFFER_EXCEEDED 
38 Reached the end of the file.  			ERROR_HANDLE_EOF 
39 The disk is full.  					ERROR_HANDLE_DISK_FULL 

E_FSYS_READ_ONLY	E_FSYS_NO_MORE_FILES		E_FSYS_NO_DISK_IN_DRIVE		
E_FSYS_UNKNOWN_COMMAND	E_FSYS_SEEK_ERROR		E_FSYS_UNKNOWN_MEDIA_TYPE			
E_FSYS_PRINTER_OUT_OF_PAPER	
						
E_FSYS_NO_SPACE_LEFT		E_FSYS_FILE_EXISTS		E_FSYS_DIR_EXISTS		
E_FSYS_NOT_A_FILE		E_FSYS_CANT_READ_FROM_WDWFILE	E_FSYS_CANT_WRITE_TO_WDWFILE	
E_FSYS_PATHNAME_MODIFIED	E_FSYS_CANT_GO_UP_FROM_ROOT_DIR	E_FSYS_BAD_CHAR_IN_PATHNAME	
E_FSYS_MALFORMED_PATHNAME	E_FSYS_FUNCTION_NUMBER_INVALID	E_FSYS_MCB_DESTROYED		
E_FSYS_MCB_INVALID		E_FSYS_BAD_REQ_LEN_STRUCT	E_FSYS_UNKNOWN_UNIT		
*/


/************************************************************************/
/* MDIO_PutMIOWindowsOnTop						*/
/*									*/
/* This goes through the window list and places all the MIO windows on	*/
/* the top, preserving the Z order relative to themselves.		*/
/************************************************************************/
void	MDIO_PutMIOWindowsOnTop (void)
{
} // MDIO_PutMIOWindowsOnTop
