
DIR=$(shell pwd)
TARGET=$(shell uname -o)


INC_DIR=$(DIR)
OBJ_DIR=/tmp/uhttpdobj

SRC_DIR=$(DIR)
LUA_DIR=$(DIR)/libubox
UBUS_DIR=$(DIR)/ubus
JSON_DIR=$(DIR)/json

INC=-I$(INC_DIR) -I./libubox -I./ -I./ubus -I./json

LDFLAGS=-lrt -lm -ldl -lcrypt

CFLAGS  = -Wall $(INC)  
CFLAGS += -std=gnu99 
CFLAGS += -DUSE_EPOLL
CFLAGS += -D_FILE_OFFSET_BITS=64
CFLAGS += -DHAVE_CGI -DHAVE_UBUS

ifdef mips

ROOT=/home/tww/xRouter_Generic_Build20131223_V1.1.1.1/staging_dir/toolchain-mipsel_r2_gcc-4.6-linaro_uClibc-0.9.33.2
MIPS_INC=${ROOT}/include/
MIPS_LIB= ${ROOT}/lib 

CC = ${ROOT}/bin/mipsel-openwrt-linux-gcc

LDFLAGS += -L$(MIPS_LIB)
INC     += -I${MIPS_INC}
else
CC = gcc

#LDFLAGS += -lssl -llua
#CFLAGS += -DHAVE_LUA  -DHAVE_TLS
endif


#ifeq (useselect, 1)
ifdef useselect
LDFLAGS +=-lpthread
CFLAGS += -DDT_USE_SELECT
else
CFLAGS += -DDT_USE_EPOLL
endif

ifdef debug
CFLAGS += -DDEBUG -g
else
CFLAGS += -DNDEBUG
endif


ifeq ($(TARGET), Msys)
LDFLAGS += -lws2_32 
OBJ_DIR=/d/dt_net_obj
else
CFLAGS +=-Wno-unused-result
endif


OBJS  = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(SRC_DIR)/*.c))
OBJS  += $(patsubst $(LUA_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(LUA_DIR)/*.c))
OBJS  += $(patsubst $(UBUS_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(UBUS_DIR)/*.c))
OBJS  += $(patsubst $(JSON_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(JSON_DIR)/*.c))

UHTTPD=uhttpd

.PHONY: clean all rebuild

all: $(OBJ_DIR) $(UHTTPD)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(UHTTPD): $(OBJS)
	$(CC)  $(OBJS) -o $(UHTTPD) $(LDFLAGS)



$(OBJ_DIR)/%.o:$(SRC_DIR)/%.c 
	$(CC) $< -o $@ -c $(CFLAGS)


$(OBJ_DIR)/%.o:$(LUA_DIR)/%.c
	$(CC) $< -o $@ -c $(CFLAGS)

$(OBJ_DIR)/%.o:$(UBUS_DIR)/%.c
	$(CC) $< -o $@ -c $(CFLAGS)

$(OBJ_DIR)/%.o:$(JSON_DIR)/%.c
	$(CC) $< -o $@ -c $(CFLAGS)

#.c.o: $(OBJS)
#	$(CC) $< -o $@ -c $(CFLAGS)



rebuild: clean all

clean:
	-rm -rf $(OBJ_DIR)
	-rm -f $(UHTTPD)


