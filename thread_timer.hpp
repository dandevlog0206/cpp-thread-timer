/*************************************************************************/
/* property.hpp                                                          */
/*************************************************************************/
/* https://www.dandevlog.com/all/programming/890/                        */
/* https://github.com/dandevlog0206/cpp-thread-timer                     */
/*************************************************************************/
/* Copyright (c) 2024 www.dandevlog.com                                  */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#pragma once

#include <condition_variable>
#include <functional>
#include <thread>
#include <chrono>
#include <mutex>

class ThreadTimer {
public:
	enum Status {
		Stop,
		Start,
		Start_Once,
		Destroyed
	};

	inline ThreadTimer()
		: ThreadTimer(std::chrono::milliseconds(1000), {}) {}

	template <class Rep, class Period>
	inline ThreadTimer(std::chrono::duration<Rep, Period> interval, std::function<void()> callback, bool start = false)
		: status((Status)start), callback(callback), thread(timer_impl, this) {
		setInterval(interval);
	}

	inline ~ThreadTimer() {
		status = Destroyed;
		cd.notify_one();
		thread.join();
	}

	inline Status getStatus() const {
		return status;
	}

	template <class Rep, class Period>
	inline void setInterval(std::chrono::duration<Rep, Period> interval) {
		std::lock_guard<std::mutex> guard(mutex_params);
		this->interval = interval;
		if (status != Stop) cd.notify_one();
	}

	inline void setCallback(std::function<void()> callback) {
		std::lock_guard<std::mutex> guard(mutex_params);
		this->callback = callback;
	}

	inline void start() {
		std::lock_guard<std::mutex> guard(mutex_params);
		status = Start;
		cd.notify_one();
	}

	inline void start_once() {
		std::lock_guard<std::mutex> guard(mutex_params);
		status = Start_Once;
		cd.notify_one();
	}

	inline void stop() {
		std::lock_guard<std::mutex> guard(mutex_params);
		status = Stop;
		cd.notify_one();
	}

private:
	static void timer_impl(ThreadTimer* timer) {
		auto& mutex   = timer->mutex_cd;
		auto& cd      = timer->cd;
		auto& status  = timer->status;
		
		while (status != Destroyed) {
			if (status == Start || status == Start_Once) {
				std::unique_lock<std::mutex> lock(mutex);
				auto res = cd.wait_for(lock, timer->interval);
				if (res == std::cv_status::no_timeout) continue;

				timer->callback();
				if (status == Start_Once) status = Stop;
			} else if (status == Stop) {
				std::unique_lock<std::mutex> lock(mutex);
				cd.wait(lock); // idle
			}
		}
	}

private:
	Status                   status;
	std::chrono::nanoseconds interval;
	std::function<void()>    callback;
	std::mutex               mutex_params;
	std::mutex               mutex_cd;
	std::condition_variable  cd;
	std::thread              thread;
};