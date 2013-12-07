/* tprolog.c */

/* System includes */
#include <unistd.h>
#include <seccomp.h>
#include <sys/mman.h>
#include <stdio.h>

// #define USE_SECCOMP

/* Self include */
#include "tprolog.h"

/* Other includes */
#include "tprologdialogs.h"

#include "edprog.h"

#include "edfail.h"
#include "edfile.h"
#include "edgui.h"
#include "edprint.h"

#include "language.h"
#include <windows.h>
#include "mio.h"

/* Macros */

/* Constants */

/* Global variables */
// The globals are constants for the entire program and include 
// registration information, etc.
Globals        gProgram;
// The user preferences.
Properties    gProperties;
BOOL         gFileManagerIgnoreFileOperation = FALSE;

/* Types */
typedef struct SrcPosition        SrcPosition;
typedef struct Language_ErrMsgs        *TuringErrorPtr;
typedef struct Language_RunStatus    RunStatus;

/* External variables */
extern void    TL (void);
extern void     Language_SetupExecutionFromObjectFile (OOTint objectFileStream, 
            unsigned char debug, unsigned long maxStackSize, 
            TLstring inName, TLstring outName, OOTargs args, 
            OOTint numArgs);
extern void    Language_GetFileName (unsigned short fileNo, TLstring str);
extern int    MIOTime_GetTicks (void);
extern int Exists(char* path);

/* Static constants */
// Just a random number used to identify this timer.
#define TURING_TIMER            512

#define MINIMUM_DELAY              1
#define MSECS_BETWEEN_EVENT_CHECK    100
#define DUMMY_WINDOW            "DummyWindow"

#define SYSEXIT_ERROR_STRING        "SysExit"
#define check(x,fmt,...) do{if(!(x)){fprintf(stderr,fmt"\n",__VA_ARGS__);return 0;}}while(0)

/* Static variables */
static FilePath        stStartupDirectory;
static FilePath        stApplicationPath, stApplicationDirectory;
static FilePath        stApplicationName;
static TuringErrorPtr    stErrorPtr;
// Redirection Arguments
static BOOL        stTuringProgramPaused = FALSE;
static BOOL        stTuringProgramRunning = TRUE;
static BOOL        stTuringProgramHalting = FALSE;
static BOOL        stQuittingEnvironment = FALSE;
static BOOL        stTuringProgramWaitingForEvent = FALSE;
static BOOL        stAllRunWindowsClosed = TRUE;
static RunStatus    stRunStatus;
static char        *stSizeMarker = OFFSET_STRING;
static UINT        stMinimumPeriod, stMaximumPeriod, stMinimumEventPeriod;
static HWND        stDummyWindow;

/* Static callback procedures */
static LRESULT CALLBACK    MyRaisedFrameWindowProcedure (HWND pmWindow, 
                              UINT pmMessage, 
                              WPARAM pmWParam, 
                              LPARAM pmLParam);
static UINT CALLBACK     MyRunWithArgsDialogProcedure (HWND pmWindow, 
                              UINT pmMessage, 
                                  WPARAM pmWParam, 
                                  LPARAM pmLParam);
static void CALLBACK     MyTimerProcedure (UINT pmID, UINT pmMsg, DWORD pmUser,
                      DWORD pmDummy1, DWORD pmDummy2);

/* Static procedures */
static BOOL    MyInitializeGlobals ();
static BOOL    MyInitializeWindowClass (void);
static BOOL MyInitializeRunFromByteCode(char *fileName);
static int    MyGetDirectoryFromPath (const char *pmPath, 
                             char *pmDirectory);
static void MyGetFilePathFromCmdLine(const char *cmdLine, unsigned int start, char *outFileName, char *outFilePath);

int main(int argc, char* argv[])
{
    int myStatus;
    OOTint    myNumErrors;
    int        myDelayWait;
    BOOL    myTuringProgramWaitingForEvent;

    FilePath myCurrentDirectory;
    //EdGUI_Init ();
    //EdPrint_Init ();

    #ifdef TCC
        check(argc==2||argc==3,"Usage: %s code.t [includedir]",argv[0]);
    #else
        check(argc==2||argc==3,"Usage: %s bytecode.tbc [includedir]",argv[0]);
    #endif

    TL ();
    FileManager ();
    Language ();
    if (!MyInitializeGlobals ()) {
        fprintf(stderr, "failed to initialize globals\n");
        return 1;
    }
        
    FileManager_SetHomeDir (stStartupDirectory);
    if (argc == 3) FileManager_SetDefaultInclude(argv[2]);

#ifdef TCC
    
    int len = strlen(argv[1]), i;
    char outputPath[len + 10];
    strcpy(outputPath, argv[1]);
    strcat(outputPath, "bc");
    return EdRun_CreateByteCodeFile(argv[1], outputPath);
        

#else

        if(!MyInitializeRunFromByteCode(argv[1]))
            return FALSE; // initialize failed

    // Initialize the MIO module
    MIO_Initialize (gProgram.applicationInstance, OS_WINDOWS, 
                stApplicationDirectory, stStartupDirectory,
                FALSE, FALSE,
            SYSEXIT_ERROR_STRING);
                
    // Get rid of the ".exe" at the end of a file name
    // Initialize MIO
    if (!MIO_Init_Run (argv[1], "-", 
                  FALSE,
                  "-",
                  FALSE,
                  FALSE,
                  myCurrentDirectory,        // Execution directory
                  FALSE,     // Graphics Mode
                  "monospace",     // Run window font name
                  12,             // Run window font size
                  0, // Run window font width
                  0, // Run window font options
                  0, // Run window dimensions
                  0, 0, // Run window width and height
                  gProperties.runConsoleTextRows,     // Run window rows
                  gProperties.runConsoleTextCols,     // Run window columns
                  gProperties.runConsoleFullScreen,
              (COLOR) RGB (0, 0, 132),
              FALSE,    // Allow/Forbid Sys.Exec
              FALSE,        // Allow/Forbid Music
              0,    // Set PP I/O Port
              FALSE))                 // Not a Test Suit Prog
    {
        return FALSE;
    }
    
    // Status message    
    stTuringProgramRunning = TRUE;
    stTuringProgramPaused = FALSE;
    stTuringProgramHalting = FALSE;
    stQuittingEnvironment = FALSE;
    
    // seccomp!!!
#define add(x,...) &&!(ret=seccomp_rule_add(ctx,SCMP_ACT_ALLOW,SCMP_SYS(x),__VA_ARGS__))
#ifdef USE_SECCOMP
    scmp_filter_ctx ctx;
    int ret;
    if((ctx=seccomp_init(SCMP_ACT_KILL))
       add(read,1,SCMP_A0(SCMP_CMP_EQ,STDIN_FILENO))
       add(write,1,SCMP_A0(SCMP_CMP_EQ,STDOUT_FILENO))
       add(write,1,SCMP_A0(SCMP_CMP_EQ,STDERR_FILENO))
       add(fstat64,1,SCMP_A0(SCMP_CMP_EQ,STDIN_FILENO))
       add(fstat64,1,SCMP_A0(SCMP_CMP_EQ,STDOUT_FILENO))
       add(fstat64,1,SCMP_A0(SCMP_CMP_EQ,STDERR_FILENO))
       add(mmap2,4,SCMP_A2(SCMP_CMP_EQ,PROT_READ|PROT_WRITE),SCMP_A3(SCMP_CMP_EQ,MAP_PRIVATE|MAP_ANONYMOUS),SCMP_A4(SCMP_CMP_EQ,-1),SCMP_A5(SCMP_CMP_EQ,(off_t)0))
       add(munmap,0)
       add(getpid,0)
       add(time,0)
       add(times,0)
       add(brk,0)
       add(exit_group,0)
       &&(ret=seccomp_load(ctx))<0) {
        fprintf(stderr,"error %d\n",ret);
        seccomp_release(ctx);
        return -ret;
    }
#endif
    do Language_ExecuteProgram (&stRunStatus, &stErrorPtr, &myNumErrors);
    while ((stRunStatus.state != Finished) && (!stTuringProgramHalting));

    // At this point, the program has either finished executing or the been
    // told to halt (permanently) either by the user closing a run window or
    // pressing stop, etc.

    // If the original window is no longer open, use the top editor window.
    MIO_Finalize_Run ();
    
    // Close all files opened by the running program
    Language_EndExecution ();
    
    stTuringProgramRunning = FALSE;
      
    if (stTuringProgramHalting && stQuittingEnvironment)
    {
        return 0;
    }

    if ((myNumErrors >= 1) &&
    (strncmp (stErrorPtr -> text, SYSEXIT_ERROR_STRING, 
          strlen (SYSEXIT_ERROR_STRING)) != 0))
    {
        FilePath    myErrorPathName;
        Language_GetFileName (stErrorPtr -> srcPos.fileNo, myErrorPathName);
	SrcPosition    *mySrc = &(stErrorPtr -> srcPos);
	fprintf (stderr,"Run time error on line %d [%d-%d] of %s: %s\n",
	    mySrc->lineNo, mySrc->linePos + 1,mySrc -> linePos + mySrc -> tokLen,
	    myErrorPathName, stErrorPtr -> text);
	return 1;
    }
#endif
#ifdef USE_SECCOMP
    seccomp_release(ctx);
#endif
    return 0;
} // WinMain


/* Emulated routines from other parts of the editor */
void FeedBack(const char *msg) {

}
/* Ed_GetLastActiveEditWindow                        */
HWND    Ed_GetLastActiveEditWindow (void)
{
    return NULL;
} // Ed_GetLastActiveEditWindow


/* Ed_SetModalDialog                            */
void    Ed_SetModalDialog (HWND pmWindow)
{
    // Do nothing
} // Ed_SetModalDialog


/* EdProp_GetStartupDirectory                        */
const char    *EdProp_GetStartupDirectory (void)
{
    return stStartupDirectory;
} // EdProp_GetStartupDirectory


/* EdRun_IsProgramPaused                        */
BOOL    EdRun_IsProgramPaused (void)
{
    return stTuringProgramPaused;
} // EdRun_IsProgramPaused


/* EdRun_IsProgramRunning                        */
BOOL    EdRun_IsProgramRunning (void)
{
    return stTuringProgramRunning;
} // EdRun_IsProgramRunning


/* EdRun_NotifyAllRunWindowsClosed                    */
/*                                    */
/* If we're in beginner mode and we've closed the run window, enlarge    */
/* the editor window.                            */
void    EdRun_NotifyAllRunWindowsClosed (void)
{
    stAllRunWindowsClosed = TRUE;
} // EdRun_NotifyAllRunWindowsClosed


/* EdRun_NotifyRunWindowOpened                        */
/*                                    */
/* If we're in beginner mode and we've opened our first run window,     */
/* minimize the editor window.                        */
void    EdRun_NotifyRunWindowOpened (HWND pmRunWindow)
{
    stAllRunWindowsClosed = FALSE;
} // EdRun_NotifyRunWindowOpened


/* EdRun_PauseResumeProgram                        */
void    EdRun_PauseResumeProgram (BOOL pmActivateSourceWindow, int pmReason)
{
    if (!stTuringProgramRunning)
    {
        // TW Assert Fail!
        return;
        
    }
    
    if (stTuringProgramPaused)
    {
        // We're paused, so let's resume
        stTuringProgramPaused = FALSE;

    // Notify the MIO module that the program is no longer paused.  Put
    // the Run windows on top if we are no longer pausing.  If we are
    // just stepping, then don't bother.
    MIO_NotifyTuringProgramResumed (TRUE);
    }
    else
    {
        // Let's pause the program
        stTuringProgramPaused = TRUE;

        // Notify the MIO module that the program is paused.
    MIO_NotifyTuringProgramPaused ();
    }
} // EdRun_PauseResumeProgram


/* EdRun_KillRunningProgram                        */
void    EdRun_KillRunningProgram (void)
{
    stTuringProgramHalting = TRUE;
} // EdRun_KillRunningProgram


/* EdRun_KillRunningProgramAndQuit                    */
/*                                    */
/* Returns: TRUE if the executor will post the quit message.        */
/*          FALSE if the caller is responsible for posting the message.    */
/*                                    */
/* This notifies the caller whether it should post the quit message, or    */
/* whether that will be done in the executor.  If a program is        */
/* currently running, the the exector will post the quit message once    */
/* it has stopped execution, closed its run windows, etc.  Otherwise    */
/* it's the responsibility of the caller to post the quit message.    */
BOOL    EdRun_KillRunningProgramAndQuit (void)
{
    stTuringProgramHalting = TRUE;
    stQuittingEnvironment = TRUE;

    return stTuringProgramRunning;
} // EdRun_KillRunningProgramAndQuit

static int	MyWriteByteCode (FileNoType pmProgramFileNo, long pmTuringFileDesciptor, 
			 BOOL pmCloseWindowsOnTerminate, 
			 BOOL pmDisplayRunWithArgs, BOOL pmCenterOutputWindow, 
			 BOOL pmStopUserClose)
{
	OOTint			myStatus = -1;
	TuringErrorPtr	myError;
	int			myErrors;
	//
    // First write the header.
    //
    TL_TLI_TLIWR (OBJECT_FILE_HEADER, sizeof (OBJECT_FILE_HEADER), &myStatus,
    		  pmTuringFileDesciptor);
    if (myStatus != 0)
    {
    	return 1;
    }

    //
    // Then write TProlog specific preferences      
    //
    TL_TLI_TLIWR (&pmCloseWindowsOnTerminate, sizeof (BOOL), &myStatus,
    		  pmTuringFileDesciptor);
    if (myStatus != 0)
    {
    	return 1;
    }
    TL_TLI_TLIWR (&pmDisplayRunWithArgs, sizeof (BOOL), &myStatus,
    		  pmTuringFileDesciptor);
    if (myStatus != 0)
    {
    	return 1;
    }
    TL_TLI_TLIWR (&pmCenterOutputWindow, sizeof (BOOL), &myStatus,
    		  pmTuringFileDesciptor);
    if (myStatus != 0)
    {
    	return 1;
    }
    TL_TLI_TLIWR (&pmStopUserClose, sizeof (BOOL), &myStatus,
    		  pmTuringFileDesciptor);
    if (myStatus != 0)
    {
    	return 1;
    }
    
    // 
    // Then write the environment preferences
    //
    TL_TLI_TLIWR (&gProperties, sizeof (Properties), &myStatus,
    		  pmTuringFileDesciptor);
    if (myStatus != 0)
    {
    	return 1;
    }
    
    //
    // Then write another header (just to make certain we're synced)
    //
    TL_TLI_TLIWR (OBJECT_FILE_HEADER, sizeof (OBJECT_FILE_HEADER), &myStatus,
    		  pmTuringFileDesciptor);
    if (myStatus != 0)
    {
        return 1;
    }

    // 
    // Then write the object files
    //
    Language_Reset ();
    Language_WriteObjectFile ("", pmProgramFileNo, &myError, &myErrors,
    	pmTuringFileDesciptor);

	return 0;
}

/************************************************************************/
/* EdRun_CreateByteCodeFile							*/
/* Creates a bytecode file to be read by the prolog with the -file option. */
/* Returns an error code */
/************************************************************************/
int	EdRun_CreateByteCodeFile (FilePath pmProgramPath,FilePath pmOutputPath)
{
    long		myTuringFileDesciptor;
	FileNoType	myProgramFileNumber;

	TextHandleType	myDummyTuringTextHandle;
    SizePtrType		myDummyTuringSizePtr;
    ResultCodeType	myResult;

    OOTint			myStatus = -1;
	FilePath mySourceDirectory;

	TuringErrorPtr	myError;   
	OOTint myErrors;

	// Make certain the test file exists

    if (!Exists(pmProgramPath))
    {
	// Test file not found
	fprintf(stderr, "file not found\n");
	return 1;
    }

    FileManager_OpenNamedHandle (pmProgramPath, &myProgramFileNumber, 
    				     &myDummyTuringTextHandle, &myDummyTuringSizePtr,
    				     &myResult);

/*    
    // Set the base source directory to be the directory 
    // in which the source file was located.
    EdFile_GetDirectoryFromPath (pmProgramPath, mySourceDirectory);
    FileManager_ChangeDirectory ((OOTstring) mySourceDirectory);
*/

    // Compile the program
    Language_CompileProgram ("", myProgramFileNumber, &myError, &myErrors);

	// Open the executable file
    TL_TLI_TLIOF (16, pmOutputPath, &myTuringFileDesciptor);
    
    if (myError != NULL)
    {
		int		myMessages = 0;
		char	msgBuffer[1024];
		
		// error header
		TL_TLI_TLIWR (OBJECT_FILE_ERROR_HEADER, sizeof (OBJECT_FILE_ERROR_HEADER), &myStatus,
    		  myTuringFileDesciptor);
		if (myStatus != 0)
		{
    		return 1;
		}

		while (myError != NULL)
		{
			WORD	myErrorTuringFileNo;
			FilePath	myErrorPathName;
			SrcPosition	*mySrc;
	    
			myErrorTuringFileNo = myError -> srcPos.fileNo;
			FileManager_FileName (myErrorTuringFileNo, myErrorPathName);
			mySrc = &(myError -> srcPos);
	    
			if (mySrc -> tokLen > 0)
			{
				sprintf (msgBuffer,"\n%d %d %d %s",
					mySrc -> lineNo, mySrc -> linePos + 1,
					mySrc -> linePos + 1 + mySrc -> tokLen, 
					myError -> text);
			}
			else
			{
				sprintf (msgBuffer, 
					"\r\nLine %d [%d] of (%s): %s",
					mySrc -> lineNo, mySrc -> linePos + 1,
					myErrorPathName, myError -> text);
			}

			TL_TLI_TLIWR (msgBuffer, strlen(msgBuffer), &myStatus,
    		  myTuringFileDesciptor);
			if (myStatus != 0)
			{
    			return 1;
			}

			myError = myError -> next;
			myMessages++;
		}

		// close the file before returning
		TL_TLI_TLICL (myTuringFileDesciptor);

		return myMessages;
	}

    
    if (myTuringFileDesciptor <= 0)
    {
        fprintf(stderr, "cannot create file\n");
        return 1;
    }

    myStatus = MyWriteByteCode(myProgramFileNumber,myTuringFileDesciptor,FALSE,FALSE,FALSE,FALSE);

    if (myStatus != 0)
    {
    	fprintf(stderr, "unable to write to file\n");
        return 1;
    }

    TL_TLI_TLICL (myTuringFileDesciptor);

    return 0; // no errors
} // EdRun_CreateByteCodeFile

#if 0
/* Static callback procedures */
/* MyDummyWindowProcedure                        */
/*                                    */
/* Callback routine for dummy window.                     */
static LRESULT CALLBACK    MyDummyWindowProcedure (HWND pmWindow, 
                        UINT pmMessage, 
                        WPARAM pmWParam, 
                        LPARAM pmLParam)
{
    return DefWindowProc (pmWindow, pmMessage, pmWParam, pmLParam);
} // MyDummyWindowProcedure


/* MyRaisedFrameWindowProcedure                        */
/*                                    */
/* Callback routine from the raised frame window.             */
static LRESULT CALLBACK    MyRaisedFrameWindowProcedure (HWND pmWindow, 
                              UINT pmMessage, 
                              WPARAM pmWParam, 
                              LPARAM pmLParam)
{
    HDC        myDeviceContext;
    PAINTSTRUCT    myPaintStruct;
    RECT    myWindowRect;
    
    if (pmMessage == WM_PAINT)
    {
        myDeviceContext = BeginPaint (pmWindow, &myPaintStruct);
        //SetMapMode (myDeviceContext, MM_TEXT);
        GetClientRect (pmWindow, &myWindowRect);
        DrawEdge (myDeviceContext, &myWindowRect, EDGE_RAISED, BF_RECT);
        EndPaint (pmWindow, &myPaintStruct);
        return 0;
    }
            
    return DefWindowProc (pmWindow, pmMessage, pmWParam, pmLParam);
} // MyRaisedFrameWindowProcedure


/* MyRunWithArgsDialogProcedure                        */
static UINT CALLBACK     MyRunWithArgsDialogProcedure (HWND pmDialog, 
                              UINT pmMessage, 
                                  WPARAM pmWParam, 
                                  LPARAM pmLParam)
{
    FilePath        myFileName;
    HWND        myItem;
    char        *myPtr;
    char        myQuote;
    int            myArgument;
    int            myArgPos;
    int            myStartPos, myEndPos;
    static RunArgs    *myStRunArgs;
    
    switch (pmMessage)
    {
        case WM_INITDIALOG:
            EdGUI_CentreDialogBox (pmDialog);
        myStRunArgs = (RunArgs *) pmLParam;
        // Set the command line arguments
        SetDlgItemText (pmDialog, ARGS_COMMAND_LINE_ARGS, 
            myStRunArgs -> commandLineArguments);
        // Select the appropriate input redirection checkbox
        myItem = GetDlgItem (pmDialog, ARGS_INPUT_FILE_NAME);
            CheckDlgButton (pmDialog, ARGS_IN_KEY, BST_UNCHECKED);
            CheckDlgButton (pmDialog, ARGS_IN_FILE, BST_UNCHECKED);
            CheckDlgButton (pmDialog, ARGS_IN_FILE_ECHO, BST_UNCHECKED);
            if (myStRunArgs -> inputRedirection == ARGS_IN_KEY)
            {
                CheckDlgButton (pmDialog, ARGS_IN_KEY, BST_CHECKED);
                ShowWindow (myItem, SW_HIDE);
            }
            else if (myStRunArgs -> inputRedirection == ARGS_IN_FILE)
            {
                CheckDlgButton (pmDialog, ARGS_IN_FILE, BST_CHECKED);
                wsprintf (myFileName, "File: %s", 
                    EdFile_GetFileName (myStRunArgs -> inputFile));
            SetDlgItemText (pmDialog, ARGS_INPUT_FILE_NAME, myFileName);
                ShowWindow (myItem, SW_SHOWNORMAL);
            }
            else
            {
                CheckDlgButton (pmDialog, ARGS_IN_FILE_ECHO, BST_CHECKED);
                wsprintf (myFileName, "File: %s", 
                    EdFile_GetFileName (myStRunArgs -> inputFile));
            SetDlgItemText (pmDialog, ARGS_INPUT_FILE_NAME, myFileName);
                ShowWindow (myItem, SW_SHOWNORMAL);
            }
        // Select the appropriate output redirection checkbox
        myItem = GetDlgItem (pmDialog, ARGS_OUTPUT_FILE_NAME);
            CheckDlgButton (pmDialog, ARGS_OUT_SCREEN, BST_UNCHECKED);
            CheckDlgButton (pmDialog, ARGS_OUT_FILE, BST_UNCHECKED);
            CheckDlgButton (pmDialog, ARGS_OUT_FILE_SCREEN, BST_UNCHECKED);
            CheckDlgButton (pmDialog, ARGS_OUT_PRINTER, BST_UNCHECKED);
            CheckDlgButton (pmDialog, ARGS_OUT_PRINTER_SCREEN, BST_UNCHECKED);
            if (myStRunArgs -> outputRedirection == ARGS_OUT_SCREEN)
            {
                CheckDlgButton (pmDialog, ARGS_OUT_SCREEN, BST_CHECKED);
                ShowWindow (myItem, SW_HIDE);
            }
            else if (myStRunArgs -> outputRedirection == ARGS_OUT_FILE)
            {
                CheckDlgButton (pmDialog, ARGS_OUT_FILE, BST_CHECKED);
                wsprintf (myFileName, "File: %s", 
                    EdFile_GetFileName (myStRunArgs -> outputFile));
            SetDlgItemText (pmDialog, ARGS_OUTPUT_FILE_NAME, myFileName);
                ShowWindow (myItem, SW_SHOWNORMAL);
            }
            else if (myStRunArgs -> outputRedirection == ARGS_OUT_FILE_SCREEN)
            {
                CheckDlgButton (pmDialog, ARGS_OUT_FILE_SCREEN, BST_CHECKED);
                wsprintf (myFileName, "File: %s", 
                    EdFile_GetFileName (myStRunArgs -> outputFile));
            SetDlgItemText (pmDialog, ARGS_OUTPUT_FILE_NAME, myFileName);
                ShowWindow (myItem, SW_SHOWNORMAL);
            }
            else if (myStRunArgs -> outputRedirection == ARGS_OUT_PRINTER)
            {
                CheckDlgButton (pmDialog, ARGS_OUT_PRINTER, BST_CHECKED);
                ShowWindow (myItem, SW_HIDE);
            }
            else
            {
                CheckDlgButton (pmDialog, ARGS_OUT_PRINTER_SCREEN, BST_CHECKED);
                ShowWindow (myItem, SW_HIDE);
            }
            return TRUE;
        case WM_COMMAND:
            switch (LOWORD (pmWParam))
            {
                case IDOK:
                    // Copy the contents of the controls to the run args struct
            myItem = GetDlgItem (pmDialog, ARGS_INPUT_FILE_NAME);
                    GetDlgItemText (pmDialog, ARGS_COMMAND_LINE_ARGS, 
                        myStRunArgs -> commandLineArguments, 
                        sizeof (myStRunArgs -> commandLineArguments));
                        
                    // Convert the run time arguments into individual arguments
                    myArgument = 0;
                    myArgPos = 0;
                    myPtr = myStRunArgs -> commandLineArguments;
                    while (TRUE)
                    {
                        // Skip over whitespace
                        while ((*myPtr == ' ') || (*myPtr == '\t'))
                        {
                            myPtr++;
                        }
                        
                        if (*myPtr == 0) break;
                        
                        myStartPos = myPtr - 
                            myStRunArgs -> commandLineArguments;
                    
                        myArgument++;
            myArgPos = 0;

                        // If we start with a quote, convert until the end quote
                        if ((*myPtr == '"') || (*myPtr == '\''))
                        {
                            myQuote = *myPtr;
                            myPtr++;
                            while ((*myPtr != myQuote) && (*myPtr != 0))
                            {
                                myStRunArgs -> ootArgs [myArgument][myArgPos++] 
                                                = *myPtr++;
                if (myArgPos == 255)
                {                    
                    EdGUI_Message1 (pmDialog, 
                        MB_ICONEXCLAMATION, 
                        IDS_TPROLOG_BAD_CMD_LINE_ARG_TITLE, 
                        IDS_TPROLOG_ARG_TOO_LONG);
                        myEndPos = myPtr - 
                            myStRunArgs -> commandLineArguments;
                    SetFocus (myItem);
                            SendMessage (myItem, EM_SETSEL, 
                                (WPARAM) myStartPos, (LPARAM) myEndPos);
                            return FALSE;
                }                                                
                            }
                            if (*myPtr == 0)
                            {
                EdGUI_Message1 (pmDialog, MB_ICONEXCLAMATION, 
                        IDS_TPROLOG_BAD_CMD_LINE_ARG_TITLE, 
                        IDS_TPROLOG_UNTERMINATED_QUOTE);
                    myEndPos = myPtr - 
                            myStRunArgs -> commandLineArguments;
                SetFocus (myItem);
                        SendMessage (myItem, EM_SETSEL, 
                            (WPARAM) myStartPos, (LPARAM) myEndPos);
                        return FALSE;
                            }
                            myPtr++;
                        }
                        else
                        {
                            while ((*myPtr != ' ') && (*myPtr != '\t') && 
                                   (*myPtr != 0))
                            {
                                myStRunArgs -> ootArgs [myArgument][myArgPos++] 
                                                = *myPtr++;
                if (myArgPos == 255)
                {
                    EdGUI_Message1 (pmDialog, 
                        MB_ICONEXCLAMATION, 
                        IDS_TPROLOG_BAD_CMD_LINE_ARG_TITLE, 
                        IDS_TPROLOG_ARG_TOO_LONG);
                        myEndPos = myPtr - 
                            myStRunArgs -> commandLineArguments;
                    SetFocus (myItem);
                            SendMessage (myItem, EM_SETSEL, 
                                (WPARAM) myStartPos, (LPARAM) myEndPos);
                            return FALSE;
                }                                                
                            } 
                        }
                    } // while (TRUE)
                    
                    myStRunArgs -> numArgs = myArgument;
                    
                    if (IsDlgButtonChecked (pmDialog, ARGS_IN_KEY) == 
                            BST_CHECKED)
                    {
                        myStRunArgs -> inputRedirection = ARGS_IN_KEY;
                    }
                    else if (IsDlgButtonChecked (pmDialog, ARGS_IN_FILE) == 
                            BST_CHECKED)
                    {
                        myStRunArgs -> inputRedirection = ARGS_IN_FILE;
                    }
                    else
                    {
                        myStRunArgs -> inputRedirection = ARGS_IN_FILE_ECHO;
                    }
                    if (IsDlgButtonChecked (pmDialog, ARGS_OUT_SCREEN) == 
                            BST_CHECKED)
                    {
                        myStRunArgs -> outputRedirection = ARGS_OUT_SCREEN;
                    }
                    else if (IsDlgButtonChecked (pmDialog, ARGS_OUT_FILE) == 
                            BST_CHECKED)
                    {
                        myStRunArgs -> outputRedirection = ARGS_OUT_FILE;
                    }
                    else if (IsDlgButtonChecked (pmDialog, 
                            ARGS_OUT_FILE_SCREEN) == BST_CHECKED)
                    {
                        myStRunArgs -> outputRedirection = ARGS_OUT_FILE_SCREEN;
                    }
                    else if (IsDlgButtonChecked (pmDialog, ARGS_OUT_PRINTER) == 
                            BST_CHECKED)
                    {
                        myStRunArgs -> outputRedirection = ARGS_OUT_PRINTER;
                    }
                    else
                    {
                        myStRunArgs -> outputRedirection = 
                            ARGS_OUT_PRINTER_SCREEN;
                    }
                    EndDialog (pmDialog, TRUE);
                    return TRUE;
                case IDCANCEL:
                    EndDialog (pmDialog, FALSE);
                    return TRUE;
        case ARGS_IN_KEY:
            myItem = GetDlgItem (pmDialog, ARGS_INPUT_FILE_NAME);
                    ShowWindow (myItem, SW_HIDE);
                    break;
        case ARGS_IN_FILE:
        case ARGS_IN_FILE_ECHO:
            myItem = GetDlgItem (pmDialog, ARGS_INPUT_FILE_NAME);
            strcpy (myFileName, myStRunArgs -> inputFile);
            if (!EdGUI_GetRedirectInputFile (pmDialog, myFileName))
            {
                CheckDlgButton (pmDialog, ARGS_IN_KEY, BST_CHECKED);
                    CheckDlgButton (pmDialog, ARGS_IN_FILE, BST_UNCHECKED);
                    CheckDlgButton (pmDialog, ARGS_IN_FILE_ECHO, 
                        BST_UNCHECKED);
                        ShowWindow (myItem, SW_HIDE);
                break;
            }
            strcpy (myStRunArgs -> inputFile, myFileName);
                    wsprintf (myFileName, "File: %s", 
                        EdFile_GetFileName (myStRunArgs -> inputFile));
                SetWindowText (myItem, myFileName);
                    ShowWindow (myItem, SW_SHOWNORMAL);
                    break;    
        case ARGS_OUT_SCREEN:
        case ARGS_OUT_PRINTER:
        case ARGS_OUT_PRINTER_SCREEN:
            myItem = GetDlgItem (pmDialog, ARGS_OUTPUT_FILE_NAME);
                    ShowWindow (myItem, SW_HIDE);
                    break;
        case ARGS_OUT_FILE:
        case ARGS_OUT_FILE_SCREEN:
            myItem = GetDlgItem (pmDialog, ARGS_OUTPUT_FILE_NAME);
            strcpy (myFileName, myStRunArgs -> outputFile);
            if (!EdGUI_GetRedirectOutputFile (pmDialog, myFileName))
            {
                    CheckDlgButton (pmDialog, ARGS_OUT_SCREEN, 
                        BST_CHECKED);
                    CheckDlgButton (pmDialog, ARGS_OUT_FILE, 
                        BST_UNCHECKED);
                    CheckDlgButton (pmDialog, ARGS_OUT_FILE_SCREEN, 
                        BST_UNCHECKED);
                    CheckDlgButton (pmDialog, ARGS_OUT_PRINTER, 
                        BST_UNCHECKED);
                    CheckDlgButton (pmDialog, ARGS_OUT_PRINTER_SCREEN, 
                        BST_UNCHECKED);
                        ShowWindow (myItem, SW_HIDE);
                break;
            }
            strcpy (myStRunArgs -> outputFile, myFileName);
                    wsprintf (myFileName, "File: %s", 
                        EdFile_GetFileName (myStRunArgs -> outputFile));
                SetWindowText (myItem, myFileName);
                    ShowWindow (myItem, SW_SHOWNORMAL);
                    break;    
            } // switch
            break;
    } // switch
    return FALSE;
} // MyRunWithArgsDialogProcedure


/* MyTimerProcedure                            */
static void CALLBACK     MyTimerProcedure (UINT pmID, UINT pmMsg, DWORD pmWindow,
                      DWORD pmDummy1, DWORD pmDummy2)
{
    PostMessage ((HWND) pmWindow, WM_TIMER, TURING_TIMER, (LPARAM) NULL);
} // MyTimerProcedure
#endif

/* MyInitializeGlobals                            */
static BOOL    MyInitializeGlobals ()
{
    char    *myPtr;
    SYSTEMTIME    mySystemTime;
    DWORD    myDummy;
    DWORD    myVersionSize;
    char    *myVersionData, *myVersionInfo;
    UINT    myVersionInfoSize;
    check(getcwd(stApplicationPath,PATHNAME_LENGTH),"getcwd failed",'\0');

    strcpy (stApplicationDirectory, stApplicationPath);
    myPtr = strrchr (stApplicationDirectory, '/');
    *myPtr = 0;
    strcpy (stApplicationName, myPtr + 1);

    // If we're at a top level directory, then we need to restore the 
    // last slash.
    if (stApplicationDirectory [2] == 0)
    {
    stApplicationDirectory [2] = '/';
    stApplicationDirectory [3] = 0;
    }
    
    // Initialize the current (startup) directory
    //EdFile_GetCurrentDirectory (stStartupDirectory);

    gProgram.applicationInstance = NULL;
    gProgram.globalsInitialized = TRUE;
    gProgram.isTuring = TRUE;
    gProgram.isJava = FALSE;
    gProgram.isCpp = FALSE;
    
    gProgram.language = LANGUAGE_KIND_TURING;
    //EdGUI_LoadString (IDS_TPROLOG_TURING_PROLOG_NAME,
    //gProgram.environmentName, sizeof (gProgram.environmentName));
    gProgram.installKind = INSTALL_KIND_OPEN;
    gProgram.miniVersion = FALSE;
    gProgram.restrictedVersion = FALSE;
    gProgram.assistedByIBM = FALSE;
#if 0
    gProgram.expiryDateString [0] = 0;
    ZeroMemory (&mySystemTime, sizeof (SYSTEMTIME));
    mySystemTime.wYear = 2100;
    mySystemTime.wMonth = 1;
    mySystemTime.wDay = 1;
    if (!SystemTimeToFileTime (&mySystemTime, &gProgram.expiryDate))
    {
        EdFail_Warn (IDS_TPROLOG_SYSTEMTIMETOFILETIMEFAIL, __FILE__, __LINE__, 
                 GetLastError ());
        return FALSE;
    }
    gProgram.licensedTo [0] = 0;
#endif 
    strcpy (gProgram.versionNumber, "Unknown");
#if 0    
    // Get the applicaton name
    myVersionSize = GetFileVersionInfoSize (stApplicationPath, &myDummy);
    if (myVersionSize != 0)
    {
    myVersionData = malloc (myVersionSize);
    if (myVersionData != NULL)
    {
        if (GetFileVersionInfo (stApplicationPath, myDummy, 
                               myVersionSize, myVersionData))
            {
        if (VerQueryValue (myVersionData, 
                "\\StringFileInfo\\04090000\\ProductVersion", 
                &myVersionInfo, &myVersionInfoSize))
        {
            strncpy (gProgram.versionNumber, myVersionInfo, 
                     myVersionInfoSize);
            gProgram.versionNumber [myVersionInfoSize] = 0;
        }
            }
        free (myVersionData);
        }
    }
#endif 
    
    return TRUE;
} // MyInitializeGlobals
#if 0
static void MyGetFilePathFromCmdLine(const char *cmdLine, unsigned int start, char *outFileName, char *outFilePath)
{
    if (strrchr (cmdLine, '\\') == NULL)
    {
        strcpy (outFileName, &cmdLine [start]);
    }
    else
    {
        strcpy (outFileName, strrchr (cmdLine, '\\') + 1);
    }

    // is the path quoted? Then skip the quotes.
    if (cmdLine [start] == '"') {
        // on the file name
        outFileName[strlen(outFileName) - 1] = 0;
        strcpy (outFilePath, &cmdLine [start + 1]);
        // make sure to remove closing quote on path too
        outFilePath[strlen(outFilePath) - 1] = 0;
    } else {
        strcpy (outFilePath, &cmdLine [start]);
    }

    // if the file name ends with .tbc, replace it with the original
    if(strcmp(&outFileName[strlen(outFileName) - 4],".tbc") == 0) {
        // .tbc minus 2 chars = .t
        outFileName[strlen(outFileName) - 2] = 0;
    }
}

/* MyGetDirectoryFromPath                        */
static int    MyGetDirectoryFromPath (const char *pmPath, char *pmDirectory)
{
    char    *myPtr;
    
    strcpy (pmDirectory, pmPath);
    myPtr = strrchr (pmDirectory, '\\');
    if (myPtr == NULL)
    {
        // ERROR! No backslash in pathname!
        return 1;
    }
    myPtr++;
    *myPtr = 0;
    return 0;
} // EdFile_GetDirectoryFromPath

/* MyInitializeWindowClass                        */
static BOOL    MyInitializeWindowClass (void)
{
    char    myRaisedFrameWindowClassName [256];
    int        myResult;
    WNDCLASSEX    myRaisedFrameClass, myDummyClass;

    // Get the class name
    EdGUI_LoadString (IDS_MIO_RAISED_FRAME_WINDOW_NAME, 
        myRaisedFrameWindowClassName, 
    sizeof (myRaisedFrameWindowClassName));
             
    /* Register the raised window class */
    myRaisedFrameClass.cbSize =        sizeof (myRaisedFrameClass);
    // Set window class to redraw when window size changes
    myRaisedFrameClass.style =           CS_HREDRAW | CS_VREDRAW;
    // Procedure to be called with messages for this window class
    myRaisedFrameClass.lpfnWndProc =   MyRaisedFrameWindowProcedure;
    // The extra space in class struct
    myRaisedFrameClass.cbClsExtra =    0;
    // The extra space in window struct
    myRaisedFrameClass.cbWndExtra =    0;
    // The application's handle
    myRaisedFrameClass.hInstance =     gProgram.applicationInstance;
    // Set the icon for this window class
    myRaisedFrameClass.hIcon =           NULL;
    // Set the cursor for this window class
    myRaisedFrameClass.hCursor =       LoadCursor (NULL, IDC_ARROW);
    // Set the background colour for this window
    myRaisedFrameClass.hbrBackground = GetSysColorBrush (COLOR_BTNFACE);
    // Set the menu for this window class
    myRaisedFrameClass.lpszMenuName =  NULL;
    // Name of the window class
    myRaisedFrameClass.lpszClassName = myRaisedFrameWindowClassName; 
    // Set the icon for this class.
    myRaisedFrameClass.hIconSm =       NULL;
    
    myResult = RegisterClassEx (&myRaisedFrameClass);
    if (myResult == 0)
    {
        EdFail_Warn (IDS_TPROLOG_REGISTERCLASSFAIL, __FILE__, __LINE__, 
                 GetLastError ());
        return FALSE;
    }
    
    /* Register the raised window class */
    myDummyClass.cbSize =        sizeof (myDummyClass);
    // Set window class to redraw when window size changes
    myDummyClass.style =     CS_HREDRAW | CS_VREDRAW;
    // Procedure to be called with messages for this window class
    myDummyClass.lpfnWndProc =   MyDummyWindowProcedure;
    // The extra space in class struct
    myDummyClass.cbClsExtra =    0;
    // The extra space in window struct
    myDummyClass.cbWndExtra =    0;
    // The application's handle
    myDummyClass.hInstance =     gProgram.applicationInstance;
    // Set the icon for this window class
    myDummyClass.hIcon =     NULL;
    // Set the cursor for this window class
    myDummyClass.hCursor =       NULL;
    // Set the background colour for this window
    myDummyClass.hbrBackground = NULL;
    // Set the menu for this window class
    myDummyClass.lpszMenuName =  NULL;
    // Name of the window class
    myDummyClass.lpszClassName = DUMMY_WINDOW; 
    // Set the icon for this class.
    myDummyClass.hIconSm =       NULL;
    
    myResult = RegisterClassEx (&myDummyClass);
    if (myResult == 0)
    {
        EdFail_Warn (IDS_TPROLOG_REGISTERCLASSFAIL, __FILE__, __LINE__, 
                 GetLastError ());
        return FALSE;
    }
    
    return TRUE;
} // MyInitializeWindowClass
#endif
static BOOL MyInitializeRunFromByteCode(char *fileName)
{
    long    myTuringFileDesciptor;
    int        myStatus;
    char    myObjectFileHeader [6];
    
    // Open the executable file
    TL_TLI_TLIOF (9, fileName, &myTuringFileDesciptor);
    check(myTuringFileDesciptor>0,"Unable to open object file %s",fileName);

#if 0 
    // If we're reading from the executable, skip the header
    if (!useSeparateFile)
    {
        if (strcmp (stSizeMarker, OFFSET_STRING) == 0)
        {
            EdGUI_Message1 (NULL, 0, IDS_TPROLOG_APPLICATION_NAME,
                        IDS_TPROLOG_OFFSET_NOT_SET);
            return 0;
        }
        TL_TLI_TLISK (* (int *) stSizeMarker, myTuringFileDesciptor);
    } 
#endif

    // First read the header.
    TL_TLI_TLIRE (myObjectFileHeader, sizeof (OBJECT_FILE_HEADER), &myStatus,
              myTuringFileDesciptor);
    check(!myStatus,"Unable to read object file %s. (%d)",fileName,1);
    myObjectFileHeader [sizeof (OBJECT_FILE_HEADER)-1] = 0;

    check(strcmp (myObjectFileHeader, OBJECT_FILE_ERROR_HEADER),"The file %s did not compile correctly.",fileName);
    check(!strcmp (myObjectFileHeader, OBJECT_FILE_HEADER),"Bad object file header in %s. (%d)",fileName,1);
    BOOL trash;
    // Then read TProlog specific preferences      
    TL_TLI_TLIRE (&trash, sizeof (BOOL), &myStatus, myTuringFileDesciptor);
    check(!myStatus,"Unable to read object file %s. (%d)",fileName,2);
    TL_TLI_TLIRE (&trash, sizeof (BOOL), &myStatus, myTuringFileDesciptor);
    check(!myStatus,"Unable to read object file %s. (%d)",fileName,3);
    TL_TLI_TLIRE (&trash, sizeof (BOOL), &myStatus, myTuringFileDesciptor);
    check(!myStatus,"Unable to read object file %s. (%d)",fileName,4);
    TL_TLI_TLIRE (&trash, sizeof (BOOL), &myStatus, myTuringFileDesciptor);
    check(!myStatus,"Unable to read object file %s. (%d)",fileName,5);
    
    // Then read the environment preferences
    TL_TLI_TLIRE (&gProperties, sizeof (Properties), &myStatus,
              myTuringFileDesciptor);
    check(!myStatus,"Unable to read object file %s. (%d)",fileName,6);
    
    // Then read another header (just to make certain we're synced)
    TL_TLI_TLIRE (myObjectFileHeader, sizeof (OBJECT_FILE_HEADER), &myStatus,
              myTuringFileDesciptor);
    check(!myStatus,"Unable to read object file %s. (%d)",fileName,7);
    myObjectFileHeader [sizeof (OBJECT_FILE_HEADER)-1] = 0;
    check(!strcmp (myObjectFileHeader, OBJECT_FILE_HEADER),"Bad object file header in %s. (%d)",fileName,2);
    // Then read the object files
    Language_SetupExecutionFromObjectFile (myTuringFileDesciptor, 0, 
        8192, "", "", (char*){""}, 1);//...,argv,argc
   
    TL_TLI_TLICL (myTuringFileDesciptor);
    return TRUE;
}
void Language_Execute_Graphics(){}
void Language_Execute_System(){}
void MDIOWin_Init(){}
void MIOWinText_Init(){}
void MIOWinGraph_Init(){}
void MIOWinDlg_Init(){}
void MDIOWinTop_Init(BOOL pmStopUserClose){}
void MIOWin_Init(BOOL pmCenterOutputWindow,BOOL pmStopUserClose){}
void MIOWin_Finalize(){}
void MIOWin_PropertiesSet(MIOWin_Properties pmProperties){}
void MIOWin_PropertiesImplementChanges(WIND pmWindow){}
void MIOWin_AddText(WIND pmWindow,char *pmBuffer,int pmFlag){}
void MIOWin_AssertInnerWindow(WIND pmWindow){}
void MIOWin_AssertOuterWindow(WIND pmWindow){}
void MIOWin_AssertWindowType(WIND pmWindow,int pmType){}
void MIOWin_CalculateInnerWindowSize(WindowAttribPtr pmWindowAttribs,int *pmMinWindowWidth, int *pmMinWindowHeight,int *pmWidth, int *pmHeight,int *pmGraphicsRunWindowWidth, int *pmGraphicsRunWindowHeight,int *pmMaxWindowWidth, int *pmMaxWindowHeight){}
void MIOWin_CalculateWindowDisplacement(WIND pmWindow,WindowAttribPtr pmWindowAttribs){}
void MIOWin_CalculateWindowPosition(WIND pmWindow,int pmXPos,int pmYPos,int *pmWindowLeft, int *pmWindowTop){}
void MIOWin_CalculateWindowSize(WIND pmWindow,WindowAttribPtr pmWindowAttribs){}
void MIOWin_CaretDisplay(WIND pmInnerOrOuterWindow){}
void MIOWin_CaretHide(WIND pmInnerOrOuterWindow){}
void MIOWin_CaretMove(WIND pmInnerOrOuterWindow){}
void MIOWin_CloseWindow(WIND pmWindow){}
void MIOWin_Dispose(WIND pmWindow){}
void MIOWin_InitRun(){}
void MIOWin_KeystrokeHandler(WIND pmWindow,UCHAR pmKeystroke){}
void MIOWin_OutputText(WIND pmWindow,char *pmBuffer){}
void MIOWin_SaveWindowToFile(WIND pmWindow,const char *pmPathName){}
void MIOWin_SetRunWindowTitle(WIND pmWindow){}
void MIOWin_UngetCharacter(OOTint pmChar,WIND pmWindow){}
