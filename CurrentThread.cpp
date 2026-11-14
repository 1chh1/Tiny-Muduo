#include "CurrentThread.h"
#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread{
    __thread int t_cachetid = 0;

    void cachetid()
    {
        if(t_cachetid == 0)
        {
            // 通过 Linux 系统调用，获取当前线程的 tid 值
            t_cachetid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}