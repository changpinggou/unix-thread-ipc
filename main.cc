/*
 * =====================================================================================
 *       Filename:  thead.c
 *    Description:  getspecific
 *        Created:  05/10/2011 12:09:43 AM
 * =====================================================================================
 */

#include<stdio.h>
#include<string.h>

#include "common/message_queue_android.h"

const int kMessageType1 = 100;
const int kMessageType2 = 101;
const int kMessageType3 = 102;

class TestMessage : public MessageData {
 public:
  TestMessage(ThreadId sender, int data) : sender_(sender), data_(data) {}
  ~TestMessage() {}
  ThreadId Sender() {return sender_;}
  int Data() {return data_;}
 private:
  ThreadId sender_;
  int data_;
};

class BackgroundMessageHandler
    : public ThreadMessageQueue::HandlerInterface {
 public:
  explicit BackgroundMessageHandler(ThreadId sender)
      : sender_(sender),
        count_(0) {}

  int Count() {return count_;}
  // ThreadMessageQueue::HandlerInterface override
  virtual void HandleThreadMessage(int message_type,
                                   MessageData *message_data) {
    assert(message_data);
    TestMessage* message = static_cast<TestMessage*>(message_data);
    assert(sender_ == message->Sender());
    printf("worker thread receive message_type:%d  from thread:%d in %s\n",message_type, 
            int(message->Sender()), __func__);
    // Accumulate the data.
    count_ += message->Data();
    printf("worker thread receive total message data :%d\n",count_);  
    LOG(("Got data %d", count_));
    if (count_ == 600) {
      // Tell the main thread we got all data.
      LOG(("Sending to parent thread"));
      printf("Tell the main thread we got all data.\n");
      sleep(2);
      message = new TestMessage(
          ThreadMessageQueue::GetInstance()->GetCurrentThreadId(), count_);
      ThreadMessageQueue::GetInstance()->Send(sender_,
                                              kMessageType3,
                                              message);
    }
  }

 private:
  ThreadId sender_;
  int count_;
};

class MainMessageHandler
    : public ThreadMessageQueue::HandlerInterface {
 public:
  MainMessageHandler() : done_(false) { }
  // ThreadMessageQueue::HandlerInterface override
  virtual void HandleThreadMessage(int message_type,
                                   MessageData *message_data) {
    TestMessage* message = static_cast<TestMessage*>(message_data);
    printf("main thread receive message_type:%d  in %s\n",message_type,__func__); 
    sleep(1000);
    assert (message->Data() == 600);
    // The worker must have received all data. Let's stop it.
    LOG(("Got %d from worker thread", message->Data()));
    AndroidMessageLoop::Stop(message->Sender());
    done_ = true;
  }
  bool IsDone() const { return done_; }

 private:
  bool done_;
};

class TestThread : public Thread {
 public:
  explicit TestThread(ThreadId parent_id) : parent_id_(parent_id) {}
  virtual void Run() {
    BackgroundMessageHandler handler1(parent_id_);
    BackgroundMessageHandler handler2(parent_id_);
    ThreadMessageQueue::GetInstance()->RegisterHandler(
        kMessageType1, &handler1);
    ThreadMessageQueue::GetInstance()->RegisterHandler(
        kMessageType2, &handler2);

    // Run the message loop
    AndroidMessageLoop::Start();
    LOG(("Test thread shutting down."));
  }

 private:
  ThreadId parent_id_;
};

bool TestThreadMessageQueue(std::string16* error) {
  AndroidThreadMessageQueue* queue = static_cast<AndroidThreadMessageQueue*>(
      ThreadMessageQueue::GetInstance());
  queue->InitThreadMessageQueue();

  // The message queue for this thread is already initialized:
  // - if this is the main browser thread, the queue was initialized
  //   in NP_Initialized.
  // - if this is a worker, the queue was initialized in
  //   Thread::ThreadMain().
  // Our message handler
  MainMessageHandler local_handler;
  queue->RegisterHandler(kMessageType3, &local_handler);

  // Start the worker.
  ThreadId self_id = queue->GetCurrentThreadId();
  scoped_ptr<TestThread> thread(new TestThread(self_id));
  ThreadId worker_thread_id = thread->Start();

  // Send three messages, sleep, send another three.
  TestMessage* message = new TestMessage(self_id, 1);
  queue->Send(worker_thread_id, kMessageType1, message);
  message = new TestMessage(self_id, 2);
  queue->Send(worker_thread_id, kMessageType1, message);
  message = new TestMessage(self_id, 3);
  queue->Send(worker_thread_id, kMessageType1, message);
  sleep(1);
  message = new TestMessage(self_id, 100);
  queue->Send(worker_thread_id, kMessageType2, message);
  message = new TestMessage(self_id, 200);
  queue->Send(worker_thread_id, kMessageType2, message);
  message = new TestMessage(self_id, 300);
  queue->Send(worker_thread_id, kMessageType2, message);

  while (!local_handler.IsDone()) {
    AndroidMessageLoop::RunOnce();
  }
  LOG(("Message from worker received."));
  thread->Join();
  LOG(("Test thread joined successfuly, test completed."));
  return true;
}


int main(){
  std::string16 error;
  TestThreadMessageQueue(&error);
}
