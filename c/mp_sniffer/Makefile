
DIR=$(shell pwd)
#TARGET=$(shell uname -o)
use_mipsbacktrace=1


OBJ_DIR=$(DIR)/obj

INC_DIR=$(DIR)/inc
SRC_DIR=$(DIR)/src
JSON_DIR=$(DIR)/json

UTILS_DIR=$(SRC_DIR)/utils
EVENT_DIR=$(SRC_DIR)/event
CORE_DIR=$(SRC_DIR)/core

INC=-I$(INC_DIR) \
	-I$(JSON_DIR)	\
	-I$(UTILS_DIR) \
	-I$(EVENT_DIR) \

INC_CORE=-I$(CORE_DIR)

ifdef sniffer_build
CFLAGS  +=$(SNIFFER_CFLAGS)
LDFLAGS +=$(SNIFFER_LDFLAGS)
else
CFLAGS  +=$(shell pcap-config --cflags)
LDFLAGS +=$(shell pcap-config --libs)
endif

CFLAGS  += -Wall $(INC)  
#CFLAGS += -Wextra
CFLAGS  += -std=gnu99 
CFLAGS  += -D_FILE_OFFSET_BITS=64
CFLAGS  += -DUSE_EMS_MEMORY_MGMT
#LDFLAGS +=-lrt


ifeq ($(TARGETTYPE), 7620n)
	CFLAGS += -DBOARD_MT7620N
	CFLAGS += -Wno-unused-result
else ifeq ($(TARGETTYPE), 9344)
	CFLAGS += -DBOARD_AR9344 -I$(TOOLCHAINBINDIR)/include 
	LDFLAGS +=-L$(TOOLCHAINBINDIR)/lib -lpthread
else
	CFLAGS += -DGENERIC_LINUX
	CFLAGS += -Wno-unused-result
	use_mipsbacktrace=0
	CC = gcc
endif


ifdef log
CFLAGS += -DEMS_LOGGER_FILE
endif


ifdef release
use_mipsbacktrace=0
CFLAGS += -DNDEBUG -Os
else
CFLAGS += -DDEBUG -g -rdynamic
LDFLAGS += -rdynamic

ifeq ($(use_mipsbacktrace), 1)
	BACKTRACE_DIR=$(UTILS_DIR)/backtrace
	CFLAGS += -I$(BACKTRACE_DIR) -fexceptions
	LDFLAGS += -fexceptions -ldl
endif
endif

ifdef syslog
CFLAGS += -DEMS_USE_SYSLOG
endif

ifdef mempool
CFLAGS += -DUSE_MEM_POOL

ifdef mempool_trace
CFLAGS += -DMEM_POOL_TRACE
endif
endif


JSON_OBJS  = $(patsubst $(JSON_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(JSON_DIR)/*.c))
UTILS_OBJS =$(patsubst $(UTILS_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(UTILS_DIR)/*.c))
EVENT_OBJS =$(patsubst $(EVENT_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(EVENT_DIR)/*.c))
CORE_OBJS =$(patsubst $(CORE_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(CORE_DIR)/*.c))

ifeq ($(use_mipsbacktrace), 1)
BACKTRACE_OBJS=$(patsubst $(BACKTRACE_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(BACKTRACE_DIR)/*.c))
else
BACKTRACE_OBJS=
endif

SNIFFER=sniffer

.PHONY: clean all rebuild

install:all
all: $(OBJ_DIR) $(SNIFFER) 

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(SNIFFER): $(JSON_OBJS) $(UTILS_OBJS) $(EVENT_OBJS) $(CORE_OBJS) $(BACKTRACE_OBJS)
	$(CC) -o $(SNIFFER) \
		$(JSON_OBJS) \
		$(UTILS_OBJS) \
		$(EVENT_OBJS) \
		$(CORE_OBJS) \
		$(BACKTRACE_OBJS) \
		$(LDFLAGS) 

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

$(OBJ_DIR)/%.o:$(JSON_DIR)/%.c
	$(CC) -c $(CFLAGS) \
		$< -o $@ 

rebuild: clean all

clean:
	-rm -rf $(OBJ_DIR)
	-rm -rf $(SNIFFER)
