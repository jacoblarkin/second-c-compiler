#CC = /opt/homebrew/bin/gcc-12
CC = clang
#CFLAGS = -Wall -Wextra -Wconversion -Wpedantic -Werror -std=c2x
CFLAGS = -Wall -Wextra -Wconversion -Wpedantic -std=c2x
#CFLAGS = -Wall -Wextra -Wconversion -Wpedantic -Wshadow -std=gnu2x -fanalyzer
LFLAGS = 

INCLUDES = 
LIBS = 

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
EXE = ccomp

all: $(EXE)
	@echo Compiler has been compiled! Executable is named $(EXE).

debug: CFLAGS += -DDEBUG -g -O0 -fsanitize=address 
debug: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(EXE) $(OBJS) $(LFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f *.o $(EXE)
