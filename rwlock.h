/*!
\file rwlock.h
\brief Platform independent Read-Write Lock implementation using C++11, working
	in one of three modes: reader priority, writer priority or fair priority.
	Implementation of the description provided on Wikipedia:
	https://en.wikipedia.org/wiki/Readers%E2%80%93writers_problem
\authors Konstantin A. Pankov, explorus@mail.ru
\copyright MIT License
\version 1.0
\date 05/07/2020
\warning In developing. Not a stable tested code.

The MIT License

Copyright(c) 2018 Konstantin Pankov, explorus@mail.ru

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#ifndef _CRWLOCK_H_
#define _CRWLOCK_H_

#include <mutex>
#include <condition_variable>

/*!
\brief semaphore class
\description The semaphore appeared in the C ++ 20 standard and at the time of
	writing this code is still not implemented in the standard library in 
	Visual Studio, so we need to have our own implementation, especially since
	it is quite simple.
*/
class Semaphore {
	size_t m_avail{ 1 }; // binary semaphore
	std::mutex m_mutex;
	std::condition_variable m_cv;
public:
	void Acquire()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_cv.wait(lock, [this] { return m_avail > 0; });
		m_avail--;
	}

	void Release()
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_avail++;
		}
		m_cv.notify_one();
	}
};

/*!
\brief RWLockMode enumeration contains RWLock operating modes
*/
typedef enum
{
	ReadersPriority,
	WritersPriority,
	FairOrder
} RWLockMode;

/*!
\brief Read-Write Lock class
You can use it as follows:
\code
	RWLock g_lock = RWLock(FairOrder);

	void writer()
	{
		g_lock.AcquireExclusive();
		// writing is performed
		g_lock.ReleaseExclusive();
	}

	void reader()
	{
		g_lock.AcquireShared();
		//reading is performed
		g_lock.ReleaseShared();
	}
\endcode
*/
class RWLock
{
	RWLockMode m_mode;
	Semaphore m_order, m_resource, m_rmutex, m_wmutex;
	long m_readcount{ 0 };
	long m_writecount{ 0 };

	void AcquireExclusive1()
	{
		m_resource.Acquire();
	}

	void AcquireExclusive2()
	{
		m_wmutex.Acquire();
		if (++m_writecount == 1)
			m_order.Acquire();
		m_wmutex.Release();
		m_resource.Acquire();
	}

	void AcquireExclusive3()
	{
		m_order.Acquire();
		m_resource.Acquire();
		m_order.Release();
	}

	void ReleaseExclusive1()
	{
		m_resource.Release();
	}

	void ReleaseExclusive2()
	{
		m_resource.Release();
		m_wmutex.Acquire();
		if (--m_writecount == 0)
			m_order.Release();
		m_wmutex.Release();
	}

	void ReleaseExclusive3()
	{
		m_resource.Release();
	}

	void AcquireShared1()
	{
		m_rmutex.Acquire();
		if (++m_readcount == 1)
			m_resource.Acquire();
		m_rmutex.Release();
	}

	void AcquireShared2()
	{
		m_order.Acquire();
		m_rmutex.Acquire();
		if (++m_readcount == 1)
			m_resource.Acquire();
		m_rmutex.Release();
		m_order.Release();
	}

	void AcquireShared3()
	{
		m_order.Acquire();
		m_rmutex.Acquire();
		if (++m_readcount == 1)
			m_resource.Acquire();
		m_order.Release();
		m_rmutex.Release();
	}

	void ReleaseShared1()
	{
		m_rmutex.Acquire();
		if (--m_readcount == 0)
			m_resource.Release();
		m_rmutex.Release();
	}

	void ReleaseShared2()
	{
		m_rmutex.Acquire();
		if (--m_readcount == 0)
			m_resource.Release();
		m_rmutex.Release();
	}

	void ReleaseShared3()
	{
		m_rmutex.Acquire();
		if (--m_readcount == 0)
			m_resource.Release();
		m_rmutex.Release();
	}
public:
	RWLock(RWLockMode mode = FairOrder) : m_mode(mode) {}
	RWLock(const RWLock&) = delete;
	const RWLock& operator=(const RWLock&) = delete;

	// Exclusive (writers)
	void AcquireExclusive()
	{
		switch (m_mode)
		{
		case ReadersPriority: return AcquireExclusive1();
		case WritersPriority: return AcquireExclusive2();
		default: return AcquireExclusive3();
		}
	}

	void ReleaseExclusive()
	{
		switch (m_mode)
		{
		case ReadersPriority: return ReleaseExclusive1();
		case WritersPriority: return ReleaseExclusive2();
		default: return ReleaseExclusive3();
		}
	}

	// Shared (readers)
	void AcquireShared()
	{
		switch (m_mode)
		{
		case ReadersPriority: return AcquireShared1();
		case WritersPriority: return AcquireShared2();
		default: return AcquireShared3();
		}
	}

	void ReleaseShared()
	{
		switch (m_mode)
		{
		case ReadersPriority: return ReleaseShared1();
		case WritersPriority: return ReleaseShared2();
		default: return ReleaseShared3();
		}
	}

};

#endif