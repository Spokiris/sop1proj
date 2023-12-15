#ifndef MUTEX_L_H
#define MUTEX_L_H

#include <pthread.h>    

extern pthread_mutex_t get_event_with_delay_mutex; // Mutex lock
extern pthread_mutex_t get_seat_with_delay_mutex; // Mutex lock
extern pthread_mutex_t seat_index_mutex; // Mutex lock
extern pthread_mutex_t ems_create_mutex; // Mutex lock
extern pthread_mutex_t ems_reserve_mutex; // Mutex lock
extern pthread_mutex_t ems_show_mutex; // Mutex lock
extern pthread_mutex_t ems_list_mutex; // Mutex lock
extern pthread_mutex_t ems_wait_mutex; // Mutex lock



#endif 