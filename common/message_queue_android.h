#ifndef GEARS_BASE_COMMON_MESSAGE_QUEUE_ANDROID_H__
#define GEARS_BASE_COMMON_MESSAGE_QUEUE_ANDROID_H__

#include <assert.h>
#include <deque>
#include <map>
#include <unistd.h>

#include "message_queue.h"

#include "event.h"
#include "thread.h"
#include "thread_locals.h"
#include "linked_ptr.h"
#include "scoped_ptr.h"


class ThreadSpecificQueue;
// A concrete (and naive) implementation that uses Event.
class AndroidThreadMessageQueue : public ThreadMessageQueue {
 public:
  AndroidThreadMessageQueue();
  ThreadId MainThreadId();

  // ThreadMessageQueue
  virtual bool InitThreadMessageQueue();

  virtual ThreadId GetCurrentThreadId();

  virtual bool Send(ThreadId thread_handle,
                    int message_type,
                    MessageData *message);

  void HandleThreadMessage(int message_type, MessageData *message_data);

 private:

  void InitThreadEndHook();
  static void ThreadEndHook(void* value);
  static ThreadSpecificQueue* GetQueue(ThreadId thread_id);

  static Mutex queue_mutex_;  // Protects the message_queues_ map.
  typedef std::map<ThreadId, linked_ptr<ThreadSpecificQueue> > QueueMap;
  static QueueMap message_queues_;
  ThreadId  main_thread_id_;

  friend class ThreadSpecificQueue;
  friend class AndroidMessageLoop;
};

// A message queue for a thread.
class ThreadSpecificQueue {
 public:
  ThreadSpecificQueue(ThreadId thread_id) : thread_id_(thread_id) { }
  // Sends the message to the thread owning this queue.
  void SendMessage(int message_type, MessageData* message_data);
  // Dispatches messages in a loop until exit_type is received.
  void GetAndDispatchMessages(int exit_type);
  // Waits for one or messages to arrive and dispatches them to the
  // appropriate handlers.
  void RunOnce();
  // Dispatches them to the appropriate handlers.
  void DispatchOnce();

 protected:
  struct Message {
    Message(int type, MessageData* data)
        : message_type(type),
          message_data(data) {
    }
    int message_type;
    linked_ptr<MessageData> message_data;
  };

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ThreadSpecificQueue);

  // The ThreadId owning this queue.
  ThreadId thread_id_;
  // Protects event_queue_.
  Mutex event_queue_mutex_;
  // Queue of messages for this thread.
  std::deque<Message> event_queue_;
  // Event signalled when the queue is filled.
  Event event_;
  // Keep track of whether a call to NPN_PluginThreadAsyncCall is
  // in-flight, to prevent spamming the browser's message queue.
  static Mutex async_mutex_;
  static bool async_in_flight_;

  // Atomically move all messages from the queue to the return structure.
  void PopMessages(std::deque<Message> *messages);
  // Asynchronous callback on the main thread after
  // NPN_PluginThreadAsyncCall. This calls RunOnce() on the main
  // thread's message queue.
  static void AsyncCallback(void* instance);
};

#endif //GEARS_BASE_COMMON_MESSAGE_QUEUE_ANDROID_H__