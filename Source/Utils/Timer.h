#pragma once

#include <chrono>

class Timer {
private:
    using Clock = std::chrono::high_resolution_clock;
    using Time = std::chrono::time_point<Clock>;

    Time m_startTime;
    Time m_endTime;

public:
    void Start()
    {
        m_startTime = Clock::now();
    }

    void Stop()
    {
        m_endTime = Clock::now();
    }
    
    float ElapsedTime()
    {
        return ((std::chrono::duration<float, std::milli>)(m_endTime - m_startTime)).count();
    }
};
