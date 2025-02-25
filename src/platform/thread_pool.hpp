#pragma once

#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <vector>

namespace craft {
class ThreadPool {
public:
  template <class F, class... Args>
  static auto Enqueue(F &&f, Args &&...args) -> std::future<typename std::invoke_result_t<F, Args...>> {
    static ThreadPool pool(std::thread::hardware_concurrency());

    pool.EnqueueImpl(std::move(f), std::forward<Args>(args)...);
  }

  static size_t Concurrency() { return std::thread::hardware_concurrency(); }

private:
  explicit ThreadPool(size_t num_thrs) : m_stop_flag(false) {
    for (size_t i = 0; i < num_thrs; ++i) {
      m_workers.emplace_back([this](std::stop_token stoken) {
        while (true) {
          std::move_only_function<void()> task;
          {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_condition.wait(lock, [this, &stoken] { return stoken.stop_requested() || !m_tasks.empty(); });

            if (stoken.stop_requested() && m_tasks.empty())
              return;

            task = std::move(m_tasks.front());
            m_tasks.pop();
          }

          task();
        }
      });
    }
  }

  template <class F, class... Args>
  auto EnqueueImpl(F &&f, Args &&...args) -> std::future<typename std::invoke_result_t<F, Args...>> {
    using return_type = typename std::invoke_result_t<F, Args...>;

    std::packaged_task<return_type()> task(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<return_type> res = task.get_future();
    {
      std::unique_lock<std::mutex> lock(m_queue_mutex);
      if (m_stop_flag)
        throw std::runtime_error("Enqueue on stopped ThreadPool");
      m_tasks.emplace([t = std::move(task)]() mutable { t(); });
    }
    m_condition.notify_one();
    return res;
  }

  ~ThreadPool() {
    {
      std::unique_lock<std::mutex> lock(m_queue_mutex);
      m_stop_flag = true;
    }

    m_condition.notify_all();
  }

private:
  std::vector<std::jthread> m_workers;
  std::queue<std::move_only_function<void()>> m_tasks;

  std::mutex m_queue_mutex;
  std::condition_variable m_condition;
  bool m_stop_flag;
};
} // namespace craft