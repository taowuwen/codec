
DIR=$(shell pwd)
TARGET=$(shell uname -o)

DIR=.
OBJ_DIR=/tmp/fwdobj

INC_DIR=$(DIR)/inc
SRC_DIR=$(DIR)/src
JSON_DIR=$(DIR)/json
TEST_DIR=$(DIR)/testing/c

FWD_DIR=$(SRC_DIR)/fwd
MASTER_DIR=$(SRC_DIR)/master
UTILS_DIR=$(SRC_DIR)/utils

INC=-I$(INC_DIR) \
	-I${JSON_DIR} \
	-I$(SRC_DIR) \
	-I$(FWD_DIR) \
	-I$(MASTER_DIR) \
	-I$(UTILS_DIR)

CFLAGS  = -Wall $(INC)  
CFLAGS += -std=gnu99 
CFLAGS += -DUSE_EPOLL
CFLAGS += -D_FILE_OFFSET_BITS=64
LDFLAGS=-lrt -lm -ldl -lpthread
CC = gcc


ifdef usetls
CFLAGS += -DHAVE_TLS
LDFLAGS += -lssl -lcrypt
endif


ifdef useselect
LDFLAGS +=-lpthread
CFLAGS += -DDT_USE_SELECT
else
CFLAGS += -DDT_USE_EPOLL
endif


ifdef debug
CFLAGS += -DDEBUG -g
LDFLAGS += -rdynamic
else
CFLAGS += -DNDEBUG -Os
#CFLAGS += -DNDEBUG -g -rdynamic
endif


ifeq ($(TARGET), Msys)
LDFLAGS += -lws2_32 
OBJ_DIR=/d/dt_net_obj
else
CFLAGS +=-Wno-unused-result
endif


JSON_OBJS  += $(patsubst $(JSON_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(JSON_DIR)/*.c))
FWD_OBJS = $(patsubst $(FWD_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(FWD_DIR)/*.c))
MASTER_OBJS =$(patsubst $(MASTER_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(MASTER_DIR)/*.c))
UTILS_OBJS =$(patsubst $(UTILS_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(UTILS_DIR)/*.c))

TEST_OBJS =$(patsubst $(TEST_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(TEST_DIR)/*.c))


FWD_MASTER=fwd_master
FWD_CHILD=fwd
FWD_CM_TESTING=fwd_cm_testing

.PHONY: clean all rebuild

all: $(OBJ_DIR) $(FWD_MASTER) $(FWD_CHILD) $(FWD_CM_TESTING)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(FWD_MASTER): $(MASTER_OBJS) $(UTILS_OBJS)
	$(CC)  $(MASTER_OBJS) $(UTILS_OBJS) \
		-o $(FWD_MASTER) $(LDFLAGS)

$(FWD_CHILD): $(FWD_OBJS) $(JSON_OBJS) $(UTILS_OBJS)
	$(CC)  $(FWD_OBJS) $(JSON_OBJS) $(UTILS_OBJS) \
		-o $(FWD_CHILD) $(LDFLAGS)

$(FWD_CM_TESTING): $(TEST_OBJS)	$(JSON_OBJS) $(UTILS_OBJS)
	$(CC)   $(TEST_OBJS)   		\
		$(JSON_OBJS) 		\
		$(UTILS_OBJS)		\
		$(OBJ_DIR)/fwd_sock.o	\
		$(OBJ_DIR)/fwd_buffer.o	\
		$(OBJ_DIR)/fwd_event.o	\
		$(OBJ_DIR)/fwd_transinfo.o	\
		-o $(FWD_CM_TESTING) $(LDFLAGS)
	

$(OBJ_DIR)/%.o:$(FWD_DIR)/%.c 
	$(CC) -c $(CFLAGS) $< -o $@ 

$(OBJ_DIR)/%.o:$(MASTER_DIR)/%.c 
	$(CC) -c $(CFLAGS) $< -o $@ 

$(OBJ_DIR)/%.o:$(UTILS_DIR)/%.c 
	$(CC) -c $(CFLAGS) $< -o $@ 

$(OBJ_DIR)/%.o:$(SRC_DIR)/%.c 
	$(CC) -c $(CFLAGS) $< -o $@ 

$(OBJ_DIR)/%.o:$(JSON_DIR)/%.c
	$(CC) -c $(CFLAGS) $< -o $@ 

$(OBJ_DIR)/%.o:$(TEST_DIR)/%.c
	$(CC) -c $(CFLAGS) -I$(TEST_DIR) $< -o $@ 

rebuild: clean all

clean:
	-rm -rf $(OBJ_DIR)
	-rm -f $(FWD_MASTER)
	-rm -f $(FWD_CHILD)
	-rm -f $(FWD_CM_TESTING)
	-rm -rf /tmp/fwd_child_*.log
	-rm -rf /tmp/t_fwd_cm*.log
	-rm -rf /tmp/fwd_logger.log


