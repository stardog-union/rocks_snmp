#!/usr/bin/make -f
# -*- Makefile -*-

#####
# Overhead stuff
#####
M := $(if $(MK_START),$(firstword $(NEXT_M_STACK)),$(CURDIR))
NEXT_M_STACK := $(wordlist 2, 40, $(NEXT_M_STACK))

# 1 ## only process this file once though multiple nvmakes could ask for it
ifeq (,$(filter $M,$(MODULES_LOADED)))

all: build

######
#   I. What does this file provide to the world
#      (Path for include, list of files for libs & bins
#      without file extensions)
######

$M/INC_PATH_PUBLISHED := util libmevent snmpagent rocksdb/include
$M/ARCS_PUBLISHED     := librockssnmp
$M/BINS_PUBLISHED     :=
$M/A_ROOT_MODULE      := 1

######
#  II. What other modules are needed for this module
#      to compile (list of module directory names and
#      specific lib file names)
######

#$M/MODULES_NEEDED   :=  libmevent snmpagent rocksdb
#$M/LIBRARIES_NEEDED :=  libmevent libsnmpagent librocksdb

ifeq ($(filter debug,$(MAKECMDGOALS)),debug)
$M/MODULES_NEEDED   +=
$M/LIBRARIES_NEEDED +=
endif

######
# III. What is the build directory structure relative to
#      directory containing this file.  If left blank,
#      basemake.mak will provide defaults.
#####E

$M/SRC_PATH :=
$M/INC_PATH :=
$M/OBJ_PATH :=
$M/LIB_PATH := .
$M/BIN_PATH := .

# 2 ## What include paths need to be searched, beyond those implied by $M/MODULES_NEEDED
##  and $M/INC_PATH.  These are for the local build only.
$M/INC_SEARCH :=

######
#  IV. List sources and build deliverables.  Should be
#      listed as relative to SRC_PATH and LIB_PATH/BIN_PATH
#      respectively.  *_PUBLISHED from above automatically
#      added. (BUILD_SRCS only used for Linux dependency generation)
######
$M/BUILD_SRCS_LIB := stats_table.cpp
$M/BUILD_SRCS_UTIL := util/logging.cpp
$M/BUILD_SRCS_EVENT := libmevent/meventmgr.cpp libmevent/meventobj.cpp \
			libmevent/reader_writer.cpp libmevent/statemachine.cpp \
	              	libmevent/tcp_event.cpp
$M/BUILD_SRCS_SNMP := snmpagent/snmp_agent.cpp snmpagent/snmp_getresponse.cpp snmpagent/snmp_openpdu.cpp \
			snmpagent/snmp_pdu.cpp snmpagent/snmp_registerpdu.cpp \
			snmpagent/snmp_responsepdu.cpp snmpagent/snmp_closepdu.cpp snmpagent/snmp_value.cpp \
		     	snmpagent/val_error.cpp snmpagent/val_integer.cpp snmpagent/val_integer64.cpp \
			snmpagent/val_string.cpp snmpagent/val_table.cpp


#request_response.cpp request_response_buf.cpp \

$M/BUILD_SRCS_TEST := stats_test.cpp

$M/BUILD_SRCS := $($M/BUILD_SRCS_LIB) $($M/BUILD_SRCS_UTIL) $($M/BUILD_SRCS_EVENT) $($M/BUILD_SRCS_SNMP)
$M/BUILD_BINS :=
$M/BUILD_ARCS := librockssnmp
$M/BUILD_DLLS :=

ifeq ($(filter debug,$(MAKECMDGOALS)),debug)
$M/BUILD_SRCS += $($M/BUILD_SRCS_TEST)
$M/TEST_BINS  := stats_test
endif

######
#   V. Set any global compile features through defining
#     feature's flag variable: MK_THREADS, MK_SOCKETS
######

MK_THREADS := 1
MK_NO_WINDOWS := 1

######
#  VI. Include basemake.mak which contains all necessary
#      functions and rules to build based upon data above.
######
ifeq ($P,)
#%.mk: %.mk.in ; cd $(@D);configure
endif

include util/basemake.mk

######
# VII. Add any custom dependencies and/or rules.
#      NOTE:  Beginning here, all files MUST use full path
#      naming.  basemake.mak establishes these variables:
#       $P - Path prefix, absolute path name less this module's directory
#       $M - Module path, absolute path name including this directory
#       $O - Object file extension (.o in Linux, .obj in Windows)
#       $B/$E - Binary file extension (. in Linux, .exe in Windows)
#       $A/$L - Static library extension (.a in Linux, .lib in Windows)
#       $S/$D - dynamic library extension (.s in Linux, .dll in Windows)
#       $($M/SRCS) - BUILD_SRCS with full path prefixed
#       $($M/BINS) - BUILD_BINS with full path prefixed and $B suffixed
#       $($M/ARCS) - BUILD_ARCS with full path prefixed and $A suffixed
#       $($M/DLLS) - BUILD_DLLS with full path prefixed and $D suffixed
#       $(MS) - Module path to sources
#       $(MI) - Module path to includes
#       $(MO) - Module path to object files
#       $(ML) - Module path to libraries (static and dynamic)
#       $(MB) - Module path to binaries
#####

## change module specific flags if needed
$M/CFLAGS   +=
$M/CPPFLAGS +=
$M/CXXFLAGS +=
$M/LDFLAGS  +=
$M/LDLIBS   +=
#GLDLIBS += -lrt

$(ML)/librockssnmp.$A: $(call GET_DEPS2,$M/BUILD_SRCS)

$(MB)/stats_test.$B: $(call GET_DEPS2,$M/BUILD_SRCS_TEST)


endif
