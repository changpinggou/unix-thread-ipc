/*
 * =====================================================================================
 *
 *       Filename:  thread_exit.cc
 *
 *    Description:  thread exit callback
 *
 *        Version:  1.0
 *        Created:  2010/11/11 15:40:30
 *       Revision:  none
 *       Compiler:  gcc/msvc
 *
 *         Author:  Laurence Von
 *        Company:  NTES
 *
 * =====================================================================================
 */

#include "gears/base/common/thread_exit.h"
#include <assert.h>

const ThreadLocals::Slot ThreadExit::kThreadLocalsKey = ThreadLocals::Alloc();

//static
ThreadExit &ThreadExit::GetMe() {
  const ThreadLocals::Slot &key = kThreadLocalsKey;
  ThreadExit *locals = reinterpret_cast<ThreadExit*>(ThreadLocals::GetValue(key));
  if (!locals) {
    locals = new ThreadExit;
    ThreadLocals::SetValue(key, locals, &ThreadExit::Destructor);
  }
  return *locals;
}

ThreadExit::ThreadExit() {
}

ThreadExit::~ThreadExit() {
  for(iterator itr = todo_.begin(); itr != todo_.end(); ++itr) {
    itr->second(itr->first);
  }
}

//static
void ThreadExit::Add(void *closure, deallocator_type dealloc) {
  ThreadExit::GetMe().AddInternal(closure, dealloc);
}

//static
void ThreadExit::Remove(void *closure) {
  ThreadExit::GetMe().RemoveInternal(closure);
}

void ThreadExit::AddInternal(void *closure, deallocator_type dealloc) {
  assert(todo_.find(closure) == todo_.end());
  todo_[closure] = dealloc;
}

void ThreadExit::RemoveInternal(void *closure) {
  iterator itr = todo_.find(closure);
  assert(itr != todo_.end());
  todo_.erase(itr);
}
