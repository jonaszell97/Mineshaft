#ifndef MINESHAFT_WORKER_H
#define MINESHAFT_WORKER_H

#include <thread>
#include <vector>

namespace mc {

class Worker {
public:
   explicit Worker(bool start)
      : running(start)
   {
      if (start) startImpl();
   }

   Worker() : running(false) {}
   ~Worker() { stop(); }

   using Task = void(*)(void*);

   template<class Fn>
   void push_task(Fn task, void *arg)
   {
      {
         std::lock_guard<std::mutex> lk(mutex);
         taskQueue.emplace_back((Task)task, arg);
      }

      condition.notify_all();
   }

   void start()
   {
      {
         std::lock_guard<std::mutex> lk(mutex);
         if (running) {
            return;
         }

         running = true;
      }

      startImpl();
   }

   void stop()
   {
      {
         std::lock_guard<std::mutex> lk(mutex);
         if (!running) {
            return;
         }

         running = false;
      }

      condition.notify_all();
      thread.join();
   }

private:
   void startImpl()
   {
      thread = std::thread(
         [this] {
           for (;;) {
              std::vector<std::pair<Task, void*>> localTaskQueue;
              {
                 std::unique_lock<std::mutex> lk(mutex);
                 condition.wait(lk, [&] { return !taskQueue.empty() + !running; });

                 if (!running) {
                    for (auto& taskPair : taskQueue) {
                       taskPair.first(taskPair.second);
                    }

                    taskQueue.clear();
                    return;
                 }

                 std::swap(taskQueue, localTaskQueue);
              }

              for (auto& taskPair : localTaskQueue) {
                 taskPair.first(taskPair.second);
              }
           }
        });
   }

private:
   std::condition_variable condition;
   std::vector<std::pair<Task, void*>> taskQueue;
   std::mutex mutex;
   std::thread thread;
   bool running = false;
};

} // namespace mc

#endif //MINESHAFT_WORKER_H
