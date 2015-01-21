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
#include "message_queue_android.h"

static const ThreadLocals::Slot kThreadLocalKey = ThreadLocals::Alloc();
// The thread message queue singleton.
static AndroidThreadMessageQueue g_instance;


//------------------------------------------------------------------------------
// AndroidThreadMessageQueue

Mutex AndroidThreadMessageQueue::queue_mutex_;
AndroidThreadMessageQueue::QueueMap AndroidThreadMessageQueue::message_queues_;

AndroidThreadMessageQueue::AndroidThreadMessageQueue() {
  main_thread_id_ = GetCurrentThreadId();
}

ThreadId AndroidThreadMessageQueue::MainThreadId() {
  return main_thread_id_;
}

// static
ThreadMessageQueue *ThreadMessageQueue::GetInstance() {
  return &g_instance;
}

bool AndroidThreadMessageQueue::InitThreadMessageQueue() {
  MutexLock lock(&queue_mutex_);
  ThreadId thread_id = GetCurrentThreadId();
  if (message_queues_.find(thread_id) == message_queues_.end()) {
    ThreadSpecificQueue* queue = new ThreadSpecificQueue(thread_id);
    if (thread_id != MainThreadId()) {
      InitThreadEndHook();
    }
    message_queues_[thread_id] = linked_ptr<ThreadSpecificQueue>(queue);
  }
  return true;
}

ThreadId AndroidThreadMessageQueue::GetCurrentThreadId() {
  return pthread_self();
}

bool AndroidThreadMessageQueue::Send(ThreadId thread,
                                     int message_type,
                                     MessageData *message_data) {
  MutexLock lock(&queue_mutex_);

  // Find the queue for the target thread.
  AndroidThreadMessageQueue::QueueMap::iterator queue;
  queue = message_queues_.find(thread);
  if (queue == message_queues_.end()) {
    delete message_data;
    return false;
  }
  queue->second->SendMessage(message_type, message_data);
  return true;
}

void AndroidThreadMessageQueue::HandleThreadMessage(int message_type,
                                                    MessageData *message_data) {
  RegisteredHandler handler;
  if (GetRegisteredHandler(message_type, &handler)) {
    handler.Invoke(message_type, message_data);
  }
}

void AndroidThreadMessageQueue::InitThreadEndHook() {
  // We use a ThreadLocal to get called when an OS thread terminates
  // and use that opportunity to remove all observers that remain
  // registered on that thread.
  //
  // We store the thread id in the ThreadLocal variable because on some
  // OSes (linux), the destructor proc is called from a different thread,
  // and on others (windows), the destructor proc is called from the
  // terminating thread.
  //
  // Also, we only do this for the actual singleton instance of the
  // MessageService class as opposed to instances created for unit testing.
  if (!ThreadLocals::HasValue(kThreadLocalKey)) {
    ThreadId *id = new ThreadId(GetCurrentThreadId());
    ThreadLocals::SetValue(kThreadLocalKey, id, &ThreadEndHook);
  }
}

ThreadSpecificQueue* AndroidThreadMessageQueue::GetQueue(ThreadId id) {
  ThreadSpecificQueue* queue = NULL;
  {
    MutexLock lock(&queue_mutex_);
    // Find the queue for this thread.
    QueueMap::iterator queue_iter;
    queue_iter = message_queues_.find(id);
    assert (queue_iter != message_queues_.end());
    queue = queue_iter->second.get();
  }
  assert(queue);
  return queue;
}

// static
void AndroidThreadMessageQueue::ThreadEndHook(void* value) {
  ThreadId *id = reinterpret_cast<ThreadId*>(value);
  if (id) {
    MutexLock lock(&queue_mutex_);
    message_queues_.erase(*id);
    delete id;
  }
}

//------------------------------------------------------------------------------
// AndroidMessageLoop

// static
void AndroidMessageLoop::Start() {
  // Start the loop on the current thread.
  ThreadId thread_id =
      AndroidThreadMessageQueue::GetInstance()->GetCurrentThreadId();
  AndroidThreadMessageQueue::GetQueue(thread_id)->GetAndDispatchMessages(
      kAndroidLoop_Exit);
}

// static
void AndroidMessageLoop::RunOnce() {
  // Run the loop once on the current thread.
  ThreadId thread_id =
      AndroidThreadMessageQueue::GetInstance()->GetCurrentThreadId();
  AndroidThreadMessageQueue::GetQueue(thread_id)->RunOnce();
}

// static
void AndroidMessageLoop::Stop(ThreadId thread_id) {
  // Stop the target thread's loop.
  AndroidThreadMessageQueue::GetQueue(thread_id)->SendMessage(kAndroidLoop_Exit,
                                                              NULL);
}

//------------------------------------------------------------------------------
// ThreadSpecificQueue

Mutex ThreadSpecificQueue::async_mutex_;
bool ThreadSpecificQueue::async_in_flight_ = false;

void ThreadSpecificQueue::SendMessage(int message_type,
                                      MessageData* message_data) {
  // Put a message in the queue. Note that the Message object
  // takes ownership of message_data.
  {
    MutexLock lock(&event_queue_mutex_);
    event_queue_.push_back(Message(message_type, message_data));
  }
  event_.Signal();
  if (thread_id_ == g_instance.MainThreadId()) {
    // If sending to the main thread, also put a message into the
    // browser's message loop so this mechanism still works if the
    // main thread is sat waiting in the browser's idle loop. It is
    // harmless if the queue clears before this callback occurs - it
    // just runs an empty queue.
    MutexLock lock(&async_mutex_);
    /*
    if (!async_in_flight_) {
      // At most only one outstanding NPN_PluginThreadAsyncCall
      // scheduled on the browser's message queue.
      async_in_flight_ = true;
      NPN_PluginThreadAsyncCall(NULL, AsyncCallback, this);
    }
    */
  }
}

void ThreadSpecificQueue::AsyncCallback(void* instance) {
  // Callback from the browser's message queue.
  ThreadSpecificQueue* queue =
      reinterpret_cast<ThreadSpecificQueue *>(instance);
  {
    MutexLock lock(&async_mutex_);
    async_in_flight_ = false;
  }
  queue->DispatchOnce();
}

void ThreadSpecificQueue::PopMessages(std::deque<Message> *messages) {
  // Get and removes all messages from the queue.
  messages->clear();
  {
    MutexLock lock(&event_queue_mutex_);
    event_queue_.swap(*messages);
  }
}

void ThreadSpecificQueue::GetAndDispatchMessages(int exit_type) {
  bool done = false;
  while(!done) {
    event_.Wait();
    // Move existing messages to a local queue.
    std::deque<Message> local_event_queue;
    PopMessages(&local_event_queue);
    // Dispatch the local events
    while (!local_event_queue.empty()) {
      Message msg = local_event_queue.front();
      local_event_queue.pop_front();
      if (msg.message_type == exit_type) {
        done = true;
        break;
      }
      g_instance.HandleThreadMessage(msg.message_type, msg.message_data.get());
    }
  }
}

void ThreadSpecificQueue::RunOnce() {
  event_.Wait();
  DispatchOnce();
}

void ThreadSpecificQueue::DispatchOnce() {
  // Move existing messages to a local queue.
  std::deque<Message> local_event_queue;
  PopMessages(&local_event_queue);
  // Dispatch the local events
  while (!local_event_queue.empty()) {
    Message msg = local_event_queue.front();
    local_event_queue.pop_front();
    g_instance.HandleThreadMessage(msg.message_type, msg.message_data.get());
  }
}

//------------------------------------------------------------------------------
