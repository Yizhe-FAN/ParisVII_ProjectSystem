# the compiler: gcc for C program
CC=gcc
CPP_FILES:=$(wildcard */*.c) 
OBJ_FILES:=$(addprefix obj/,$(notdir $(CPP_FILES:.c=.o)))
OBJ_DIR=./obj/
BIN_DIR=./bin/

TEST_FILES:=$(wildcard $(BIN_DIR)test*.exe)

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS=-std=gnu99 -g -O3 -ffast-math -pthread -Wall
GTKFLAGS=`pkg-config --cflags gtk+-3.0`

# the build target executable:
all: clean $(OBJ_DIR) main.exe

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

main.exe: $(BIN_DIR)
	$(CC) $(CFLAGS) $(GTKFLAGS) -o main.exe mandelbrot.c channel.c `pkg-config --libs gtk+-3.0` -lm
	$(CC) $(CFLAGS) $(GTKFLAGS) -o main_tube.exe mandelbrot_tube.c channel.c `pkg-config --libs gtk+-3.0` -lm
	$(CC) $(CFLAGS) -o example.exe example.c channel.c -lm
	$(CC) $(CFLAGS) -o channel_global_fork_test.exe channel_global_fork_test.c channel.c -lm
	$(CC) $(CFLAGS) $(GTKFLAGS) -o main_syc.exe mandelbrot_syc.c channel_syc.c `pkg-config --libs gtk+-3.0` -lm
	$(CC) $(CFLAGS) -o benchmark.exe benchmark.c channel.c -lm
#	$(CC) $(CFLAGS) $(GTKFLAGS) -o main_lockfree.exe mandelbrot_lockfree_mac.c channel_lockfree_mac.c `pkg-config --libs gtk+-3.0` -lm
#	$(CC) $(CFLAGS) -o benchmark_lockfree.exe benchmark_lockfree_mac.c channel_lockfree_mac.c -lm
clean   :
	$(RM) ./*.exe
