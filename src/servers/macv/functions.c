#include "inc.h"
#include "structures.c"
#include <minix/config.h>
#include <stdio.h>

void initialize() // set values in structures to -1
{
    for(int i = 0; i < MAX_MUTEX_SLOTS; i++)
    {
        mutex_slots[i].mutex_id=-1;
        mutex_slots[i].owner=-1;
        mutex_slots[i].first=-1;
    }
    for (int i = 0; i < NR_PROCS; ++i) 
    {
        mutex_queue[i].who = -1;
        mutex_queue[i].prev = -1;
        mutex_queue[i].next = -1; 
	    cond_vars[i].cond_var_id = -1;
        cond_vars[i].who = -1;
        cond_vars[i].mutex_id = -1;
    }
}

int find(int mutex_id) // find mutex, return -1 if it doesn't exist
{ 
    int result = -1;
    for(int i = 0; i < MAX_MUTEX_SLOTS; i++)
    {
        if(mutex_slots[i].mutex_id == mutex_id && mutex_slots[i].owner != -1)
        {
            result = i;
        }
    }
    return result;
}

int find_empty(int where) // find empty slot in structure defined by "where"
{
    int idx;
    if(where == 0) // mutex_slots
    {
        for(int i = 0; i < MAX_MUTEX_SLOTS; i++)
        {
            if(mutex_slots[i].owner == -1)
            {
                idx = i;
                break;
            }
        }
        return idx;
    }
    else if(where == 1) // mutex_queue
    {
        for(int i = 0; i < NR_PROCS; i++)
        {
            if(mutex_queue[i].who == -1)
            {
                idx =i;
                break;
            }
        }
        return idx;
    }
    else if(where == 2) // cond_vars
    {
        for(int i = 0; i < NR_PROCS; i++)
        {
            if(cond_vars[i].who == -1)
            {
                idx = i;
                break;
            }
        }
        return idx;
    }
}

int Enqueue(endpoint_t who, int mutex_id, int to_send)
{
    int found = find(mutex_id);
    if(found == -1) // we didn't find it so we need to add it
    {
        int idx = find_empty(0);
        mutex_slots[idx].mutex_id = mutex_id;
        mutex_slots[idx].owner = who;
        mutex_slots[idx].first = -1;
        if(to_send) // we want to tell someone that he got mutex
        {
            message you_got_mutex;
            you_got_mutex.m_type = 0;
            sendnb(who, &you_got_mutex);
        }
        return 0;
    }

    // we found mutex in mutex_slots
    if(mutex_slots[found].first == -1) // noone in mutex_queue
    {
        int idx = find_empty(1);
        mutex_slots[found].first = idx; // adding index of first in queue
        mutex_queue[idx].who = who;
        mutex_queue[idx].prev = -1;
        mutex_queue[idx].next = -1;
    }
    else if(mutex_slots[found].first!=-1)
    {
        int idx = find_empty(1);
        int temp = mutex_slots[found].first;
        while(mutex_queue[temp].next != -1) // we are looking for the last in queue
        {
            temp = mutex_queue[temp].next;
        }
        mutex_queue[idx].prev = temp;
        mutex_queue[idx].next = -1; // we are last now
        mutex_queue[idx].who = who;
        mutex_queue[temp].next = idx;
    }
    return 1;
}

int Dequeue(endpoint_t who, int mutex_id)
{
    int found = find(mutex_id);
    if(found == -1 || (found != -1 && mutex_slots[found].owner != who))
    {
        return -1;
    }
    // we found it and we are the owners
    if(mutex_slots[found].first == -1)
    {
        mutex_slots[found].mutex_id = -1;
        mutex_slots[found].owner = -1;
        mutex_slots[found].first = -1;
        return 0;
    }
    int clean_me = mutex_slots[found].first;
    mutex_slots[found].owner = mutex_queue[clean_me].who;

    // telling new owner that he got the mutex
    message you_got_mutex;
    you_got_mutex.m_type = 0;
    sendnb(mutex_slots[found].owner, &you_got_mutex);
    
    int new_first = mutex_queue[clean_me].next;
    if(new_first == -1)
    {
        mutex_slots[found].first = -1; // queue is empty now
    }
    else
    {
        mutex_slots[found].first = new_first;
        mutex_queue[new_first].prev = -1;
    }
    mutex_queue[clean_me].who = -1;
    mutex_queue[clean_me].prev = -1;
    mutex_queue[clean_me].next = -1;
    return 0;
}

void check_queue(int proc_id)
{
    int idx = -1;
    for(int i = 0; i < NR_PROCS; i++)
    {
        if(mutex_queue[i].who == proc_id)
        {
            idx = i;
            break;
        }
    }
    if(idx != -1) // we found him so we want to send him a message to wake up
    {
        if(mutex_queue[idx].prev == -1 && mutex_queue[idx].next == -1) // he is alone
        {
            for(int j = 0; j < MAX_MUTEX_SLOTS; j++)
            {
                if(mutex_slots[j].first == idx)
                {
                    mutex_slots[j].first = -1; // queue is empty now
                    break;
                }
            }
            mutex_queue[idx].who = -1;
        }
        else if(mutex_queue[idx].prev == -1) // first in the queue (but not alone)
        {
            int second = mutex_queue[idx].next;
            for(int j = 0; j < MAX_MUTEX_SLOTS; j++)
            {
                if(mutex_slots[j].first == idx)
                {
                    mutex_slots[j].first = second; // second is now first
                    break;
                }
            }
            mutex_queue[idx].who = -1;
            mutex_queue[idx].next = -1;
            mutex_queue[second].prev = -1;
        }
        else if(mutex_queue[idx].next == -1) // last in the queue
        {
            int penultimate = mutex_queue[idx].prev;
            mutex_queue[penultimate].next = -1;
            mutex_queue[idx].who = -1;
            mutex_queue[idx].prev = -1;
        }
        else // somewhere in the middle of the queue
        {
            int before = mutex_queue[idx].prev;
            int after = mutex_queue[idx].next;
            mutex_queue[before].next = after;
            mutex_queue[after].prev = before;
            mutex_queue[idx].who = -1;
            mutex_queue[idx].prev = -1;
            mutex_queue[idx].next = -1;
        }
        message wake_up;
        wake_up.m_type = EINTR;
        sendnb(proc_id, &wake_up);
    }
}

void check_cond_vars(int proc_id)
{
    int proc_mutex;
    int idx = -1;
    for(int i = 0; i < NR_PROCS; i++)
    {
        if(cond_vars[i].who == proc_id)
        {
            idx = i;
            proc_mutex = cond_vars[i].mutex_id;
            break;
        }
    }
    if(idx != -1) // he was
    {
        cond_vars[idx].cond_var_id = -1;
        cond_vars[idx].who = -1;
        cond_vars[idx].mutex_id = -1;
        int idx = -1;
        for(int i = 0; i < MAX_MUTEX_SLOTS; i++)
        {
            if(mutex_slots[i].mutex_id == proc_mutex)
            {
                idx = i;
                break;
            }
        }
        if(idx != -1) // if proc_mutex already exists add him to queue
        {
            Enqueue(proc_id, proc_mutex, 0);
        }
        else
        {
            int idx = find_empty(0);
            mutex_slots[idx].mutex_id = proc_mutex;
            mutex_slots[idx].owner = proc_id;
            mutex_slots[idx].first = -1;

            message you_got_mutex;
            you_got_mutex.m_type = 0;
            sendnb(proc_id, &you_got_mutex);
        }
    }
}

void clean_mutex_slots(int proc_id)
{
    for(int i = 0; i < MAX_MUTEX_SLOTS; i++)
    {
        if(mutex_slots[i].owner == proc_id)
        {
            if(mutex_slots[i].first == -1) // noone in queue
            {
                mutex_slots[i].mutex_id = -1;
                mutex_slots[i].owner = -1;
                mutex_slots[i].first = -1;
            }
            else if(mutex_slots[i].first != -1)
            {
                int clean_me = mutex_slots[i].first;
                int second = mutex_queue[clean_me].next;
                if(second == -1)
                {
                    mutex_slots[i].first = -1;
                }
                else
                {
                    mutex_queue[second].prev = -1;
                    mutex_slots[i].first = second;
                }
                mutex_slots[i].owner = mutex_queue[clean_me].who;
                mutex_queue[clean_me].who = -1;
                mutex_queue[clean_me].prev = -1;
                mutex_queue[clean_me].next = -1;

                message you_got_mutex;
                you_got_mutex.m_type = 0;
                sendnb(mutex_slots[i].owner, &you_got_mutex);
            }
        }
    }
}

void clean_mutex_queue(int proc_id)
{
    int idx = -1;
    for(int i = 0; i < NR_PROCS; i++)
    {
        if(mutex_queue[i].who == proc_id)
        {
            idx = i;
            break;
        }
    }
    if(idx != -1) // we found him so we want to send him a message to wake up
    {
        if(mutex_queue[idx].prev == -1 && mutex_queue[idx].next == -1) // he is alone
        {
            for(int j = 0; j < MAX_MUTEX_SLOTS; j++)
            {
                if(mutex_slots[j].first == idx)
                {
                    mutex_slots[j].first = -1; // queue is empty now
                    break;
                }
            }
            mutex_queue[idx].who = -1;
        }
        else if(mutex_queue[idx].prev == -1) // first in the queue (but not alone)
        {
            int second = mutex_queue[idx].next;
            for(int j = 0; j < MAX_MUTEX_SLOTS; j++)
            {
                if(mutex_slots[j].first == idx)
                {
                    mutex_slots[j].first = second; // second is now first
                    break;
                }
            }
            mutex_queue[idx].who = -1;
            mutex_queue[idx].next = -1;
            mutex_queue[second].prev = -1;
        }
        else if(mutex_queue[idx].next == -1) // last in the queue
        {
            int penultimate = mutex_queue[idx].prev;
            mutex_queue[penultimate].next = -1;
            mutex_queue[idx].who = -1;
            mutex_queue[idx].prev = -1;
        }
        else // somewhere in the middle of the queue
        {
            int before = mutex_queue[idx].prev;
            int after = mutex_queue[idx].next;
            mutex_queue[before].next = after;
            mutex_queue[after].prev = before;
            mutex_queue[idx].who = -1;
            mutex_queue[idx].prev = -1;
            mutex_queue[idx].next = -1;
        }
    }
}

void clean_cond_vars(int proc_id)
{
    for(int i = 0; i < NR_PROCS; i++)
    {
        if(cond_vars[i].who == proc_id)
        {
            cond_vars[i].cond_var_id = -1;
            cond_vars[i].who = -1;
            cond_vars[i].mutex_id = -1;
		}
    }
}

int do_lock(message *m)
{
    endpoint_t who = m->m_source;
    int mutex_id = m->m1_i1;
    int result = Enqueue(who, mutex_id, 0);
    return result;
}

int do_unlock(message *m)
{
    endpoint_t who = m->m_source;
    int mutex_id = m->m1_i1;
    int result = Dequeue(who, mutex_id);
    return result;
}

int do_wait(message *m)
{
    endpoint_t who = m->m_source;
    int cond_var_id = m->m1_i1;
    int mutex_id = m->m1_i2;
    int check_mutex = Dequeue(who, mutex_id);
    if(check_mutex == -1) // who isn't owner of mutex_id
    {
        return -1;
    }
    int idx = find_empty(2);
    cond_vars[idx].cond_var_id = cond_var_id;
    cond_vars[idx].who = who;
    cond_vars[idx].mutex_id = mutex_id;
    return 0;
}

int do_broadcast(message *m)
{
    int cond_var_id = m->m1_i1;
    for(int i = 0;i < NR_PROCS; i++)
    {
        if(cond_vars[i].cond_var_id == cond_var_id)
        {
            Enqueue(cond_vars[i].who, cond_vars[i].mutex_id, 1);
            cond_vars[i].cond_var_id = -1;
            cond_vars[i].who = -1;
            cond_vars[i].mutex_id = -1;
        }
    }
    return 0;
}
