#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "eventlist.h"
#include "mutex_l.h"

static struct EventList *event_list = NULL;
static unsigned int state_access_delay_ms = 0;

// pthread_mutex_t cebola;


/// Calculates a timespec from a delay in milliseconds.
/// @param delay_ms Delay in milliseconds.
/// @return Timespec with the given delay.
static struct timespec delay_to_timespec(unsigned int delay_ms)
{
  return (struct timespec){delay_ms / 1000, (delay_ms % 1000) * 1000000};
}

/// Gets the event with the given ID from the state.
/// @note Will wait to simulate a real system accessing a costly memory
/// resource.
/// @param event_id The ID of the event to get.
/// @return Pointer to the event if found, NULL otherwise.
static struct Event *get_event_with_delay(unsigned int event_id)
{
  pthread_mutex_lock(&cebola);
  struct timespec delay = delay_to_timespec(state_access_delay_ms);
  nanosleep(&delay, NULL); // Should not be removed
  struct Event *event = get_event(event_list, event_id);
  pthread_mutex_unlock(&cebola);
  return event;
}

/// Gets the seat with the given index from the state.
/// @note Will wait to simulate a real system accessing a costly memory
/// resource.
/// @param event Event to get the seat from.
/// @param index Index of the seat to get.
/// @return Pointer to the seat.
static unsigned int *get_seat_with_delay(struct Event *event, size_t index)
{
  pthread_mutex_lock(&tomate);
  struct timespec delay = delay_to_timespec(state_access_delay_ms);
  nanosleep(&delay, NULL); // Should not be removed
  unsigned int *seat = &event->data[index];
  pthread_mutex_unlock(&tomate);
  return seat;
}

/// Gets the index of a seat.
/// @note This function assumes that the seat exists.
/// @param event Event to get the seat index from.
/// @param row Row of the seat.
/// @param col Column of the seat.
/// @return Index of the seat.
static size_t seat_index(struct Event *event, size_t row, size_t col)
{
  pthread_mutex_lock(&alface);
  size_t index = (row - 1) * event->cols + col - 1;
  pthread_mutex_unlock(&alface);
  return index;
}

int ems_init(unsigned int delay_ms)
{
  
  if (event_list != NULL)
  {
    fprintf(stderr, "EMS state has already been initialized\n");
    return 1;
  }

  event_list = create_list();
  state_access_delay_ms = delay_ms;
  int result = event_list == NULL;
  
  return result;
}

int ems_terminate()
{
  pthread_mutex_lock(&couve);
  if (event_list == NULL)
  {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }

  free_list(event_list);
  pthread_mutex_unlock(&couve);
  pthread_mutex_destroy(&couve);
  return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols)
{
  
  pthread_mutex_lock(&rucula);
  if (event_list == NULL)
  {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }

  if (get_event_with_delay(event_id) != NULL)
  {
    fprintf(stderr, "Event already exists\n");
    return 1;
  }

  struct Event *event = malloc(sizeof(struct Event));

  if (event == NULL)
  {
    fprintf(stderr, "Error allocating memory for event\n");
    return 1;
  }

  event->id = event_id;
  event->rows = num_rows;
  event->cols = num_cols;
  event->reservations = 0;
  event->data = malloc(num_rows * num_cols * sizeof(unsigned int));

  if (event->data == NULL)
  {
    fprintf(stderr, "Error allocating memory for event data\n");
    free(event);
    return 1;
  }

  for (size_t i = 0; i < num_rows * num_cols; i++)
  {
    event->data[i] = 0;
  }

  if (append_to_list(event_list, event) != 0)
  {
    fprintf(stderr, "Error appending event to list\n");
    free(event->data);
    free(event);
    return 1;
  }
  
  pthread_mutex_unlock(&rucula);
  
  return 0;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t *xs,
                size_t *ys)
{
  pthread_mutex_lock(&broculos);
  
  if (event_list == NULL)
  {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }

  struct Event *event = get_event_with_delay(event_id);

  if (event == NULL)
  {
    fprintf(stderr, "Event not found\n");
    return 1;
  }

  unsigned int reservation_id = ++event->reservations;

  size_t i = 0;
  for (; i < num_seats; i++)
  {
    size_t row = xs[i];
    size_t col = ys[i];

    if (row <= 0 || row > event->rows || col <= 0 || col > event->cols)
    {
      fprintf(stderr, "Invalid seat\n");
      break;
    }

    if (*get_seat_with_delay(event, seat_index(event, row, col)) != 0)
    {
      fprintf(stderr, "Seat already reserved\n");
      break;
    }

    *get_seat_with_delay(event, seat_index(event, row, col)) = reservation_id;
  }

  // If the reservation was not successful, free the seats that were reserved.
  if (i < num_seats)
  {
    event->reservations--;
    for (size_t j = 0; j < i; j++)
    {
      *get_seat_with_delay(event, seat_index(event, xs[j], ys[j])) = 0;
    }
    return 1;
  }
  pthread_mutex_unlock(&broculos);
  return 0;
}

int ems_show(unsigned int event_id, int fd)
{
  pthread_mutex_lock(&couve);
  char buffer[256];
  
  if (event_list == NULL)
  {
    strcpy(buffer, "EMS state must be initialized\n");
    write(fd, buffer, strlen(buffer));
    return 1;
  }

  struct Event *event = get_event_with_delay(event_id);

  if (event == NULL)
  {
    strcpy(buffer, "Event not found\n");
    write(fd, buffer, strlen(buffer));
    return 2;
  }

  for (size_t i = 1; i <= event->rows; i++)
  {
    for (size_t j = 1; j <= event->cols; j++)
    {
      unsigned int *seat = get_seat_with_delay(event, seat_index(event, i, j));
      sprintf(buffer, "%u", *seat);
      write(fd, buffer, strlen(buffer));

      if (j < event->cols)
      {
        write(fd, " ", 1);
      }
    }

    write(fd, "\n", 1);
  }
  pthread_mutex_unlock(&couve);
  return 0;
}

int ems_list_events(int fd)
{
  pthread_mutex_lock(&espinafre);
  char buffer[256];
  
  if (event_list == NULL)
  {
    strcpy(buffer, "EMS state must be initialized\n");
    write(fd, buffer, strlen(buffer));
    return 1;
  }

  if (event_list->head == NULL)
  {
    strcpy(buffer, "No events\n");
    write(fd, buffer, strlen(buffer));
    return 0;
  }

  struct ListNode *current = event_list->head;
  while (current != NULL)
  {
    strcpy(buffer, "Event: ");
    sprintf(buffer + strlen(buffer), "%u\n", (current->event)->id);
    write(fd, buffer, strlen(buffer));
    current = current->next;
  }
  pthread_mutex_unlock(&espinafre);
  return 0;
}

int ems_wait(unsigned int delay_ms)
{
  pthread_mutex_lock(&cebolinho);
  struct timespec delay = delay_to_timespec(delay_ms);
  nanosleep(&delay, NULL);
  pthread_mutex_unlock(&cebolinho);
  return 0;
}




static pthread_mutex_t barrier_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t barrier_cond = PTHREAD_COND_INITIALIZER;
static int barrier_count = 0;
static int barrier_holder = 0;

void ems_barrier()
{
  pthread_mutex_lock(&barrier_mutex);
  barrier_count++;

  if (barrier_count == barrier_holder)
  {
    pthread_cond_broadcast(&barrier_cond);
    barrier_count = 0;
  }
  else
  {
    pthread_cond_wait(&barrier_cond, &barrier_mutex);
  }

  pthread_mutex_unlock(&barrier_mutex);
}