
DIR=$(shell pwd)
#TARGET=$(shell uname -o)
use_mipsbacktrace=1

SHELL=/bin/bash


DIR=.
OBJ_DIR=/tmp/multissid
EMS_ROOT=./ems_c
BIN_DIR=$(EMS_ROOT)/bin
CONF_DIR=$(EMS_ROOT)/conf

INC_DIR=$(DIR)/inc
SRC_DIR=$(DIR)/src
JSON_DIR=$(DIR)/json
RADIUS_DIR=$(DIR)/radius/src

UTILS_DIR=$(SRC_DIR)/utils
EVENT_DIR=$(SRC_DIR)/event

CORE_DIR=$(SRC_DIR)/core

INC=-I$(INC_DIR) \
	-I${RADIUS_DIR} \
	-I$(JSON_DIR)	\
	-I$(UTILS_DIR) \
	-I$(EVENT_DIR)

INC_CORE=-I$(CORE_DIR)


CFLAGS  += -Wall $(INC)  
#CFLAGS += -Wextra
CFLAGS  += -std=gnu99 
CFLAGS  += -D_FILE_OFFSET_BITS=64
CFLAGS  += -DUSE_EMS_MEMORY_MGMT


ifeq ($(TARGET), 7620n)
	CFLAGS += -DBOARD_MT7620N
	CFLAGS += -Wno-unused-result
else ifeq ($(TARGET), 9344)
	CFLAGS += -DBOARD_AR9344
else
	CFLAGS += -DGENERIC_LINUX
	CFLAGS += -Wno-unused-result
	use_mipsbacktrace=0
endif

ifdef mips

BIN=$(shell echo ${HOME})/bin

MIPSROOT=$(BIN)/mipsroot

export STAGING_DIR=$(shell dirname $(shell readlink -f $(MIPSROOT)))

LDFLAGS += -L$(MIPSROOT)/lib/
CFLAGS  += -I$(MIPSROOT)/include/

CC = ${MIPSROOT}/bin/mipsel-openwrt-linux-gcc

MIPSTARGET=$(BIN)/mipstarget
LDFLAGS += -L$(MIPSTARGET)/usr/lib/
CFLAGS  += -I$(MIPSTARGET)/usr/include/

else
CC = gcc
endif


ifdef usetls
CFLAGS += -DHAVE_TLS
LDFLAGS += -lssl -lcrypt
endif

ifdef black
CFLAGS += -DUSE_URL_BLACKLIST
endif

ifdef log
CFLAGS += -DEMS_LOGGER_FILE
endif

ifdef useselect
CFLAGS += -DUSE_SELECT
else
CFLAGS += -DUSE_EPOLL
endif

ifdef nupdate
CFLAGS += -DDENY_UPDATE
endif


ifdef debug
CFLAGS += -DDEBUG -g -rdynamic
LDFLAGS += -rdynamic

ifeq ($(use_mipsbacktrace), 1)
	BACKTRACE_DIR=$(UTILS_DIR)/backtrace
	CFLAGS += -I$(BACKTRACE_DIR) -fexceptions
	LDFLAGS += -fexceptions -ldl
endif

else
CFLAGS += -DNDEBUG -Os
endif

ifdef weixin
CFLAGS += -DWEIXIN
endif

ifdef syslog
CFLAGS += -DEMS_USE_SYSLOG
endif

ifdef uci
LDFLAGS += -luci -lubox
endif

ifdef threads
LDFLAGS +=-lrt -lm -ldl -lpthread
CFLAGS += -DUSE_MULTI_THREAD
endif

ifdef mempool
CFLAGS += -DUSE_MEM_POOL

ifdef mempool_trace
CFLAGS += -DMEM_POOL_TRACE
endif

endif



JSON_OBJS  = $(patsubst $(JSON_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(JSON_DIR)/*.c))
RADIUS_OBJS= $(patsubst $(RADIUS_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(RADIUS_DIR)/*.c))
UTILS_OBJS =$(patsubst $(UTILS_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(UTILS_DIR)/*.c))
EVENT_OBJS =$(patsubst $(EVENT_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(EVENT_DIR)/*.c))
CORE_OBJS =$(patsubst $(CORE_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(CORE_DIR)/*.c))

ifeq ($(use_mipsbacktrace), 1)
BACKTRACE_OBJS=$(patsubst $(BACKTRACE_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(BACKTRACE_DIR)/*.c))
else
BACKTRACE_OBJS=
endif

EMS_C=ems

.PHONY: clean all rebuild

install:all
all: $(EMS_ROOT) $(OBJ_DIR) $(BIN_DIR) $(CONF_DIR) $(EMS_C) 


$(EMS_ROOT):
	mkdir -p $(EMS_ROOT)

$(CONF_DIR):
	mkdir -p $(CONF_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(EMS_C): $(JSON_OBJS) $(RADIUS_OBJS) $(UTILS_OBJS) $(EVENT_OBJS) $(CORE_OBJS) $(BACKTRACE_OBJS)
	$(CC)  $(LDFLAGS) \
		-o $(EMS_C) \
		$(JSON_OBJS) \
		$(RADIUS_OBJS) \
		$(UTILS_OBJS) \
		$(EVENT_OBJS) \
		$(CORE_OBJS) \
		$(BACKTRACE_OBJS)
	-cp -f $(EMS_C) $(BIN_DIR)
	-cp -f app_setup.sh $(BIN_DIR)
	-cp -f radius/conf/firewall   $(CONF_DIR)
	-cp -f radius/conf/dictionary $(CONF_DIR)
	-cp -f radius/conf/emsd       $(CONF_DIR)
	-cp -f radius/conf/appinfo    $(EMS_ROOT)
	-cp -f radius/conf/setup      $(EMS_ROOT)

$(OBJ_DIR)/%.o:$(CORE_DIR)/%.c
	$(CC) -c $(CFLAGS) \
		$(INC_CORE) \
		$< -o $@ 

ifeq ($(use_mipsbacktrace), 1)
$(OBJ_DIR)/%.o:$(BACKTRACE_DIR)/%.c
	$(CC) -c $(CFLAGS) \
		$(INC_CORE) \
		$< -o $@ 
endif

$(OBJ_DIR)/%.o:$(EVENT_DIR)/%.c 
	$(CC) -c $(CFLAGS) \
		$< -o $@ 

$(OBJ_DIR)/%.o:$(UTILS_DIR)/%.c 
	$(CC) -c $(CFLAGS) \
		$< -o $@ 

$(OBJ_DIR)/%.o:$(RADIUS_DIR)/%.c
	$(CC) -c $(CFLAGS) \
		$< -o $@ 

$(OBJ_DIR)/%.o:$(JSON_DIR)/%.c
	$(CC) -c $(CFLAGS) \
		$< -o $@ 

rebuild: clean all

clean:
	-rm -rf $(OBJ_DIR)
	-rm -f $(EMS_AAA)
	-rm -rf $(EMS_ROOT)
	-rm -rf $(EMS_C)

test:
	make -C $(shell pwd)/tests

test_clean:
	make -C $(shell pwd)/tests clean

