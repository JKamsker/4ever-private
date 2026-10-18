// TThread.h: portable threads and synchronization, in place of the Win32 thread, critical section
// and event handles. Built on the C++ standard library, so Windows and Linux run the same code.
//
// CTCriticalSection is recursive like a CRITICAL_SECTION. CTEvent is a Win32 event: an auto reset
// event wakes one waiter and clears itself, a manual reset one stays set until Reset. CTThread
// runs a function on its own thread; a thread still running when its CTThread is destroyed is
// left to finish on its own, as a closed Win32 thread handle would.
//////////////////////////////////////////////////////////////////////

#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <utility>


//////////////////////////////////////////////////////////////////////
// CTCriticalSection

class CTCriticalSection
{
public:
	void Lock()
	{
		m_mutex.lock();
	};

	void Unlock()
	{
		m_mutex.unlock();
	};

protected:
	std::recursive_mutex m_mutex;
};


//////////////////////////////////////////////////////////////////////
// Smart sync class for critical section

class CCSLock
{
public:
	CCSLock( CTCriticalSection *pCS)
	{
		pCS->Lock();
		m_pCS = pCS;
	};

	virtual ~CCSLock()
	{
		m_pCS->Unlock();
	};

protected:
	CTCriticalSection *m_pCS;
};

#define SMART_LOCKCS(x)					CCSLock lock_object(x);


//////////////////////////////////////////////////////////////////////
// CTEvent

class CTEvent
{
public:
	CTEvent( BOOL bManualReset = FALSE, BOOL bSignaled = FALSE)
		: m_bManualReset(bManualReset), m_bSignaled(bSignaled)
	{
	};

	void Set()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_bSignaled = TRUE;

		if(m_bManualReset)
			m_vCOND.notify_all();
		else
			m_vCOND.notify_one();
	};

	void Reset()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_bSignaled = FALSE;
	};

	// TRUE when the event was set before the timeout (milliseconds, INFINITE to wait forever).
	BOOL Wait( DWORD dwTimeout = INFINITE)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		auto vSIGNALED = [this]() { return m_bSignaled != FALSE; };

		if( dwTimeout == INFINITE )
			m_vCOND.wait( lock, vSIGNALED);
		else if(!m_vCOND.wait_for( lock, std::chrono::milliseconds(dwTimeout), vSIGNALED))
			return FALSE;

		if(!m_bManualReset)
			m_bSignaled = FALSE;

		return TRUE;
	};

protected:
	std::mutex m_mutex;
	std::condition_variable m_vCOND;

	BOOL m_bManualReset;
	BOOL m_bSignaled;
};


//////////////////////////////////////////////////////////////////////
// CTThread

// Thread priorities, with the values of the Win32 THREAD_PRIORITY_* they stand for.
enum TTHREAD_PRIORITY
{
	TTHREAD_PRIORITY_BELOW_NORMAL = -1,
	TTHREAD_PRIORITY_NORMAL = 0,
	TTHREAD_PRIORITY_HIGHEST = 2,
	TTHREAD_PRIORITY_TIME_CRITICAL = 15
};

class CTThread
{
public:
	CTThread()
	{
	};

	~CTThread()
	{
		Detach();
	};

	CTThread( const CTThread&) = delete;
	CTThread& operator=( const CTThread&) = delete;

	// Runs fn(args...) on a new thread. FALSE when a thread is still attached or none could start.
	template< class FN, class... ARGS >
	BOOL Start( FN&& fn, ARGS&&... args)
	{
		if(IsRunning())
			return FALSE;

		try
		{
			m_vTHREAD = std::thread( std::forward<FN>(fn), std::forward<ARGS>(args)...);
		}
		catch( const std::system_error& )
		{
			return FALSE;
		}

		return TRUE;
	};

	// TRUE from Start until Join or Detach, even after the function has returned.
	BOOL IsRunning() const
	{
		return m_vTHREAD.joinable() ? TRUE : FALSE;
	};

	// Waits for the thread to end. Not from the thread itself.
	void Join()
	{
		if(m_vTHREAD.joinable())
			m_vTHREAD.join();
	};

	// Lets the thread finish on its own.
	void Detach()
	{
		if(m_vTHREAD.joinable())
			m_vTHREAD.detach();
	};

	// Scheduling hint: Windows only, the other systems keep the default priority.
	void SetPriority( TTHREAD_PRIORITY nPriority)
	{
#ifdef _WIN32
		if(m_vTHREAD.joinable())
			::SetThreadPriority( m_vTHREAD.native_handle(), nPriority);
#endif
	};

protected:
	std::thread m_vTHREAD;
};
