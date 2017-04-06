
DIR=$(shell pwd)/
TARGET=$(shell uname -o)


INC_DIR=$(DIR)
OBJ_DIR=$(DIR)obj

SRC_DIR=$(DIR)
LUA_DIR=$(DIR)/lua



CC=gcc
#CC=/mingw64/bin/gcc
INC=-I$(INC_DIR) -I./libubox -I./
LDFLAGS=-ljson
CFLAGS=-Wall -O2 -g -DDEBUG $(INC)  -DHAVE_CGI -DHAVE_TLS -std=gnu99
#-std=c99

ifeq ($(TARGET), Msys)
LDFLAGS += -lws2_32
OBJ_DIR=/d/dt_net_obj
else
CFLAGS +=-Wno-unused-result
endif


OBJS  = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(SRC_DIR)/*.c))
#OBJS  += $(patsubst $(LUA_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(LUA_DIR)/*.c))

UHTTPD=uhttpd

.PHONY: clean all rebuild

all: $(OBJ_DIR) $(UHTTPD)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(UHTTPD): $(OBJS)
	$(CC)  $(OBJS) -o $(UHTTPD) $(LDFLAGS) 



$(OBJ_DIR)/%.o:$(SRC_DIR)%.c
	$(CC) $< -o $@ -c $(CFLAGS)

#.c.o: $(OBJS)
#	$(CC) $< -o $@ -c $(CFLAGS)



rebuild: clean all

clean:
	-rm -rf $(OBJ_DIR)
	-rm -f $(UHTTPD)


