#ifndef RWL_H
#define RWL_H

extern void rwl_report();

extern void rwl_init();

extern void rwl_get_lock();
extern void rwl_put_lock();

extern void rwl_get_unlock();
extern void rwl_put_unlock();

#endif
