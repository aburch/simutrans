/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef UTILS_THREAD_POOL_H
#define UTILS_THREAD_POOL_H

#ifdef MULTI_THREAD

#include <pthread.h>
#include <vector>
#include <queue>
#include <functional>
#include "../utils/simthread.h"

/**
 * Simple thread pool implementation for parallel lambda execution
 * Provides basic parallel execution capabilities with wait-for-completion
 */
class thread_pool_t
{
private:
	struct task_t {
		std::function<void()> func;
		task_t(std::function<void()> f) : func(f) {}
	};

	std::vector<pthread_t> workers;
	std::queue<task_t> tasks;
	pthread_mutex_t queue_mutex;
	pthread_cond_t condition;
	pthread_cond_t finished_condition;
	bool stop;
	int active_tasks;
	int pending_tasks;

	static void* worker_thread(void* arg);
	void worker_loop();

public:
	/**
	 * Constructor - creates thread pool with specified number of worker threads
	 * @param thread_count Number of worker threads to create
	 */
	thread_pool_t(int thread_count);

	/**
	 * Destructor - stops all threads and cleans up resources
	 */
	~thread_pool_t();

	/**
	 * Submit a lambda function for parallel execution
	 * @param func Lambda function to execute
	 */
	void enqueue(std::function<void()> func);

	/**
	 * Wait for all submitted tasks to complete
	 */
	void wait_for_all();

	/**
	 * Get the number of worker threads
	 */
	int get_thread_count() const { return workers.size(); }
};

#else

// Dummy implementation for single-threaded builds
class thread_pool_t
{
public:
	thread_pool_t(int /*thread_count*/) {}
	~thread_pool_t() {}
	
	void enqueue(std::function<void()> func) {
		func(); // Execute immediately in single-threaded mode
	}
	
	void wait_for_all() {
		// Nothing to wait for in single-threaded mode
	}
	
	int get_thread_count() const { return 1; }
};

#endif

#endif