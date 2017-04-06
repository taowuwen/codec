
DIR=$(shell pwd)
TARGET=$(shell uname -o)

SHELL=/bin/bash

DIR=.
OBJ_DIR=/tmp/radius_obj

INC_DIR=$(DIR)/inc
LIB_DIR=$(DIR)/lib
SRC_DIR=$(DIR)/src

INC=-I$(INC_DIR) -I$(LIB_DIR)


CFLAGS  = -Wall $(INC)  
#CFLAGS += -Wextra
CFLAGS += -std=gnu99 
CFLAGS += -D_FILE_OFFSET_BITS=64
LDFLAGS=-lrt -lm -ldl -lpthread

ifdef mips

STAGING_DIR="/home/tww/company/openwrt_devel/openwrt/staging_dir"
export STAGING_DIR

ROOT=/home/tww/bin/mipsroot
MIPS_INC=${ROOT}/include/
MIPS_LIB= ${ROOT}/lib 

CC = ${ROOT}/bin/mipsel-openwrt-linux-gcc

LDFLAGS += -L$(MIPS_LIB)
INC     += -I${MIPS_INC}

CFLAGS += -DARCH_MIPS
else
CC = gcc
endif


ifdef usetls
CFLAGS += -DHAVE_TLS
LDFLAGS += -lssl -lcrypt
endif


ifdef useselect
LDFLAGS +=-lpthread
CFLAGS += -DUSE_SELECT
else
CFLAGS += -DUSE_EPOLL
endif


ifndef ndebug
CFLAGS += -DDEBUG -g -DCP_DEBUG
LDFLAGS += -rdynamic
else
CFLAGS += -DNDEBUG -Os
#CFLAGS += -DNDEBUG -g -rdynamic
endif

CFLAGS +=-Wno-unused-result


LIB_OBJS =$(patsubst $(LIB_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(LIB_DIR)/*.c))

radexample=radexample

.PHONY: clean all rebuild

all: $(OBJ_DIR) $(radexample)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(radexample): $(LIB_OBJS) $(OBJ_DIR)/radexample.o
	$(CC)  $(LDFLAGS) \
		-o $(radexample) \
		$(LIB_OBJS) \
		$(OBJ_DIR)/radexample.o

$(OBJ_DIR)/%.o:$(SRC_DIR)/%.c 
	$(CC) -c $(CFLAGS) \
		$< -o $@ 

$(OBJ_DIR)/%.o:$(LIB_DIR)/%.c 
	$(CC) -c $(CFLAGS) \
		$< -o $@ 


rebuild: clean all

clean:
	-rm -rf $(OBJ_DIR)
	-rm -f $(EMS_AAA)

