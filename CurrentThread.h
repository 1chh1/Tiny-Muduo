#pragma once

namespace CurrentThread{
    extern __thread int t_cachetid;

    void cachetid();

    inline int tid()
    {
        if(__builtin_expect(t_cachetid == 0, 0))
        {
            cachetid();
        }
        return t_cachetid;
    }
}