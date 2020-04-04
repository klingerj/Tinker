#include "System/WorkerThreadPool.h"

int main()
{
    g_WorkerThreadPool.NewJob([]() {
        int a = 0;
        int b = 1;
        return a + b;
        });

    exit(EXIT_SUCCESS);
}
