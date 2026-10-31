#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

namespace blobber {
class PreparationWorker;
struct PreparationCancelled : std::runtime_error {
    PreparationCancelled() : std::runtime_error("Loading cancelled.") {}
};
// Preparation-only checkpoints. No callbacks, tasks or synchronization in ordinary frames.
struct PreparationContext {
    void (*point)(void*, const char*) = nullptr;
    void* owner = nullptr;
    PreparationWorker* worker = nullptr;
};
inline thread_local PreparationContext preparationContext;
class PreparationScope {
  public:
    explicit PreparationScope(PreparationContext context) : previous_(preparationContext) {
        preparationContext = context;
    }
    ~PreparationScope() { preparationContext = previous_; }
    PreparationScope(const PreparationScope&) = delete;

  private:
    PreparationContext previous_;
};
// Phase labels have static storage and outlive the submitted task. Never pass temporary strings.
inline void preparationPoint(const char* phase) {
    if (preparationContext.point)
        preparationContext.point(preparationContext.owner, phase);
}
// Exactly one queued task and one result; callers consume before submitting another.
class PreparationWorker {
  public:
    PreparationWorker() : thread_([this] { loop(); }) {}
    ~PreparationWorker() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
        }
        ready_.notify_one();
        thread_.join();
    }
    PreparationWorker(const PreparationWorker&) = delete;
    void cancel() noexcept { cancelled_ = true; }
    void reset() noexcept {
        cancelled_ = false;
        phase_ = "Loading";
    }
    bool cancelled() const noexcept { return cancelled_; }
    template <class F> auto run(F&& f) -> std::invoke_result_t<F> {
        using T = std::invoke_result_t<F>;
        std::packaged_task<T()> task(std::forward<F>(f));
        auto result = task.get_future();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (task_ || busy_)
                throw std::logic_error("A loading task is already active");
            task_ = [&task, this] {
                PreparationScope scope({[](void* owner, const char* phase) {
                                            auto& worker = *static_cast<PreparationWorker*>(owner);
                                            worker.phase_ = phase;
                                            if (worker.cancelled_)
                                                throw PreparationCancelled();
                                        },
                                        this, nullptr});
                task();
            };
            busy_ = true;
        }
        ready_.notify_one();
        // Keep task and borrowed inputs alive even when the UI callback fails.
        std::exception_ptr pumpError;
        while (result.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
            try {
                preparationPoint(phase_.load());
            } catch (...) {
                if (!pumpError)
                    pumpError = std::current_exception();
                cancel();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        // The packaged result becomes ready before loop releases its task reference.
        {
            std::unique_lock<std::mutex> lock(mutex_);
            idle_.wait(lock, [this] { return !busy_; });
        }
        if (pumpError)
            std::rethrow_exception(pumpError);
        if (cancelled_)
            throw PreparationCancelled();
        return result.get();
    }

  private:
    std::mutex mutex_;
    std::condition_variable ready_, idle_;
    std::function<void()> task_;
    bool stopping_ = false, busy_ = false;
    std::atomic<bool> cancelled_{false};
    std::atomic<const char*> phase_{"Loading"};
    std::thread thread_;
    void loop() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                ready_.wait(lock, [this] { return stopping_ || bool(task_); });
                if (stopping_)
                    return;
                task = std::move(task_);
                task_ = {};
            }
            task();
            {
                std::lock_guard<std::mutex> lock(mutex_);
                busy_ = false;
            }
            idle_.notify_one();
        }
    }
};
template <class F> auto prepareCpu(F&& f) -> std::invoke_result_t<F> {
    if (preparationContext.worker)
        return preparationContext.worker->run(std::forward<F>(f));
    return std::forward<F>(f)();
}
} // namespace blobber
