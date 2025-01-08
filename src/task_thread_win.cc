/*
  task_thread_win.cc
  libtq
  2024-11-08
  Push Chen
*/

/*
MIT License

Copyright (c) 2023 Push Chen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#if defined(_WIN32)

#include "task_thread.h"

namespace libtq {

void __magic_change_thread_name__(const char* name) {
  struct {
    DWORD dwType;
    LPCSTR szName;
    DWORD dwThreadID;
    DWORD dwFlags;
  } threadname_info = {0x1000, name, static_cast<DWORD>(-1), 0};
  
  DWORD arg_count = sizeof(threadname_info) / sizeof(DWORD);
  const ULONG_PTR * arg = reinterpret_cast<ULONG_PTR*>(&threadname_info);
  __try {
    ::RaiseException(0x406D1388, 0, arg_count, arg);
  } __except (EXCEPTION_EXECUTE_HANDLER) {  // NOLINT
  }
}

void __set_current_thread_name__(const char* name) {
// SetThreadDescription is only visible in Visual Studio 2017 version 15.6 and later versions. 
// See  https://learn.microsoft.com/en-us/visualstudio/debugger/how-to-set-a-thread-name-in-native-code?view=vs-2022
#if defined(_MSC_VER) && _MSC_VER >=1913
  typedef HRESULT(WINAPI* SetThreadDescription)(HANDLE, PCWSTR);
  #pragma warning(push)
  #pragma warning(disable: 4191)
	static auto set_thread_description_func = reinterpret_cast<SetThreadDescription>(
    ::GetProcAddress(::GetModuleHandleA("Kernel32.dll"), "SetThreadDescription"));
  #pragma warning(pop)
  if (set_thread_description_func) {
		const int kMaxBufferLen = 256;
		WCHAR wThreadName[kMaxBufferLen];
		int len = MultiByteToWideChar(CP_ACP, 0, name, -1, wThreadName, 0);
		if (!len) {
			return;
		}
		if (len >= kMaxBufferLen) {
			len = kMaxBufferLen - 1;
			wThreadName[len] = L'\0';
		}
		MultiByteToWideChar(CP_ACP, 0, name, -1, wThreadName, len);
		set_thread_description_func(::GetCurrentThread(), wThreadName);
  } else {
#endif
    __magic_change_thread_name__(name);
#if defined(_MSC_VER) && _MSC_VER >=1913
  }
#endif
}

int __priority_map__(thread_priority prio) {
  switch(prio) {
    case thread_priority::k_broken:
      return THREAD_PRIORITY_NORMAL;
    case thread_priority::k_low:
      return THREAD_PRIORITY_BELOW_NORMAL;
    case thread_priority::k_normal:
      return THREAD_PRIORITY_NORMAL;
    case thread_priority::k_high:
      return THREAD_PRIORITY_ABOVE_NORMAL;
    case thread_priority::k_highest:
      return THREAD_PRIORITY_HIGHEST;
    case thread_priority::k_realtime:
      return THREAD_PRIORITY_TIME_CRITICAL;
    default:
      return THREAD_PRIORITY_NORMAL;
  };
}

struct __entrance__ : private thread {
  __entrance__() = delete;
  __entrance__(const __entrance__&) = delete;
  __entrance__(__entrance__ &&) = delete;
  __entrance__& operator = (const __entrance__&) = delete;
  __entrance__& operator = (__entrance__&&) = delete;
  static void run(thread* t) {
    (t->*&__entrance__::entrance_)();
  }
};

DWORD WINAPI __windows_run_thread__(void * thread_param) {
  thread* pt = (thread *)thread_param;
  __entrance__::run(pt);
  return NULL;
}

thread::~thread() {
  if (this->joinable_) {
    ::WaitForSingleObject(handler_, INFINITE);
    ::CloseHandle(handler_);
  }
}

bool thread::create_thread_with_attr_() {
  DWORD thread_id = 0;
  handler_ = ::CreateThread(
    nullptr, (SIZE_T)this->attr_.stack_size, 
    &__windows_run_thread__, this, 
    STACK_SIZE_PARAM_IS_A_RESERVATION, &thread_id
  );
  if (!handler_) {
    return false;
  }
  return true;
}

void thread::detach() {
  if (!this->validate_) {
    return;
  }
  if (!this->joinable_) {
    return;
  }
  // Do detach
  ::CloseHandle(handler_);
  this->joinable_ = false;
}
void thread::change_priority(thread_priority priority) {
  if (this->thread_id_ != std::this_thread::get_id()) {
    return;
  }
  if (priority == thread_priority::k_broken) {
    priority = thread_priority::k_low;
  }
  if (priority == this->current_priority_) {
    return;
  }
  // DO change thread priority
  (void)::SetThreadPriority(this->handler_, __priority_map__(priority));
  this->current_priority_ = priority;
}

void thread::entrance_() {
  this->thread_id_ = std::this_thread::get_id();
  // Config the thread name and priority
  this->change_priority(this->attr_.priority);
  if (this->attr_.name != nullptr) {
    __set_current_thread_name__(this->attr_.name);
  }
  this->validate_ = true;
  this->main();
  this->validate_ = false;
}


} // namespace libtq

#endif