#pragma once
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
namespace xqn {
using Json = nlohmann::json;
class Kernel {
  public:
    Kernel();
    ~Kernel();
    Kernel(const Kernel &) = delete;
    Kernel &operator=(const Kernel &) = delete;
    Json call(const std::string &operation, const Json &data = Json::object());

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace xqn
