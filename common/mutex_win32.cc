// Copyright 2008, Google Inc.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Google Inc. nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#if defined(WIN32) || defined(OS_WINCE)

#include <assert.h>
#include <windows.h>

#include "gears/base/common/mutex.h"
#include "gears/base/common/stopwatch.h"  // For GetCurrentTimeMillis()
#include "gears/base/common/vista_utils.h"

void ThreadYield() {
  Sleep(0);  // equivalent to 'yield' in Win32
}


//ported from sqlite
int OsSleep(int microseconds){
  ::Sleep((microseconds+999)/1000);
  return ((microseconds+999)/1000)*1000;
}


Mutex::Mutex()
#ifdef DEBUG
    : is_locked_(false)
#endif
{
  InitializeCriticalSection(&crit_sec_);
}


Mutex::~Mutex() {
  DeleteCriticalSection(&crit_sec_);
}


void Mutex::Lock() {
  EnterCriticalSection(&crit_sec_);
#ifdef DEBUG
  assert(!is_locked_); // Google frowns upon mutex re-entrancy
  is_locked_ = true;
#endif
}


void Mutex::Unlock() {
#ifdef DEBUG
  assert(is_locked_);
  is_locked_ = false;
#endif
  LeaveCriticalSection(&crit_sec_);
}


// CondVar::Event implementation
//
// This class adds reference counting to windows manual events.
// Since Windows XP & earlier do not implement native condition variables,
// we're implementing a condition variable using windows events.


class CondVar::Event : public RefCounted {
 public:
  Event();
  ~Event();
  HANDLE Handle() const;
 private:
  HANDLE event_;
};


CondVar::Event::Event() {
  event_ = CreateEvent(NULL, TRUE, FALSE, NULL);
}


CondVar::Event::~Event() {
#ifdef DEBUG
  BOOL result = CloseHandle(event_);
  assert(result == TRUE);
#else
  CloseHandle(event_);
#endif
}


HANDLE CondVar::Event::Handle() const {
  return event_;
}


// CondVar implementation
//
// Each event will correspond to a single call to CondVar::SignalAll.
// An event is created when the first interested party calls Wait.
// Subsequent calls to Wait will wait on the same event.
// When signal is called, the CondVar removes its reference to the event then
// sets the event to wake up the current waiters.
// While those waiters are being serviced, subsequent calls to Wait will result
// in the creation of a new event.


CondVar::CondVar() {
}


CondVar::~CondVar() {
#ifdef DEBUG
  // Make sure nobody is waiting.
  MutexLock locker(&current_event_mutex_);
  if (current_event_.get()) {
    assert(current_event_->GetRef() == 1);
    current_event_.reset();
  }
#endif
}


void CondVar::Wait(Mutex *mutex) {
  WaitWithTimeout(mutex, INFINITE);
}


bool CondVar::WaitWithTimeout(Mutex *mutex, int milliseconds) {
#ifdef DEBUG
  assert(mutex->is_locked_);
#endif
  scoped_refptr<Event> event;
  {
    MutexLock locker(&current_event_mutex_);
    if (current_event_.get() == 0) {
      current_event_ = new Event;
    }
    event = current_event_;
  }
  mutex->Unlock();
  DWORD result = WaitForSingleObject(event->Handle(), milliseconds);
  mutex->Lock();
  return result == WAIT_TIMEOUT;
}


void CondVar::SignalAll() {
  scoped_refptr<Event> old_event;
  {
    MutexLock locker(&current_event_mutex_);
    if (current_event_.get() == 0) return;
    old_event = current_event_;
    current_event_.reset();
  }
#ifdef DEBUG
  BOOL result = SetEvent(old_event->Handle());
  assert(result == TRUE);
#else
  SetEvent(old_event->Handle());
#endif
}


#define BATON_FATAL_ERROR_HANDLING(msg)\
  do{\
    std::string16 err_msg________(msg);\
    err_msg________ += L", Last error = %i";\
    DWORD err___________ = ::GetLastError();\
    LOG16((err_msg________.c_str(), err___________));\
    mutex_.Close();\
    event_.Close();\
  }while(0)

Baton::Baton(const std::string16 &name) : name_(name), mutex_(), event_(),
                              mutex_got_(false){
}

bool Baton::init(){
  std::string16 name = name_ + L" Mutex for Baton____";
  if(!mutex_.Create(NULL, FALSE, name.c_str())){
    BATON_FATAL_ERROR_HANDLING(L"Failed to create mutex for baton");
    return false;
  }

  if(GetLastError() != ERROR_ALREADY_EXISTS) {
    VistaUtils::SetObjectToLowIntegrity(mutex_);
  }

  name = name_ + L"Event For Baton____";
  if(!event_.Create(NULL, TRUE, FALSE, name.c_str())){
    BATON_FATAL_ERROR_HANDLING(L"Failed to create event for baton");
    return false;
  }

  if(GetLastError() != ERROR_ALREADY_EXISTS) {
    VistaUtils::SetObjectToLowIntegrity(event_);
  }

  return true;
}

bool Baton::get(){
  DWORD res = ::WaitForSingleObject(mutex_, INFINITE);
  //WAIT_ABANDONED is safe, coz our thread may be terminate abnormally by IE
  //and the event_ state responses for job status, not mutex
  if(WAIT_ABANDONED == res || WAIT_OBJECT_0 == res){
    res = ::WaitForSingleObject(event_, 0);
    if(WAIT_OBJECT_0 == res){
      //event_ is signaled, we shall skip any job step, and return false
      mutex_.Release();
      return false;
    }else{
      //event_ is not yet signaled, return true to proceed the job
      mutex_got_ = true;
      return true;
    }
  }
  return false;
}

//pass the baton to next one
bool Baton::pass(){
  assert(mutex_got_);
  bool ret = false;
  if(mutex_got_ && mutex_.Release() != FALSE){
    mutex_got_ = false;
    ret = true;
  }
  return ret;
}


bool Baton::finish(){

  assert(mutex_got_);
  if(!mutex_got_)
    return false;

  bool ret = true;
  
  if(!event_.Set()){
    LOG16((L"Failed to signal the event while finishing a baton"));
    ret = false;
  }

  //anyway, we shall finish up the mutex
  if(!mutex_.Release()){
    BATON_FATAL_ERROR_HANDLING(L"Failed to finish up a baton");
    ret = false;
  }
  
  return ret;
}



#undef BATON_FATAL_ERROR_HANDLING

#endif  // defined(WIN32) || defined(OS_WINCE)
