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
#define DT_REG 8

int main(int argc, char * argv[]) {
    int fdin = 0;
    int fdout = 0;

    unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;
    // int MAX_PROC = atoi(argv[1]);
    DIR * dir;
    struct dirent * entry;
    const char * directory_path;

    if (argc > 1) {
        directory_path = argv[1];
        dir = opendir(directory_path);

        if (dir == NULL) {
            perror("Invalid directory\n");
            return 1;
        }

    }

    if (argc > 2) {
        char * endptr;
        unsigned long int delay = strtoul(argv[2], & endptr, 10);

        if ( * endptr != '\0' || delay > UINT_MAX) {
            fprintf(stderr, "Invalid delay value or value too large\n");
            return 1;
        }

        state_access_delay_ms = (unsigned int) delay;
    }

    if (ems_init(state_access_delay_ms)) {
        fprintf(stderr, "Failed to initialize EMS\n");
        return 1;
    }

    while (1) {
        unsigned int event_id, delay;
        size_t num_rows, num_columns, num_coords;
        size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
        
        while ((entry = readdir(dir)) != NULL) {
            if (entry -> d_type == DT_REG) {

                const char * filename = entry -> d_name;

                if (strstr(filename, ".jobs") != NULL) {
                    char input_path[PATH_MAX];
                    char output_path[PATH_MAX];

                    snprintf(input_path, PATH_MAX, "%s/%s", directory_path, filename);

                    fdin = open(input_path, O_RDONLY);
                    
                    if (fdin == -1) {
                        perror("Failed to open file\n");
                        return 1;
                    }

                    int cmd;
                    while ((cmd = get_next(fdin)) != EOC) {
                        
                        switch (cmd) {

                        case CMD_CREATE:
                            if (parse_create(fdin, & event_id, & num_rows, & num_columns) != 0) {
                                fprintf(stderr, "Invalid command. See HELP for usage\n");
                                continue;
                            }

                            if (ems_create(event_id, num_rows, num_columns)) {
                                fprintf(stderr, "Failed to create event\n");
                            }

                            break;

                        case CMD_RESERVE:
                            num_coords = parse_reserve(fdin, MAX_RESERVATION_SIZE, & event_id, xs, ys);

                            if (num_coords == 0) {
                                fprintf(stderr, "Invalid command. See HELP for usage\n");
                                continue;
                            }

                            if (ems_reserve(event_id, num_coords, xs, ys)) {
                                fprintf(stderr, "Failed to reserve seats\n");
                            }

                            break;

                        case CMD_SHOW:
                            if (parse_show(fdin, & event_id) != 0) {
                                fprintf(stderr, "Invalid command. See HELP for usage\n");
                                continue;
                            }

                            char * dot = strrchr(filename, '.');
                            if (dot != NULL) {
                                * dot = '\0'; // Terminate the string at the dot
                            }

                            snprintf(output_path, PATH_MAX, "%s/%s.out", directory_path, filename);

                            fdout = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                            if (fdout == -1) {
                                perror("Failed to open output file");
                                return 1;
                            }

                            int stdout_save = dup(STDOUT_FILENO);
                            dup2(fdout, STDOUT_FILENO);

                            if (ems_show(event_id)) {
                                fprintf(stderr, "Failed to show event\n");
                            }

                            dup2(stdout_save, STDOUT_FILENO);

                            if (fsync(fdout) == -1) {
                                perror("Failed to fsync");
                                return 1;
                            }

                            close(stdout_save);
                            close(fdout);
                            break;

                        case CMD_LIST_EVENTS:
                            if (ems_list_events()) {
                                fprintf(stderr, "Failed to list events\n");
                            }

                            break;

                        case CMD_WAIT:
                            if (parse_wait(fdin, & delay, NULL) == -1) { // thread_id is not implemented
                                fprintf(stderr, "Invalid command. See HELP for usage\n");
                                continue;
                            }

                            if (delay > 0) {
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
                                "  BARRIER\n" // Not implemented
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
        printf("> ");
        fflush(stdout);

        switch (get_next(STDIN_FILENO)) {
        case CMD_CREATE:
            if (parse_create(STDIN_FILENO, & event_id, & num_rows, & num_columns) != 0) {
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                continue;
            }

            if (ems_create(event_id, num_rows, num_columns)) {
                fprintf(stderr, "Failed to create event\n");
            }

            break;

        case CMD_RESERVE:
            num_coords = parse_reserve(STDIN_FILENO, MAX_RESERVATION_SIZE, & event_id, xs, ys);

            if (num_coords == 0) {
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                continue;
            }

            if (ems_reserve(event_id, num_coords, xs, ys)) {
                fprintf(stderr, "Failed to reserve seats\n");
            }

            break;

        case CMD_SHOW:
            if (parse_show(STDIN_FILENO, & event_id) != 0) {
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                continue;
            }

            if (ems_show(event_id)) {
                fprintf(stderr, "Failed to show event\n");
            }

            break;

        case CMD_LIST_EVENTS:
            if (ems_list_events()) {
                fprintf(stderr, "Failed to list events\n");
            }

            break;

        case CMD_WAIT:
            if (parse_wait(STDIN_FILENO, & delay, NULL) == -1) { // thread_id is not implemented
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                continue;
            }

            if (delay > 0) {
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
                "  BARRIER\n" // Not implemented
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