
DIR=$(shell pwd)
TARGET=$(shell uname -o)

SHELL=/bin/bash


DIR=.
OBJ_DIR=/tmp/upgrade_status
EMS_ROOT=./ems_c
BIN_DIR=$(EMS_ROOT)/bin
CONF_DIR=$(EMS_ROOT)/conf

install_root=/usr/bin

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
LDFLAGS +=-lrt -lm -ldl -lpthread


ifeq ($(TARGET), 7620n)
	CFLAGS += -DBOARD_MT7620N
	CFLAGS += -Wno-unused-result
else ifeq ($(TARGET), 9344)
	CFLAGS += -DBOARD_AR9344
else
	CFLAGS += -DGENERIC_LINUX
	CFLAGS += -Wno-unused-result
endif

ifdef mips

ROOT=$(shell echo ${HOME})/bin/mipsroot
export STAGING_DIR=$(shell dirname $(shell readlink -f $(ROOT)))

MIPS_INC=${ROOT}/include/
MIPS_LIB= ${ROOT}/lib 

CC = ${ROOT}/bin/mipsel-openwrt-linux-gcc

LDFLAGS += -L$(MIPS_LIB)
INC     += -I${MIPS_INC}
else
CC = gcc
endif


ifdef usetls
CFLAGS += -DHAVE_TLS
LDFLAGS += -lssl -lcrypt
endif

ifdef test
CFLAGS += -DFOR_TEST_INM
endif

ifdef black
CFLAGS += -DUSE_URL_BLACKLIST
endif

ifdef log
CFLAGS += -DEMS_LOGGER_FILE
endif

ifdef useselect
LDFLAGS +=-lpthread
CFLAGS += -DUSE_SELECT
else
CFLAGS += -DUSE_EPOLL
endif

ifdef nupdate
CFLAGS += -DDENY_UPDATE
endif


ifdef debug
CFLAGS += -DDEBUG -g
LDFLAGS += -rdynamic
else
CFLAGS += -DNDEBUG -Os
endif

ifdef skiperror
CFLAGS += -DSKIP_ERROR
endif

ifdef weixin
CFLAGS += -DWEIXIN
endif


JSON_OBJS  = $(patsubst $(JSON_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(JSON_DIR)/*.c))
RADIUS_OBJS= $(patsubst $(RADIUS_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(RADIUS_DIR)/*.c))
UTILS_OBJS =$(patsubst $(UTILS_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(UTILS_DIR)/*.c))
EVENT_OBJS =$(patsubst $(EVENT_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(EVENT_DIR)/*.c))
CORE_OBJS =$(patsubst $(CORE_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(CORE_DIR)/*.c))

EMS_C=ems

.PHONY: clean all rebuild

all: $(EMS_ROOT) $(OBJ_DIR) $(BIN_DIR) $(CONF_DIR) $(EMS_C)


$(EMS_ROOT):
	mkdir -p $(EMS_ROOT)

$(CONF_DIR):
	mkdir -p $(CONF_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(EMS_C): $(JSON_OBJS) $(RADIUS_OBJS) $(UTILS_OBJS) $(EVENT_OBJS) $(CORE_OBJS)
	$(CC)  $(LDFLAGS) \
		-o $(EMS_C) \
		$(JSON_OBJS) \
		$(RADIUS_OBJS) \
		$(UTILS_OBJS) \
		$(EVENT_OBJS) \
		$(CORE_OBJS)
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

jshn:
	make -C $(JSON_DIR)/jshn

rebuild: clean all

clean:
	-rm -rf $(OBJ_DIR)
	-rm -f $(EMS_AAA)
	-rm -rf $(EMS_ROOT)


install:
	-cp -rf ems_c  $(install_root)
	-ln -sf ems ${install_root}/ems_c/bin/ems_getupdatefile
	-ln -sf ems ${install_root}/ems_c/bin/ems_download
	-ln -sf ems ${install_root}/ems_c/bin/ems_getconf
	-ln -sf ems ${install_root}/ems_c/bin/ems_getdc
	-ln -sf ems ${install_root}/ems_c/bin/ems_log
	-ln -sf ems ${install_root}/ems_c/bin/ems_statistic
	-ln -sf ems ${install_root}/ems_c/bin/ems_updatestatus
	-ln -sf ems ${install_root}/ems_c/bin/ems_totalinfo
	-cp -f ems.conf /etc/ems.conf
	-cp -f statisticd /etc/init.d/statisticd
	-chmod a+x /etc/init.d/statisticd
	-cp -f $(JSON_DIR)/jshn/jshn    ${install_root}/ems_c/bin
	-cp -f $(JSON_DIR)/jshn/jshn.sh ${install_root}/ems_c/bin
