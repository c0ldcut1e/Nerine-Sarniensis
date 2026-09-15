#ifndef BACKGROUND_SCANNER_HPP
#define BACKGROUND_SCANNER_HPP

#include <chrono>
#include <mutex>
#include <thread>

template<typename T>
class BackgroundScanner
{
public:
    BackgroundScanner(T (*scan)(), int intervalMilliseconds);

    T Get();

private:
    static void ThreadMain(BackgroundScanner<T> *scanner);

    T (*m_scan)();
    int m_intervalMilliseconds;
    std::mutex m_mutex;
    T m_value;
    bool m_threadStarted;
};

template<typename T>
BackgroundScanner<T>::BackgroundScanner(T (*scan)(), int intervalMilliseconds)
{
    m_scan                 = scan;
    m_intervalMilliseconds = intervalMilliseconds;
    m_threadStarted        = false;
}

template<typename T>
T BackgroundScanner<T>::Get()
{
    if (!m_threadStarted)
    {
        m_threadStarted = true;
        std::thread(ThreadMain, this).detach();
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_value;
}

template<typename T>
void BackgroundScanner<T>::ThreadMain(BackgroundScanner<T> *scanner)
{
    while (true)
    {
        T value = scanner->m_scan();

        {
            std::lock_guard<std::mutex> lock(scanner->m_mutex);
            scanner->m_value = value;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(scanner->m_intervalMilliseconds));
    }
}

#endif
