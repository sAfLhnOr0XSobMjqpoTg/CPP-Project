#include "../include/ThreadManager.h"
#include <stdexcept>

ThreadManager::ThreadManager(size_t numThreads) 
    : numThreads(numThreads), running(false) {
    if (numThreads <= 0) {
        throw std::invalid_argument("Number of threads must be positive");
    }
    threadLoads.resize(numThreads, 0);
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

    // Create worker threads
    threads.resize(numThreads);
    for (size_t i = 0; i < numThreads; ++i) {
        threads[i] = std::thread(&ThreadManager::workerThread, this, i);
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
            thread.join();
        }
    }
    threads.clear();
}

void ThreadManager::addTask(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(taskMutex);
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
            taskCondition.wait(lock);
        }

        if (!running || taskQueue.empty()) {
            return;
        }

        task = std::move(taskQueue.front());
        taskQueue.pop();
    }

    task();
}

void ThreadManager::workerThread(size_t threadIndex) {
    while (running) {
        processNextTask();
    }
}