/************************************************************************/
/* mio.c 								*/
/*									*/
/* General notes: Turing's various forms of output are somewhat		*/
/* complicated by the fact that windows, files, network connections and	*/
/* even the printer are all sort of interchangeable.  For example, you	*/
/* can "put" to all four types.  However, of course, they're not	*/
/* completely interchangeable.  For example, you can't send graphics	*/
/* output to a network connection.					*/
/*									*/
/* These items are described by MIOFILE data structures.		*/
/*									*/
/* Added into this mix is redirection, so that stdin and stdout may not	*/
/* be windows after all.						*/
/*									*/
/*	REDIRECTION							*/
/* So, we have stMIOStd{in,out,err}.  These are simply place holders	*/
/* that the Turing executor uses when it want to send output to these	*/
/* three streams							*/
/* stMIOStdinRedirect actually represents were input is to be read	*/
/*   from.  If reading from a file, filePtr represent the file to be	*/
/*   read from.  Otherwise, windowPtr points to the default window	*/
/* stMIOStdoutRedirect represents where standard output should be 	*/
/*   redirected to.  It is possible for both filePtr and windowPtr to	*/
/*   hold values if output is being redirected to file and window.	*/
/* stMIOStdinEcho represents where input should be echoed to.  It can	*/
/*   possible for both filePtr and windowPtr to hold values if input	*/
/*   is to be echoed to both a file and a window.  Note that it is	*/
/*   slightly complicated by the fact that input from the keyboard is	*/
/*   echoed keystroke by keystroke to the window and only when Enter 	*/
/*   is pressed is a line echoed to a file.				*/
/*									*/
/*	SELECTED WINDOW							*/
/* Various graphics commands in Turing don't specify a window.  This    */
/* means that they are sent to the currently selected  window		*/
/* (MIO_selectedRunWindow).						*/
/*									*/
/*	INPUT								*/
/* Turing never reads input without checking to make certain that the	*/
/* input is already available.  In that way, it never truly blocks.	*/
/* Instead, it can keep calling the event loop until the appropriate	*/
/* input is available, and then it does the read immediately following	*/
/* (no possibility of something else doing the read between the two.	*/
/* It checks for the possibility of input by calling MIO_HasEvent.	*/
/* For keyboard input, it calls MIO_HasEvent.				*/
/*									*/
/* For reading a line, token or a fixed number of characters from a	*/
/* window, MIO_HasEvent calls MIOWin_IsInputAvailable.			*/
/* For reading a single char in real time (getch), MIO_HasEvent also	*/
/* calls MIOWin_IsInputAvailable.					*/
/* For reading a mouse button press, MIO_HasEvent calls			*/
/* MIOMouse_ButtonMoved1.						*/
/*									*/
/*	KEYBOARD INPUT							*/
/* Keyboard input is handles with two circular queues, the keyboard	*/
/* queue and the line queue.  As characters are entered into the	*/
/* window, they are placed in the keyboard queue.  When checking for	*/
/* input, the characters in the keyboard queue are moved to the line	*/
/* queue and echoed as well.						*/
/* When reading for a line, token or fixed number of characters, the	*/
/* input is only considered once a newline has been entered (at that	*/
/* point, the user can't edit any more.					*/
/* When reading for a single character, a check is made to see if	*/
/* there's a character in the keyboard queue.				*/
/*									*/
/*	CARET								*/
/* Whenever an inner window gains/loses focus, the caret is created/	*/
/* destroyed, and MIO_caretOwner is set.				*/
/************************************************************************/

/*******************/
/* System includes */
/*******************/
#include <malloc.h>
#include <string.h>

/****************/
/* Self include */
/****************/
#include "mio.h"

/******************/
/* Other includes */
/******************/
#include "language.h"

#include "mioerr.h"

#include "mdio.h"

#include "miowin.h"
#include "mdiowin.h"

#include "miodir.h"
#include "mioerror.h"
#include "miofile.h"
#include "miofont.h"
#include "miolexer.h"
#include "miomouse.h"
#include "miomusic.h"
#include "mionet.h"
#include "miopic.h"
#include "miosprite.h"
#include "miosys.h"
#include "miotime.h"
#include "miowindow.h"
#include "miohashmap.h"

#include "mioglgraph.h"

#include "edint.h"
#include <stdio.h>

// Test to make certain we're not accidentally including <windows.h> which
// might allow for windows contamination of platform independent code.
// TW - Move below the net.h command
#ifdef _INC_WINDOWS
xxx
#endif

/**********/
/* Macros */
/**********/
#define INIT_LAST_ERROR		if (stCurrentlyExecutingAProgram) \
				    Language_Execute_SetErrno (0, NULL);

/*************/
/* Constants */
/*************/
//#define EOF				-1

#define INITIAL_PRINT_BUFFER_SIZE	2000

// The different types of files (things that one can 'put' to)
#define FILE_KIND_NONE			0
#define FILE_KIND_FILE			11
#define FILE_KIND_PRINTER		22
#define FILE_KIND_NET			33
#define WINDOW_KIND_NONE		0
#define WINDOW_KIND_TEXT		77
#define WINDOW_KIND_GRAPHICS		88

// Predefined stream numbers in Turing
#define TURINGSTDIN			(-2)
#define TURINGSTDOUT			(-1)
#define TURINGSTDERR			(0)

// Parameters for MyCloseRunWindows
#define HIDDEN_ONLY			TRUE
#define ALL_WINDOWS			FALSE

// Number of special file types (window, net, printer)
#define NUM_SPECIAL_FILE_TYPES		3
#define SPECIAL_FILE_WINDOW		0
#define SPECIAL_FILE_NET		1
#define SPECIAL_FILE_PRINTER		2

// TW To be deleted
#define DEFAULT_WIN			NULL

// Maximum line style for Draw.DashedLine (MIO_CheckLineStyle) 
#define MAX_LINE_STYLE			4

// Information for stream manager calls
#define MAX_STREAMS		 	 100

#define FILE_STREAM_ID_BASE		   1
#define WINDOW_STREAM_ID_BASE		1001
#define NET_STREAM_ID_BASE		2001

#define ID_OPEN			 	 500

#define STDOUT_WINDOW			 600
#define STDERR_WINDOW			 601

// Window attributes
#define TEXT_STDOUT_ATTRIB		"text,buttonbar,popup"
#define GRAPHICS_STDOUT_ATTRIB		"graphics,buttonbar,popup"
#define TOP_RIGHT_ATTRIB		",position:right,top"
#define MIDDLE_ATTRIB			",position:center,middle"

// Information for stream manager calls
#define MAX_IDS		 	 	1000

#define FONT_ID_BASE		   	5001
#define DIR_ID_BASE			6001
#define PIC_ID_BASE			7001
#define SPRITE_ID_BASE			8001
#define LEX_ID_BASE			9001
#define HASHMAP_ID_BASE			10001

/********************/
/* Global variables */
/********************/
INSTANCE		MIO_applicationInstance;
WIND			MIO_selectedRunWindow;
MIOWinInfoPtr		MIO_selectedRunWindowInfo;
int			MIO_parallelIOPort = 0;
BOOL			MIO_paused, MIO_finished;
WIND			MIO_caretOwner;
char			*MIO_programName = NULL;

/*********/
/* Types */
/*********/
typedef struct MIOPRINTER
{
    char	*buffer;	// Buffer holding the data to be printed
    DWORD	currentSize;	// The current size of the data in the buffer
    DWORD	maxSize;	// The maximum size of the buffer
    char	*title;		// The title appearing on the printout
    BOOL	noHeader;	// TRUE if no header to appear on printout
    char	*fontName;	// The font to be used
    int		fontSize;	// The font size to be used
    BOOL	landscape;	// TRUE if output to appear in landscape
} MIOPRINTER;

typedef struct WindowList
{
    WIND		window;
    struct WindowList	*next;
} WindowList, *WindowListPtr;

typedef struct Stream
{
    int		id;
    void	*info;
} Stream;

// Used to store ID numbers for Fonts, Directories, Pictures, Sprites
// and Lexical Things.  
typedef struct ID
{
    int		id;		// The id number slot # + x_ID_BASE
    int		type;		// What this ID is used for
    int		count;		// # times this ID used.
    void	*info;		// Information about the item
    void	*compareInfo;	// Information about the item used to determine
				// if a new item should be allocated or 
				// another one reused.
} ID;

/**********************/
/* External variables */
/**********************/
extern void *(*TL_TLI_TLIFOP) ();
extern void (*TL_TLI_TLIFFL) (), (*TL_TLI_TLIFCL) (), (*TL_TLI_TLIFUG) ();
extern void (*TL_TLI_TLIFPC) (), (*TL_TLI_TLIFPS) (), (*TL_TLI_TLIFSK) ();
extern void (*TL_TLI_TLIFZ) ();
extern int  (*TL_TLI_TLIFGC) (), (*TL_TLI_TLIFRE) (), (*TL_TLI_TLIFWR) ();
extern long (*TL_TLI_TLIFTL) ();
extern void TL_TLI_TLISF ();
extern int TL_TLI_TLIEFR ();
extern void Language_Execute_System_Setactive ();

/********************/
/* Static constants */
/********************/
static char *stSpecialFileTypes [NUM_SPECIAL_FILE_TYPES] =
   {"%window(", "%net(", "%printer(" };
static char	stTextStdoutWindow [100];
static char	stGraphicsStdoutWindow [100];
static char	*stStdErrorWindowAttribs = "text:3,20,popup,position:right,bottom,title:Standard Error";
static char	*stSysExitString;

/********************/
/* Static variables */
/********************/
// Is input from a file being echoed
static BOOL		stEchoInputFromFile;
// If echoing input, has the echo reached a newline yet.  This is used 
static BOOL		stInputEchoReachedNewline = TRUE;	
// Are we currently executing an OOT program
static BOOL		stCurrentlyExecutingAProgram = FALSE;
// The array holding the stream numbers
static Stream		stStreams [MAX_STREAMS];
static int		stStreamCounter;
// The array holding the ID numbers and object information
// Routines that modify this array: MIO_IDAdd, MIO_IDFree, MIO_IDRemove
static ID		stIDs [MAX_IDS];
static int		stIDCounter;
// Indicate whether to output debugging information
static BOOL		stMoreInfo = FALSE;
static BOOL		stErrorInfo = FALSE;
// String holding information about the last file opened
static char		stLastFileDescription [1024];
static BOOL		stLastFileOpenedWasFile;
static BOOL		stLastFileClosedWasFile;
// Used for Test Suite execution
static BOOL		stIsTestSuiteProgram;
static const char	*stTestSuiteInputDirectory;
static const char	*stTestSuiteOutputDirectory;


/******************************/
/* Static callback procedures */
/******************************/

/*********************/
/* Static procedures */
/*********************/
static MIOFILE 		*MyCreateMIOFile (void);
static MIOPRINTER 	*MyCreateMIOPrinter (const char *pmAttribs);
static void		MyFreeMIOPrinter (MIOPRINTER *pmMIOPrinter);
static void		MyGetIDSlot (int pmIDNumber, int *pmIDType, 
				     int *pmIDSlot);
static int		MyGetSpecialFileType (OOTstring pmFileName);
static MIOFILE		*MyMakeMIOFILEFromFile (void *pmFile, int pmFileType);
static MIOFILE		*MyMakeMIOFILEFromWindow (WIND pmWindow);
static void		MyMoveToFrontOfRunWindowList (WIND pmWindow);
static void		MySetRunWindowTitles (void);
static void		MyWipeMIOFile (MIOFILE *pmMIOFile);


/***********************/
/* External procedures */
/***********************/
/************************************************************************/
/* MIO_Initialize							*/
/*									*/
/* Called from edrun when Turing is initialized.			*/
/*									*/
/* The pmSysExitString parameter is used to specify a string to be	*/
/* passed to MIOError_Abort that will cause Turing to abort, but will	*/
/* intercepted by the environment and not produce an error message.	*/
/************************************************************************/
void	MIO_Initialize (INSTANCE pmApplicationInstance, int pmOS,
			const char *pmOOTDir, const char *pmHomeDir,
		        BOOL pmCenterOutputWindow, BOOL pmStopUserClose,
			const char *pmSysExitString)
{
    int	cnt;
    
    MIO_applicationInstance = pmApplicationInstance;
    MIO_caretOwner = NULL;
    
    MDIO_Init (); 			// Initialize machine dependencies
    MIOWin_Init (pmCenterOutputWindow, pmStopUserClose);	
    					// Initialize window stuff
    
    TL_TLI_TLIFCL = fclose;
    TL_TLI_TLIFOP = fopen;
    TL_TLI_TLIFPC = fputc;
    TL_TLI_TLIFPS = fputs;
    TL_TLI_TLIFGC = fgetc;
    TL_TLI_TLIFUG = ungetc;
    TL_TLI_TLIFSK = fseek;
    TL_TLI_TLIFTL = ftell;
    TL_TLI_TLIFFL = fflush;
    TL_TLI_TLIFRE = fread;
    TL_TLI_TLIFWR = fwrite;
    TL_TLI_TLIFZ  = rewind;
    
    // We are not yet executing a program
    stCurrentlyExecutingAProgram = FALSE;

    // Initialize individual MIO modules
    MIOError_Init ();
    MIOFile_Init (pmOS, pmOOTDir, pmHomeDir);
    MIOLexer_Init ();
    MIOTime_Init ();

    strcpy (stTextStdoutWindow, TEXT_STDOUT_ATTRIB);
    strcpy (stGraphicsStdoutWindow, GRAPHICS_STDOUT_ATTRIB);
    if (pmCenterOutputWindow)
    {
	strcat (stTextStdoutWindow, MIDDLE_ATTRIB);
	strcat (stGraphicsStdoutWindow, MIDDLE_ATTRIB);
    }
    else
    {
	strcat (stTextStdoutWindow, TOP_RIGHT_ATTRIB);
	strcat (stGraphicsStdoutWindow, TOP_RIGHT_ATTRIB);
    }
    
    // Set all the ID's to 0
    for (cnt = 0 ; cnt < MAX_IDS ; cnt++)
    {
    	stIDs [cnt].id = 0;
    }
    stIDCounter = 0;

} // MIO_Init


/************************************************************************/
/* MIO_Finalize								*/
/************************************************************************/
void	MIO_Finalize (void)
{
} // MIO_Finalize


/************************************************************************/
/* MIO_Init_Free							*/
/*									*/
/* This free's up various resources, closes all the Run windows, etc.	*/
/* It should be called whenever someone compiles a new program or does	*/
/* a compiler reset.							*/
/************************************************************************/
void	MIO_Init_Free (void)
{
    
    //
    // Close any open run windows
    //
    MIO_CloseAllRunWindows ();

    // Free all allocated objects in the debugger (which should be marked 
    // free at this point)
    EdInt_NotifyDebuggerInitRun ();
} // MIO_Init_Free


/************************************************************************/
/* MIO_Init_Run								*/
/************************************************************************/
BOOL	MIO_Init_Run (const char *pmProgramName,
		      const char *pmInputPath, BOOL pmEchoInput,
		      const char *pmOutputPath, BOOL pmOutputToScreenAndFile,
		      BOOL pmOutputToPrinter, const char *pmExecutionDirectory,
		      BOOL pmDefaultGraphicsMode, const char *pmFontName, 
		      int pmFontSize, int xpmFontWidth, 
		      int xpmFontOptions, int xpmWindowDimension, 
		      int xpmWindowWidth, int xpmWindowHeight, 
		      int pmWindowRows, int pmWindowColumns,
		      BOOL pmFullScreen, COLOR pmSelectionColour,
		      BOOL pmAllowSysExec, BOOL pmAllowSound,
		      int pmParallelIOPort, BOOL pmTestSuiteProgram)
{
    MIOWin_Properties	myMIOWindowProperties;
    int			myLen;
    void		*myFile;
    int			myDefaultWindowMode;
    BOOL		myStdinUsesWindow = FALSE, myStdoutUsesWindow = FALSE;
    BOOL		myStdoutIsRedirected = FALSE;
    char		*myWindowAttribs;
    char    		myDescription [2048];
    char		myTestSuiteInputPath [2048];
    static unsigned int	myStProgramNameSize = 0;

    EdInt_AddFailMessage ("MIO_Init_Run called");
    
    EdInt_Init_Run ();
    
    //
    // Save program name (minus recognized suffixes)
    //
    if (strlen (pmProgramName) > myStProgramNameSize)
    {
	if (MIO_programName != NULL)
	{
	    free (MIO_programName);
	}
	MIO_programName = malloc (strlen (pmProgramName) + 1);
	myStProgramNameSize = strlen (pmProgramName);
    }
    strcpy (MIO_programName, pmProgramName);
    myLen = strlen (MIO_programName);
    if (strcmp (&MIO_programName [myLen - 2], ".t") == 0)
    {
	MIO_programName [strlen (MIO_programName) - 2] = 0;
    }
    else if ((strcmp (&MIO_programName [myLen - 3], ".ti") == 0) ||
	     (strcmp (&MIO_programName [myLen - 3], ".tu") == 0))
    {
	MIO_programName [strlen (MIO_programName) - 3] = 0;
    }
    else if ((strcmp (&MIO_programName [myLen - 4], ".tur") == 0) ||
	     (strcmp (&MIO_programName [myLen - 4], ".dem") == 0))
    {
	MIO_programName [strlen (MIO_programName) - 4] = 0;
    }

    MIO_Init_Free ();

    // We'll do a memory test here (Normally commented out!)
//    MDIO_MemoryLeakTest ();

    MIO_paused = FALSE;
    MIO_finished = FALSE;

    // Initialize everything
    // Reset the TLIB end-of-file marker on stdin
    TL_TLI_TLIEFR(TURINGSTDIN);		/* Reset TLIB eof marker */

    // Set the execution directory.
    //FileManager_ChangeExecDirectory ((OOTstring) pmExecutionDirectory);
    //MIOFile_SetExecutionDirectory (pmExecutionDirectory);
    
    // We are now executing a program
    stCurrentlyExecutingAProgram = TRUE;

    // Initialize for run individual MIO modules
    MIO_parallelIOPort = pmParallelIOPort;
    
    //MIOFont_Init_Run ();
    //MIOMouse_Init_Run ();	    	// Set button chooser
    //MIOMusic_Init_Run (pmAllowSound);	// Set whether sound allowed and
    					// default note octave/duration
    //MIOSprite_Init_Run ();		// Set whether sprites in use/timer
    MIOTime_Init_Run ();	    	// Set the start of app time
    //MIOSys_Init_Run (pmAllowSysExec);   // Set whether Sys.Exec allowed

	//MIOGLGraph_InitRun (); // init SDL

    return TRUE;
} // MIO_Init_Run


/************************************************************************/
/* MIO_Finalize_Run							*/
/************************************************************************/
void	MIO_Finalize_Run (void)
{
    int	cnt;

    // We are no longer executing a program
    stCurrentlyExecutingAProgram = FALSE;
    
    // Before the sprites disappear, we do a final repaint copying all 
    // the sprites to offscreen bitmap for each window that has sprites.
    // Notify the debugger so that items closed now will still show up in the
    // debugger allocated objects section.
    EdInt_NotifyDebuggerFinalizeRun ();

    // Mark all windows as finished execution
    MIO_NotifyTuringProgramFinished ();

    //
    // Free any previously allocated IDs and clear the open IDs
    //
    for (cnt = 0 ; cnt < MAX_IDS ; cnt++)
    {
    	MIO_IDFree (cnt);
    }
    stIDCounter = 0;
    // Finalize individual MIO modules
    MIOLexer_Finalize_Run ();
} // MIO_Finalize_Run


/************************************************************************/
/* MIO_AddToRunWindowList						*/
/************************************************************************/
void	MIO_AddToRunWindowList (void *pmWindow)
{
} // MIO_AddToRunWindowList


/************************************************************************/
/* MIO_CheckColourRange							*/
/************************************************************************/
void	MIO_CheckColourRange (OOTint pmClr)
{
    int		myMaxColours = MIO_selectedRunWindowInfo -> numColours;
    
    if (pmClr < 0)
    {
    	ABORT_WITH_ERRMSG (E_DRAW_CLR_NUM_TOO_SMALL, 
	    	"Color value of %d is out of bounds", pmClr);
    }
    if (pmClr >= myMaxColours)
    {
    	ABORT_WITH_ERRMSG (E_DRAW_CLR_NUM_TOO_LARGE, 
	    	"Color value of %d is out of bounds.  Max color number = %d", 
    	    	pmClr, myMaxColours);
    }
} // MIO_CheckColourRange


/************************************************************************/
/* MIO_CheckInputIsFromKeyboard						*/
/*									*/
/* This checks to see if we're reading from the default window and if 	*/
/* input has been redirected from a file.				*/
/************************************************************************/
void	MIO_CheckInputIsFromKeyboard (const char *pmRoutineName)
{
} // MIO_CheckInputIsFromKeyboard


/************************************************************************/
/* MIO_CheckLineStyleRange						*/
/************************************************************************/
void	MIO_CheckLineStyleRange (OOTint pmStyle)
{
    if (pmStyle < 0)
    {
    	ABORT_WITH_ERRMSG (E_DRAW_CLR_NUM_TOO_SMALL, 
	    	"Style value of %d is out of bounds", pmStyle);
    }
    if (pmStyle > MAX_LINE_STYLE)
    {
    	ABORT_WITH_ERRMSG (E_DRAW_CLR_NUM_TOO_LARGE, 
	    	"Line style value of %d is out of bounds.  Max line style = %d", 
	    	pmStyle, MAX_LINE_STYLE);
    }
} // MIO_CheckLineStyleRange


/************************************************************************/
/* MIO_CheckOuputIsToWindow						*/
/*									*/
/* This checks to see if we're reading from the default window and if 	*/
/* input has been redirected from a file.				*/
/************************************************************************/
void	MIO_CheckOuputIsToWindow (const char *pmRoutineName)
{
    ABORT_WITH_ERRMSG (E_NOT_ALLOWED_IN_TEXT_MODE, 
	    	      "Output from \"%s\" cannot sent to a file", 
    	              pmRoutineName);
} // MIO_CheckOuputIsToWindow


/************************************************************************/
/* MIO_CheckOuputWindowIsInGraphicsMode					*/
/*									*/
/* This checks to see if we're reading from the default window and if 	*/
/* input has been redirected from a file.				*/
/************************************************************************/
void	MIO_CheckOuputWindowIsInGraphicsMode (const char *pmRoutineName)
{
    ABORT_WITH_ERRMSG (E_NOT_ALLOWED_IN_TEXT_MODE, 
	    	      "Output from \"%s\" cannot sent to a text window", 
    	              pmRoutineName);
} // MIO_CheckOuputWindowIsInGraphicsMode


/************************************************************************/
/* MIO_CloseAllRunWindows						*/
/************************************************************************/
void	MIO_CloseAllRunWindows (void)
{
} // MIO_CloseAllRunWindows


/************************************************************************/
/* MIO_DebugOut								*/
/************************************************************************/
void	MIO_DebugOut (const char *pmFormat, ...)
{
    char	myString [1024];
    va_list	myArgList;

    va_start (myArgList, pmFormat);
    MDIO_vsprintf (myString, pmFormat, myArgList);
    va_end (myArgList);
    
    fputs (myString, stderr);
    fputs ("\n", stderr);
} // MIO_DebugOut


/************************************************************************/
/* MIO_ErrorInfo							*/
/************************************************************************/
void	MIO_ErrorInfo (const char *pmFormat, ...)
{
    char	myString [1024];
    va_list	myArgList;

    if (stErrorInfo)
    {
    	va_start (myArgList, pmFormat);
    	MDIO_vsprintf (myString, pmFormat, myArgList);
    	va_end (myArgList);
    
    	fputs ("[Error] ", stderr);
    	fputs (myString, stderr);
    	fputs ("\n", stderr);
    }
} // MIO_ErrorInfo


/************************************************************************/
/* MIO_GetTopMostWindow							*/
/*									*/
/* Return a "topmost" Run window if there is one.  If no window is	*/
/* designated "topmost", then return NULL.				*/
/************************************************************************/
MIOWinInfoPtr	MIO_GetTopMostWindow (void)
{
    return NULL;
} // MIO_GetTopMostWindow


/************************************************************************/
/* MIO_IsAnyRunWindowVisible						*/
/************************************************************************/
BOOL	MIO_IsAnyRunWindowVisible (void)
{
    return FALSE;
} // MIO_IsAnyRunWindowVisible


/************************************************************************/
/* MIO_ListTopMostWindows						*/
/*									*/
/* Return a "topmost" Run window if there is one.  If no window is	*/
/* designated "topmost", then return NULL.				*/
/************************************************************************/
MIOWinInfoPtr	MIO_ListTopMostWindows (BOOL pmStart)
{
    return NULL;
} // MIO_GetTopMostWindow


/************************************************************************/
/* MIO_MakePopupWindowVisible						*/
/************************************************************************/
void	MIO_MakePopupWindowVisible (void)
{
} // MIO_MakePopupWindowVisible


/************************************************************************/
/* MIO_MallocString							*/
/************************************************************************/
char	*MIO_MallocString (const char *pmString)
{
    char	*myString;
    
    myString = malloc (strlen (pmString) + 1);
    if (myString != NULL)
    {
    	strcpy (myString, pmString);
    }
    return myString;
} // MIO_MallocString


/************************************************************************/
/* MIO_MoreInfo								*/
/************************************************************************/
void	MIO_MoreInfo (const char *pmFormat, ...)
{
    char	myString [1024];
    va_list	myArgList;
    SRCPOS	mySrcPos;
    char	myFilePath [4096], *myFilePtr;

    if (stMoreInfo)
    {
	// Output source position
	Language_Execute_RunSrcPosition (&mySrcPos);
	FileManager_FileName (mySrcPos.fileNo, myFilePath);
	if (strrchr (myFilePath, '\\') == NULL)
	{
	    myFilePtr = myFilePath;
	}
	else
	{
	    myFilePtr = strrchr (myFilePath, '\\') + 1;
	}
	MDIO_sprintf (myString, "[Info] Line %d of %s: ", mySrcPos.lineNo, 
		      myFilePtr);
    	fputs (myString, stderr);

	// Provide extra information
    	va_start (myArgList, pmFormat);
    	MDIO_vsprintf (myString, pmFormat, myArgList);
    	va_end (myArgList);
    
    	fputs (myString, stderr);
    	fputs ("\n", stderr);
    }
} // MIO_MoreInfo


/************************************************************************/
/* MIO_NotifyTuringProgramFinished					*/
/************************************************************************/
void	MIO_NotifyTuringProgramFinished (void)
{
} // MIO_NotifyTuringProgramFinished


/************************************************************************/
/* MIO_NotifyTuringProgramPaused					*/
/************************************************************************/
void	MIO_NotifyTuringProgramPaused (void)
{
} // MIO_NotifyTuringProgramPaused


/************************************************************************/
/* MIO_NotifyTuringProgramResumed					*/
/************************************************************************/
void	MIO_NotifyTuringProgramResumed (BOOL pmPutMIOWindowsOnTop)
{
} // MIO_NotifyTuringProgramResumed


/************************************************************************/
/* MIO_RemoveFromRunWindowList						*/
/************************************************************************/
void	MIO_RemoveFromRunWindowList (void *pmWindow)
{
} // MIO_RemoveFromRunWindowList


/************************************************************************/
/* MIO_SetNextWindowActive						*/
/************************************************************************/
void	MIO_SetNextWindowActive (WIND pmWindow)
{
} // MIO_SetNextWindowActive


/************************************************************************/
/* Called by executor							*/
/************************************************************************/
/************************************************************************/
/* MIO									*/
/*									*/
/* Called when Turing is initialized by Language_Execute		*/
/************************************************************************/
void	MIO (void)
{
} // MIO


/************************************************************************/
/* MIO_Init								*/
/*									*/
/* Called each time program is executed by Language_Execute_Initialize	*/
/************************************************************************/
void	MIO_Init (void)
{
} // MIO_Init


/************************************************************************/
/* MIO_DrawPic								*/
/************************************************************************/
void	MIO_DrawPic (OOTint pmX, OOTint pmY, char *pmBuffer, OOTint pmMode)
{
    MIOPic_DrawPic (pmX, pmY, pmBuffer, pmMode);
} // MIO_DrawPic


/************************************************************************/
/* MIO_Getch								*/
/************************************************************************/
char	MIO_Getch (void)
{
    MIO_CheckInputIsFromKeyboard ("getch");

    return MIOWin_Getch (MIO_selectedRunWindow);
} // MIO_Getch


/************************************************************************/
/* MIO_GetEvent								*/
/************************************************************************/
void	MIO_GetEvent (TW_OOT_EVENT *pmEventPtr)
{
// TW    *pmEventPtr = (TW_OOT_EVENT *) EVENT_Get (DEFAULT_WIN);
} // MIO_GetEvent


/************************************************************************/
/* MIO_Hasch								*/
/************************************************************************/
OOTboolean	MIO_Hasch (void)
{
    MIO_CheckInputIsFromKeyboard ("hasch");

    return MIOWin_Hasch (MIO_selectedRunWindow);
} // MIO_Hasch


/************************************************************************/
/* MIO_HasEvent								*/
/*									*/
/* Called by OOT to determine if there's the appropriate event		*/
/* available.  We use this to determine when to display the cursor.	*/
/************************************************************************/
OOTboolean	MIO_HasEvent (MIOFILE *pmMIOFile, EventDescriptor *pmEvent)
{
return TRUE;
} // MIO_HasEvent


/************************************************************************/
/* The 4 MIO_ID... routines handle the allocation of ID numbers	to 	*/
/* Fonts, Directories, Pictures, Sprites, and  Lexical Things.  	*/
/*									*/
/*   It uses a concepts of a slot number (the index into the stIDs 	*/
/* array and the ID number, which is actually returned as the ID of the */
/* object. The ID number is the sum of the x_ID_BASE and the slot	*/
/* number. The x_ID_BASE is different for each type of object.		*/
/*   The id field of the stIDs array is positive when the slot is 	*/
/* allocated to an ID and negative the same ID when the slot has been	*/
/* freed. In that way Turing can detect an attempt to use a freed item.	*/
/************************************************************************/

/************************************************************************/
/* MIO_IDAdd								*/
/*									*/
/*   This routine is called by each of the Font/Directory/etc. creation	*/
/* routines.  It first allocates a slot number and then adds the	*/
/* appropriate base, the notifies the debugger that the item with 	*/
/* that ID has been allocated.						*/
/*   This routine should allocate a new slot number each time, 		*/
/* incrementing the slot number each time an object is allocated.  In	*/
/* that way it is unlikely that using a freed ID number will mistakenly	*/
/* use a current item with the same ID number.				*/ 
/************************************************************************/
int	MIO_IDAdd (int pmIDType, void *pmInfo, SRCPOS *pmSrcPos, 
		   const char *pmDescription, void *pmCompareInfo)
{
    // Get first open id number
    int	myInitialValue = stIDCounter;
    int	myIDNumber;
    
    // If the current slot is not open, then we've already cycled once 
    // around all the ID's
    if (stIDs [stIDCounter].id > 0)
    {
    	// Look for an freed ID
    	stIDCounter = (stIDCounter + 1) % MAX_IDS;
        while (stIDCounter != myInitialValue)
        {
	    if (stIDs [stIDCounter].id < 0)
	    {
	    	break;
	    }
    	    stIDCounter = (stIDCounter + 1) % MAX_IDS;
    	}
    	if (stIDCounter == myInitialValue)
    	{
	    ABORT_WITH_ERRNO (E_OUT_OF_ITEM_IDS);
    	}
    }

    if (pmIDType == FONT_ID)
    {
    	myIDNumber = stIDCounter + FONT_ID_BASE;
    }
    else if (pmIDType == DIR_ID)
    {
    	myIDNumber = stIDCounter + DIR_ID_BASE;
    }
    else if (pmIDType == PIC_ID)
    {
    	myIDNumber = stIDCounter + PIC_ID_BASE;
    }
    else if (pmIDType == SPRITE_ID)
    {
    	myIDNumber = stIDCounter + SPRITE_ID_BASE;
    }
    else if (pmIDType == LEXER_ID)
    {
    	myIDNumber = stIDCounter + LEX_ID_BASE;
    }
	else if (pmIDType == HASHMAP_ID)
    {
    	myIDNumber = stIDCounter + HASHMAP_ID_BASE;
    }
    else
    {
    	// TW Error!
    }

    stIDs [stIDCounter].id = myIDNumber;
    stIDs [stIDCounter].type = pmIDType;
    stIDs [stIDCounter].count = 1;
    stIDs [stIDCounter].info = pmInfo;
    stIDs [stIDCounter].compareInfo = pmCompareInfo;
    EdInt_NotifyDebuggerObjectAllocated (pmIDType, myIDNumber, 
					 pmSrcPos, pmDescription);
        
    stIDCounter = (stIDCounter + 1) % MAX_IDS;

    return (myIDNumber);    
} // MIO_IDAdd


/************************************************************************/
/* MIO_IDCompare							*/
/*									*/
/* Return the ID that matches the info passed in.			*/
/************************************************************************/
int	MIO_IDCompare (int pmIDType, void *pmInfo, int pmInfoSize)
{
    int		mySlotNumber;
    
    for (mySlotNumber = 0 ; mySlotNumber < MAX_IDS ; mySlotNumber++)
    {
	if ((stIDs [mySlotNumber].id > 0) && 
	    (stIDs [mySlotNumber].type == pmIDType))
	{
	    if (memcmp (stIDs [mySlotNumber].compareInfo, pmInfo, 
			pmInfoSize) == 0)
	    {
		return stIDs [mySlotNumber].id;
	    }
	} // if (stIDs [pmSlotNumber].id > 0)
    } // for
    
    return 0;
} // MIO_IDCompare


/************************************************************************/
/* MIO_IDDecrement							*/
/*									*/
/* Decrement the count.  This is used for FONT, to avoid multiple	*/
/* instances of the same font requiring more and more ID's.  Instead a	*/
/* count is used.  The count should never reach 0.			*/
/************************************************************************/
void	MIO_IDDecrement (int pmIDNumber)
{
    int		myIDSlot;
    
    MyGetIDSlot (pmIDNumber, NULL, &myIDSlot);

    if (stIDs [myIDSlot].id > 0)
    {
	stIDs [myIDSlot].count--;
    }
} // MIO_IDDecremenent


/************************************************************************/
/* MIO_IDFree								*/
/*									*/
/*   This routine is called by MIO_Finalize_Run when the system is	*/
/* attempting to free all allocated items.  It goes through each slot 	*/
/* in the table and MIO_IDFree to free the item allocated to the slot.	*/
/* MIO_IDFree calls the appropriate free item routine for the item	*/
/* type.  The free item routine calls MIO_IDRemove.			*/
/************************************************************************/
void	MIO_IDFree (int pmSlotNumber)
{
    int		myIDNumber, myType;
    
    if (stIDs [pmSlotNumber].id > 0)
    {
	// The following line stops any of the free routines from performing
	// a decrememt.  Instead the item will be freed.
	stIDs [pmSlotNumber].count = 1;

    	myIDNumber = stIDs [pmSlotNumber].id;
	myType = stIDs [pmSlotNumber].type;

	switch (myType)
	{
	    case FONT_ID:
    		MIOFont_Free (myIDNumber);
		break;
	    case DIR_ID:
		MIODir_Close (myIDNumber);
		break;
	    case PIC_ID:
    		MIOPic_Free (myIDNumber);
		break;
	    case SPRITE_ID:
    		MIOSprite_Free (myIDNumber);
		break;
	    case LEXER_ID:
    		MIOLexer_End (myIDNumber);
		break;
		case HASHMAP_ID:
    		MIOHashmap_Free (myIDNumber);
		break;
	    default:
    		// TW - Abort!
		break;
	} // switch

	if (stIDs [pmSlotNumber].compareInfo != NULL)
	{
	    free (stIDs [pmSlotNumber].compareInfo);
	}
    } // if (stIDs [pmSlotNumber].id > 0)
    
    // Set the slot to 0.
    stIDs [pmSlotNumber].id = 0;
} // MIO_IDFree
    

/************************************************************************/
/* MIO_IDGet								*/
/************************************************************************/
void	*MIO_IDGet (int pmIDNumber, int pmIDType)
{
    int		myActualIDType, myActualIDSlot;
    
    MyGetIDSlot (pmIDNumber, &myActualIDType, &myActualIDSlot);
    
    if (pmIDType != myActualIDType)
    {
    	char	myMessage [256];
    	int	myMessageNumber;
	char	*myFirstPart, *mySecondPart, *myKind, *myKindWhat;
    	
    	switch (pmIDType)
    	{
    	    case FONT_ID:
		myFirstPart = "font ID";
		myKind = "font";
		myKindWhat = "created";
    	    	myMessageNumber = E_FONT_NOT_AN_ID;
		break;
    	    case DIR_ID:
		myFirstPart = "directory stream";
		myKind = "directory";
		myKindWhat = "opened";
    	    	myMessageNumber = E_DIR_NOT_AN_ID;
		break;
    	    case PIC_ID:
		myFirstPart = "picture ID";
		myKind = "picture";
		myKindWhat = "created";
    	    	myMessageNumber = E_PIC_NOT_AN_ID;
		break;
    	    case SPRITE_ID:
		myFirstPart = "sprite ID";
		myKind = "sprite";
		myKindWhat = "created";
    	    	myMessageNumber = E_SPRITE_NOT_AN_ID;
		break;
    	    case LEXER_ID:
		myFirstPart = "lexer ID";
		myKind = "lexer object";
		myKindWhat = "created";
    	    	myMessageNumber = E_SPRITE_NOT_AN_ID;
		break;
		case HASHMAP_ID:
		myFirstPart = "HASHMAP ID";
		myKind = "HASHMAP object";
		myKindWhat = "created";
    	    	myMessageNumber = E_SPRITE_NOT_AN_ID;
		break;
	}
    	switch (myActualIDType)
    	{
    	    case FONT_ID:
		mySecondPart = "font ID";
		break;
    	    case DIR_ID:
		mySecondPart = "directory stream";
		break;
    	    case PIC_ID:
		mySecondPart = "picture ID";
		break;
    	    case SPRITE_ID:
		mySecondPart = "sprite ID";
		break;
    	    case LEXER_ID:
		mySecondPart = "lexer ID";
		break;
		case HASHMAP_ID:
		mySecondPart = "hashmap ID";
		break;
	}

	if (myActualIDType == UNKNOWN_ID)
	{
    	    MDIO_sprintf (myMessage, 
		"Illegal %s number '%d'.  ('%d' is a not a legal identifier.)",
		myFirstPart, pmIDNumber, pmIDNumber);
	}
	else if (myActualIDType == ZERO_ID)
	{
    	    MDIO_sprintf (myMessage, 
		"Illegal %s number '0'.  (Probable cause: %s was not "
		"successfully %s.)", myFirstPart, myKind, myKindWhat);
	}
	else
	{
    	    MDIO_sprintf (myMessage, 
		      "Illegal %s number '%d'.  ('%d' is a legal %s.)",
		      myFirstPart, pmIDNumber, pmIDNumber, mySecondPart);
	}

	ABORT_WITH_ERRMSG (myMessageNumber, myMessage);
	
    	// Never actually reaches here.
    	return NULL;
    } // if 
    
    if (pmIDNumber == stIDs [myActualIDSlot].id)
    {
    	return stIDs [myActualIDSlot].info;
    }
    else if (pmIDNumber == -stIDs [myActualIDSlot].id)
    {
    	switch (pmIDType)
    	{
    	    case FONT_ID:
	    	ABORT_WITH_ERRNO (E_FONT_FREED);
    	        break;
    	    case DIR_ID:
	    	ABORT_WITH_ERRNO (E_DIR_CLOSED);
    	        break;
    	    case PIC_ID:
	    	ABORT_WITH_ERRNO (E_PIC_FREED);
    	        break;
    	    case SPRITE_ID:
	    	ABORT_WITH_ERRNO (E_SPRITE_FREED);
    	        break;
    	    case LEXER_ID:
	    	ABORT_WITH_ERRNO (E_LEX_ENDED);
    	        break;
			case HASHMAP_ID:
	    	ABORT_WITH_ERRNO (E_HASHMAP_FREED);
    	        break;
    	} // switch
    }
    else 
    {
    	switch (pmIDType)
    	{
    	    case FONT_ID:
	    	ABORT_WITH_ERRNO (E_FONT_NEVER_NEWED);
    	        break;
    	    case DIR_ID:
	    	ABORT_WITH_ERRNO (E_DIR_NEVER_OPENED);
    	        break;
    	    case PIC_ID:
	    	ABORT_WITH_ERRNO (E_PIC_NEVER_NEWED);
    	        break;
    	    case SPRITE_ID:
	    	ABORT_WITH_ERRNO (E_SPRITE_NEVER_NEWED);
    	        break;
    	    case LEXER_ID:
	    	ABORT_WITH_ERRNO (E_LEX_NEVER_INITIALIZED);
    	        break;
			case HASHMAP_ID:
	    	ABORT_WITH_ERRNO (E_HASHMAP_NEVER_INITIALIZED);
    	        break;
    	} // switch
    }
    
    // Never actually reaches here.
    return NULL;
} // MIO_IDGet


/************************************************************************/
/* MIO_IDGetCount							*/
/************************************************************************/
int	MIO_IDGetCount (int pmIDNumber, int pmIDType)
{
    int		myIDSlot;
    
    // Call MIO_IDGet to check to make certain it's a legal ID.
    MIO_IDGet (pmIDNumber, pmIDType);

    MyGetIDSlot (pmIDNumber, NULL, &myIDSlot);

    if (stIDs [myIDSlot].id > 0)
    {
	return stIDs [myIDSlot].count;
    }

    return 0;
} // MIO_IDGetCount


/************************************************************************/
/* MIO_IDIncrement							*/
/************************************************************************/
void	MIO_IDIncrement (int pmIDNumber)
{
    int		myIDSlot;
    
    MyGetIDSlot (pmIDNumber, NULL, &myIDSlot);

    if (stIDs [myIDSlot].id > 0)
    {
	stIDs [myIDSlot].count++;
    }
} // MIO_IDIncrement


/************************************************************************/
/* MIO_IDRemove								*/
/************************************************************************/
void	MIO_IDRemove (int pmIDNumber, int pmIDType)
{
    int		myIDSlot;
    
    // Call MIO_IDGet to check to make certain it's a legal ID.
    MIO_IDGet (pmIDNumber, pmIDType);
    
    MyGetIDSlot (pmIDNumber, NULL, &myIDSlot);

    // Tell the debugger it is no longer in use.
    EdInt_NotifyDebuggerObjectDeallocated (pmIDNumber);

    stIDs [myIDSlot].id = -stIDs [myIDSlot].id;
    stIDs [myIDSlot].info = NULL;
} // MIO_IDRemove


/************************************************************************/
/* MIO_PlayDone								*/
/************************************************************************/
OOTboolean	MIO_PlayDone (void)
{
    return TRUE;
} // MIO_PlayDone


/************************************************************************/
/* MIO_RectanglesIntersect						*/
/*									*/
/* Return true if the two rectangles intersect.				*/
/* Assumes (0,0) in upper-left coordinates.				*/
/************************************************************************/
BOOL	MIO_RectanglesIntersect (MYRECT *pmRect1, MYRECT *pmRect2)
{
    if (pmRect1 -> right < pmRect2 -> left) return FALSE;
    if (pmRect1 -> left > pmRect2 -> right) return FALSE;
    if (pmRect1 -> bottom < pmRect2 -> top) return FALSE;
    if (pmRect1 -> top > pmRect2 -> bottom) return FALSE;
    return TRUE;
} // MIO_RectanglesIntersect


/************************************************************************/
/* MIO_RectanglesSetIntersect						*/
/*									*/
/* Return true if the two rectangles intersect.				*/
/* Assumes (0,0) in upper-left coordinates.				*/
/************************************************************************/
BOOL	MIO_RectanglesSetIntersect (MYRECT *pmRect1, MYRECT *pmRect2)
{
    if (pmRect1 -> right < pmRect2 -> left) return FALSE;
    if (pmRect1 -> left > pmRect2 -> right) return FALSE;
    if (pmRect1 -> bottom < pmRect2 -> top) return FALSE;
    if (pmRect1 -> top > pmRect2 -> bottom) return FALSE;

    // They intersect, make pmRect1 the intersection of the two
    pmRect1 -> left = MAX (pmRect1 -> left, pmRect2 -> left);
    pmRect1 -> right = MIN (pmRect1 -> right, pmRect2 -> right);
    pmRect1 -> top = MAX (pmRect1 -> top, pmRect2 -> top);
    pmRect1 -> bottom = MIN (pmRect1 -> bottom, pmRect2 -> bottom);

    return TRUE;
} // MIO_RectanglesSetIntersect


/************************************************************************/
/* MIO_RectanglesUnion							*/
/*									*/
/* pmRect1 becomes a union of pmRect1 and pmRect2			*/
/* Assumes (0,0) in upper-left coordinates.				*/
/************************************************************************/
void	MIO_RectanglesUnion (MYRECT *pmRect1, MYRECT *pmRect2)
{
    pmRect1 -> left = MIN (pmRect1 -> left, pmRect2 -> left);
    pmRect1 -> right = MAX (pmRect1 -> right, pmRect2 -> right);
    pmRect1 -> top = MIN (pmRect1 -> top, pmRect2 -> top);
    pmRect1 -> bottom = MAX (pmRect1 -> bottom, pmRect2 -> bottom);
} // MIO_RectanglesUnion


/************************************************************************/
/* MIO_RegisterClose							*/
/************************************************************************/
void	MIO_RegisterClose (OOTint pmID)
{
    if (stLastFileClosedWasFile)
    {
    	// Tell the debugger it is no longer in use.
    	EdInt_NotifyDebuggerObjectDeallocated (pmID);

	MIO_MoreInfo ("Stream %d closed", pmID);
    }
} // MIO_RegisterClose


/************************************************************************/
/* MIO_RegisterOpen							*/
/************************************************************************/
void	MIO_RegisterOpen (OOTint pmID, OOTint pmMode)
{
    if (stLastFileOpenedWasFile)
    {
    	SRCPOS	mySrcPos;
    	char	myDescription [1024];
    	
    	Language_Execute_RunSrcPosition (&mySrcPos);
    	strcpy (myDescription, stLastFileDescription);
	strcat (myDescription, " [");
	if (pmMode & 0x02) strcat (myDescription, "get, ");
	if (pmMode & 0x04) strcat (myDescription, "put, ");
	if (pmMode & 0x08) strcat (myDescription, "read, ");
	if (pmMode & 0x10) strcat (myDescription, "write, ");
	if (pmMode & 0x20) strcat (myDescription, "mod, ");
	if (pmMode & 0x01) strcat (myDescription, "seek, ");
	// There must have been at least one item in the mode.  Eliminate
	// the last two characters.
	strcpy (&myDescription [strlen (myDescription) - 2], "]");
	
        EdInt_NotifyDebuggerObjectAllocated (FILE_STREAM, pmID, &mySrcPos,
	    myDescription);

	MIO_MoreInfo ("File \"%s\" opened as stream %d", stLastFileDescription, pmID);
    }    	
} // MIO_RegisterOpen


/************************************************************************/
/* MIO_SendInfoToStderr							*/
/************************************************************************/
void	MIO_SendInfoToStderr (BOOL pmLibErrors, BOOL pmLibInfo)
{
    stErrorInfo = pmLibErrors;
    stMoreInfo = pmLibInfo;
} // MIO_SendInfoToStderr


/************************************************************************/
/* MIO_SetActive							*/
/************************************************************************/
OOTboolean MIO_SetActive (MIOFILE *pmMIOFile)
{
} // MIO_SetActive


/************************************************************************/
/* MIO_SizePic								*/
/************************************************************************/
OOTint	MIO_SizePic (OOTint pmX1, OOTint pmY1, OOTint pmX2, OOTint pmY2)
{
    return (MIOPic_SizePic (pmX1, pmY1, pmX2, pmY2));
} // MIO_SizePic


#ifdef NOT_YET_NEEDED
/************************************************************************/
/* MIO_StreamAdd							*/
/*									*/
/* The 3 MIO_Stream... routines handle the allocation of stream numbers	*/
/* to the Window and Net modules.  It can also handle File streams, but	*/
/* the Turing engine doesn't use them.  (Needs to be modified to handle	*/
/* streams -2, -1 and 0.						*/
/************************************************************************/
int	MIO_StreamAdd (int pmStreamType, void *pmInfo)
{
    // Get first open stream number
    int	myInitialValue = stStreamCounter;
    int	myStreamNumber;
    
    if ((stStreams [stStreamCounter].id >= 0) &&
        (stStreams [stStreamCounter].id != ID_OPEN))
    {
    	stStreamCounter = (stStreamCounter + 1) % MAX_STREAMS;
        while (stStreamCounter != myInitialValue)
        {
	    if ((stStreams [stStreamCounter].id < 0) ||
	        (stStreams [stStreamCounter].id == ID_OPEN))
	    {
	    	break;
	    }
    	    stStreamCounter = (stStreamCounter + 1) % MAX_STREAMS;
    	}
    	if (stStreamCounter == myInitialValue)
    	{
    	    // TW No open slots!
    	}
    }

    if (pmStreamType == FILE_STREAM)
    {
    	myStreamNumber = stStreamCounter + FILE_STREAM_ID_BASE;
    }
    else if (pmStreamType == WINDOW_STREAM)
    {
    	myStreamNumber = stStreamCounter + WINDOW_STREAM_ID_BASE;
    }
    else if (pmStreamType == NET_STREAM)
    {
    	myStreamNumber = stStreamCounter + NET_STREAM_ID_BASE;
    }
    else
    {
    	// TW Error!
    }

    stStreams [stStreamCounter].id = myStreamNumber;
    stStreams [stStreamCounter].info = pmInfo;
        
    stStreamCounter = (stStreamCounter + 1) % MAX_STREAMS;

    return (myStreamNumber);    
} // MIO_StreamAdd


/************************************************************************/
/* MIO_StreamGet							*/
/************************************************************************/
void	*MIO_StreamGet (int pmStreamID, int pmStreamType)
{
    int		myActualStreamType, myActualStreamSlot;
    
    // Convert default window and stderr window
    if ((pmStreamID == STDOUT_WINDOW) && (pmStreamType == WINDOW_STREAM))
    {
    	if (stDefaultRunWindow == NULL)
    	{
    	    // TW Error!
    	}
    	else
    	{
    	    return MIOWin_GetInfo (stDefaultRunWindow);
    	}
    }
    else if ((pmStreamID == STDERR_WINDOW) && (pmStreamType == WINDOW_STREAM))
    {
    	return MIOWin_GetInfo (stderr -> windowPtr);
    }

    if (((-2 <= pmStreamID) && (pmStreamID <= 0)) ||
	((FILE_STREAM_ID_BASE <= pmStreamID) && 
    	 (pmStreamID < FILE_STREAM_ID_BASE + MAX_STREAMS)))
    {    	 
        myActualStreamType = FILE_STREAM;
        myActualStreamSlot = pmStreamID - FILE_STREAM_ID_BASE;
    }
    else if ((WINDOW_STREAM_ID_BASE <= pmStreamID) && 
    	     (pmStreamID < WINDOW_STREAM_ID_BASE + MAX_STREAMS))
    {
        myActualStreamType = WINDOW_STREAM;
        myActualStreamSlot = pmStreamID - WINDOW_STREAM_ID_BASE;
    }
    else if ((NET_STREAM_ID_BASE <= pmStreamID) && 
    	     (pmStreamID < NET_STREAM_ID_BASE + MAX_STREAMS))
    {
        myActualStreamType = NET_STREAM;
        myActualStreamSlot = pmStreamID - NET_STREAM_ID_BASE;
    }
    else
    {
    	myActualStreamType = UNKNOWN_STREAM;
    }
    
    if (pmStreamType != myActualStreamType)
    {
    	char	myMessage [256];
    	int	myMessageNumber;
    	
    	switch (pmStreamType)
    	{
    	    case FILE_STREAM:
    	    	myMessageNumber = E_STREAM_NOT_AN_ID;
    	    	switch (myActualStreamType)
    	    	{
    	    	    case WINDOW_STREAM:
    	    	    	MDIO_sprintf (myMessage, 
  				      "Illegal file stream number '%d'.  "
  				      "('%d' is a legal window ID.)",
  				      pmStreamID, pmStreamID);
    	    	        break;
    	    	    case NET_STREAM:
    	    	    	MDIO_sprintf (myMessage, 
  				      "Illegal file stream number '%d'.  "
   				      "('%d' is a legal net stream number.)",
  				      pmStreamID, pmStreamID);
    	    	        break;
    	    	    case UNKNOWN_STREAM:
    	    	    	MDIO_sprintf (myMessage, 
  				      "Illegal file stream number '%d'.",
  				      pmStreamID);
    	    	        break;
    	    	} // switch
    	        break;
    	    case WINDOW_STREAM:
    	    	myMessageNumber = E_WINDOW_NOT_AN_ID;
    	    	switch (myActualStreamType)
    	    	{
    	    	    case FILE_STREAM:
    	    	    	MDIO_sprintf (myMessage, 
  				      "Illegal window ID number '%d'.  "
   				      "('%d' is a legal file stream number.)",
  				      pmStreamID, pmStreamID);
    	    	        break;
    	    	    case NET_STREAM:
    	    	    	MDIO_sprintf (myMessage, 
  				      "Illegal window ID number '%d'.  "
   				      "('%d' is a legal net stream number.)",
  				      pmStreamID, pmStreamID);
    	    	        break;
    	    	    case UNKNOWN_STREAM:
    	    	    	MDIO_sprintf (myMessage, 
  				      "Illegal window ID number '%d'.",
  				      pmStreamID);
    	    	        break;
    	    	} // switch
    	        break;
    	    case NET_STREAM:
    	    	myMessageNumber = E_NET_NOT_AN_ID;
    	    	switch (myActualStreamType)
    	    	{
    	    	    case FILE_STREAM:
    	    	    	MDIO_sprintf (myMessage, 
  				      "Illegal net stream number '%d'.  "
  				      "('%d' is a legal file stream number.)",
  				      pmStreamID, pmStreamID);
    	    	        break;
    	    	    case WINDOW_STREAM:
    	    	    	MDIO_sprintf (myMessage, 
  				      "Illegal net stream number '%d'.  "
  				      "('%d' is a legal window ID.)",
  				      pmStreamID, pmStreamID);
    	    	        break;
    	    	    case UNKNOWN_STREAM:
    	    	    	MDIO_sprintf (myMessage, 
  				      "Illegal net stream number '%d'.",
  				      pmStreamID);
    	    	        break;
    	    	} // switch
    	        break;
    	} // switch

	ABORT_WITH_ERRMSG (myMessageNumber, myMessage);
	
    	// Never actually reaches here.
    	return NULL;
    } // if 
    
    if (pmStreamID == stStreams [myActualStreamSlot].id)
    {
    	return stStreams [myActualStreamSlot].info;
    }
    else if (pmStreamID == -stStreams [myActualStreamSlot].id)
    {
    	switch (pmStreamType)
    	{
    	    case FILE_STREAM:
	    	ABORT_WITH_ERRNO (E_STREAM_CLOSED);
    	        break;
    	    case WINDOW_STREAM:
	    	ABORT_WITH_ERRNO (E_WINDOW_CLOSED);
    	        break;
    	    case NET_STREAM:
	    	ABORT_WITH_ERRNO (E_NET_NEVER_OPENED);
    	        break;
    	} // switch
    }
    else 
    {
    	switch (pmStreamType)
    	{
    	    case FILE_STREAM:
	    	ABORT_WITH_ERRNO (E_STREAM_NEVER_OPENED);
    	        break;
    	    case WINDOW_STREAM:
	    	ABORT_WITH_ERRNO (E_WINDOW_NEVER_OPENED);
    	        break;
    	    case NET_STREAM:
	    	ABORT_WITH_ERRNO (E_NET_NEVER_OPENED);
    	        break;
    	} // switch
    }
    
    // Never actually reaches here.
    return NULL;
} // MIO_StreamGet


/************************************************************************/
/* MIO_StreamRemove							*/
/************************************************************************/
void	MIO_StreamRemove (int pmStreamID, int pmStreamType)
{
    // Call MIO_StreamGet to check to make certain it's a legal ID.
    MIO_StreamGet (pmStreamID, pmStreamType);
    
    stStreams [pmStreamID % 1000].id = -stStreams [pmStreamID % 1000].id;
    stStreams [pmStreamID % 1000].info = NULL;
} // MIO_StreamRemove
#endif // #ifdef NOT_YET_NEEDED


/************************************************************************/
/* MIO_SubstituteRunWindow						*/
/************************************************************************/
void	MIO_SubstituteRunWindow (WIND pmOldWindow, WIND pmNewWindow, 
				 const char *pmDescription)
{
    MIOWinInfoPtr	myInfo = MIOWin_GetInfo (pmNewWindow);
    
    EdInt_AddFailMessage ("Substitute Run Window %x %x (%s)", 
    	pmOldWindow, pmNewWindow, pmDescription);
    
    if (MIO_selectedRunWindow == pmOldWindow)
    {
	MIO_selectedRunWindow = pmNewWindow;
	MIO_selectedRunWindowInfo = MIOWin_GetInfo (pmNewWindow);
    }

    // Substitute the new window for the old in the MIOFILE structure
    // associated with the old window.
    if (myInfo -> turingMIOFilePtr != NULL)
    {
    	((MIOFILE *) (myInfo -> turingMIOFilePtr)) -> windowPtr = pmNewWindow;
    }
    
} // MIO_SubstituteRunWindow


/************************************************************************/
/* MIO_TakePic								*/
/************************************************************************/
void	MIO_TakePic (OOTint pmX1, OOTint pmY1, OOTint pmX2, OOTint pmY2, 
		     char *pmBuffer, char *pmBufferDescriptor)
{
    MIOPic_TakePic (pmX1, pmY1, pmX2, pmY2, pmBuffer);
    *pmBufferDescriptor = 0;
} // MIO_TakePic


/************************************************************************/
/* MIO_UpdateSpritesIfNecessary						*/
/************************************************************************/
void	MIO_UpdateSpritesIfNecessary (void)
{
    MIOSprite_UpdateIfNecessary (FALSE, FALSE);
} // MIO_UpdateSpritesIfNecessary


/******************************/
/* Static callback procedures */
/******************************/


/*********************/
/* Static procedures */
/*********************/
/************************************************************************/
/* MyCreateMIOFile							*/
/************************************************************************/
static MIOFILE 	*MyCreateMIOFile (void)
{
    MIOFILE	*myMIOFile = (MIOFILE *) malloc (sizeof (MIOFILE));

    if (myMIOFile) 
    {
    	memset (myMIOFile, 0, sizeof (MIOFILE));
    }

    return myMIOFile;
} // MyCreateMIOFile


/************************************************************************/
/* MyCreateMIOPrinter							*/
/************************************************************************/
static MIOPRINTER 	*MyCreateMIOPrinter (const char *pmAttribs)
{
    MIOPRINTER	*myMIOPrinter = (MIOPRINTER *) malloc (sizeof (MIOPRINTER));

    if (myMIOPrinter) 
    {
    	memset (myMIOPrinter, 0, sizeof (MIOPRINTER));
    	myMIOPrinter -> buffer = malloc (INITIAL_PRINT_BUFFER_SIZE);
    	if (myMIOPrinter -> buffer == NULL)
    	{
    	    free (myMIOPrinter);
    	    return NULL;
    	}
    	
    	myMIOPrinter -> buffer [0] = 0;
    	myMIOPrinter -> currentSize = 0;
    	myMIOPrinter -> maxSize = INITIAL_PRINT_BUFFER_SIZE;

	// TW Eventually change this to read the "title:xyz" from the pmAttribs
	// that are passed in, along with noheader, fontsize, font and landscape
	myMIOPrinter -> title = malloc (strlen (MIO_programName) + 10);
	strcpy (myMIOPrinter -> title, "From ");
	strcat (myMIOPrinter -> title, MIO_programName);
    }

    return myMIOPrinter;
} // MyCreateMIOFile


/************************************************************************/
/* MyFreeMIOPrinter							*/
/************************************************************************/
static void	MyFreeMIOPrinter (MIOPRINTER *pmMIOPrinter)
{
    if (pmMIOPrinter -> buffer != NULL)
    {
    	free (pmMIOPrinter -> buffer);
    	pmMIOPrinter -> buffer = NULL;
    }
    if (pmMIOPrinter -> title != NULL)
    {
    	free (pmMIOPrinter -> title);
    	pmMIOPrinter -> title = NULL;
    }
    if (pmMIOPrinter -> fontName != NULL)
    {
    	free (pmMIOPrinter -> fontName);
    	pmMIOPrinter -> fontName = NULL;
    }
    free (pmMIOPrinter);
} // MyFreeMIOFile


/************************************************************************/
/* MyGetIDSlot								*/
/************************************************************************/
static void	MyGetIDSlot (int pmIDNumber, int *pmIDType, int *pmIDSlot)
{
    int		myActualIDType, myActualIDSlot;

    if ((FONT_ID_BASE <= pmIDNumber) && 
    	(pmIDNumber < FONT_ID_BASE + MAX_IDS))
    {    	 
        myActualIDType = FONT_ID;
        myActualIDSlot = pmIDNumber - FONT_ID_BASE;
    }
    else if ((DIR_ID_BASE <= pmIDNumber) && 
    	(pmIDNumber < DIR_ID_BASE + MAX_IDS))
    {    	 
        myActualIDType = DIR_ID;
        myActualIDSlot = pmIDNumber - DIR_ID_BASE;
    }
    else if ((PIC_ID_BASE <= pmIDNumber) && 
    	(pmIDNumber < PIC_ID_BASE + MAX_IDS))
    {    	 
        myActualIDType = PIC_ID;
        myActualIDSlot = pmIDNumber - PIC_ID_BASE;
    }
    else if ((SPRITE_ID_BASE <= pmIDNumber) && 
    	(pmIDNumber < SPRITE_ID_BASE + MAX_IDS))
    {    	 
        myActualIDType = SPRITE_ID;
        myActualIDSlot = pmIDNumber - SPRITE_ID_BASE;
    }
    else if ((LEX_ID_BASE <= pmIDNumber) && 
    	(pmIDNumber < LEX_ID_BASE + MAX_IDS))
    {    	 
        myActualIDType = LEXER_ID;
        myActualIDSlot = pmIDNumber - LEX_ID_BASE;
    }
	else if ((HASHMAP_ID_BASE <= pmIDNumber) && 
    	(pmIDNumber < HASHMAP_ID_BASE + MAX_IDS))
    {    	 
        myActualIDType = HASHMAP_ID;
        myActualIDSlot = pmIDNumber - HASHMAP_ID_BASE;
    }
    else if (pmIDNumber == 0)
    {
    	myActualIDType = ZERO_ID;
    }
    else
    {
    	myActualIDType = UNKNOWN_ID;
    }

    if (pmIDType != NULL) *pmIDType = myActualIDType;
    if (pmIDSlot != NULL) *pmIDSlot = myActualIDSlot;
} // MyGetIDSlot


/************************************************************************/
/* MyGetSpecialFileType							*/
/************************************************************************/
static int	MyGetSpecialFileType (OOTstring pmFileName)
{
    int cnt;
    int myLength = strlen (pmFileName);

    for (cnt = 0; cnt < NUM_SPECIAL_FILE_TYPES; cnt++) 
    {
	int mySpecialFileTypeLength = strlen (stSpecialFileTypes [cnt]);

	if ((myLength > mySpecialFileTypeLength) && 
	    (strncmp (pmFileName, stSpecialFileTypes [cnt], 
	    				mySpecialFileTypeLength) == 0) &&
	    (pmFileName [myLength - 1] == ')'))
	{
	    return cnt;
	}
    }
    return -1;
} // MyGetSpecialFileType


/************************************************************************/
/* MyMakeMIOFILEFromFile						*/
/************************************************************************/
static MIOFILE	*MyMakeMIOFILEFromFile (void *pmFile, int pmFileType)
{
    static MIOFILE	myStMIOFile;

    MyWipeMIOFile (&myStMIOFile);
    
    myStMIOFile.filePtr = pmFile;
    myStMIOFile.fileType = pmFileType;
    
    return &myStMIOFile;
} // MyMakeMIOFILEFromFile


/************************************************************************/
/* MyMakeMIOFILEFromWindow						*/
/************************************************************************/
static MIOFILE	*MyMakeMIOFILEFromWindow (WIND pmWindow)
{
    static MIOFILE	myStMIOFile;

    MyWipeMIOFile (&myStMIOFile);
    
    myStMIOFile.windowPtr = (void *) pmWindow;
    
    switch (MIOWin_GetWindowType (pmWindow))
    {
    	case WINDOW_TYPE_MIO_TEXT:
	    myStMIOFile.windowType = WINDOW_KIND_TEXT;
    	    break;
    	case WINDOW_TYPE_MIO_GRAPHICS:
	    myStMIOFile.windowType = WINDOW_KIND_GRAPHICS;
    	    break;
	default:
	    // TW Assertion failure here
	    break;
    } // return	    	    
    
    return &myStMIOFile;
} // MyMakeMIOFILEFromWindow


/************************************************************************/
/* MyMoveToFrontOfRunWindowList						*/
/************************************************************************/
static void	MyMoveToFrontOfRunWindowList (WIND pmWindow)
{
} // MyMoveToFrontOfRunWindowList


/************************************************************************/
/* MySetRunWindowTitles							*/
/************************************************************************/
static void	MySetRunWindowTitles (void)
{
} // MySetRunWindowTitles

/************************************************************************/
/* MyWipeMIOFile							*/
/************************************************************************/
static void	MyWipeMIOFile (MIOFILE *pmMIOFile)
{
    memset (pmMIOFile, 0, sizeof (MIOFILE));
} // MyWipeMIOFile

