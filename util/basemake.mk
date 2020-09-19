#!/usr/bin/make -f
# -*- Makefile -*-

#######################################################################
#  I. Establish operating system specific variables
#######################################################################

ifndef MK_VARIABLES_ONCE
MK_VARIABLES_ONCE := 1
ifneq ($(filter debug prof,$(MAKECMDGOALS)),)
DEBUG := 1

ifeq ($(filter-out debug,$(MAKECMDGOALS)),)
debug: build
endif

ifeq ($(filter-out prof,$(MAKECMDGOALS)),)
prof: build
MK_PROF := 1
endif

ifneq ($(filter test,$(MAKECMDGOALS)),)
test: build
endif
endif

# create the "standard" copy location if MK_BIN_DIR is defined
ifdef MK_BIN_DIR
MK_TEMP := $(if $(wildcard $(MK_BIN_DIR)),,$(shell mkdir $(MK_BIN_DIR)))
endif

.SUFFIXES:
# .SUFFIXES: .c .cpp .h .d

ifeq ($(OS),Windows_NT)
ifneq ($(MK_NO_WINDOWS),)
$(error Build not appropriate for Windows platform)
endif

# Windows extensions
O := obj
B := exe
E := exe
A := lib
L := lib
S := dll
D := dll

CC := cl
CXX := cl
CPP := cl
AR  = lib /OUT:$@
RANLIB := echo

MK_INCLUDES := -I$(subst /XxXx,,$(dir $M)XxXx)
MK_LIBS :=
DEBUG_CPPFLAGS = /ZI /Gm /D "_DEBUG"
# removed /MLd from above
DEBUG_LDFLAGS = /incremental:yes /debug /pdbtype:sept

# .SUFFIXES: .obj .exe .lib .dll

# this is a "function" to translate file name into dependency lists per .d files
GET_DEPS = $(foreach file, $($(1)), $($(file).DEPS))
#GET_DEPS2 = $(foreach file, $($(1)), $($(addprefix $(MS)/,$(file)).DEPS))

#GET_DEPS2 = $(addprefix $(MO)/,$(addsuffix .d,$(basename $(filter %.cpp %.c,$(wildcard $($(1)))))))
GET_DEPS2 = $(addprefix $(MO)/,$(addsuffix .d,$(basename $(filter %.cpp %.c,$(notdir $(wildcard $($(1)))))))) $($(1))

ifdef DEBUG
WIN_DEFAULT :=Debug
else
WIN_DEFAULT :=Release
endif

else

# Linux extensions
O := o
B := e
E := e
A := a
L := a
S := so
D := so

CC := gcc
CXX := g++
CPP := gcc
## Sept 15, 2020 was "ar cru", and -s option would eliminate need for ranlib
AR  = ar crD $@
RANLIB := ranlib

#MK_INCLUDES := -I$(subst /XxXx,,$(dir $M)XxXx)
MK_LIBS :=
#MK_LIBS := -L/usr/lib/gcc-lib/i386-redhat-linux/3.2.3/
DEBUG_CPPFLAGS := -D_DEBUG
#DEBUG_LDFLAGS := -ggdb

.SUFFIXES:
#.SUFFIXES: .o .a .s

# this is a "function" to translate file name into dependency lists per .d files
GET_DEPS = $(addsuffix .$O,$(basename $($(1))))
GET_DEPS2 = $(addsuffix .$O,$(basename $(addprefix $(MO)/,$(notdir $($(1))))))
GET_DEPS3 = $(addprefix $(MS)/,$($(1)))

#GET_DEPS = $($1)


endif
#end of MK_VARIABLES_ONCE
endif

#######################################################################
#  II. Before loading other make modules, establish what this make contributes
#       in the way of libraries (archives)
#######################################################################

ALL_ARCS_PUBLISHED += $($M/ARCS_PUBLISHED)
ifeq ($(strip $($M/ARCS_ARE_SO)),1)
$M/ARCS_PUBLISHED := $(addsuffix .$S,$($M/ARCS_PUBLISHED))
else
$M/ARCS_PUBLISHED := $(addsuffix .$A,$($M/ARCS_PUBLISHED))
endif
$M/ARCS_PUBLISHED := $(addprefix ./,$($M/ARCS_PUBLISHED))


$M/BINS_PUBLISHED := $(addprefix ./,$($M/BINS_PUBLISHED))
$M/BINS_PUBLISHED := $(addsuffix .$E,$($M/BINS_PUBLISHED))

#######################################################################
#  III. Establish the root module of this subtree.
#      That will allow the path prefix to be established.
#######################################################################

MODULES_LOADED := $(MODULES_LOADED) $M

ifndef PREFIX_ESTABLISHED

# set base before recursive loading begins
ifndef MK_START
MK_START := $M
ALL_MODULES :=
endif

ifneq ($(strip $($M/A_ROOT_MODULE)),1)

# start recursive loading of lower level makes until
#  root is found.
NEXT_M_STACK := $M $(NEXT_M_STACK)

M := $(subst /XxXx,,$(dir $M)XxXx)
NEXT_M_STACK := $M $(NEXT_M_STACK)
include $M/module.mk

# pop
M := $(firstword $(NEXT_M_STACK))
NEXT_M_STACK := $(wordlist 2, 20, $(NEXT_M_STACK))

else

P := $(dir $M)
MK_ROOT := $M

endif

PREFIX_ESTABLISHED := 1

## generic stuff that just needs to be defined once
.PHONY: clean allclean debug tar


endif

#######################################################################
# IV. Set up default compiler flags for module and global
#######################################################################

ifeq ($(OS),Windows_NT)

$M/CFLAGS   = /nologo $(GCFLAGS)
$M/CPPFLAGS = /D "WIN32" /D "_CONSOLE" /D "_MBCS" $(GCPPFLAGS)
$M/CXXFLAGS = /nologo /W3 /GX /Od /GZ $(GCXXFLAGS)
ifndef DEBUG
#$M/CXXFLAGS += /ML
endif

$M/DEPS = /P

$M/LDFLAGS  = /link /nologo /subsystem:console /machine:I386 /pdb:"$(subst .$B,.pdb,$@)" /incremental:no $(GLDFLAGS)
$M/LDLIBS   += kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib $(GLDLIBS)
$M/OUTPUT_OPTION = /FD /Fo"$(MO)/" /Fe"$(MB)/" /Fd"$(basename $@)"

else

#20071004, matthewv: moved to -O0 from -O2 due to link errors caused after adding
#  CONFIG_SMP for atomic.h
##20080409, matthewv: -Wno-unreachable-code added due to rash of unexplained warnings with
#  constructors ... gcc 4.2
#linux stuff
#  -Wuninitialized
ifndef DEBUG
$M/CFLAGS = -O2 -pipe -fomit-frame-pointer -finline-functions -Wall -Werror -fms-extensions
# now causes a warning $M/CPPFLAGS = -I/usr/local/include
$M/CPPFLAGS =
$M/CXXFLAGS = -O2 -pipe -fomit-frame-pointer -finline-functions -fPIC -Werror
else
$M/CFLAGS = -ggdb3 -O0 -Wall -fms-extensions -Werror

ifdef MK_PROF
$M/CFLAGS += -pg
endif

# now causes a warning $M/CPPFLAGS = -I/usr/local/include
$M/CPPFLAGS =
$M/CXXFLAGS = -ggdb3 -O0 -Wall -fms-extensions -Werror

ifdef MK_PROF
$M/CXXFLAGS += -pg
endif

endif

$M/DEPS = -M | sed -e 's@ /usr/i[^ ]*@@g' -e 's@^\(.*\).o:@$(MO)/\1.d $(MO)/\1.o:@' > $@

$M/LDFLAGS =
$M/LDLIBS += -L/usr/lib $(GLDLIBS)
# working linux , not OSX $M/LDLIBS += -L/usr/lib -L/lib  -lnsl -lrt -lcrypt $(GLDLIBS)
#$M/LDLIBS += -L/usr/lib -L/lib  -lnsl -lstdc++ -lrt -lcrypt $(GLDLIBS)
#$M/LDLIBS += -L/usr/lib -L/lib -L/usr/lib/gcc-lib/i386-redhat-linux/3.2.3/ -lnsl -lstdc++ -lrt -lcrypt $(GLDLIBS)

ifdef MK_PROF
$M/LDFLAGS += -pg
endif

ifeq ($(strip $($M/ARCS_ARE_SO)),1)
#$M/LDFLAGS += -x -shared
#  -- static flag to help bugler get along with fewer dependencies
$M/LDFLAGS += -static
endif

$M/CPPFLAGS += $(GCPPFLAGS)
$M/OUTPUT_OPTION = -o $@
endif


## globals, ensure the definition only happens once

ifdef DEBUG
ifndef DEBUG_DONE
GCPPFLAGS += $(DEBUG_CPPFLAGS)
GLDFLAGS += $(DEBUG_LDFLAGS)
DEBUG_DONE:=1
endif
endif

ifdef MK_THREADS
ifndef MK_THREADS_DONE
ifneq ($(OS),Windows_NT)
GCPPFLAGS += -D_POSIX_PTHREAD_SEMANTICS -D_REENTRANT
GLDLIBS += -lpthread
else
ifdef DEBUG
GCPPFLAGS += /MTd
else
GCPPFLAGS += /MT
endif
endif
MK_THREADS_DONE :=1
endif
endif

ifdef RPM_PACKAGE_RELEASE
ifndef MK_RPM_DONE
ifneq ($(OS),Windows_NT)
GCPPFLAGS += -DRPM_PACKAGE_RELEASE="\"$(RPM_PACKAGE_RELEASE)\""
else
GCPPFLAGS += /D RPM_PACKAGE_RELEASE="$(RPM_PACKAGE_RELEASE)"
endif
MK_RPM_DONE := 1
endif
endif


## custom Nuview target directory for binaries and libraries
#ifeq (1,0)
ifndef MK_MISC_DONE
ifneq ($(OS),Windows_NT)

UNIX_PROCESSOR :=$(shell uname -p)
UNIX_MACHINE :=$(shell uname -m)
UNIX_DEFAULT :=_mvUNKNOWN_ARCH

ifeq ($(UNIX_PROCESSOR),sparc)
UNIX_DEFAULT :=sun4
#UNIX_DEFAULT2 :=$PUNIX/dev/sun4
UNIX_NAME := solaris
UNIX_DEFINE :=SOLARIS
SHELL = /bin/bash
GCPPFLAGS += -DMK_BIG_ENDIAN
GLDLIBS += -lsocket
endif

ifeq ($(UNIX_PROCESSOR),i686)
UNIX_DEFAULT :=linux
#UNIX_DEFAULT2 :=$PUNIX/dev/linux
UNIX_NAME := linux
UNIX_DEFINE :=LINUX
#GCPPFLAGS += -D_LIBC_REENTRANT
endif

## try to adapt to RedHat 7.3
ifeq ($(UNIX_PROCESSOR),unknown)
ifeq ($(UNIX_MACHINE),i686)
UNIX_DEFAULT :=linux
UNIX_NAME := linux
UNIX_DEFINE :=LINUX
endif
ifeq ($(UNIX_MACHINE),x86_64)
UNIX_DEFAULT :=linux64
UNIX_NAME := linux
UNIX_DEFINE :=LINUX
MK_64 := 1
GCPPFLAGS += -m64
endif
endif

ifeq ($(UNIX_PROCESSOR),athlon)
UNIX_DEFAULT :=linux64
UNIX_NAME := linux
UNIX_DEFINE :=LINUX
endif

## try to adapt to MacOS
ifeq ($(UNIX_PROCESSOR),i386)
UNIX_MACHINE :=$(shell uname -s)
ifeq ($(UNIX_MACHINE),Darwin)
UNIX_DEFAULT :=osx
UNIX_NAME := osx
UNIX_DEFINE :=OSX
MK_OSX :=1
endif
endif

ifeq ($(UNIX_PROCESSOR),x86_64)
ifeq ($(UNIX_MACHINE),x86_64)
UNIX_DEFAULT :=linux64
UNIX_NAME := linux
UNIX_DEFINE :=LINUX
MK_64 := 1
GCPPFLAGS += -m64
endif
endif


# create this directory level now
#MK_TEMP := $(if $(wildcard $(UNIX_DEFAULT2)),,$(shell mkdir $(UNIX_DEFAULT2)))

# tell source files the architecture ... but why in lower case?
GCPPFLAGS += -D$(UNIX_DEFAULT) -D$(UNIX_DEFINE)
endif  #if not NT

MK_MISC_DONE :=1

endif  #if not MK_MISC_DONE
#endif  #if 0

ifdef MK_DINKUM
ifndef MK_DINKUM_DONE
ifneq ($(OS),Windows_NT)
CXX=gcc
#GCPPFLAGS += -fno-builtin -D_ALT_NS=1 -I$Pdinkumware/include
GCPPFLAGS += -D_ALT_NS=1 -I$Pdinkumware/include
GLDLIBS += -L$Pdinkumware/lib_$(UNIX_DEFAULT)
ifdef MK_64
ifdef DEBUG
GLDLIBS += -lgcc_v4_64_native_standard_db -lstdc++
else
GLDLIBS += -lgcc_v4_64_native_standard -lstdc++
endif
else
ifdef DEBUG
GLDLIBS += -lgcc_v4_native_standard -lstdc++
else
GLDLIBS += -lgcc_v4_native_standard -lstdc++
endif
endif
else
endif
MK_DINKUM_DONE :=1
MK_DINKUM_TAR = dinkumware
endif
endif

#######################################################################
# V. Recursively load makefiles for other modules
#######################################################################

# protect against duplicates
# $M/MODULES_NEEDED := $(addprefix $P,$(sort $($M/MODULES_NEEDED)))
$M/MODULES_NEEDED := $(addprefix $P,$($M/MODULES_NEEDED))

# what modules still need to be loaded
$M/NEW_MODULES := $(filter-out $(ALL_MODULES), $($M/MODULES_NEEDED))
ALL_MODULES += $($M/NEW_MODULES)

# load any missing modules
# ifneq ($($M/NEW_MODULES),)
ifneq ($($M/MODULES_NEEDED),)

#$M/NEW_MAKES := $(addsuffix /module.mk, $($M/NEW_MODULES))
$M/NEW_MAKES := $(addsuffix /module.mk, $($M/MODULES_NEEDED))

# push
MODULE_STACK := $M $(MODULE_STACK)

# the next sequence assumes that NONE of the module.mk's
#  will be configured dynamically (might still work, but
#  not tested)
NEXT_M_STACK := $($M/MODULES_NEEDED) $(NEXT_M_STACK)
include $($M/NEW_MAKES)

## NOTE: if NEXT_M_STACK fails,
##  try "include $(foreach dir,$(M/NEW_MODULES),$(call M,$dir)
##  with M := $(1) in the line before ... just a thought ...

# pop
M := $(firstword $(MODULE_STACK))
MODULE_STACK := $(wordlist 2, 20, $(MODULE_STACK))
endif

#######################################################################
#  VI. Build module specific path variables
#######################################################################

# Establish default Windows Visual Studio Paths
ifeq ($(OS),Windows_NT)

$M/OBJ_PATH := $(if $($M/OBJ_PATH),$($M/OBJ_PATH),$(WIN_DEFAULT))
$M/LIB_PATH := $(if $($M/LIB_PATH),$($M/LIB_PATH),$(WIN_DEFAULT))
$M/BIN_PATH := $(if $($M/BIN_PATH),$($M/BIN_PATH),$(WIN_DEFAULT))

else


##
## Establish a destination path: one given with module or our default
##

$M/OBJ_PATH := $(if $($M/OBJ_PATH),$($M/OBJ_PATH),_$(UNIX_DEFAULT))
$M/LIB_PATH := $(if $($M/LIB_PATH),$($M/LIB_PATH),_$(UNIX_DEFAULT))

endif

##
## Make the path fully qualified.  If given path starts with a /, assume
##  it is already full formed
## Note: some paths are terminated with /./ to help other construction rules
##
MS := $M$(if $($M/SRC_PATH),/$($M/SRC_PATH),)
MI := $M$(if $($M/INC_PATH),/$($M/INC_PATH),)
MO := $M$(if $($M/OBJ_PATH),/$($M/OBJ_PATH),)
ML := $M$(if $($M/LIB_PATH),/$($M/LIB_PATH),)
MB := $M$(if $($M/BIN_PATH),/$($M/BIN_PATH),)
ME := $(MB)

##
## Add standard suffix and then qualified path prefixes to our files
##
$M/BUILD_BINS := $(addsuffix .$B,$($M/BUILD_BINS))
$M/BUILD_DLLS := $(addsuffix .$D,$($M/BUILD_DLLS))
$M/TEST_BINS := $(addsuffix .$B,$($M/TEST_BINS))

ifeq ($(strip $($M/ARCS_ARE_SO)),1)
$M/BUILD_ARCS := $(addsuffix .$S,$($M/BUILD_ARCS))
else
$M/BUILD_ARCS := $(addsuffix .$A,$($M/BUILD_ARCS))
endif

$M/BUILD_OBJS := $(addprefix $(MO)/,$(addsuffix .$O,$(basename $(notdir $($M/BUILD_SRCS)))))

$M/BUILD_SRCS := $(addprefix $(MS)/,$($M/BUILD_SRCS))
$M/BUILD_BINS := $(addprefix $(MB)/,$($M/BUILD_BINS))
$M/BUILD_ARCS := $(addprefix $(ML)/,$($M/BUILD_ARCS))
$M/BUILD_DLLS := $(addprefix $(ML)/,$($M/BUILD_DLLS))
$M/TEST_BINS := $(addprefix $(MB)/,$($M/TEST_BINS))

# only create .d files of .cpp files that exist at this moment, RPC
# generated files will get them in due time

$M/BUILD_DEPS := $($M/BUILD_DEPS) $(addprefix $(MO)/,$(addsuffix .d,$(basename $(notdir $(filter %.cpp %.c,$(wildcard $($M/BUILD_SRCS)))))))

#
# drive unit tests by their output logs
#
ifneq ($(strip $($M/TEST_BINS)),)
$M/TEST_LOG := $Ptest_logs/$(notdir $M).xml

$($M/TEST_LOG): $($M/TEST_BINS)
	if [ ! -d $(@D) ]; then mkdir $(@D); fi
	if [ -e $@ ]; then rm $@; fi
	-for i in $<; do $$i /xml=$@; done

test: $($M/TEST_LOG)

endif

#######################################################################
#   VII. Target specific variables and implicit rules
#######################################################################

# only define implicit rules once
#ifeq ($(MK_START),$M)
ifndef MK_RULES_ONCE
MK_RULES_ONCE := 1

# add DEFS to the underlying compile macros
COMPILE.c = $(CC) $(CFLAGS) $(CPPFLAGS) $(MK_INCLUDES) $(TARGET_ARCH) -c
COMPILE.cc = $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(MK_INCLUDES) $(TARGET_ARCH) -c
NO_COMPILE.cc = $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(MK_INCLUDES) $(TARGET_ARCH)


#LINK.o = $(CC) $(TARGET_ARCH)
LINK.o = $(CXX) $(TARGET_ARCH)
LINK.c = $(CC) $(CFLAGS) $(CPPFLAGS) $(MK_INCLUDES) $(TARGET_ARCH)
LINK.cc = $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(MK_INCLUDES) $(TARGET_ARCH)
#OUTPUT_OPTION =

#######################################################################
#   VIII. Target specific variables and implicit rules
#######################################################################

clean:
	$(RM) $($M/BUILD_ARCS) $($M/BUILD_BINS) $($M/TEST_BINS) $($M/TEST_LOG) $($M/BUILD_OBJS) $($M/BUILD_DEPS) $(MO)/*.idb $(MO)/*.pdb
	$(if $(wildcard $^),$(RM) $(wildcard $^))

allclean:
	$(RM) -f $(MK_ALL_CLEAN)
	$(if $^,$(RM) $^)

tar:
	tar -C $P -czf $M/sources.tgz  $(subst $P,,--exclude $M/sources.tgz $(ALL_MODULES) $(MK_DINKUM_TAR) $M)
endif

# set default target on start make only
ifeq ($(MK_START),$M)
build: $($M/BUILD_ARCS) $($M/BUILD_BINS) $($M/TEST_BINS)

ifdef MK_BIN_DIR
build: $(addprefix $(MK_BIN_DIR)/,$(notdir $($M/BUILD_BINS)))

endif
endif

$(MO)/%.$O: $(MS)/%.c
	$(COMPILE.c) $< $(OUTPUT_OPTION)

$(MO)/%.$O: $(MS)/%.cpp
	$(COMPILE.cc) $< $(OUTPUT_OPTION)

$(MO)/%.$O: $(MS)/*/%.cpp
	$(COMPILE.cc) $< $(OUTPUT_OPTION)

## this rule seldom is used ...
$(MO)/%.d: $(MS)/%.c
	@echo -- Creating dependency file for $<
ifeq ($(OS),Windows_NT)
	$(patsubst /Z%,,@$(COMPILE.c) $< $($M/DEPS))
	@mvdeps $(addsuffix .i,$(basename $(notdir $<))) $@
	@$(RM) $(addsuffix .i,$(basename $(notdir $<)))
else
	$(COMPILE.c) -MM -E -MT $(basename $@).d -MT $(basename $@).o -MF $@ $<
	@echo $(basename $@).o: $(basename $@).d >>$@
endif

## save line before experimenting with -MG removal
## 	$(COMPILE.c) -MM -MG -E -MT $(basename $@).d -MT $(basename $@).o -MF $@ $<


# the unix rule with the -MT is the latest and greatest.  Other .d rules should be updated
#   mev 12/18/03

$(MO)/%.d: $(MS)/%.cpp
	@echo -- Creating dependency file for $<
ifeq ($(OS),Windows_NT)
	$(patsubst /Z%,,@$(COMPILE.cc) $< $($M/DEPS))
	@mvdeps $(addsuffix .i,$(basename $(notdir $<))) $@
	@$(RM) $(addsuffix .i,$(basename $(notdir $<)))
else
	$(COMPILE.cc) -MM -E -MT $(basename $@).d -MT $(basename $@).o -MF $@ $<
	@echo $(basename $@).o: $(basename $@).d >>$@
endif

$(MO)/%.d: $(MS)/*/%.cpp
	@echo -- Creating dependency file for $<
ifeq ($(OS),Windows_NT)
	$(patsubst /Z%,,@$(COMPILE.cc) $< $($M/DEPS))
	@mvdeps $(addsuffix .i,$(basename $(notdir $<))) $@
	@$(RM) $(addsuffix .i,$(basename $(notdir $<)))
else
	$(COMPILE.cc) -MM -E -MT $(basename $@).d -MT $(basename $@).o -MF $@ $<
	@echo $(basename $@).o: $(basename $@).d >>$@
endif


ifeq ($(OS),Windows_NT)
define DefaultDep
@echo 2 -- Creating dependency 3 file for $<
$(patsubst /Z%,,@$(COMPILE.cc) $< $($M/DEPS))
@mvdeps $(addsuffix .i,$(basename $(notdir $<))) $@
@$(RM) $(addsuffix .i,$(basename $(notdir $<)))
endef
else
define DefaultDep
@echo 3 -- Creating dependency file for $<
set -e; $(COMPILE.cc) -x cpp-output -MM -MG -E $< \
| sed -e 's@ /usr/[^ ]*@@g' \
-e 's@^\(.*\).o:@$(dir $<)\1.d $(dir $<)\1.o:@' > $@; \
[ -e $@ ] || rm -f $@
endef
endif

#$(MB)/%.$B: $(MS)/%.c
#	$(LINK.c) $(filter %.c,$^) $(OUTPUT_OPTION) $(LDFLAGS) $(MK_LIBS) $(LDLIBS)

#$(MB)/%.$B: $(MS)/%.cpp
#	$(LINK.cc) $(filter %.cpp,$^) $(OUTPUT_OPTION) $(LDFLAGS) $(MK_LIBS) $(LDLIBS)

# this rule will build a binary when the source files have a different name
#ifeq ($(OS),Windows_NT)
$(MB)/%.$B:
	$(if $(filter %.c,$^), $(LINK.c)  $(filter %.c,$^) $(OUTPUT_OPTION) $(LDFLAGS) $(MK_LIBS) $(LDLIBS))
	$(if $(filter %.cpp,$^), $(LINK.cc) $(filter %.cpp,$^) $(OUTPUT_OPTION) $(LDFLAGS) $(MK_LIBS) $(LDLIBS))
	$(if $(filter %.$O,$^), $(LINK.cc) $(filter %.$O,$^) $(OUTPUT_OPTION) $(LDFLAGS) $(MK_LIBS) $(LDLIBS))
#endif

$(ML)/%.$S:
	$(if $(filter %.c,$^), $(LINK.c)  $(filter %.c,$^) $(OUTPUT_OPTION) $(LDFLAGS) $(filter-out -l$(basename $(notdir $@)),$(MK_LIBS))  $(LDLIBS))
	$(if $(filter %.cpp,$^), $(LINK.cc) $(filter %.cpp,$^) $(OUTPUT_OPTION) $(LDFLAGS) $(filter-out -l$(basename $(notdir $@)),$(MK_LIBS)) $(LDLIBS))
	$(if $(filter %.$O,$^), $(LINK.cc) -shared $(filter %.$O,$^) $(OUTPUT_OPTION) $(LDFLAGS) $(filter-out -l$(basename $(notdir $@)),$(MK_LIBS))  $(LDLIBS))


#	$(if $(filter %.$O,$^), ld -x -shared $(filter %.$O,$^) $(OUTPUT_OPTION) $(LDFLAGS)  $(LDLIBS))
#	$(if $(filter %.$O,$^), ld --shared -Ur $(filter %.$O,$^) $(OUTPUT_OPTION) $(LDFLAGS) $(filter-out -l$(basename $(notdir $@)),$(MK_LIBS))  $(LDLIBS))

#	$(if $(filter %.$O,$^), $(LINK.cc) $(filter %.$O,$^) $(OUTPUT_OPTION) $(LDFLAGS) $(filter-out -l$(basename $(notdir $@)),$(MK_LIBS))  $(LDLIBS))


#warning:  The following archive rule if more than one source file (.c or .cpp) is part of the dependencies

$(ML)/%.$L:
	$(if $(filter %.c,$^), $(COMPILE.c)  $(filter %.c,$^) $(OUTPUT_OPTION))
	$(if $(filter %.cpp,$^), $(COMPILE.cc) $(filter %.cpp,$^) $(OUTPUT_OPTION))
	$(RM) $@
	$(AR) $(patsubst  $(MS)/%.c,$(MO)/%.$O,$(filter %.c,$^)) $(patsubst $(MS)/%.cpp,$(MO)/%.$O,$(filter %.cpp,$^)) $(filter %.$O,$^)
	$(RANLIB) $@

# warning:  The following rule would sometimes, not always, kick into action
#           even if the dependency had more than one .cpp file.  Therefore only
#           one of the two or more .cpp files would be compiled and linked.
#$(MB)/%.$B: $(MO)/%.$O
#	$(LINK.o) $(filter %.$O,$^) $(OUTPUT_OPTION) $(LDFLAGS) $(MK_LIBS) $(LDLIBS)

##
## setup stem rule to force copy of binary to Nuview directory if MK_BIN_DIR
##  environment variable defined.
##
ifdef MK_BIN_DIR

$(MK_BIN_DIR)/%.so: $(ML)/%.so
	cp $<* $(@D)/.

$(MK_BIN_DIR)/%: $(MB)/%
	cp $< $@

endif



#setup include paths and libraries based upon needed modules & libs
#  the ARCS_PUBLISHED was slightly massaged just before including
#  other makes.  This just finishes the massaging
#$M/ARCS_PUBLISHED := $(addprefix $(ML)/,$($M/ARCS_PUBLISHED))
$M/ARCS_PUBLISHED := $(patsubst ./%,$(ML)/%,$($M/ARCS_PUBLISHED))

$M/BINS_PUBLISHED := $(patsubst ./%,$(MB)/%,$($M/BINS_PUBLISHED))
#$M/BINS_PUBLISHED := $(addsuffix .$B,$($M/BINS_PUBLISHED))
#$M/BINS_PUBLISHED := $(addprefix $(MB)/,$($M/BINS_PUBLISHED))
ALL_BINS_PUBLISHED := $(ALL_BINS_PUBLISHED) $($M/BINS_PUBLISHED)

ifeq ($(OS),Windows_NT)
MK_LIBS += $($M/ARCS_PUBLISHED)
else

MK_TEMP :=$(MK_LIBS)

#ifeq ($(strip $($M/MK_BINS_NEED_ARCS)),1)
MK_LIBS :=$(foreach lib,$($M/ARCS_PUBLISHED),-L$(dir $(lib)) -l$(patsubst lib%,%,$(basename $(notdir $(lib)))))
#endif

MK_LIBS +=$(MK_TEMP) $(foreach lib,$($M/LIBRARIES_NEEDED),-l$(patsubst lib%,%,$(notdir $(lib))))

endif


##
## old - give everyone same include list in a "default" order
#MK_INCLUDES += $(sort $(if $($M/INC_PATH_PUBLISHED),$(addprefix -I$M/,$($M/INC_PATH_PUBLISHED))) -I$M $(if $($M/INC_PATH),-I$M/$($M/INC_PATH)))

##
## new - create include list per module, with include list of modules used (recursive)
##       There does not appear to be an easy way to filter each module
##       needed against its predecessors, so we do the first ten the
##       the hard way.
$M/INC_PATHS_USED := $(addprefix -I$M/, $($M/INC_PATH_PUBLISHED))

MK_TEMP=$(word 1,$($M/MODULES_NEEDED))
ifneq ($(MK_TEMP),)
$M/INC_PATHS_USED +=$(filter-out $($M/INC_PATHS_USED),$($(MK_TEMP)/INC_PATHS_USED))
endif

MK_TEMP=$(word 2,$($M/MODULES_NEEDED))
ifneq ($(MK_TEMP),)
$M/INC_PATHS_USED +=$(filter-out $($M/INC_PATHS_USED),$($(MK_TEMP)/INC_PATHS_USED))
endif

MK_TEMP=$(word 3,$($M/MODULES_NEEDED))
ifneq ($(MK_TEMP),)
$M/INC_PATHS_USED +=$(filter-out $($M/INC_PATHS_USED),$($(MK_TEMP)/INC_PATHS_USED))
endif

MK_TEMP=$(word 4,$($M/MODULES_NEEDED))
ifneq ($(MK_TEMP),)
$M/INC_PATHS_USED +=$(filter-out $($M/INC_PATHS_USED),$($(MK_TEMP)/INC_PATHS_USED))
endif

MK_TEMP=$(word 5,$($M/MODULES_NEEDED))
ifneq ($(MK_TEMP),)
$M/INC_PATHS_USED +=$(filter-out $($M/INC_PATHS_USED),$($(MK_TEMP)/INC_PATHS_USED))
endif

MK_TEMP=$(word 6,$($M/MODULES_NEEDED))
ifneq ($(MK_TEMP),)
$M/INC_PATHS_USED +=$(filter-out $($M/INC_PATHS_USED),$($(MK_TEMP)/INC_PATHS_USED))
endif

MK_TEMP=$(word 7,$($M/MODULES_NEEDED))
ifneq ($(MK_TEMP),)
$M/INC_PATHS_USED +=$(filter-out $($M/INC_PATHS_USED),$($(MK_TEMP)/INC_PATHS_USED))
endif

MK_TEMP=$(word 8,$($M/MODULES_NEEDED))
ifneq ($(MK_TEMP),)
$M/INC_PATHS_USED +=$(filter-out $($M/INC_PATHS_USED),$($(MK_TEMP)/INC_PATHS_USED))
endif

MK_TEMP=$(word 9,$($M/MODULES_NEEDED))
ifneq ($(MK_TEMP),)
$M/INC_PATHS_USED +=$(filter-out $($M/INC_PATHS_USED),$($(MK_TEMP)/INC_PATHS_USED))
endif

## do the rest now
MK_TEMP := $(foreach mod, $(wordlist 10,100,$($M/MODULES_NEEDED)), $($(mod)/INC_PATHS_USED))
$M/INC_PATHS_USED += $(filter-out $($M/INC_PATHS_USED),$(MK_TEMP))
## debug $(warning 2. m is $M and IncPathsUsed is $($M/INC_PATHS_USED) with modules $($M/MODULES_NEEDED))


##
## Copy .so files to "standard place"
##

ifdef MK_BIN_DIR

ifeq ($(strip $($M/ARCS_ARE_SO)),1)
build: $(addprefix $(MK_BIN_DIR)/,$(notdir $($M/BUILD_ARCS)))
endif

ifneq ($(strip $($M/BINS_PUBLISHED)),)
build: $(addprefix $(MK_BIN_DIR)/,$(notdir $($M/BINS_PUBLISHED)))
endif

endif

#######################################################################
# IX. Set library to binary dependencies, recusively apply libraries
#      from included module levels
#######################################################################

# create prerequisites out of libraries_need list
# - take list of needed libraries
# - search known list of published libraries (with paths) to find the full name
#    of the partial we know
# fails if two modules cross list libraries AND both publish binaries
MK_TEMP := $($M/LIBRARIES_NEEDED)
MK_TEMP := $(addprefix %/,$(MK_TEMP))
MK_TEMP := $(foreach mod, $($M/MODULES_NEEDED), $(filter $(foreach lib, $(MK_TEMP),$(lib).$A $(lib).$S),$($(mod)/ARCS_PUBLISHED)))

ifneq ($(words $(MK_TEMP)),$(words $($M/LIBRARIES_NEEDED)))
#ifneq (,$(MK_TEMP))
$(error Libraries needed not found in published list of modules needed ($M, $(MK_TEMP), $($M/LIBRARIES_NEEDED)).)
#$(error Libraries needed not found in published list of modules needed ($M, $(MK_TEMP), $(ALL_ARCS_PUBLISHED)).)
endif

$M/LIBRARIES_NEEDED := $(MK_TEMP)

$M/LIBRARIES_NEEDED2 := $(foreach mod, $($M/MODULES_NEEDED), $($(mod)/LIBRARIES_NEEDED) $($(mod)/LIBRARIES_NEEDED2))


MK_TEMP := $($M/BINARIES_NEEDED)
MK_TEMP := $(addprefix %/,$(MK_TEMP))
MK_TEMP := $(foreach mod, $($M/MODULES_NEEDED), $(filter $(foreach exe, $(MK_TEMP),$(exe).$E),$($(mod)/BINS_PUBLISHED)))

ifneq ($(words $(MK_TEMP)),$(words $($M/BINARIES_NEEDED)))
$(error BINARIES needed not found in published list of modules needed ($M, $(MK_TEMP), $($M/BINARIES_NEEDED)).)
endif

$M/BINARIES_NEEDED := $(MK_TEMP)

#
# Here is a long, hard lesson.  There is no need to fool around transforming the
#  the libraries into -L paths and names.  Assign the library to the binary just
#  like any object file.  The linker knows what to do.  (tested, at least in Unix)
#  Also see MK_LIBS supporting code further down.

ifneq ($(strip $($M/BUILD_ARCS)),)
$($M/BUILD_ARCS): $($M/LIBRARIES_NEEDED) $($M/BINARIES_NEEDED)
endif

ifneq ($(strip $($M/BUILD_BINS)),)
## user should manual add arcs_published as dependency to build_bins
##  since the role could be reversed (bin needed to build arc)
#$($M/BUILD_BINS): $($M/ARCS_PUBLISHED) $($M/LIBRARIES_NEEDED)
$($M/BUILD_BINS): $($M/LIBRARIES_NEEDED) $($M/BINARIES_NEEDED)

#ifdef $M/MK_BINS_NEED_ARCS
$($M/BUILD_BINS): $($M/ARCS_PUBLISHED)
#endif
endif

ifneq ($(strip $($M/TEST_BINS)),)
## user should manual add arcs_published as dependency to build_bins
##  since the role could be reversed (bin needed to build arc)
$($M/TEST_BINS): $($M/LIBRARIES_NEEDED) $($M/BINARIES_NEEDED)

$($M/TEST_BINS): $($M/ARCS_PUBLISHED)
endif


#
## This rule will just build misc binaries and libraries if the current makefile
##  is really just an assembly / bundle of other modules
#

ifeq ($(strip $($M/BUILD_ARCS) $($M/BUILD_BINS)),)
build: $($M/BINARIES_NEEDED) $($M/LIBRARIES_NEEDED)
endif


#allclean: $(wildcard $($M/BUILD_ARCS) $($M/BUILD_BINS) $($M/BUILD_OBJS) $($M/BUILD_DEPS) $(MO)/*.idb $(MO)/*.pdb)
MK_ALL_CLEAN:= $(MK_ALL_CLEAN) $(wildcard $($M/BUILD_ARCS) $($M/BUILD_BINS) $($M/TEST_BINS) $($M/TEST_LOG) $($M/BUILD_OBJS) $($M/BUILD_DEPS) $(MO)/*.idb $(MO)/*.pdb)

##
## set up rules based upon target paths
##  BUT only do so if there are build sources for
##  the path, helps deal with gnumake bug where first match
##  of path is used instead of longest match
##
ifneq ($(strip $($M/BUILD_SRCS)),)

# this MK_TEMP rule builds the target directory if missing
MK_TEMP := $(if $(wildcard $(MB)),,$(shell mkdir $(MB)))
$(MB)/% : CFLAGS   = $($M/CFLAGS)
$(MB)/% : CPPFLAGS = $($M/CPPFLAGS)
$(MB)/% : CXXFLAGS = $($M/CXXFLAGS)
$(MB)/% : LDFLAGS  = $($M/LDFLAGS)
$(MB)/% : LDLIBS   = $($M/LDLIBS)
$(MB)/% : MK_INCLUDES  = -I$P -I$M $(if $($M/INC_PATH),-I$M/$($M/INC_PATH)) $($M/INC_PATHS_USED) $(addprefix -I,$($M/INC_SEARCH))

$(MB)/% : OUTPUT_OPTION = $($M/OUTPUT_OPTION)
$(MB)/% : M        := $M
$(MB)/% : MS       := $(MS)
$(MB)/% : MI       := $(MI)
$(MB)/% : MO       := $(MO)
$(MB)/% : ML       := $(ML)
$(MB)/% : MB       := $(MB)
$(MB)/% : ME       := $(ME)

# make target specific library lists ... if bins being built
#  old code would apply all potential libraries to all files ... this did not work
# - the list is given twice to allow for circular dependencies
# - The list is given a "cleansing" to apply paths to libraries that might not have
#   resolved during LIBRARIES_NEEDED processing due to cross listing modules
ifneq ($(strip $($M/BUILD_BINS) $($M/TEST_BINS)),)
MK_TEMP := $($M/LIBRARIES_NEEDED) $($M/LIBRARIES_NEEDED2) $($M/LIBRARIES_NEEDED) $($M/LIBRARIES_NEEDED2)
#$($M/BUILD_BINS) : MK_LIBS  := $(foreach lib, $(MK_TEMP), $(if $(subst ./,,$(dir $(lib))),$(lib), $(filter %$(lib),$(ALL_ARCS_PUBLISHED))))
#ifeq ($(UNIX_PROCESSOR),sparc)
#solaris appears to have a modern linker, remove redundancy
#  this could be written better ...
MK_TEMP := $(sort $(foreach lib, $(MK_TEMP), $(if $(subst ./,,$(dir $(lib))),$(lib), $(filter %$(lib),$(ALL_ARCS_PUBLISHED)))))
MK_TEMP_SO := $(sort $(filter %.so,$(MK_TEMP)))
MK_TEMP := $(filter-out %.so,$(MK_TEMP))

## removed comments from ifdef 12/7/05 for bugler
ifdef $M/MK_BINS_NEED_ARCS
MK_TEMP += $($M/ARCS_PUBLISHED)
endif

# yep, need MK_TEMP 3 times for nvTransform3 to build ...
# $($M/BUILD_BINS) : MK_LIBS  := $(MK_TEMP) $(MK_TEMP_SO) $(MK_TEMP) $(MK_TEMP) $(MK_LIBS)

#endif
endif

ifneq ($(MB),$(ML))
MK_TEMP := $(if $(wildcard $(ML)),,$(shell mkdir $(ML)))
$(ML)/% : CFLAGS   = $($M/CFLAGS)
$(ML)/% : CPPFLAGS = $($M/CPPFLAGS)
$(ML)/% : CXXFLAGS = $($M/CXXFLAGS)
$(ML)/% : LDFLAGS  = $($M/LDFLAGS)
$(ML)/% : LDLIBS   = $($M/LDLIBS)
$(ML)/% : MK_INCLUDES  = -I$P -I$M $(if $($M/INC_PATH),-I$M/$($M/INC_PATH)) $($M/INC_PATHS_USED)

$(ML)/% : OUTPUT_OPTION = $($M/OUTPUT_OPTION)
$(ML)/% : M        := $M
$(ML)/% : MS       := $(MS)
$(ML)/% : MI       := $(MI)
$(ML)/% : MO       := $(MO)
$(ML)/% : ML       := $(ML)
$(ML)/% : MB       := $(MB)
$(ML)/% : ME       := $(ME)
endif

ifneq ($(MB),$(MO))
MK_TEMP := $(if $(wildcard $(MO)),,$(shell mkdir $(MO)))
$(MO)/% : CFLAGS   = $($M/CFLAGS)
$(MO)/% : CPPFLAGS = $($M/CPPFLAGS)
$(MO)/% : CXXFLAGS = $($M/CXXFLAGS)
$(MO)/% : LDFLAGS  = $($M/LDFLAGS)
$(MO)/% : LDLIBS   = $($M/LDLIBS)
$(MO)/% : MK_INCLUDES  = -I$P -I$M $(if $($M/INC_PATH),-I$M/$($M/INC_PATH)) $($M/INC_PATHS_USED)

$(MO)/% : OUTPUT_OPTION = $($M/OUTPUT_OPTION)
$(MO)/% : M        := $M
$(MO)/% : MS       := $(MS)
$(MO)/% : MI       := $(MI)
$(MO)/% : MO       := $(MO)
$(MO)/% : ML       := $(ML)
$(MO)/% : MB       := $(MB)
$(MO)/% : ME       := $(ME)
endif

endif

#######################################################################
#  X. Create dependency files ... and load them ... in proper dependency order
#######################################################################

ifeq ($(filter tar clean allclean,$(MAKECMDGOALS)),)
#ifneq ($(wildcard $($M/BUILD_DEPS)),)
#include $(wildcard $($M/BUILD_DEPS))
ifneq ($($M/BUILD_DEPS),)
$M/BUILD_DEPSDEPS := $(foreach mod, $($M/MODULES_NEEDED), $($(mod)/BUILD_DEPS))
$($M/BUILD_DEPS): $($M/BUILD_DEPSDEPS)
include  $($M/BUILD_DEPS)
endif
endif
