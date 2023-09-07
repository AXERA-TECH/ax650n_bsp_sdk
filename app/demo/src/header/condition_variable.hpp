/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#pragma once
#if ((defined(__GNUC__) && __GNUC__ == 7) && (defined(__GNUC_MINOR__) && __GNUC_MINOR__ == 5))
#include <chrono>
#include <mutex>

namespace std {
  /// cv_status
  enum class cv_status { no_timeout, timeout };

/// condition_variable
  class condition_variable
  {
    // GCC 7.5.0 using system_clock, here we change to steady_clock
    typedef chrono::steady_clock	__clock_t;
    typedef __gthread_cond_t		__native_type;

// #ifdef __GTHREAD_COND_INIT
//     __native_type			_M_cond = __GTHREAD_COND_INIT;
// #else
    __native_type			_M_cond;
// #endif

  public:
    typedef __native_type* 		native_handle_type;

    condition_variable() noexcept {
      __gthrw_(pthread_cond_init) (&_M_cond, NULL);
    }

    ~condition_variable() noexcept {
      // XXX no thread blocked
      /* int __e = */ __gthread_cond_destroy(&_M_cond);
      // if __e == EBUSY then blocked
    }

    condition_variable(const condition_variable&) = delete;
    condition_variable& operator=(const condition_variable&) = delete;

    void
    notify_one() noexcept {
      int __e = __gthread_cond_signal(&_M_cond);

      // XXX not in spec
      // EINVAL
      if (__e) {
        __throw_system_error(__e);
      }
    }

    void
    notify_all() noexcept {
      int __e = __gthread_cond_broadcast(&_M_cond);

      // XXX not in spec
      // EINVAL
      if (__e) {
        __throw_system_error(__e);
      }
    }

    void
    wait(unique_lock<mutex>& __lock) noexcept {
      int __e = __gthread_cond_wait(&_M_cond, __lock.mutex()->native_handle());
      if (__e) {
        std::terminate();
      }
    }

    template<typename _Predicate>
      void
      wait(unique_lock<mutex>& __lock, _Predicate __p)
      {
	while (!__p())
	  wait(__lock);
      }

    template<typename _Duration>
      cv_status
      wait_until(unique_lock<mutex>& __lock,
		 const chrono::time_point<__clock_t, _Duration>& __atime)
      { return __wait_until_impl(__lock, __atime); }

    template<typename _Clock, typename _Duration>
      cv_status
      wait_until(unique_lock<mutex>& __lock,
		 const chrono::time_point<_Clock, _Duration>& __atime)
      {
	// DR 887 - Sync unknown clock to known clock.
	const typename _Clock::time_point __c_entry = _Clock::now();
	const __clock_t::time_point __s_entry = __clock_t::now();
	const auto __delta = __atime - __c_entry;
	const auto __s_atime = __s_entry + __delta;

	return __wait_until_impl(__lock, __s_atime);
      }

    template<typename _Clock, typename _Duration, typename _Predicate>
      bool
      wait_until(unique_lock<mutex>& __lock,
		 const chrono::time_point<_Clock, _Duration>& __atime,
		 _Predicate __p)
      {
	while (!__p())
	  if (wait_until(__lock, __atime) == cv_status::timeout)
	    return __p();
	return true;
      }

    template<typename _Rep, typename _Period>
      cv_status
      wait_for(unique_lock<mutex>& __lock,
	       const chrono::duration<_Rep, _Period>& __rtime)
      {
	using __dur = typename __clock_t::duration;
	auto __reltime = chrono::duration_cast<__dur>(__rtime);
	if (__reltime < __rtime)
	  ++__reltime;
	return wait_until(__lock, __clock_t::now() + __reltime);
      }

    template<typename _Rep, typename _Period, typename _Predicate>
      bool
      wait_for(unique_lock<mutex>& __lock,
	       const chrono::duration<_Rep, _Period>& __rtime,
	       _Predicate __p)
      {
	using __dur = typename __clock_t::duration;
	auto __reltime = chrono::duration_cast<__dur>(__rtime);
	if (__reltime < __rtime)
	  ++__reltime;
	return wait_until(__lock, __clock_t::now() + __reltime, std::move(__p));
      }

    native_handle_type
    native_handle()
    { return &_M_cond; }

  private:
    template<typename _Dur>
      cv_status
      __wait_until_impl(unique_lock<mutex>& __lock,
			const chrono::time_point<__clock_t, _Dur>& __atime)
      {
	auto __s = chrono::time_point_cast<chrono::seconds>(__atime);
	auto __ns = chrono::duration_cast<chrono::nanoseconds>(__atime - __s);

	__gthread_time_t __ts =
	  {
	    static_cast<std::time_t>(__s.time_since_epoch().count()),
	    static_cast<long>(__ns.count())
	  };

	__gthread_cond_timedwait(&_M_cond, __lock.mutex()->native_handle(),
				 &__ts);

	return (__clock_t::now() < __atime
		? cv_status::no_timeout : cv_status::timeout);
      }
  };
}
#else
#include <condition_variable>
#endif