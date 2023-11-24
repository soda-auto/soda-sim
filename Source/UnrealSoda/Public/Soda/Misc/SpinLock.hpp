// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#ifndef _UTIL_LOCK_SPINLOCK_H_
#define _UTIL_LOCK_SPINLOCK_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#if defined(__clang__) && (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 1 ) ) && __cplusplus >= 201103L
    #include <atomic>
    #define __UTIL_LOCK_SPINLOCK_ATOMIC_STD
#elif defined(_MSC_VER) && (_MSC_VER >= 1700) && defined(_HAS_CPP0X) && _HAS_CPP0X
    #include <atomic>
    #define __UTIL_LOCK_SPINLOCK_ATOMIC_STD
#elif defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 5) || __GNUC__ > 4) && (__cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__))
    #include <atomic>
    #define __UTIL_LOCK_SPINLOCK_ATOMIC_STD
#endif


/**
 * ==============================================
 * ======            asm pause             ======
 * ==============================================
 */
#if defined(_MSC_VER)
    #include <windows.h> // YieldProcessor

    /*
     * See: http://msdn.microsoft.com/en-us/library/windows/desktop/ms687419(v=vs.85).aspx
     * Not for intel c++ compiler, so ignore http://software.intel.com/en-us/forums/topic/296168
     */
    #define __UTIL_LOCK_SPIN_LOCK_PAUSE() YieldProcessor()

#elif defined(__GNUC__) || defined(__clang__)
    #if defined(__i386__) || defined(__x86_64__)
        /**
         * See: Intel(R) 64 and IA-32 Architectures Software Developer's Manual V2
         * PAUSE-Spin Loop Hint, 4-57
         * http://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-instruction-set-reference-manual-325383.html?wapkw=instruction+set+reference
         */
        #define __UTIL_LOCK_SPIN_LOCK_PAUSE() __asm__ __volatile__("pause")
    #elif defined(__ia64__) || defined(__ia64)
        /**
         * See: Intel(R) Itanium(R) Architecture Developer's Manual, Vol.3
         * hint - Performance Hint, 3:145
         * http://www.intel.com/content/www/us/en/processors/itanium/itanium-architecture-vol-3-manual.html
         */
        #define __UTIL_LOCK_SPIN_LOCK_PAUSE() __asm__ __volatile__ ("hint @pause")
    #elif defined(__arm__) && !defined(__ANDROID__)
        /**
         * See: ARM Architecture Reference Manuals (YIELD)
         * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.subset.architecture.reference/index.html
         */
        #define __UTIL_LOCK_SPIN_LOCK_PAUSE() __asm__ __volatile__ ("yield")
    #endif

#endif /*compilers*/

// set pause do nothing
#if !defined(__UTIL_LOCK_SPIN_LOCK_PAUSE)
    #define __UTIL_LOCK_SPIN_LOCK_PAUSE()
#endif/*!defined(CAPO_SPIN_LOCK_PAUSE)*/


/**
 * ==============================================
 * ======            cpu yield             ======
 * ==============================================
 */
#if defined(_MSC_VER)
    #define __UTIL_LOCK_SPIN_LOCK_CPU_YIELD() SwitchToThread()
    
#elif defined(__linux__) || defined(__unix__)
    #include <sched.h>
    #define __UTIL_LOCK_SPIN_LOCK_CPU_YIELD() sched_yield()
#endif

#ifndef __UTIL_LOCK_SPIN_LOCK_CPU_YIELD
    #define __UTIL_LOCK_SPIN_LOCK_CPU_YIELD() __UTIL_LOCK_SPIN_LOCK_PAUSE()
#endif

/**
 * ==============================================
 * ======           thread yield           ======
 * ==============================================
 */
#if defined(__UTIL_LOCK_SPINLOCK_ATOMIC_STD)
    #include <thread>
    #include <chrono>
    #define __UTIL_LOCK_SPIN_LOCK_THREAD_YIELD() ::std::this_thread::yield()
    #define __UTIL_LOCK_SPIN_LOCK_THREAD_SLEEP() ::std::this_thread::sleep_for(::std::chrono::milliseconds(1))
#elif defined(_MSC_VER)
    #define __UTIL_LOCK_SPIN_LOCK_THREAD_YIELD() Sleep(0)
    #define __UTIL_LOCK_SPIN_LOCK_THREAD_SLEEP() Sleep(1)
#endif

#ifndef __UTIL_LOCK_SPIN_LOCK_THREAD_YIELD
    #define __UTIL_LOCK_SPIN_LOCK_THREAD_YIELD()
    #define __UTIL_LOCK_SPIN_LOCK_THREAD_SLEEP() __UTIL_LOCK_SPIN_LOCK_CPU_YIELD()
#endif

/**
 * ==============================================
 * ======           spin lock wait         ======
 * ==============================================
 * @note
 *   1. busy-wait
 *   2. asm pause
 *   3. thread give up cpu time slice but will not switch to another process
 *   4. thread give up cpu time slice (may switch to another process)
 *   5. sleep (will switch to another process when necessary)
 */
 
#define __UTIL_LOCK_SPIN_LOCK_WAIT(x) \
{ \
    unsigned char try_lock_times = static_cast<unsigned char>(x); \
    if (try_lock_times < 4) {} \
    else if (try_lock_times < 16) { __UTIL_LOCK_SPIN_LOCK_PAUSE(); } \
    else if (try_lock_times < 32) { __UTIL_LOCK_SPIN_LOCK_THREAD_YIELD(); } \
    else if (try_lock_times < 64) { __UTIL_LOCK_SPIN_LOCK_CPU_YIELD(); } \
    else { __UTIL_LOCK_SPIN_LOCK_THREAD_SLEEP(); } \
}
    
    
#ifdef __UTIL_LOCK_SPINLOCK_ATOMIC_STD
class SpinLock
{
private:
    typedef enum {Unlocked = 0, Locked = 1} LockState;
    ::std::atomic_uint m_enStatus;

public:
    SpinLock() {
        m_enStatus.store(Unlocked);
    }

    void Lock()
    {
        unsigned char try_times = 0;
        while (m_enStatus.exchange(static_cast<unsigned int>(Locked), ::std::memory_order_acq_rel) == Locked)
            __UTIL_LOCK_SPIN_LOCK_WAIT(try_times ++); /* busy-wait */
    }

    void Unlock()
    {
        m_enStatus.store(static_cast<unsigned int>(Unlocked), ::std::memory_order_release);
    }

    bool IsLocked()
    {
        return m_enStatus.load(::std::memory_order_acquire) == Locked;
    }

    bool TryLock()
    {
        return m_enStatus.exchange(static_cast<unsigned int>(Locked), ::std::memory_order_acq_rel) == Unlocked;
    }
          
    bool TryUnlock()
    {
        return m_enStatus.exchange(static_cast<unsigned int>(Unlocked), ::std::memory_order_acq_rel) == Locked;
    }

};
#else

#if defined(__clang__)
    #if !defined(__GCC_ATOMIC_INT_LOCK_FREE) && (!defined(__GNUC__) || __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 1))
        #error Clang version is too old
    #endif
    #if defined(__GCC_ATOMIC_INT_LOCK_FREE)
        #define __UTIL_LOCK_SPINLOCK_ATOMIC_GCC_ATOMIC 1
    #else
        #define __UTIL_LOCK_SPINLOCK_ATOMIC_GCC 1
    #endif
#elif defined(_MSC_VER)
    #include <WinBase.h>
    #define __UTIL_LOCK_SPINLOCK_ATOMIC_MSVC 1
            
#elif defined(__GNUC__) || defined(__clang__) || defined(__clang__) || defined(__INTEL_COMPILER)
    #if defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 1))
        #error GCC version must be greater or equal than 4.1
    #endif
            
    #if defined(__INTEL_COMPILER) && __INTEL_COMPILER < 1100
        #error Intel Compiler version must be greater or equal than 11.0
    #endif
            
    #if defined(__GCC_ATOMIC_INT_LOCK_FREE)
        #define __UTIL_LOCK_SPINLOCK_ATOMIC_GCC_ATOMIC 1
    #else
        #define __UTIL_LOCK_SPINLOCK_ATOMIC_GCC 1
    #endif
#else
    #error Currently only gcc, msvc, intel compiler & llvm-clang are supported
#endif

class SpinLock
{
private:
    typedef enum {Unlocked = 0, Locked = 1} LockState;
    volatile unsigned int m_enStatus;

public:
    SpinLock() : m_enStatus(Unlocked) {}

    void Lock()
    {
        unsigned char try_times = 0;
    #ifdef __UTIL_LOCK_SPINLOCK_ATOMIC_MSVC
        while(InterlockedExchange(&m_enStatus, Locked) == Locked)
            __UTIL_LOCK_SPIN_LOCK_WAIT(try_times ++); /* busy-wait */
    #elif defined(__UTIL_LOCK_SPINLOCK_ATOMIC_GCC_ATOMIC)
        while (__atomic_exchange_n(&m_enStatus, Locked, __ATOMIC_ACQ_REL) == Locked)
            __UTIL_LOCK_SPIN_LOCK_WAIT(try_times ++); /* busy-wait */
    #else
        while(__sync_lock_test_and_set(&m_enStatus, Locked) == Locked)
            __UTIL_LOCK_SPIN_LOCK_WAIT(try_times ++); /* busy-wait */
    #endif
    }

    void Unlock()
    {
    #ifdef __UTIL_LOCK_SPINLOCK_ATOMIC_MSVC
        InterlockedExchange(&m_enStatus, Unlocked);
    #elif defined(__UTIL_LOCK_SPINLOCK_ATOMIC_GCC_ATOMIC)
        __atomic_store_n(&m_enStatus, Unlocked, __ATOMIC_RELEASE);
    #else
        __sync_lock_release(&m_enStatus, Unlocked);
    #endif
    }

    bool IsLocked()
    {
    #ifdef __UTIL_LOCK_SPINLOCK_ATOMIC_MSVC
        return InterlockedExchangeAdd(&m_enStatus, 0) == Locked;
    #elif defined(__UTIL_LOCK_SPINLOCK_ATOMIC_GCC_ATOMIC)
        return __atomic_load_n(&m_enStatus, __ATOMIC_ACQUIRE) == Locked;
    #else
        return __sync_add_and_fetch(&m_enStatus, 0) == Locked;
    #endif
    }

    bool TryLock()
    {

    #ifdef __UTIL_LOCK_SPINLOCK_ATOMIC_MSVC
        return InterlockedExchange(&m_enStatus, Locked) == Unlocked;
    #elif defined(__UTIL_LOCK_SPINLOCK_ATOMIC_GCC_ATOMIC)
        return __atomic_exchange_n(&m_enStatus, Locked, __ATOMIC_ACQ_REL) == Unlocked;
    #else
        return __sync_bool_compare_and_swap(&m_enStatus, Unlocked, Locked);
    #endif
    }
          
    bool TryUnlock()
    {
    #ifdef __UTIL_LOCK_SPINLOCK_ATOMIC_MSVC
        return InterlockedExchange(&m_enStatus, Unlocked) == Locked;
    #elif defined(__UTIL_LOCK_SPINLOCK_ATOMIC_GCC_ATOMIC)
        return __atomic_exchange_n(&m_enStatus, Unlocked, __ATOMIC_ACQ_REL) == Locked;
    #else
        return __sync_bool_compare_and_swap(&m_enStatus, Locked, Unlocked);
    #endif
    }

};

#endif

#endif /* SPINLOCK_H_ */
