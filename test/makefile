	
HEADERS = ../src/twc.h pipe_utes.h

TEST_MAIN_SRC=test_main.c

OBJECTS = twc.o pipe_utes.o

TEST=test_main

SRC_DIR=../src

VPATH=../src

CFLAGS=-I./../src

TEST_MAIN_SRC=$(shell pwd)/test_main.c
TEST_MAIN_OBJ=$(shell pwd)/test_main.o 
TEST_OBJECTS=$(TEST_MAIN_OBJ) $(OBJECTS)

CD=$(shell pwd)

all: $(TEST) 

clean:
	-rm -f $(OBJECTS)
	-rm -f $(TEST)
	-rm -f $(TEST_OBJECTS)
	
%.o: %.c $(HEADERS)
	${CC} ${CFLAGS} -g -c $< -o $@

$(TEST_MAIN_OBJ):$(HEADERS)
	gcc -g -c $(TEST_MAIN_SRC) -o $@ 

$(TEST): $(TEST_OBJECTS)
	gcc -g $(TEST_OBJECTS) -o $@ 

