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

ems: main.c constants.h operations.o parser.o eventlist.o
	$(CC) $(CFLAGS) $(SLEEP) -o ems main.c operations.o parser.o eventlist.o

%.o: %.c %.h
	$(CC) $(CFLAGS) -c ${@:.o=.c}

run: ems
	@./ems ./jobs

clean:
	rm -f *.o ems
	rm ./jobs/*.out || rm ./jobs1/*.out

format:
	@which clang-format >/dev/null 2>&1 || echo "Please install clang-format to run this command"
	clang-format -i *.c *.h

test: ems
	@./ems ./jobs1 2 2
	@for file in ./jobs/*.out; do \
    	echo "Testing $$file..."; \
    	diff -q "$$file" "$${file%.out}.result" > /dev/null; \
    	 if [ $$? -eq 0 ]; then \
        	  echo "\033[0;32mTest passed\033[0m"; \
    	  else \
        	 echo "\033[0;31mTest failed\033[0m"; \
    	 fi; \
	  done