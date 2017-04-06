DIR=$(shell pwd)
TARGET=$(shell uname -o)

DIR=.
OBJ_DIR=./obj

#INC_DIR=$(DIR)/inc
SRC_DIR=$(DIR)/
#JSON_DIR=$(DIR)/json
#TEST_DIR=$(DIR)/testing/c
#
#FWD_DIR=$(SRC_DIR)/fwd
#MASTER_DIR=$(SRC_DIR)/master
#UTILS_DIR=$(SRC_DIR)/utils

#INC=-I$(INC_DIR) \
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


ifdef chp2_use_array
	CFLAGS += -DDS_CHP2_USE_ARRAY
else
	CFLAGS += -DDS_CHP2_USE_ARRAY
endif


ifdef debug
CFLAGS += -DDEBUG -g
LDFLAGS += -rdynamic
else
CFLAGS += -DNDEBUG -Os
#CFLAGS += -DNDEBUG -g -rdynamic
endif

CFLAGS +=-Wno-unused-result


JSON_OBJS  += $(patsubst $(JSON_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(JSON_DIR)/*.c))
FWD_OBJS = $(patsubst $(FWD_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(FWD_DIR)/*.c))
MASTER_OBJS =$(patsubst $(MASTER_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(MASTER_DIR)/*.c))
UTILS_OBJS =$(patsubst $(UTILS_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(UTILS_DIR)/*.c))
TEST_OBJS =$(patsubst $(TEST_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(TEST_DIR)/*.c))


TARGETS=chp2_array

.PHONY: clean all rebuild

all: $(OBJ_DIR) $(TARGETS)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)


chp2_array: $(OBJ_DIR)/chp2_array.o
	$(CC) -o chp2_array $(LDFLAGS)  $(OBJ_DIR)/chp2_array.o
	
$(OBJ_DIR)/%.o:$(SRC_DIR)/%.c 
	$(CC) -c $(CFLAGS) $< -o $@ 

$(OBJ_DIR)/%.o:$(JSON_DIR)/%.c
	$(CC) -c $(CFLAGS) $< -o $@ 

$(OBJ_DIR)/%.o:$(TEST_DIR)/%.c
	$(CC) -c $(CFLAGS) -I$(TEST_DIR) $< -o $@ 

rebuild: clean all

clean:
	-rm -rf $(OBJ_DIR)
	-rm -rf $(TARGETS)
