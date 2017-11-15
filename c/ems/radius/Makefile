
DIR=$(shell pwd)
TARGET=$(shell uname -o)

SHELL=/bin/bash

DIR=.
OBJ_DIR=/tmp/radius_obj

LIB_DIR=$(DIR)/src
TEST_DIT=$(DIR)/example

INC=-I$(LIB_DIR)


CFLAGS  = -Wall $(INC)  
#CFLAGS += -Wextra
CFLAGS += -std=gnu99 
CFLAGS += -D_FILE_OFFSET_BITS=64
LDFLAGS=-lrt -lm -ldl -lpthread

CC = gcc

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

$(OBJ_DIR)/%.o:$(TEST_DIT)/%.c 
	$(CC) -c $(CFLAGS) \
		$< -o $@ 

$(OBJ_DIR)/%.o:$(LIB_DIR)/%.c 
	$(CC) -c $(CFLAGS) \
		$< -o $@ 


rebuild: clean all

clean:
	-rm -rf $(OBJ_DIR)
	-rm -f $(EMS_AAA)

