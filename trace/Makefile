SRC = $(wildcard *.c)
DIR = $(notdir ${SRC})
OBJ = $(patsubst %.c,%.o,$(notdir ${SRC}))

TARGET = main

CC = gcc
CFLAGS = -g -Wall

${TARGET}:${OBJ}
	@$(CC) $(OBJ) -o $@

${DIR_OBJ}/%.o:${DIR_SRC}/%.c
	@$(CC) $(CFLAGS) -c $< -o $@

.PHONY:all clean
clean:
	rm -rf *.o
	rm -rf ${TARGET}
	rm -rf *~
	rm -rf utils/*~
	rm -rf *.smr
	rm -rf *.nsw
	rm -rf *.txt
all:
	@echo $(SRC)
	@echo $(DIR)
	@echo $(OBJ)
