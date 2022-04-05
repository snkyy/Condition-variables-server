#include <stdio.h>
#include <minix/config.h>
#include <minix/endpoint.h>

struct mutex
{
    int mutex_id;
    endpoint_t owner; // current owner of this mutex
    int first; // index of first person in queue for this mutex
};

struct queue
{
    endpoint_t who; // who is waiting
    int prev;
    int next;
};

struct cond_var
{
    int cond_var_id;
    endpoint_t who; // who is waiting
    int mutex_id; // what was his mutex
};

struct mutex mutex_slots[1024];
struct queue mutex_queue[NR_PROCS];
struct cond_var cond_vars[NR_PROCS];
