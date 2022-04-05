#define _SYSTEM 1
#define _MINIX 1
#include <sys/cdefs.h>
#include <lib.h>
#include "namespace.h"
#include <sys/cdefs.h>
#include <lib.h>
#include <unistd.h>
#include <minix/rs.h>
#include <errno.h>
#include <stdlib.h>

int cs_lock(int mutex_id)
{
	endpoint_t macv_pt;
	if(minix_rs_lookup("macv", &macv_pt) != OK)
	{
		errno = ENOSYS;
		return -1;
	}
	message m;
	m.m1_i1 = mutex_id;
	int result = _syscall(macv_pt, 0, &m);
	while(result == EINTR)
	{
		m.m1_i1 = mutex_id;
        	int result = _syscall(macv_pt, 0, &m);
	}
	return result;	
}

int cs_unlock(int mutex_id)
{
	endpoint_t macv_pt;
        if(minix_rs_lookup("macv", &macv_pt) != OK)
        {
                errno = ENOSYS;
                return -1;
        }
        message m;
        m.m1_i1 = mutex_id;
        int result = _syscall(macv_pt, 1, &m);
	if(result == -1)
	{
		errno = -EPERM;
	}
        return result;
}

int cs_wait(int cond_var_id, int mutex_id)
{
	endpoint_t macv_pt;
        if(minix_rs_lookup("macv", &macv_pt) != OK)
        {
                errno = ENOSYS;
                return -1;
        }
	message m;
	m.m1_i1 = cond_var_id;
	m.m2_i2 = mutex_id;
	int result = _syscall(macv_pt, 2, &m);
        while(result == EINTR) // take care of signal and go back to queue for mutex
        {
            m.m1_i1 = mutex_id;
            result = _syscall(macv_pt, 0, &m);
        }
        if(result == -1)
        {
                errno = -EINVAL;
		return result;
        }
        return result;
}

int cs_broadcast(int cond_var_id)
{
	endpoint_t macv_pt;
        if(minix_rs_lookup("macv", &macv_pt) != OK)
        {
                errno = ENOSYS;
                return -1;
        }
	message m;
        m.m1_i1 = cond_var_id;
	int result = _syscall(macv_pt, 3, &m);
	return result;
}
