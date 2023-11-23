#include <limits.h>

#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <dirent.h>

#include <sys/types.h>

#include <sys/stat.h>

#include <fcntl.h>

#include <string.h>

#include "constants.h"

#include "operations.h"

#include "parser.h"

#define PATH_MAX 4096
#define DT_REG 8 // Directory entry is a regular file

int main(int argc, char *argv[])
{
  int fdin = 0;  // default to stdin -> to be used in FILESYS CMD_SHOW
  int fdout = 0; // default to stdout -> to be used in FILESYS CMD_SHOW

  unsigned int state_access_delay_ms =
      STATE_ACCESS_DELAY_MS; // Default delay value

  DIR *dir;                   // Directory stream
  struct dirent *entry;       // Directory entry
  const char *directory_path; // Directory path

  /* First argument is a FilePath */
  if (argc > 1)
  {
    directory_path = argv[1];
    dir = opendir(directory_path);

    if (dir == NULL)
    {
      perror("Invalid directory\n");
      return 1;
    }
  }

  /* Third argument is the Delay */
  if (argc > 3)
  {
    // int MAX_PROC = atoi(argv[2]);                   // Second argument is the
    // MAX number of simultaneos processes

    char *endptr; // Pointer to the end of the string
    unsigned long int delay = strtoul(argv[3], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX)
    {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_ms =
        (unsigned int)delay; // Convert delay to unsigned int
  }

  /* Warning to abusive usage */
  if (argc > 4)
  {
    fprintf(stderr, "Usage: %s <directory_path> <MAX_PROC> <delay>\n", argv[0]);
    return 1;
  }

  /* Initialize EMS */
  if (ems_init(state_access_delay_ms))
  {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  /* Main loop */
  while (1)
  {
    unsigned int event_id, delay;
    size_t num_rows, num_columns, num_coords;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];

    /* File loop only available w/args */
    if (argc > 1)
    {
      /* Read files in directory */
      while ((entry = readdir(dir)) != NULL)
      {

        if (entry->d_type == DT_REG)
        {

          const char *filename = entry->d_name;

          if (strstr(filename, ".jobs") != NULL)
          {

            char input_path[PATH_MAX];  // Input file path
            char output_path[PATH_MAX]; // Output file path

            snprintf(input_path, PATH_MAX, "%s/%s", directory_path,
                     filename); // Create input file path

            fdin = open(input_path, O_RDONLY); // Open input file descriptor

            if (fdin == -1)
            {
              perror("Failed to open file\n");
              return 1;
            }

            int cmd; // Command number

            /* Loop to keep reading the file until the end*/
            while ((cmd = get_next(fdin)) != EOC)
            {

              switch (cmd)
              {

              case CMD_CREATE:
                if (parse_create(fdin, &event_id, &num_rows, &num_columns) !=
                    0)
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
                num_coords = parse_reserve(fdin, MAX_RESERVATION_SIZE,
                                           &event_id, xs, ys);

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

                /* FILESYS CMD_SHOW */
              case CMD_SHOW:
                if (parse_show(fdin, &event_id) != 0)
                { // Parse the command
                  fprintf(stderr, "Invalid command. See HELP for usage\n");
                  continue;
                }

                char *dot =
                    strrchr(filename, '.'); // Find the last dot in the filename
                if (dot != NULL)
                {
                  *dot = '\0'; // Terminate the string at the dot
                }

                snprintf(output_path, PATH_MAX, "%s/%s.out", directory_path,
                         filename); // Create output file path

                fdout = open(output_path, O_WRONLY | O_CREAT | O_TRUNC,
                             0666); // Open output file file descriptor

                if (fdout == -1)
                {
                  perror("Failed to open output file");
                  return 1;
                }

                int stdout_save =
                    dup(fileno(stdout)); // Save standard output file descriptor
                                         // and duplicate it

                if (stdout_save == -1)
                {
                  perror("Failed to duplicate stdout");
                  return 1;
                }

                if (dup2(fdout, fileno(stdout)) ==
                    -1)
                { // Redirect standard output to fdout
                  perror("Failed to redirect stdout");
                  return 1;
                }

                if (ems_show(event_id))
                { // EMS_Show printed to standard output
                  fprintf(stderr, "Failed to show event\n");
                }

                fflush(stdout); // Flush standard output

                if (dup2(stdout_save, fileno(stdout)) ==
                    -1)
                { // Restore standard output file descriptor to its
                  // original value
                  perror("Failed to restore stdout");
                  return 1;
                }

                int sync = fsync(fdout); // Force File Sync to output file
                if (sync == -1)
                { // Force File Sync to output file
                  perror("Failed to fsync");
                  return 1;
                }

                close(
                    stdout_save); // Close saved standard output file descriptor
                close(fdout);     // Close output file descriptor
                break;

              case CMD_WAIT:
                if (parse_wait(fdin, &delay, NULL) ==
                    -1)
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
                printf("Available commands:\n"
                       "  CREATE <event_id> <num_rows> <num_columns>\n"
                       "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
                       "  SHOW <event_id>\n"
                       "  LIST\n"
                       "  WAIT <delay_ms> [thread_id]\n" // thread_id is not
                                                         // implemented
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
          close(fdin);
        }
      }
      closedir(dir);
    }

    /* Interactive terminal loop (maybe FIXME?) */
    printf("> ");
    fflush(stdout);

    switch (get_next(STDIN_FILENO))
    {
    case CMD_CREATE:
      if (parse_create(STDIN_FILENO, &event_id, &num_rows, &num_columns) != 0)
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
      num_coords =
          parse_reserve(STDIN_FILENO, MAX_RESERVATION_SIZE, &event_id, xs, ys);

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
      if (parse_show(STDIN_FILENO, &event_id) != 0)
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
      if (parse_wait(STDIN_FILENO, &delay, NULL) ==
          -1)
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
      printf("Available commands:\n"
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