
JSON_DIR=$(shell pwd)
JSON_DIR=.

OBJ_DIR=/tmp/json_obj

INC= -I$(JSON_DIR)

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
	CFLAGS += -DGENERIC_LINUX -DJSONC
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


ifdef debug
CFLAGS += -DDEBUG -g
LDFLAGS += -rdynamic
else
CFLAGS += -DNDEBUG -Os
endif

JSON_OBJS  = $(patsubst $(JSON_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(JSON_DIR)/*.c))

JSON=jshn


all: $(OBJ_DIR) $(JSON)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o:$(JSON_DIR)/%.c
	$(CC) -c $(CFLAGS) $< -o $@ 

$(JSON): $(JSON_OBJS)
	$(CC)  $(LDFLAGS) -o $(JSON) $(JSON_OBJS)

clean:
	-rm -rf $(JSON) $(OBJ_DIR)
