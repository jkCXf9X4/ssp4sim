#include "utils/task_thread_pool.hpp"

#include <cstddef>
#include <functional>
#include <mutex>
#include <utility>

namespace ssp4sim::utils
{

    ThreadPool::ThreadPool(size_t num_threads)
    {
        workers.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i)
        {
            workers.emplace_back([this]
                                 { worker_thread(); });
        }
        LOG_DEBUG(log, "[{}] Threads started", __func__);
    }

    ThreadPool::~ThreadPool()
    {
        LOG_DEBUG(log, "[{}] Destroying threadpool", __func__);
        stop = true;

        task_semaphore.release(static_cast<std::ptrdiff_t>(workers.size()));

        LOG_DEBUG(log, "[{}] Waiting for all tasks to complete", __func__);
        for (std::thread &worker : workers)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }
        LOG_DEBUG(log, "[{}] Threadpool successfully destroyed", __func__);
    }

    void ThreadPool::worker_thread()
    {
        while (true)
        {
            task_semaphore.acquire();

            if (stop)
            {
                break;
            }

            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                if (!tasks.empty())
                {
                    IF_LOG({
                        LOG_DEBUG(log, "[{}] Task starting", __func__);
                    });

                    task = std::move(tasks.front());
                    tasks.pop();
                }
            }

            if (task)
            {
                task();
                IF_LOG({
                    LOG_DEBUG(log, "[{}] Task completed", __func__);
                });
            }
        }

        LOG_DEBUG(log, "[{}] Thread finished", __func__);
    }

}
