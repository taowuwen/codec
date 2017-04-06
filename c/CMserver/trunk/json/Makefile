
SRC=$(shell ls *.c 2>/dev/null)
SRCPP=$(shell ls *.cpp 2>/dev/null)
OBJ=$(SRC:.c=.o) $(SRCPP:.cpp=.o)
TARG=libjson.a
CC=gcc;
GCC=g++
all: $(TARG)
$(TARG):$(OBJ)
	ar rcs $(TARG) $(OBJ)
%.o:%.c 
	@echo $(CC) -c $< $(CFLAGS) -o $@
	@$(CC) -c $< $(CFLAGS) -o $@  $(INCLUDEDIR)
#	@cp *.o $(BUILD_DIR)
%.o:%.cpp 
	@echo $(GCC) -c $< $(CXXFLAGS) -o $@
	@$(GCC) -c $< $(CXXFLAGS) -o $@  $(INCLUDEDIR)
#	@cp *.o $(BUILD_DIR)
.PHONY:clean
clean:
	-rm *.o *.a
