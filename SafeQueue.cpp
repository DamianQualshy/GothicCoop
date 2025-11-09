#include <queue>
#include <mutex>
#include <condition_variable>

namespace GOTHIC_ENGINE {
#ifndef SAFE_QUEUE
#define SAFE_QUEUE
    template <class T>
    class SafeQueue
    {
    public:
        SafeQueue(void)
            : q()
            , m()
            , c()
        {}

        ~SafeQueue(void)
        {}

        void enqueue(T t)
        {
            std::lock_guard<std::mutex> lock(m);
            q.push(t);
            c.notify_one();
        }

        T dequeue(void)
        {
            std::unique_lock<std::mutex> lock(m);
            while (q.empty())
            {
                c.wait(lock);
            }
            T val = q.front();
            q.pop();
            return val;
        }

        bool isEmpty()
        {
            return q.empty();
        }

        int size()
        {
            return q.size();
        }

    private:
        std::queue<T> q;
        mutable std::mutex m;
        std::condition_variable c;
    };
#endif
}