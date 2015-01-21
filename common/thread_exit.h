/*
 * =====================================================================================
 *
 *       Filename:  thread_exit.h
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
#ifndef GEARS_BASE_COMMON_THREAD_EXIT_H__
#define GEARS_BASE_COMMON_THREAD_EXIT_H__

#include <map>
#include "gears/base/common/common.h"
#include "gears/base/common/thread_locals.h"

class ThreadExit {
 public:
  typedef void (*deallocator_type)(void*);
  typedef std::map<void*, deallocator_type> map_type;
  typedef map_type::iterator iterator;

  template <typename Ty_>
  static void Add(Ty_* me) {
    ThreadExit::Add(me, &Deallocator<Ty_>);
  }
  static void Add(void *closure, deallocator_type dealloc);
  static void Remove(void *closure);

 private:
  static void Destructor(void *me) {
    ThreadExit *pThis = (ThreadExit*)me;
    delete pThis;
  }

  ThreadExit();
  ~ThreadExit();

  template <typename Ty_>
  static void Deallocator(void *me) {
   Ty_* pThis = (Ty_*)me;
   delete pThis;
  }

  void AddInternal(void *closure, deallocator_type dealloc);
  void RemoveInternal(void *closure);
  static ThreadExit &GetMe();
  map_type todo_;

  static const ThreadLocals::Slot kThreadLocalsKey;
};

#endif //GEARS_BASE_COMMON_THREAD_EXIT_H__
