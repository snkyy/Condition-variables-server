#include "functions.c"

int identifier = 0x1234;
int call_type, r;
endpoint_t who_e;
endpoint_t SELF_E;
static int verbose = 0;
message m;

/* SEF functions and variables. */
static void sef_local_startup(void);
static int sef_cb_init_fresh(int type, sef_init_info_t *info);
static void sef_cb_signal_handler(int signo);

int main(int argc, char *argv[]) 
{
    initialize();

    /* setup */
    env_setargs(argc, argv);
    sef_local_startup();

    while(TRUE) 
    {
        if((r = sef_receive(ANY, &m)) != OK)
        {
            fprintf(stderr, "receive failed %d.\n", r);
        }

        who_e = m.m_source;
        call_type = m.m_type;
        int result;

        if(who_e == PM_PROC_NR) // we got a message from PM
        {
            int proc_id = m.PM_PROC;
            if(m.m_type == PM_UNPAUSE)
            {
                check_cond_vars(proc_id);
                check_queue(proc_id);
            }
            else if(m.m_type == PM_EXIT || m.m_type ==  PM_DUMPCORE)
            {
                clean_mutex_slots(proc_id);
                clean_mutex_queue(proc_id);
                clean_cond_vars(proc_id);
            }
            continue;
        }
        switch (call_type) 
        {
            case 0:
                result = do_lock(&m);
                if(result == 0)
                {
                    m.m_type = result;
                    sendnb(who_e, &m);
                }
                break;
            case 1:
                result = do_unlock(&m);
                m.m_type = result;
                sendnb(who_e, &m);
                break;
            case 2:
                result = do_wait(&m);
                if(result == -1)
                {
                    m.m_type = result;
                    sendnb(who_e, &m);
                }
                break;
            case 3:
                do_broadcast(&m);
                m.m_type = 0;
                sendnb(who_e, &m);
                break;
        }
    }
    return -1;
}

static void sef_local_startup()
{
    /* Register init callbacks. */
    sef_setcb_init_fresh(sef_cb_init_fresh);
    sef_setcb_init_restart(sef_cb_init_fresh);

    /* No live update support for now. */

    /* Register signal callbacks. */
    sef_setcb_signal_handler(sef_cb_signal_handler);

    /* Let SEF perform startup. */
    sef_startup();
}

static int sef_cb_init_fresh(int UNUSED(type), sef_init_info_t *UNUSED(info))
{
    /* Initialize the macv server. */

    SELF_E = getprocnr();

    if(verbose)
    printf("MACV: self: %d\n", SELF_E);

    return(OK);
}

static void sef_cb_signal_handler(int signo)
{
    /* Only check for termination signal, ignore anything else. */
    if (signo != SIGTERM) return;
}
