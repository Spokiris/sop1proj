#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "constants.h"
#include "operations.h"
#include "parser.h"
#include <sched.h>

/*
  execute_coms(fd, delay) if fd == 0 standart input/output is used
*/
int execute_coms(int fd, unsigned int delay) {
  
  if (ems_init(delay))
  {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }
  
  while (1)
  {
    
    unsigned int event_id;
    
    size_t num_rows, num_columns, num_coords;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];

    printf("> ");
    fflush(stdout);

    switch (get_next(fd))
    {
    case CMD_CREATE:
      if (parse_create(fd, &event_id, &num_rows, &num_columns) != 0)
      {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (ems_create(event_id, num_rows, num_columns))
      {
        fprintf(stderr, "Failed to create event\n");
      }

      break;

    case CMD_RESERVE:
      num_coords = parse_reserve(fd, MAX_RESERVATION_SIZE, &event_id, xs, ys);

      if (num_coords == 0)
      {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (ems_reserve(event_id, num_coords, xs, ys))
      {
        fprintf(stderr, "Failed to reserve seats\n");
      }

      break;

    case CMD_SHOW:
      if (parse_show(fd, &event_id) != 0)
      {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (ems_show(event_id))
      {
        fprintf(stderr, "Failed to show event\n");
      }

      break;

    case CMD_LIST_EVENTS:
      if (ems_list_events())
      {
        fprintf(stderr, "Failed to list events\n");
      }

      break;

    case CMD_WAIT:
      if (parse_wait(fd, &delay, NULL) == -1)
      { // thread_id is not implemented
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (delay > 0)
      {
        printf("Waiting...\n");
        ems_wait(delay);
      }

      break;

    case CMD_INVALID:
      fprintf(stderr, "Invalid command. See HELP for usage\n");
      break;

    case CMD_HELP:
      printf(
          "Available commands:\n"
          "  CREATE <event_id> <num_rows> <num_columns>\n"
          "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
          "  SHOW <event_id>\n"
          "  LIST\n"
          "  WAIT <delay_ms> [thread_id]\n" // thread_id is not implemented
          "  BARRIER\n"                     // Not implemented
          "  HELP\n");

      break;

    case CMD_BARRIER: // Not implemented
    case CMD_EMPTY:
      break;

    case EOC:
      ems_terminate();
      return 0;
    }
  }
}

unsigned int parse_delay(char *argv)
{   
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;
  char *endptr;
  unsigned long int delay = strtoul(argv, &endptr, 10);

  if (*endptr != '\0' || delay > UINT_MAX)
  {
    fprintf(stderr, "Invalid delay value or value too large\n");
    return 1;
  }

    state_access_delay_ms = (unsigned int)delay;
    return state_access_delay_ms;
}

/*
main parses just the input
*/
int main(int argc, char *argv[])
{
  unsigned int delay = parse_delay(argv[1]);
  if(argc == 2)
  {
    execute_coms(0, delay); // executes program w/ parsed delay and standart input/output
  }

  if (argc > 2) // second arg is a PATH
  {
        DIR *currDir;
        struct dirent *entry;

        currDir = opendir(argv[2]); // open argv[2] directory

        if (currDir == NULL) {
            perror("Unable to read directory");
            return 1;
        }

        while ((entry = readdir(currDir)) != NULL) {
            
            char *ext = strrchr(entry->d_name, '.');
            if (ext != NULL && strcmp(ext, ".jobs") == 0) {
                
                int fd = open(entry->d_name, O_RDONLY);
                if (fd < 0) {
                    perror("open error");
                    return -1;
                }
                //create.out file FIXME
                execute_coms(fd, delay); // executes program w/ parsed delay and file input/output

                close(fd);
            }
        }

        closedir(currDir);
    }
    if(argc == 1)
    {
      execute_coms(0, 0);
    }
}

