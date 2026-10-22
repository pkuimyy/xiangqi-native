#pragma once
#include <filesystem>
#include <oxq/core/game_model.hpp>
#include <unordered_map>
#include <vector>
#include <xiangqi/kernel.hpp>
namespace xqn {
using NodeId = oxq::core::NodeId;
class Document {
  public:
    explicit Document(Kernel &kernel);
    static std::vector<Document> parse_pgn(Kernel &kernel, const std::string &text);
    static Document from_model(Kernel &kernel, oxq::core::GameModel model);
    const oxq::core::GameModel &model() const { return model_; }
    NodeId current() const { return current_; }
    void select(NodeId id);
    void back();
    void next();
    void start();
    void end();
    NodeId play(int from, int to);
    void set_comments(std::vector<std::string> comments);
    void set_evaluation(const Json &evaluation);
    void erase_current();
    void promote_current();
    bool undo();
    bool redo();
    bool modified() const { return revision_ != saved_revision_; }
    void mark_saved() { saved_revision_ = revision_; }
    const Json &node(NodeId id) const;
    Json position();
    Json transport() const;
    std::string pgn();
    std::vector<std::byte> oxq() const;

  private:
    struct State {
        oxq::core::GameModel model;
        NodeId current;
        std::uint64_t revision;
    };
    Kernel *kernel_;
    oxq::core::GameModel model_;
    NodeId current_{};
    std::unordered_map<NodeId, Json> cache_;
    std::vector<State> undo_, redo_;
    std::uint64_t revision_{}, saved_revision_{}, next_revision_{1};
    void checkpoint();
    void rebuild();
    void write_meta(NodeId id, const Json &meta);
};
std::vector<std::byte> read_binary(const std::filesystem::path &path);
void save_atomic(const std::filesystem::path &path, const std::vector<std::byte> &bytes);
std::vector<Document> open_documents(Kernel &kernel, const std::filesystem::path &path);
} // namespace xqn
