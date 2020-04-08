#include "../Include/System/WorkerThreadPool.h"
#include <iostream>

int main()
{
    volatile int a = 0;
    {
        WorkerThreadPool pool;
        pool.Startup(16);

        constexpr uint16 numJobs = 1024;
        WorkerJob* jobs[numJobs];
        for (uint16 i = 0; i < numJobs; ++i)
        {
            jobs[i] = pool.NewJob([&]() {
                a += 1;
            });
        }
        
        for (uint16 i = 0; i < numJobs; ++i)
        {
            while (!jobs[i]->m_done);
            delete jobs[i];
        }

        pool.Shutdown();
        std::cout << "A: " << a << std::endl;
    }

    exit(EXIT_SUCCESS);
}
