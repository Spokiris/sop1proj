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

#include <sys/wait.h>

#include <pthread.h>

#include "mutex_l.h"

#define PATH_MAX 4096
#define DT_REG 8 // Directory entry is a regular file

pthread_mutex_t cebola = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
  int fdin;
  int fdout;
} thread_args;  

// pthread_mutex_t lock; // Mutex lock

void* run_ems(void *args) 
{
  (void)args;
  thread_args *t_args = (thread_args*)args;
  int fdin = t_args->fdin;
  
  int fdout = t_args->fdout;

  
  unsigned int event_id, delay;
  size_t num_rows, num_columns, num_coords;
  size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];

   
  
  int cmd; // Command number 
 
  while ((cmd = get_next(fdin)) != EOC)
  {
    printf("cmd %d\n", cmd);  
    switch (cmd)
    {

    case CMD_CREATE:
  ;
      if (parse_create(fdin, &event_id, &num_rows, &num_columns))
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
      printf("CMD_SHOW\n");
    
      if (parse_show(fdin, &event_id) != 0)
      { // Parse the command
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }
      
      if (ems_show(event_id, fdout) != 0)
      { // EMS_Show printed to standard output
        fprintf(stderr, "Failed to show event\n");
      }

      break;

    case CMD_LIST_EVENTS:
      if (ems_list_events(fdout))
      {
        fprintf(stderr, "Failed to list events\n");
      }

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

    case CMD_BARRIER: //  implemented
        ems_barrier();
        break;

    case CMD_EMPTY:
      break;

    case EOC:
      ems_terminate();
    }
  }
  free(args);
  close(fdin);      // Close input file descriptor
  close(fdout);     // Close output file descriptor
  return NULL;
}


int main(int argc, char *argv[])
{


  int fdin = 0;  // default to stdin -> to be used in FILESYS CMD_SHOW && CMD_LIST_EVENTS
  int fdout = 0; // default to stdout -> to be used in FILESYS CMD_SHOW && CMD_LIST_EVENTS




  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS; // Default delay value



  DIR *dir;                   // Directory stream
  struct dirent *entry;       // Directory entry
  const char *directory_path; // Directory path




  int MAX_PROC = 0; // MAX number of simultaneos processes
  int active_proc;  // Number of active processes
  pid_t pid;        // Process ID
  pid_t wpid;       // Wait Process ID
  int status;       // Process status
  // int files = 0;




 
  int MAX_THREADS = 0;    // MAX number of simultaneos threads


  /* First argument is a FilePath */
  if (argc > 1)
  {
    directory_path = argv[1];
    dir = opendir(directory_path);

    if (dir == NULL)
    {
      perror("Invalid directory\n");
      exit(1);
    }
  }

  /* Second argument is the MAX number of simultaneos processes */
  if (argc > 2)
  {
    MAX_PROC = atoi(argv[2]); // Second argument is the MAX number of simultaneos processes
    active_proc = 0;

    if (MAX_PROC < 0)
    {
      perror("Invalid MAX number of simultaneos processes\n");
      exit(1);
    }
  }

  if (argc > 3)
  {
    MAX_THREADS = atoi(argv[3]); // Second argument is the MAX number of simultaneos processes
    if (MAX_THREADS < 0)
    {
      perror("Invalid MAX number of threads\n");
      exit(1);
    }
  }

  /* Third argument is the Delay */
  if (argc > 4)
  {
    char *endptr; // Pointer to the end of the string
    unsigned long int delay = strtoul(argv[3], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX)
    {
      fprintf(stderr, "Invalid delay value or value too large\n");
      exit(1);
    }

    state_access_delay_ms =
        (unsigned int)delay; // Convert delay to unsigned int
  }

  /* Warning to abusive usage */
  if (argc > 5)
  {
    fprintf(stderr, "Usage: %s <directory_path> <MAX_PROC> <delay>\n", argv[0]);
    exit(1);
  }

  /* Initialize EMS */
  if (ems_init(state_access_delay_ms))
  {
    fprintf(stderr, "Failed to initialize EMS\n");
    exit(1);
  }

  pthread_mutex_init(&cebola, NULL);  // Initialize mutex lock
  /* Main loop */
  while (1)
  {
    /* File loop only available w/args */
    if (argc > 1)
    {
      /* Read files in directory */
      while ((entry = readdir(dir)) != NULL)
      {
        printf("%s\n", entry->d_name);
        
        if (entry->d_type == DT_REG)
        {

          const char *filename = entry->d_name;

          if (strstr(filename, ".jobs") != NULL)
          {

            char input_path[PATH_MAX];  // Input file path
            char output_path[PATH_MAX]; // Output file path
            // input file descriptor
            snprintf (input_path, PATH_MAX, "%s/%s", directory_path,
                     filename); // Create input file path

            fdin = open(input_path, O_RDONLY); // Open input file descriptor

            if (fdin == -1)
            {
              perror("Failed to open file\n");
              return 1;
            }
            // input file descriptor
            
            // output file descriptor
            char *dot = strrchr(filename, '.'); // Find the last dot in the filename
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
            // output file descriptor

            /*PID INIT*/
            active_proc++; // Increase the number of active processes
            //files += 1;
            //printf("%d",files);
            pid = fork();  // Create a new process

            /*PID CHECK*/
            if (pid == 0)
            { // Check if the process is a child

              pthread_t threads[MAX_THREADS]; // Thread ID
              
              for(int i = 0; i < MAX_THREADS; i++)
              {
                thread_args *args = malloc(sizeof(thread_args));
                args->fdin = fdin;
                args->fdout = fdout;
                pthread_create(&threads[i], NULL, run_ems, args);
              }

              for(int i = 0; i < MAX_THREADS; i++)
              {
                pthread_join(threads[i], NULL);
              } 
              active_proc--;    // Decrease the number of active processes
              exit(status);     // Exit the child process
            }

            
            else if (pid > 0)
            { // Check if the process is a parent
              if (active_proc == MAX_PROC)
              {                                  // Check if the MAX number of simultaneos processes was reached
                wpid = waitpid(-1, &status, WNOHANG);   // Wait for any child process to finish
                active_proc--;                   // Decrease the number of active processes
              }

              if (WIFEXITED(status))
              { // Check if the child process exited normally

                printf("Child process %d exited with status %d\n", wpid, WEXITSTATUS(status));
              }
              else if (WIFSIGNALED(status))
              { // Check if the child process was terminated by a signal

                printf("Child process %d was terminated by signal %d\n", wpid, WTERMSIG(status));
              }
               while (active_proc > 0)
                    { // Wait for all child processes to finish CHECK POINT
                      waitpid(0, &status, WNOHANG);
                      active_proc--;
                    }
            }
            else if (wpid == -1)
            {
              perror("waitpid"); // Error while waiting for child process
            }
            else
            {
              perror("Fork failed"); // Fork failed
              return 1;
            }
          }
        }
      }

      

      closedir(dir); // Close directory stream
      return 0;      // Exit the program
    }
  }
}