
#pragma once

#include "ssp4sim_definitions.hpp"
#include "invocable.hpp"

#include <atomic>
#include <cstdint>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <semaphore>
#include <stack>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "cutecpp/log.hpp"

#include "invocable.hpp"

namespace ssp4sim::utils
{
    struct task_info
    {
        ssp4sim::graph::Invocable *node;
        ssp4sim::graph::StepData step;
    };
    /**
     * @brief Simple thread pool for executing queued tasks.
     */
    class ThreadPool2
    {
    private:
        Logger log = Logger("ssp4sim.utils.ThreadPool2", LogLevel::info);

        std::mutex mtx;
        std::condition_variable cv;

        std::mutex queue_mutex;
        std::vector<std::thread> workers;

        std::stack<task_info> tasks;

        size_t que = 0;
        std::size_t epoch = 0; //  gate flag shared by all threads

        std::atomic<bool> terminate{false};

    public:
        std::vector<std::atomic<bool>> dones;

        explicit ThreadPool2(size_t num_threads);

        /**
         * @brief terminate all worker threads and wait for completion.
         */
        ~ThreadPool2();

        void ready(int nodes);

        /**
         * @brief Queue a new task for execution.
         *

         * @return Future representing the result of the task.
         */
        void enqueue(task_info &task);

    private:
        /**
         * @brief Function executed by each worker thread to process tasks.
         */
        void worker_thread(int id, std::atomic<bool> &done);
    };

} // namespace ssp4sim::utils
