#pragma once
#include "queue"
#include "mutex"

template <typename T>
class SharedQueue
{
public:
    SharedQueue() = default;

    void Push(const T& item);
    void Push(T&& item);
    T Pop();
    T& Front();
    void Clear();
    bool Empty();
    size_t Size();

private:
    std::queue<T> m_Queue;
    std::mutex m_Mutex;
    std::condition_variable m_Cond;
};

template<typename T>
inline void SharedQueue<T>::Push(const T& item)
{
    std::unique_lock mlock(m_Mutex);
    m_Queue.push(item);
    m_Cond.notify_one();
}

template<typename T>
inline void SharedQueue<T>::Push(T&& item)
{
    std::unique_lock mlock(m_Mutex);
    m_Queue.push(item);
    m_Cond.notify_one();
}

template<typename T>
inline T SharedQueue<T>::Pop()
{
    std::unique_lock mlock(m_Mutex);
    m_Cond.wait(mlock, [this]() { return !m_Queue.empty(); });

    T item = m_Queue.front();
    m_Queue.pop();

    return item;
}

template<typename T>
inline T& SharedQueue<T>::Front()
{
    std::unique_lock mlock(m_Mutex);
    m_Cond.wait(mlock, [this]() { return !m_Queue.empty(); });
    return m_Queue.front();
}

template<typename T>
inline void SharedQueue<T>::Clear()
{
    if (Size() == 0) return;

    while (!Empty())
    {
        Pop();
    }
}

template<typename T>
inline bool SharedQueue<T>::Empty()
{
    std::unique_lock mlock(m_Mutex);
    bool empty = m_Queue.empty();
    return empty;
}

template<typename T>
inline size_t SharedQueue<T>::Size()
{
    std::unique_lock mlock(m_Mutex);
    size_t size = m_Queue.size();
    return size;
}
