
DIR=$(shell pwd)
TARGET=$(shell uname -o)

SHELL=/bin/bash

DIR=.
OBJ_DIR=/tmp/mpaudit_obj
AUDIT_OUT=./audit_out
BIN_DIR=$(AUDIT_OUT)/bin
CONF_DIR=$(AUDIT_OUT)/conf

DIR_INC=$(DIR)/inc
DIR_SRC=$(DIR)/src
DIR_JSON=$(DIR)/json

DIR_API=$(DIR_SRC)/api
DIR_CORE=$(DIR_SRC)/core
DIR_EVENT=$(DIR_SRC)/event
DIR_UTIL=$(DIR_SRC)/utils

DIR_LIB=$(DIR_SRC)/lib
DIR_LIB_CTRL=$(DIR_LIB)/ctrl
DIR_LIB_NET=$(DIR_LIB)/net
DIR_LIB_OUT=$(DIR_LIB)/out
DIR_LIB_SYS=$(DIR_LIB)/sys

INC=-I$(DIR_INC) \
	-I$(DIR_JSON) \
	-I$(DIR_API) \
	-I$(DIR_UTIL) \
	-I$(DIR_EVENT) \
	-I$(DIR_CORE) \

CFLAGS  +=$(shell pcap-config --cflags)
LDFLAGS +=$(shell pcap-config --libs)
CFLAGS  += -Wall $(INC)  
#CFLAGS += -Wextra
CFLAGS  += -std=gnu99 
CFLAGS  += -D_FILE_OFFSET_BITS=64
LDFLAGS +=-lrt -lm -ldl -lpthread -lpcap

ifeq ($(TARGET), 7620n)
	CFLAGS += -DBOARD_MT7620N
else ifeq ($(TARGET), 9344)
	CFLAGS += -DBOARD_AR9344
else
	CFLAGS += -DGENERIC_LINUX
	CFLAGS +=-Wno-unused-result
endif


ifdef mips

TOOLCHAIN_ROOT=$(shell echo ${HOME})/bin/mipsroot
export STAGING_DIR=$(shell dirname $(shell readlink -f $(TOOLCHAIN_ROOT)))

TARGET_ROOT=$(shell echo ${HOME})/bin/mipstarget

MIPS_INC=-I${TOOLCHAIN_ROOT}/include/ -I${TOOLCHAIN_ROOT}/usr/include/ \
	 -I${TARGET_ROOT}/include/ -I ${TARGET_ROOT}/usr/include/

MIPS_LIB=-L${TOOLCHAIN_ROOT}/lib  -I${TOOLCHAIN_ROOT}/usr/lib/ \
	 -L${TARGET_ROOT}/lib -L${TARGET_ROOT}/usr/lib

CC = ${TOOLCHAIN_ROOT}/bin/mipsel-openwrt-linux-gcc

LDFLAGS += $(MIPS_LIB)
INC     += ${MIPS_INC}

else
CC = gcc
endif

ifdef log
CFLAGS += -DEMS_LOGGER_FILE
endif

ifdef debug
CFLAGS += -DDEBUG -g
LDFLAGS += -rdynamic
else
CFLAGS += -DNDEBUG -Os
endif

OBJS_JSON = $(patsubst $(DIR_JSON)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(DIR_JSON)/*.c))
OBJS_CORE = $(patsubst $(DIR_CORE)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(DIR_CORE)/*.c))
OBJS_UTIL = $(patsubst $(DIR_UTIL)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(DIR_UTIL)/*.c))
OBJS_EVENT= $(patsubst $(DIR_EVENT)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(DIR_EVENT)/*.c))

# for plugins 
OBJS_CTRL= $(patsubst $(DIR_LIB_CTRL)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(DIR_LIB_CTRL)/*.c))
OBJS_NET = $(patsubst $(DIR_LIB_NET)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(DIR_LIB_NET)/*.c))
OBJS_OUT = $(patsubst $(DIR_LIB_OUT)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(DIR_LIB_OUT)/*.c))
OBJS_SYS = $(patsubst $(DIR_LIB_SYS)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(DIR_LIB_SYS)/*.c))
OBJS_API = $(patsubst $(DIR_API)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(DIR_API)/*.c))

AUDIT=mpaudit

.PHONY: clean all rebuild

install:all
all: $(OBJ_DIR) $(AUDIT_OUT) $(BIN_DIR) $(CONF_DIR) $(AUDIT)

#$(OBJS_JSON) $(OBJS_CORE) $(OBJS_UTIL) $(OBJS_EVENT) $(OBJS_CTRL) $(OBJS_NET) $(OBJS_OUT) $(OBJS_SYS) 

$(AUDIT_OUT):
	mkdir -p $(AUDIT_OUT)

$(CONF_DIR):
	mkdir -p $(CONF_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(AUDIT): $(OBJS_JSON) $(OBJS_CORE) $(OBJS_UTIL) $(OBJS_EVENT) $(OBJS_CTRL) $(OBJS_NET) $(OBJS_OUT) $(OBJS_SYS) $(OBJS_API)
	$(CC)  	-o $(AUDIT) \
		$(OBJS_JSON) \
		$(OBJS_CORE) \
		$(OBJS_UTIL) \
		$(OBJS_EVENT) \
		$(OBJS_CTRL) \
		$(OBJS_NET) \
		$(OBJS_OUT) \
		$(OBJS_API) \
		$(OBJS_SYS) \
		$(LDFLAGS) 
	cp -f $(AUDIT) $(BIN_DIR)/
	cp -f src/lib/out/16wifi_upload.sh $(BIN_DIR)/
	cp -f audit.cfg $(CONF_DIR)/

config:
	cp -f $(AUDIT) $(BIN_DIR)/
	cp -f src/lib/out/16wifi_upload.sh $(BIN_DIR)/
	cp -f audit.cfg $(CONF_DIR)/


$(OBJ_DIR)/%.o:$(DIR_CORE)/%.c
	$(CC) -c $(CFLAGS) \
		$< -o $@ 


$(OBJ_DIR)/%.o:$(DIR_UTIL)/%.c
	$(CC) -c $(CFLAGS) \
		$< -o $@ 

$(OBJ_DIR)/%.o:$(DIR_JSON)/%.c
	$(CC) -c $(CFLAGS) \
		$< -o $@ 

$(OBJ_DIR)/%.o:$(DIR_LIB_CTRL)/%.c
	$(CC) -c $(CFLAGS) \
		-I$(DIR_LIB_OUT) \
		-I$(DIR_LIB_NET) \
		$< -o $@ 

$(OBJ_DIR)/%.o:$(DIR_LIB_NET)/%.c
	$(CC) -c $(CFLAGS) \
		$< -o $@ 

$(OBJ_DIR)/%.o:$(DIR_LIB_OUT)/%.c
	$(CC) -c $(CFLAGS) \
		$< -o $@ 

$(OBJ_DIR)/%.o:$(DIR_EVENT)/%.c
	$(CC) -c $(CFLAGS) \
		$< -o $@ 

$(OBJ_DIR)/%.o:$(DIR_API)/%.c
	$(CC) -c $(CFLAGS) \
		$< -o $@ 

rebuild: clean all

clean:
	-rm -rf $(OBJ_DIR)
	-rm -f $(AUDIT)
	-rm -rf $(AUDIT_OUT)


