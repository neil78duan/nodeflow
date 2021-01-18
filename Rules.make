#rules of makefile 
#if this file is modified 
#please use 
#>make clean 
#>make
#to make all 

#debug flag 
#DEBUG =y


#profile , hot spot find
#PROFILE=y

tme23456=$(DEBUG)XX
ifeq ($(tme23456),XX)
DEBUG = y
endif

tme23457=$(PROFILE)XX
ifeq ($(PROFILE),XX)
PROFILE = y
endif

MYPROJECT_NAME = nodeflow

ARCH_MACHINE = $(shell uname -m)
OS_kernel = $(shell uname -s | tr '[A-Z]' '[a-z]')

AIM_NAME = $(OS_kernel)_$(ARCH_MACHINE)

ifeq ($(OS_kernel),linux)
	CFLAGS += -D__ND_LINUX__
	LFLAGS += -lrt -ldl -L/usr/lib64/mysql
	DLL_EXT_NAME := so
endif

ifeq ($(OS_kernel),darwin)
    CFLAGS += -D__ND_MAC__
	DLL_EXT_NAME := dylib
endif

TOPDIR = $(NODEFLOW_WORKING)
CURDIR = .

WORKDIR = $(TOPDIR)/bin
LIBDIR = $(TOPDIR)/lib/$(AIM_NAME)

CFLAGS += -c -w -O 
LFLAGS +=  -lpthread  -lm -liconv

#FOR NDSDK
CFLAGS += -I$(NDHOME)/include -I$(TOPDIR)/include
LFLAGS += -L$(NDHOME)/lib/$(AIM_NAME) -L$(LIBDIR)

ifeq ($(DEBUG),y)
    CFLAGS +=  -g -DDEBUG  -DND_DEBUG
    ifeq ($(PROFILE),y)
        CFLAGS += -pg
    else
    endif

	CLIENT_LIB = ndclient_s_d
	SRV_LIB = ndsrv_s_d
	COMMON_LIB = ndcommon_s_d

	CLIENT_DLL = ndclient_d
	SRV_DLL = ndsrv_d
	COMMON_DLL = ndcommon_d
else
	CFLAGS += -DNDEBUG

	CLIENT_LIB = ndclient_s
	SRV_LIB = ndsrv_s
	COMMON_LIB = ndcommon_s

	CLIENT_DLL = ndclient
	SRV_DLL = ndsrv
	COMMON_DLL = ndcommon
endif

NODEFLOW_PROJ_NAME := nodeflow
ND_LIBOUTPUT := $(NDHOME)/lib/$(AIM_NAME)

ND_CLIENT_LIB := $(ND_LIBOUTPUT)/lib$(CLIENT_LIB).a
ND_SRV_LIB := $(ND_LIBOUTPUT)/lib$(SRV_LIB).a
ND_COMMON_LIB := $(ND_LIBOUTPUT)/lib$(COMMON_LIB).a

ND_CLIENT_DLL := $(ND_LIBOUTPUT)/lib$(CLIENT_DLL).$(DLL_EXT_NAME)
ND_SRV_DLL := $(ND_LIBOUTPUT)/lib$(SRV_DLL).$(DLL_EXT_NAME)
ND_COMMON_DLL := $(ND_LIBOUTPUT)/lib$(COMMON_DLL).$(DLL_EXT_NAME)

PLATFORM_BITS =  $(shell  getconf LONG_BIT )

ifeq ($(PLATFORM_BITS),64)
    CFLAGS += -DX86_64
else
endif

CC = cc
CPP = c++
AR = ar rv
CC_DLL= cc -shared
CPP_DLL= c++ -shared


#set obj and src path

VPATH :=  $(SRCDIR) $(OBJDIR)

tme123=$(SRCDIR)XX
ifeq ($(tme123),XX)
#undef src
else
SRC := $(shell cd $(SRCDIR); ls | grep '\.c\>'	)
SRC_CPP := $(shell cd $(SRCDIR); ls | grep '\.cpp\>'	)
SRC_CC := $(shell cd $(SRCDIR); ls | grep '\.cc\>'	)


tme124=$(OBJDIR)XX
ifeq ($(tme124),XX)
else


#create objdir
tmp_objdir_created := $(shell [ -d $(OBJDIR) ] || mkdir $(OBJDIR))

OBJS := $(patsubst %.c, %.o,$(SRC) )  $(patsubst %.cpp, %.o,$(SRC_CPP) ) $(patsubst %.cc, %.o,$(SRC_CC) )
PathOBJS :=$(patsubst %.c, $(OBJDIR)/%.o, $(SRC) ) $(patsubst %.cpp, $(OBJDIR)/%.o, $(SRC_CPP) ) $(patsubst %.cc, $(OBJDIR)/%.o, $(SRC_CC) )

endif

endif



