.SUFFIXES:

include version.mk

#-------------------config--------------------------------
ifeq ($(wildcard $(PWD)/Makefile),)
PWD = $(shell pwd)
endif

OS = linux

# Store value of unmodified command line parameters.
ifdef MODE
  CMD_LINE_MODE = $MODE
endif

# Set default build mode
#   dbg = debug build
#   opt = release build
MODE = dbg

OUTDIR = bin-$(MODE)

ifeq ($(OS),linux)
  ARCH = $(shell gcc -dumpmachine | grep --only-matching x86_64)
endif

MAKEFLAGS += --no-print-directory

ifeq ($(MINIMAL_BUILD),1)
CFLAGS += -DMINIMAL_BUILD
CPPFLAGS += -DMINIMAL_BUILD
endif

CFLAGS += -DLINUX
CPPFLAGS += -DLINUX

# Common items, is not related to any browser.
IPC_TEST_CPPFLAGS += -DBROWSER_NONE=1
IPC_TEST_CFLAGS += -DBROWSER_NONE=1
IPC_TEST_CXXFLAGS += -DBROWSER_NONE=1

######################################################################
# OS == linux
######################################################################
ifeq ($(OS),linux)
CC = gcc
CXX = g++
OBJ_SUFFIX = .o
MKDEP = gcc -M -MF $(@D)/$(*F).pp -MT $@ $(CPPFLAGS) $<
ECHO=@echo

CPPFLAGS += -DLINUX
SQLITE_CFLAGS += -Wno-uninitialized -DHAVE_USLEEP=1
# for libjpeg:
THIRD_PARTY_CFLAGS = -Wno-main

# all the GTK headers using includes relative to this directory
#GTK_CFLAGS = -I../third_party/gtk/include/gtk-2.0 -I../third_party/gtk/include/atk-1.0 -I../third_party/gtk/include/glib-2.0 -I../third_party/gtk/include/pango-1.0 -I../third_party/gtk/include/cairo -I../third_party/gtk/lib/gtk-2.0/include -I../third_party/gtk/lib/glib-2.0/include
#CPPFLAGS += $(GTK_CFLAGS)

COMPILE_FLAGS_dbg = -g -O0
COMPILE_FLAGS_opt = -O2
COMPILE_FLAGS = -c -o $@ -fPIC -fmessage-length=0 -Wall -Werror $(COMPILE_FLAGS_$(MODE))
# NS_LITERAL_STRING does not work properly without this compiler option
COMPILE_FLAGS += -fshort-wchar

CFLAGS = $(COMPILE_FLAGS)
CXXFLAGS = $(COMPILE_FLAGS) -fno-exceptions -fno-rtti -Wno-non-virtual-dtor -Wno-ctor-dtor-privacy -funsigned-char -Wno-char-subscripts
NTES_CXXFLAGS = $(subst -fno-exceptions,,$(CXXFLAGS))

SHARED_LINKFLAGS = -o $@ -fPIC -Bsymbolic -pthread

MKLIB = ar
LIB_PREFIX = lib
LIB_SUFFIX = .a
LIBFLAGS = rcs $@

MKDLL = g++
DLL_PREFIX = lib
DLL_SUFFIX = .so
DLLFLAGS = $(SHARED_LINKFLAGS) -shared -Wl,--version-script -Wl,tools/xpcom-ld-script

MKEXE = g++
EXE_PREFIX =
EXE_SUFFIX =
EXEFLAGS = $(SHARED_LINKFLAGS)

endif

ifeq ($(MINIMAL_BUILD),1)
SPECIFIED_MAKEFILE = -f Makefile.minibuild
else
SPECIFIED_MAKEFILE=
endif #MINIMAL_BUILD=1

#-----------------------config end ---------------------------------------

#-----------------------------------------------------------------------------
# base/common (built for all browsers)

IPC_VPATH	+= \
		common \
		$(NULL)

IPC_CSRCS += \
		main.cc \
		message_queue_android.cc \
		message_queue.cc \
		stopwatch.cc \
		mutex.cc \
		mutex_posix.cc \
		thread.cc \
		thread_posix.cc \
		thread_locals.cc \
		event.cc \
		stopwatch_posix.cc \
    $(NULL)
		
#--------------rules------------------------------------
COMMON_OUTDIR       = $(OUTDIR)/$(OS)-$(ARCH)/common
SYMBOL_OUTDIR       = $(OUTDIR)/$(OS)-$(ARCH)/symbols
IPC_TEST_OUTDIR     = $(OUTDIR)

VPATH += $(IPC_VPATH)

SOURCECODE_SUFFIXES = c cc cpp m mm s
SUBSTITUTE_OBJ_SUFFIX = $(foreach SUFFIX,$(SOURCECODE_SUFFIXES), \
                           $(patsubst %.$(SUFFIX),$1/%$(OBJ_SUFFIX), \
                             $(filter %.$(SUFFIX), $2) \
                           ) \
                        )
												
IPC_OBJS            = $(call SUBSTITUTE_OBJ_SUFFIX, $(IPC_TEST_OUTDIR), $(IPC_CPPSRCS) $(IPC_CSRCS))

DEPS = \
	$(IPC_OBJS:$(OBJ_SUFFIX)=.pp)

IPC_TEST_EXE    = $(IPC_TEST_OUTDIR)/$(EXE_PREFIX)main$(EXE_SUFFIX)
INSTALLER_BASE_NAME = $(OS)-$(MODE)-$(VERSION)


default::
ifeq ($(OS),linux)
	$(MAKE)  prereqs    BROWSER=NONE
	$(MAKE)  modules    BROWSER=NONE
endif

$(IPC_TEST_OUTDIR):
	"mkdir" -p $@
	
$(COMMON_OUTDIR):
	"mkdir" -p $@

$(IPC_TEST_EXE): $(IPC_OBJS)
	$(MKEXE) $(EXEFLAGS) $(IPC_OBJS)

$(IPC_TEST_OUTDIR)/%$(OBJ_SUFFIX): %.c
	@$(MKDEP)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(IPC_TEST_CPPFLAGS) $(IPC_TEST_CFLAGS) $<

$(IPC_TEST_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(IPC_TEST_CPPFLAGS) $(IPC_TEST_CXXFLAGS) $<


prereqs:: $(IPC_TEST_OUTDIR) $(COMMON_OUTDIR)
modules:: $(IPC_TEST_EXE)

.PHONY : clean
clean::
	rm -rf bin-dbg
	rm -rf bin-opt

-include $(DEPS)
