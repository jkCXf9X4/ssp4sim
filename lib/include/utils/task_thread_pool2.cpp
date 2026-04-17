#include "utils/task_thread_pool2.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <utility>

namespace ssp4sim::utils
{

    ThreadPool2::ThreadPool2(size_t num_threads) : dones(num_threads)
    {
        workers.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i)
        {
            dones[i] = false;
            workers.emplace_back(&ThreadPool2::worker_thread, this, static_cast<int>(i), std::ref(dones[i]));
        }
        LOG_DEBUG(log, "[{}] Threads started", __func__);
    }

    ThreadPool2::~ThreadPool2()
    {
        LOG_DEBUG(log, "[{}] Destroying threadpool", __func__);
        terminate.store(true);

        {
            std::lock_guard<std::mutex> lk(mtx);
            ++epoch;
        }
        cv.notify_all();

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

    void ThreadPool2::ready(int nodes)
    {
        IF_LOG({
            LOG_DEBUG(log, "[{}] Ready", __func__);
        });

        for (std::size_t i = 0; i < workers.size(); ++i)
        {
            dones[i] = false;
        }

        {
            std::lock_guard<std::mutex> lock(mtx);
            que = static_cast<size_t>(nodes);
            ++epoch;
        }
        cv.notify_all();
    }

    void ThreadPool2::enqueue(task_info &task)
    {
        IF_LOG({
            LOG_DEBUG(log, "[{}] Enqueueing task: {}", __func__, task.node->name);
        });

        {
            std::scoped_lock lock(queue_mutex);
            tasks.emplace(std::move(task));
        }
        IF_LOG({
            LOG_DEBUG(log, "[{}] Task queued: {}", __func__, task.node->name);
        });
    }

    void ThreadPool2::worker_thread(int id, std::atomic<bool> &done)
    {
        std::size_t my_epoch = 0;

        (void)id;

        while (true)
        {
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [&]
                        { return epoch != my_epoch; });
                my_epoch = epoch;
            }

            if (terminate.load())
            {
                break;
            }

            while (true)
            {
                std::optional<task_info> task;

                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    if (que == 0)
                    {
                        break;
                    }

                    if (!tasks.empty())
                    {
                        IF_LOG({
                            LOG_DEBUG(log, "[{}] Found new task, que {}", __func__, que);
                        });

                        task = std::move(tasks.top());
                        tasks.pop();
                        --que;
                    }
                }

                if (task)
                {
                    auto &t = task.value();
                    IF_LOG({
                        LOG_DEBUG(log, "[{}] Invoking {} {}", __func__, t.node->name, t.step.to_string());
                    });

                    t.node->invoke(t.step);
                    IF_LOG({
                        LOG_DEBUG(log, "[{}] Task completed {}", __func__, t.node->name);
                    });
                }
            }
            done = true;
        }

        LOG_DEBUG(log, "[{}] Thread finished", __func__);
    }

}
