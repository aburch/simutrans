/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "thread_pool.h"

#ifdef MULTI_THREAD

thread_pool_t::thread_pool_t(int thread_count) : stop(false), active_tasks(0), pending_tasks(0)
{
	pthread_mutex_init(&queue_mutex, nullptr);
	pthread_cond_init(&condition, nullptr);
	pthread_cond_init(&finished_condition, nullptr);

	workers.reserve(thread_count);
	for (int i = 0; i < thread_count; ++i) {
		pthread_t worker;
		pthread_create(&worker, nullptr, worker_thread, this);
		workers.push_back(worker);
	}
}

thread_pool_t::~thread_pool_t()
{
	// Signal all threads to stop
	pthread_mutex_lock(&queue_mutex);
	stop = true;
	pthread_cond_broadcast(&condition);
	pthread_mutex_unlock(&queue_mutex);

	// Wait for all threads to finish
	for (pthread_t worker : workers) {
		pthread_join(worker, nullptr);
	}

	// Clean up
	pthread_mutex_destroy(&queue_mutex);
	pthread_cond_destroy(&condition);
	pthread_cond_destroy(&finished_condition);
}

void* thread_pool_t::worker_thread(void* arg)
{
	thread_pool_t* pool = static_cast<thread_pool_t*>(arg);
	pool->worker_loop();
	return nullptr;
}

void thread_pool_t::worker_loop()
{
	while (true) {
		task_t task([]{});
		bool has_task = false;

		// Get next task from queue
		pthread_mutex_lock(&queue_mutex);
		while (tasks.empty() && !stop) {
			pthread_cond_wait(&condition, &queue_mutex);
		}

		if (stop && tasks.empty()) {
			pthread_mutex_unlock(&queue_mutex);
			break;
		}

		if (!tasks.empty()) {
			task = tasks.front();
			tasks.pop();
			pending_tasks--;
			active_tasks++;
			has_task = true;
		}

		pthread_mutex_unlock(&queue_mutex);

		// Execute task
		if (has_task) {
			task.func();

			// Mark task as completed
			pthread_mutex_lock(&queue_mutex);
			active_tasks--;
			if (active_tasks == 0 && pending_tasks == 0) {
				pthread_cond_broadcast(&finished_condition);
			}
			pthread_mutex_unlock(&queue_mutex);
		}
	}
}

void thread_pool_t::enqueue(std::function<void()> func)
{
	pthread_mutex_lock(&queue_mutex);
	
	if (!stop) {
		tasks.emplace(func);
		pending_tasks++;
		pthread_cond_signal(&condition);
	}
	
	pthread_mutex_unlock(&queue_mutex);
}

void thread_pool_t::wait_for_all()
{
	pthread_mutex_lock(&queue_mutex);
	
	while (active_tasks > 0 || pending_tasks > 0) {
		pthread_cond_wait(&finished_condition, &queue_mutex);
	}
	
	pthread_mutex_unlock(&queue_mutex);
}

#endif