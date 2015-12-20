#include "ifs/os.h"
#include "ifs/console.h"
#include <exlib/include/thread.h>
#include "console.h"
#include "map"

namespace fibjs
{

static exlib::Queue<AsyncEvent> s_acPool;

static int32_t s_threads;
static exlib::atomic s_idleThreads;

class _acThread: public exlib::OSThread
{
public:
    _acThread()
    {
        start();
    }

    virtual void Run()
    {
        AsyncEvent *p;

        Runtime rt;
        DateCache dc;
        rt.m_pDateCache = &dc;

        Runtime::reg(&rt);

        while (1)
        {
            if (s_idleThreads.inc() > s_threads * 3)
            {
                s_idleThreads.dec();
                break;
            }

            p = s_acPool.get();
            if (s_idleThreads.dec() == 0)
                new _acThread();

            p->invoke();
        }
    }
};

void AsyncEvent::async()
{
    s_acPool.put(this);
}

void init_acThread()
{
    int32_t cpus = 0;

    os_base::CPUs(cpus);
    if (cpus < 3)
        cpus = 3;

    s_threads = cpus * 3;
    new _acThread();
}

}
