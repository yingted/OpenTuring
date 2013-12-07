
###
### Invocation command line was
### /usr/bin/winemaker .


SRCDIR                = .
SUBDIRS               = jpeg-6b \
			libungif
DLLS                  =
LIBS                  =
EXES                  = src.exe



### Common settings

#CEXTRA                = -mno-cygwin
CEXTRA                =
CXXEXTRA              = -mno-cygwin
RCEXTRA               =
DEFINES               = -DTPROLOG
INCLUDE_PATH          = -I. -I../../ready/src
DLL_PATH              =
DLL_IMPORTS           =
LIBRARY_PATH          =
LIBRARIES             =
OPTIONS = -I/usr/include/wine/windows -I/usr/include/wine/msvcrt


### src.exe sources and settings

src_exe_MODULE        = src.exe
src_exe_C_SRCS        = alloc.c \
			coder.c \
			comperr.c \
			compile.c \
			dbgalloc.c \
			debug.c \
			edint.c \
			edrun.c \
			error.c \
			event.c \
			execute.c \
			expn.c \
			fileio.c \
			fileman.c \
			graphics.c \
			hashmap.c \
			heap.c \
			kernel.c \
			language.c \
			limits.c \
			lists.c \
			lookup.c \
			math.c \
			mdio.c \
			mdioconfig.c \
			mdiodir.c \
			mdiodlg.c \
			mdiodraw.c \
			mdiofile.c \
			mdiofont.c \
			mdiogui.c \
			mdiojoy.c \
			mdiomusic.c \
			mdionet.c \
			mdiopc.c \
			mdiopic.c \
			mdiopicgif.c \
			mdiopicjpeg.c \
			mdiorgb.c \
			mdiosprite.c \
			mdiosys.c \
			mdiotime.c \
			mdioutil.c \
			mdiowin.c \
			mdiowindlg.c \
			mdiowingraph.c \
			mdiowintext.c \
			mdiowintop.c \
			memory.c \
			mio.c \
			mioconfig.c \
			miocrypto.c \
			miodir.c \
			miodlg.c \
			miodraw.c \
			mioerror.c \
			miofile.c \
			miofont.c \
			mioglgraph.c \
			miogui.c \
			miohashmap.c \
			miojoy.c \
			miolexer.c \
			miomouse.c \
			miomusic.c \
			mionet.c \
			miopc.c \
			miopic.c \
			miorgb.c \
			miosprite.c \
			miosys.c \
			miotext.c \
			miotime.c \
			mioview.c \
			miowin.c \
			miowindlg.c \
			miowindow.c \
			miowingraph.c \
			miowintext.c \
			miowrapper.c \
			mt.c \
			newmusic/mdiomusic.c \
			newmusic/miomusic.c \
			oexecute.c \
			orun.c \
			paraoot.c \
			parser.c \
			predef.c \
			prescan.c \
			run.c \
			scanner.c \
			scope.c \
			sdlgraph.c \
			star.c \
			string.c \
			symbol.c \
			system.c \
			tlavfs.c \
			tlavrs.c \
			tleabt.c \
			tledh.c \
			tlequit.c \
			tletr.c \
			tprolog.c \
			tprolog/error.c \
			tprolog/event.c \
			tprolog/execute.c \
			tprolog/graphics.c \
			tprolog/heap.c \
			tprolog/kernel.c \
			tprolog/language.c \
			tprolog/limits.c \
			tprolog/math.c \
			tprolog/mt.c \
			tprolog/run.c \
			tprolog/star.c \
			tprolog/string.c \
			tprolog/system.c \
			typedecl.c \
			types.c \
			unitman.c \
			util.c \
			value.c
src_exe_CXX_SRCS      =
src_exe_RC_SRCS       = mio.rc \
			tprolog.rc
src_exe_LDFLAGS       = -mwindows \
			-mno-cygwin
src_exe_ARFLAGS       =
src_exe_DLL_PATH      =
src_exe_DLLS          = odbc32 \
			ole32 \
			oleaut32 \
			winspool \
			odbccp32
src_exe_LIBRARY_PATH  =
src_exe_LIBRARIES     = uuid

src_exe_OBJS          = $(src_exe_C_SRCS:.c=.o) \
			$(src_exe_CXX_SRCS:.cpp=.o) \
			$(src_exe_RC_SRCS:.rc=.res)



### Global source lists

C_SRCS                = $(src_exe_C_SRCS)
CXX_SRCS              = $(src_exe_CXX_SRCS)
RC_SRCS               = $(src_exe_RC_SRCS)


###shit


ifeq ($(wildcard /usr/lib32),)
	LIB32=/usr/lib
else
	LIB32=/usr/lib32
endif
ifeq ($(wildcard $(LIB32)/libseccomp.a),)
	LIBSECCOMP=-I../../../libseccomp/include -lseccomp
else
	LIBSECCOMP=$(LIB32)/libseccomp.a #../../../libseccomp-libseccomp/src/libseccomp.a
endif
override CFLAGS += -m32
export CFLAGS

### Tools

#CC = winegcc
CC = gcc
CXX = wineg++
RC = wrc
AR = ar


### Generic targets

all: $(SUBDIRS) $(DLLS:%=%.so) $(LIBS) $(EXES)

### Build rules

.PHONY: all clean dummy tprolog-full.exe tprolog.exe tprolog.bin deps

$(SUBDIRS): dummy
	@cd $@ && $(MAKE)

# Implicit rules

.SUFFIXES: .cpp .cxx .rc .res
DEFINCL = $(INCLUDE_PATH) $(DEFINES) $(OPTIONS)
mio.o: mio.c
	$(CC) -c $(CFLAGS) $(CEXTRA) -I. -I../../ready/src -DTPROLOG -I/usr/include/wine/windows -o $@ $<

fileio.o : fileio.c
	$(CC) -c $(CFLAGS) -o $@ $<

.c.o:
	$(CC) -c $(CFLAGS) $(CEXTRA) $(DEFINCL) -o $@ $<

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(CXXEXTRA) $(DEFINCL) -o $@ $<

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(CXXEXTRA) $(DEFINCL) -o $@ $<

.rc.res:
	$(RC) $(RCFLAGS) $(RCEXTRA) $(DEFINCL) -fo$@ $<

# Rules for cleaning

CLEAN_FILES     = y.tab.c y.tab.h lex.yy.c core *.orig *.rej \
                  \\\#*\\\# *~ *% .\\\#*

clean:: $(SUBDIRS:%=%/__clean__) $(EXTRASUBDIRS:%=%/__clean__)
	$(RM) $(CLEAN_FILES) $(RC_SRCS:.rc=.res) $(C_SRCS:.c=.o) $(CXX_SRCS:.cpp=.o)
	$(RM) $(DLLS:%=%.so) $(LIBS) $(EXES) $(EXES:%=%.so)

$(SUBDIRS:%=%/__clean__): dummy
	cd `dirname $@` && $(MAKE) clean

$(EXTRASUBDIRS:%=%/__clean__): dummy
	-cd `dirname $@` && $(RM) $(CLEAN_FILES)

### Target specific build rules
DEFLIB = $(LIBRARY_PATH) $(LIBRARIES) $(DLL_PATH) $(DLL_IMPORTS:%=-l%)

$(src_exe_MODULE): $(src_exe_OBJS)
	$(CC) $(src_exe_LDFLAGS) -o $@ $(src_exe_OBJS) $(src_exe_LIBRARY_PATH) $(src_exe_DLL_PATH) $(DEFLIB) $(src_exe_DLLS:%=-l%) $(src_exe_LIBRARIES:%=-l%)

OBJS = fileio.o fileman.o lookup.o tleabt.o edint.o hashmap.o mio.o miowindow.o miotime.o mioerror.o miodir.o miohashmap.o miolexer.o miosys.o miowrapper.o mioconfig.o miocrypto.o miotext.o mioview.o miofile.o mdio.o language.o unitman.o error.o debug.o compile.o execute.o comperr.o memory.o scanner.o prescan.o scope.o value.o types.o symbol.o coder.o predef.o lists.o expn.o typedecl.o util.o parser.o alloc.o heap.o kernel.o string.o system.o math.o limits.o event.o mt.o star.o paraoot.o
DEPS =  ../../ready/src/edtext.o ../tlib/{TL,tl}*.o
deps: $(OBJS)
	$(MAKE) -kC ../tlib || :
	$(CFLAGS += -DCONSOLE)
	$(MAKE) -kC ../../ready/src || :

../bin/tprolog: tprolog.c deps
	$(CC) $(CFLAGS) -o $@ tprolog.c $(LIBSECCOMP) -w -I../../ready/src -I/usr/include/wine/windows $(DEPS) $(OBJS) $(LIB32)/wine/{shell32,version}.dll.so -Wl,--unresolved-symbols=ignore-in-object-files

../bin/tprologc: tprolog.c deps
	$(CC) $(CFLAGS) -o $@ tprolog.c -DTCC $(LIBSECCOMP) -w -I../../ready/src -I/usr/include/wine/windows $(DEPS) $(OBJS) $(LIB32)/wine/{shell32,version}.dll.so -Wl,--unresolved-symbols=ignore-in-object-files

#	gcc $(CEXTRA) $(DEFINCL) -I/usr/include/wine/windows -o tprolog.bin tprolog.c -w -I../../ready/src ../../ready/src/ed{file,text,fail}.o tprolog/{error,event,execute,heap,kernel,language,limits,math,mt,run,star,string,system}.o ../tlib/{TL,tl}*.o {fileman,lookup,tleabt,fileio,edint,hashmap,dbgalloc}.o mio{,win,window,time,error,dir,file,hashmap,lexer,sys,wintext,wrapper,config,crypto,text,view}.o mdio{,time,dir,file,sys,config}.o /usr/lib/wine/{shell32,msvcrt40,msvcrt,version}.dll.so libungif/liblibungif.a -Wl,--unresolved-symbols=ignore-in-object-files
#tprolog.exe:
#	winegcc $(CFLAGS) $(CEXTRA) $(DEFINCL) -o $@ tprolog.c -w -I../../ready/src ../../ready/src/ed{file,text,fail}.o tprolog/{error,event,execute,heap,kernel,language,limits,math,mt,run,star,string,system}.o ../tlib/{TL,tl}*.o {fileman,lookup,tleabt,fileio,edint,hashmap,dbgalloc}.o mio{,win,window,time,error,dir,file,hashmap,lexer,sys,wintext,wrapper,config,crypto,text,view}.o mdio{,time,dir,file,sys,config}.o -lshell32 -lmsvcrt40 -lmsvcrt libungif/liblibungif.a -lversion -Wl,--unresolved-symbols=ignore-in-object-files
#	$(CC) $(CFLAGS) $(CEXTRA) $(DEFINCL) -o $@ tprolog.c -w -I../../ready/src ../../ready/src/edtext.o tprolog/{error,event,execute,heap,kernel,language,limits,math,mt,run,star,string,system}.o ../tlib/{TL,tl}*.o {fileman,lookup,tleabt,fileio,edint,hashmap,dbgalloc}.o mio{,win,window,time,error,dir,file,hashmap,lexer,sys,wintext,wrapper,config,crypto,text,view}.o mdio{,time,dir,file,sys,config}.o -lshell32 -lmsvcrt40 -lmsvcrt libungif/liblibungif.a -lversion -Wl,--unresolved-symbols=ignore-in-object-files
#tprolog-full.exe:
#	$(CC) $(CFLAGS) $(CEXTRA) $(DEFINCL) -o $@ tprolog.c -w -I../../ready/src ../../ready/src/ed{file,gui,print,text,fail}.o tprolog/*.o ../tlib/{TL,tl}*.o {fileman,lookup,tleabt,fileio,edint,hashmap,dbgalloc}.o mio{,win,window,time,error,dir,file,font,hashmap,lexer,mouse,music,net,rgb,sprite,sys,windlg,wingraph,wintext,wrapper,config,crypto,dlg,draw,gui,joy,pc,text,view,glgraph,pic}.o mdio{,time,win,pic,util,wingraph,wintext,wintop,dir,file,font,music,net,rgb,sprite,sys,windlg,config,dlg,draw,joy,pc,picgif}.o -lgdiplus -lgdi32 -lwininet -lshell32 -lwinmm -lmsvcrt40 -lmsvcrt -lopengl32 libungif/liblibungif.a -lversion #-Wl,--unresolved-symbols=ignore-in-object-files
