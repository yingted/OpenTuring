#include "include/cinterface"
typedef	TLnat4	TL_TL_priority_t;
struct	TL_TL_ExceptionInfo {
    TLint4	quitCode;
    TLint4	libraryQuitCode;
    TLstring	errorMsg;
};
typedef	TLnat1	__x404[1];
struct	TL_TL_HandlerArea {
    TLint4	quitCode;
    TLnat4	lineAndFile;
    TLaddressint	fileTable;
    struct TL_TL_HandlerArea	*nextHandler;
    __x404	savedState;
};
typedef	TLnat4	TL_TL_hardwarePriority_t;
typedef	TL_TL_hardwarePriority_t	TL_TL_lockStatus_t;
typedef	TLnat4	TL_TL_lock_t;
struct	TL_TL_link_t {
    struct TL_TL_ProcessDescriptor	*flink;
    struct TL_TL_ProcessDescriptor	*blink;
};
struct	TL_TL_ProcessDescriptor {
    TLnat4	lineAndFile;
    TLaddressint	fileTable;
    TLaddressint	stackLimit;
    TLaddressint	stackPointer;
    struct TL_TL_HandlerArea	*handlerQhead;
    struct TL_TL_HandlerArea	*currentHandler;
    TLaddressint	name;
    struct TL_TL_ExceptionInfo	exception;
    TLnat4	waitParameter;
    struct TL_TL_ProcessDescriptor	*monitorQlink;
    TLnat4	timeOutStatus;
    TLnat4	pid;
    TLaddressint	memoryBase;
    TLnat4	timeoutTime;
    TLnat4	timeoutEpoch;
    struct TL_TL_link_t	timeoutQ;
    TLboolean	timedOut;
    TLboolean	pausing;
    TL_TL_priority_t	dispatchPriority;
    struct TL_TL_ProcessDescriptor	*runQlink;
    TLboolean	ready;
    struct TL_TL_ProcessDescriptor	*tsyncWaiter;
    TLnat4	quantum;
    TLnat4	quantumCntr;
    TLnat4	devmonLevel;
    TLaddressint	otherInfo;
};

extern void TL_TLM_TLMUDUMP ();

extern void TL_TLB_TLBMAL ();

extern void TL_TLB_TLBMFR ();

extern void TL_TLB_TLBNWU ();

extern void TL_TLB_TLBFRU ();

extern void TL_TLI_TLIFS ();

extern void TL_TLI_TLIAON ();

extern void TL_TLI_TLIAOFF ();

extern void TL_TLI_TLIUDUMP ();

extern void TL_TLI_TLIFINI ();

extern void TL_TLE_TLELF ();
typedef	TLint4	TL_Cint;
extern TLboolean	TL_TLECU;
extern struct TL_TL_ProcessDescriptor	*TL_TLKPD;

void TL_TLQUIT ();

extern void TL_TLK_TLKINI ();

extern void TL_TLK_TLKFINI ();

extern void TL_TLK_TLKUXRS ();

extern void TL_TLK_TLKUXRE ();

extern void TL_TLK_TLKUEXIT ();

extern void TL_TLK_TLKUDMPP ();

extern void TL_TLK_TLKPFORK ();

extern TLboolean TL_TLK_TLKFRKED ();

extern void TL_TLK_TLKLKON ();

extern void TL_TLK_TLKLKOFF ();

extern void TL_TLK_TLKSSYNC ();

extern void TL_TLK_TLKSWAKE ();

extern void TL_TLK_TLKSTIMO ();

extern void TL_TLK_TLKCINI ();

extern TLnat4 TL_TLK_TLKPGETP ();

extern void TL_TLK_TLKPSETP ();

extern void TL_TLK_TLKIPINI ();

extern void TL_TLK_TLKIPENT ();

extern void TL_TLK_TLKIPEXT ();

extern TLboolean TL_TLK_TLKDMINI ();

extern void TL_TLK_TLKDMENT ();

extern void TL_TLK_TLKDMEXT ();

extern void TL_TLK_TLKPPAUS ();

extern void TL_TLX_TLXPID ();

extern void TL_TLX_TLXTIM ();

extern void TL_TLX_TLXATF ();

extern void TL_TLX_TLXDT ();

extern void TL_TLX_TLXTM ();

extern void TL_TLX_TLXCL ();

extern void TL_TLX_TLXSC ();

extern void TL_TLX_TLXGE ();

extern void TL_TLX_TLXSYS ();

extern void TL_TLA_TLAVES ();

extern void TL_TLA_TLAVFS ();

extern void TL_TLA_TLAVS8 ();

extern void TL_TLA_TLAVSI ();

extern void TL_TLA_TLAVSN ();
typedef	TLchar	TL_TLS___x408[3];

extern void TL_TLS_TLSBX ();
typedef	TLchar	TL_TLS___x409[3];

extern void TL_TLS_TLSBXX ();

extern void TL_TLS_TLSCXX ();
void TL_TLS_TLSMCC (target, __x416, source, __x417, dstIndex)
TLchar	target[];
TLint4	__x416;
TLchar	source[];
TLint4	__x417;
TLint4	dstIndex;
{
    register TLaddressint	dst;
    register TLaddressint	src;
    TLaddressint	lastAddr;
    if (dstIndex <= 0) {
	TL_TLQUIT("Starting index of \"strmove\" is less than 1", (TLint4) 40);
    };
    if (dstIndex > ((__x416 - __x417) + 1)) {
	TL_TLQUIT("Attempt to \"strmove\" past the char array length", (TLint4) 40);
    };
    dst = (TLaddressint) ((unsigned long)&(target[(dstIndex - 1)]));
    src = (TLaddressint) ((unsigned long)source);
    lastAddr = (TLaddressint) ((unsigned long)&(source[((__x417 + 0) - 1)]));
    for(;;) {
	if ((unsigned long) src > (unsigned long) lastAddr) {
	    break;
	};
	(* (TLchar *) dst) = (* (TLchar *) src);
	dst += 1;
	src += 1;
    };
}
