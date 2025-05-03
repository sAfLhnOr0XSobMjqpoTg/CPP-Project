#include "../include/ThreadManager.h"
#include <stdexcept>
#include <thread>
#include <chrono>
#include <future>

// Constants for thread management
constexpr size_t MAX_TASK_QUEUE_SIZE = 1000;
constexpr size_t THREAD_TIMEOUT_MS = 100;

ThreadManager::ThreadManager(size_t numThreads) 
    : numThreads(numThreads), running(false), maxThreads(numThreads) {
    if (numThreads <= 0) {
        throw std::invalid_argument("Number of threads must be positive");
    }
    threadLoads.resize(numThreads, 0);
    threads.reserve(numThreads);
}

ThreadManager::~ThreadManager() {
    stop();
}

void ThreadManager::start() {
    if (running) {
        return;
    }

    running = true;
    activeThreads = 0;
    threadLoads.resize(numThreads, 0);
    threads.clear();

    // Create worker threads
    for (size_t i = 0; i < numThreads; ++i) {
        std::string threadName = "WorkerThread-" + std::to_string(i);
        threads.emplace_back([this, i, threadName]() {
            std::this_thread::name = threadName;
            workerThread(i);
        });
    }
}

void ThreadManager::stop() {
    {
        std::unique_lock<std::mutex> lock(taskMutex);
        running = false;
        taskCondition.notify_all();
    }

    // Join all threads
    for (auto& thread : threads) {
        if (thread.joinable()) {
            try {
                thread.join();
            } catch (const std::exception& e) {
                std::cerr << "Error joining thread: " << e.what() << std::endl;
            }
        }
    }
    threads.clear();
}

void ThreadManager::addTask(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(taskMutex);
        if (taskQueue.size() >= MAX_TASK_QUEUE_SIZE) {
            throw std::runtime_error("Task queue is full");
        }
        taskQueue.push(std::move(task));
    }
    taskCondition.notify_one();
}

bool ThreadManager::isRunning() const {
    return running;
}

void ThreadManager::setNumThreads(size_t newNumThreads) {
    if (newNumThreads <= 0) {
        throw std::invalid_argument("Number of threads must be positive");
    }
    if (newNumThreads > maxThreads) {
        throw std::invalid_argument("Cannot exceed maximum thread limit");
    }

    {
        std::unique_lock<std::mutex> lock(taskMutex);
        if (running) {
            stop();
        }
        numThreads = newNumThreads;
        threadLoads.resize(numThreads, 0);
    }
    start();
}

void ThreadManager::waitForCompletion() {
    std::unique_lock<std::mutex> lock(taskMutex);
    while (!taskQueue.empty()) {
        taskCondition.wait(lock);
    }
}

size_t ThreadManager::getActiveThreadCount() const {
    return activeThreads;
}

void ThreadManager::processNextTask() {
    std::function<void()> task;
    {
        std::unique_lock<std::mutex> lock(taskMutex);
        while (running && taskQueue.empty()) {
            if (taskCondition.wait_for(lock, std::chrono::milliseconds(THREAD_TIMEOUT_MS)) == 
                std::cv_status::timeout) {
                return;
            }
        }

        if (!running || taskQueue.empty()) {
            return;
        }

        task = std::move(taskQueue.front());
        taskQueue.pop();
    }

    try {
        task();
    } catch (const std::exception& e) {
        std::cerr << "Error executing task: " << e.what() << std::endl;
    }
}

void ThreadManager::workerThread(size_t threadIndex) {
    while (running) {
        activeThreads++;
        processNextTask();
        activeThreads--;
        threadLoads[threadIndex]++;
    }
}