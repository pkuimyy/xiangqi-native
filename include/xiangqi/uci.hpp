#pragma once
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <xiangqi/kernel.hpp>
namespace xqn {
class UciEngine {
  public:
    explicit UciEngine(const std::filesystem::path &executable);
    ~UciEngine();
    UciEngine(const UciEngine &) = delete;
    UciEngine &operator=(const UciEngine &) = delete;
    Json analyze(const std::string &fen, int depth = 15, int skill = 20, int multipv = 1,
                 std::chrono::milliseconds timeout = std::chrono::seconds(60));
    void stop() noexcept;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
Json red_view(Json evaluation, const std::string &fen);
Json parse_uci_info(const std::string &line);
} // namespace xqn
