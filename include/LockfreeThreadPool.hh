#ifndef LOCKFREETHREADPOOL_H
#define LOCKFREETHREADPOOL_H

#include <iostream>
#include <chrono>
#include <thread>
#include <functional>
#include <vector>
#include <utility>
#include <future>

#include "concurrentqueue.h"

using namespace std;
using namespace moodycamel;

typedef std::function<void()> WorkType;

class LockfreeThreadPool {
public:
        LockfreeThreadPool(uint32_t numthreads);
        ~LockfreeThreadPool();

        void Enqueue(function<void()> task);
        void Worker();
        void JoinAll();
        void Finish();

        template <typename T, typename... Params>
        void ParallelFor(uint32_t begin, uint32_t end, int32_t n_tasks, T SerialFunction, Params&&... params) {

                n_tasks = (n_tasks >= m_nthreads) ? n_tasks : m_nthreads;
                int chunk = (end - begin) / n_tasks;
                for (int i = 0; i < n_tasks; ++i) {
                        m_promises.emplace_back();
                        int mypromise = m_promises.size() - 1;
                        m_taskQueue.enqueue([=]{
                                        uint32_t threadstart = begin + i*chunk;
                                        uint32_t threadstop = (i == n_tasks - 1) ? end : threadstart + chunk;
                                        for (uint32_t it = threadstart; it < threadstop; ++it) {
                                                SerialFunction(it, params...);
                                        }
                                        m_promises[mypromise].set_value();
                                });
                }
                Finish();
        }
        template<typename InputIt, typename T>
        void ParallelMap(InputIt begin, InputIt end, InputIt outputBegin, const std::function<T(T)>& func)
        {
            int chunkSize = (end - begin) / m_nthreads;
            for (int i = 0; i < m_nthreads; i++) {
                m_promises.emplace_back();
                int mypromise = m_promises.size() - 1;
                m_taskQueue.enqueue([=]{
                    InputIt threadBegin = begin + i*chunkSize;
                    InputIt threadOutput = outputBegin + i*chunkSize;
                    InputIt threadEnd = (i == m_nthreads - 1) ? end : threadBegin + chunkSize;
                    while (threadBegin != threadEnd) {
                        *(threadOutput++) = func(*(threadBegin++));
                    }
                    m_promises[mypromise].set_value();
                });
            }
            Finish();
        }

private:
        // threads and task queue
        int m_nthreads;
        vector<thread> m_workers;
        vector<promise<void>> m_promises;
        bool m_stopWorkers;

        ConcurrentQueue<WorkType> m_taskQueue;
};

#endif /* end of include guard: THREADPOOL_H */