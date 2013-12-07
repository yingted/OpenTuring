/**********/
/* mdio.h */
/**********/

#ifndef _MDIO_H_
#define _MDIO_H_

/*******************/
/* System includes */
/*******************/
#include <stdarg.h>

/******************/
/* Other includes */
/******************/
#include "miotypes.h"

/**********/
/* Macros */
/**********/

/*************/
/* Constants */
/*************/

/*********/
/* Types */
/*********/

/**********************/
/* External variables */
/**********************/

/***********************/
/* External procedures */
/***********************/
extern void	MDIO_Init (void);
extern void	MDIO_Beep (void);
extern BOOL	MDIO_DiskFile_Close (void *pmFilePtr);
#define MDIO_DiskFile_Flush fflush
#define MDIO_DiskFile_Getc fgetc
#define MDIO_DiskFile_Open fopen
#define MDIO_DiskFile_Putc fputc
#define MDIO_DiskFile_Puts fputs
#define MDIO_DiskFile_Read(buf,size,fd) fread(buf,1,size,fd)
#define MDIO_DiskFile_ReadLine fgets
#define MDIO_DiskFile_Seek(ofs,fd,whence) fseek(fd,ofs,whence)
#define MDIO_DiskFile_Tell ftell
#define MDIO_DiskFile_Ungetc ungetc
#define MDIO_DiskFile_Write(buf,size,fd) fwrite(buf,1,size,fd)
extern void	MDIO_GetSystemInfo (int *pmResourceCount, DWORD *pmMemUsed);
extern void	MDIO_MemoryLeakTest (void);
extern void	MDIO_OutputSystemInfo (const char *pmMessage);
extern void	MDIO_ProcessMSWindowsError (DWORD pmMSWindowsErrorCode);
extern void	MDIO_PutMIOWindowsOnTop (void);
#define MDIO_sprintf sprintf
#define MDIO_vsprintf vsprintf

#endif // #ifndef _MDIO_H_
