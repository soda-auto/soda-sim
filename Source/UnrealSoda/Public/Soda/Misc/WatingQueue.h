// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include <mutex>
#include <condition_variable>
#include <deque>

template < class T >
class UNREALSODA_API TWatingQueue
{

public:
	void Push(const T& el)
	{
		std::unique_lock< std::mutex > lock(Mutex);
		Queue.push_back(el);
		Cv.notify_one();
	}

	template< class... Args >
	void EmplacePush(Args&&... args)
	{
		std::unique_lock< std::mutex > lock(Mutex);
		Queue.emplace_back(args...);
		Cv.notify_one();
	}

	bool PopFront(T& Data, uint32 Timeout)
	{
		std::unique_lock< std::mutex > lock(Mutex);
		auto now = std::chrono::system_clock::now();
		while (!Queue.size())
		{
			if (Cv.wait_until(lock, now + std::chrono::milliseconds(Timeout)) == std::cv_status::timeout) return false;
		}
		Data = Queue.front();
		Queue.pop_front();
		return true;
	}

	bool PopBack(T& Data, uint32 Timeout)
	{
		std::unique_lock< std::mutex > lock(Mutex);
		auto now = std::chrono::system_clock::now();
		while (!Queue.size())
		{
			if (Cv.wait_until(lock, now + std::chrono::milliseconds(Timeout)) == std::cv_status::timeout) return false;
		}
		Data = Queue.back();
		Queue.pop_back();
		return true;
	}

	std::size_t Size() const
	{
		std::unique_lock< std::mutex > lock(Mutex);
		return Queue.size();
	}

	mutable std::mutex Mutex;
	std::deque< T > Queue;

private:
	std::condition_variable Cv;
};