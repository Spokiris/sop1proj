CC = gcc

# Para mais informações sobre as flags de warning, consulte a informação adicional no lab_ferramentas
CFLAGS = -g -std=c17 -D_POSIX_C_SOURCE=200809L \
		 -Wall -Werror -Wextra \
		 -Wcast-align -Wconversion -Wfloat-equal -Wformat=2 -Wnull-dereference -Wshadow -Wsign-conversion -Wswitch-enum -Wundef -Wunreachable-code -Wunused \
		 -fsanitize=address -fsanitize=undefined

ifneq ($(shell uname -s),Darwin) # if not MacOS
	CFLAGS += -fmax-errors=5
endif

all: ems run_jobs

ems: main.c constants.h operations.o parser.o eventlist.o
	$(CC) $(CFLAGS) $(SLEEP) -o ems main.c operations.o parser.o eventlist.o

%.o: %.c %.h
	$(CC) $(CFLAGS) -c ${@:.o=.c}

run: ems
	@./ems

run_jobs: ems
	for file in jobs/*.jobs; do \
		./ems < $$file > $${file%.jobs}.out; \
		sed -i 's/>//g' $${file%.jobs}.out; \
	done

clean:
	rm -f *.o ems

format:
	@which clang-format >/dev/null 2>&1 || echo "Please install clang-format to run this command"
	clang-format -i *.c *.h
