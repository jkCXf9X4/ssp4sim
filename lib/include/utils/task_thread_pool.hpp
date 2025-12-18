
#pragma once

#include "ssp4sim_definitions.hpp"

#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <semaphore>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "cutecpp/log.hpp"

namespace ssp4sim::utils
{

    /**
     * @brief Simple thread pool for executing queued tasks.
     */
    class ThreadPool
    {
    private:
        Logger log = Logger("ssp4sim.utils.ThreadPool", LogLevel::info);

        std::mutex queue_mutex;
        std::vector<std::thread> workers;

        std::queue<std::function<void()>> tasks;
        std::counting_semaphore<> task_semaphore{0};
        bool stop = false;

    public:
        explicit ThreadPool(size_t num_threads);

        /**
         * @brief Stop all worker threads and wait for completion.
         */
        ~ThreadPool();

        /**
         * @brief Queue a new task for execution.
         *

         * @return Future representing the result of the task.
         */
        template <class F>
        auto enqueue(F &&f)
        {
            using return_type = std::invoke_result_t<F>;
            IF_LOG({
                log(debug)("[{}] Enqueueing task", __func__);
            });

            auto task = std::make_shared<std::packaged_task<return_type()>>(std::forward<F>(f));
            std::future<return_type> res = task->get_future();

            {
                std::scoped_lock lock(queue_mutex);
                tasks.emplace([task = std::move(task)]() mutable
                              { (*task)(); });
            }

            // notify a worker that one task is available
            task_semaphore.release();
            IF_LOG({
                log(debug)("[{}] Task queued", __func__);
            });

            return res;
        }

    private:
        /**
         * @brief Function executed by each worker thread to process tasks.
         */
        void worker_thread();
    };

} // namespace ssp4sim::utils
