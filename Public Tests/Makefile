CC = gcc

# Para mais informações sobre as flags de warning, consulte a informação adicional no lab_ferramentas
CFLAGS = -g -std=c17 -D_POSIX_C_SOURCE=200809L \
		 -Wall -Werror -Wextra \
		 -Wcast-align -Wconversion -Wfloat-equal -Wformat=2 -Wnull-dereference -Wshadow -Wsign-conversion -Wswitch-enum -Wundef -Wunreachable-code -Wunused \
		 -fsanitize=address -fsanitize=undefined 

ifneq ($(shell uname -s),Darwin) # if not MacOS
	CFLAGS += -fmax-errors=5
endif

all: ems 

test: test1 test2 test3 test4 test5 test6

ems: main.c constants.h operations.o parser.o eventlist.o
	$(CC) $(CFLAGS) $(SLEEP) -o ems main.c operations.o parser.o eventlist.o

%.o: %.c %.h
	$(CC) $(CFLAGS) -c ${@:.o=.c}

run: ems 
	@./ems

test1: ems
    @./ems ./jobs && echo "\033[0;32mtest1 passed\033[0m" || echo "\033[0;31mtest1 failed\033[0m"

test2: ems
    @./ems ./jobs 10 && echo "\033[0;32mtest2 passed\033[0m" || echo "\033[0;31mtest2 failed\033[0m"

test3: ems
    @./ems ./jobs 10 5 && echo "\033[0;32mtest3 passed\033[0m" || echo "\033[0;31mtest3 failed\033[0m"

test4: ems
    @./ems ./jobs 10 5 1 && echo "\033[0;32mtest4 passed\033[0m" || echo "\033[0;31mtest4 failed\033[0m"

test5: ems	
    @./ems ./jobs 10 10 && echo "\033[0;32mtest5 passed\033[0m" || echo "\033[0;31mtest5 failed\033[0m"

test6: ems
    @./ems /etc/shadow 10 10 && echo "\033[0;32mtest6 passed\033[0m" || echo "\033[0;31mtest6 failed\033[0m"