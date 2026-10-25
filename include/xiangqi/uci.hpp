#pragma once
#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <xiangqi/kernel.hpp>
namespace xqn {
class UciEngine {
  public:
    using Cancel = std::function<bool()>;
    using Update = std::function<void(const Json &)>;
    explicit UciEngine(const std::filesystem::path &executable, Cancel cancelled = {});
    ~UciEngine();
    UciEngine(const UciEngine &) = delete;
    UciEngine &operator=(const UciEngine &) = delete;
    Json analyze(const std::string &fen, int depth = 15, int skill = 20, int multipv = 1,
                 std::chrono::milliseconds timeout = std::chrono::seconds(60));
    Json analyze_live(const std::string &fen, Update update, Cancel cancelled = {});
    void stop() noexcept;

  private:
    Json search(const std::string &fen, int depth, int skill, int multipv, std::chrono::milliseconds timeout,
                bool continuous, Update update, Cancel cancelled);
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
Json red_view(Json evaluation, const std::string &fen);
Json parse_uci_info(const std::string &line);
} // namespace xqn
