#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <xiangqi/uci.hpp>

namespace xqn {
struct LiveAnalysisEvent {
    enum class Kind { Started, Update, Finished, Error };
    std::uint64_t revision{};
    Kind kind{};
    Json value;
    std::string message;
};
// One warm engine, latest-position wins, debounced requests, no UI-thread engine work.
class LiveAnalysis {
  public:
    using Handler = std::function<void(const LiveAnalysisEvent &)>;
    explicit LiveAnalysis(Handler handler);
    ~LiveAnalysis();
    LiveAnalysis(const LiveAnalysis &) = delete;
    LiveAnalysis &operator=(const LiveAnalysis &) = delete;
    std::uint64_t request(std::filesystem::path executable, std::string fen);
    void stop();

  private:
    struct Job {
        std::uint64_t revision;
        std::filesystem::path executable;
        std::string fen;
        std::chrono::steady_clock::time_point ready;
    };
    Handler handler_;
    std::mutex mutex_;
    std::condition_variable changed_;
    std::optional<Job> pending_;
    std::shared_ptr<UciEngine> engine_;
    std::atomic<std::uint64_t> revision_{0};
    std::atomic<bool> closing_{false};
    std::thread worker_;
    void run();
};
} // namespace xqn
