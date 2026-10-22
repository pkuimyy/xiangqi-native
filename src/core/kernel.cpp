#include "rules.hpp"
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
namespace xqn {
struct Kernel::Impl {};
Kernel::Kernel() : impl_(std::make_unique<Impl>()) {}
Kernel::~Kernel() = default;
Json Kernel::call(const std::string &op, const Json &data) {
    using namespace native;
    if (op == "initial")
        return {{"fen", initial_fen}};
    if (op == "position")
        return Position(data.at("fen")).view();
    if (op == "move") {
        Position p(data.at("fen"));
        auto m = data.contains("notation") ? p.parse(data.at("notation"))
                                           : p.parse(square(data.at("from")) + square(data.at("to")));
        auto move = p.describe(m);
        p.apply(m);
        Json result = nullptr;
        if (p.moves().empty())
            result = p.red ? "0-1" : "1-0";
        return {{"fen", p.fen()}, {"move", move}, {"check", p.check(p.red)}, {"result", result}};
    }
    if (op == "parse")
        return parse_pgn(data.at("pgn"));
    if (op == "hydrate" || op == "layout") {
        auto nodes = hydrate(data);
        if (op == "layout")
            layout(nodes);
        return {{"nodes", nodes}};
    }
    if (op == "stringify")
        return stringify_pgn(data);
    throw std::runtime_error("Unknown kernel operation: " + op);
}
namespace native {
Json hydrate(const Json &data) {
    std::unordered_map<std::string, Json> source;
    for (auto &n : data.at("nodes"))
        if (!source.emplace(n.at("id").get<std::string>(), n).second)
            throw std::runtime_error("Duplicate tree node");
    auto root = data.at("root").get<std::string>();
    if (!source.contains(root))
        throw std::runtime_error("Missing root");
    Json nodes = Json::array();
    std::unordered_set<std::string> visited;
    struct Pending {
        std::string id, parent, fen;
    };
    std::vector<Pending> stack = {{root, "", data.at("fen")}};
    while (!stack.empty()) {
        auto entry = stack.back();
        stack.pop_back();
        if (!visited.insert(entry.id).second)
            throw std::runtime_error("Cyclic/shared tree node");
        if (nodes.size() >= 100000)
            throw std::runtime_error("Too many tree nodes");
        auto n = source.at(entry.id);
        Position p(entry.fen);
        if (entry.id == root) {
            n["move"] = nullptr;
            n["parent"] = nullptr;
        } else {
            auto m = p.parse(square(n.at("move").at("from")) + square(n.at("move").at("to")));
            n["move"] = p.describe(m);
            p.apply(m);
            n["parent"] = entry.parent;
        }
        n["fen"] = p.fen();
        n["x"] = 0;
        n["y"] = 0;
        if (!n.contains("comments"))
            n["comments"] = Json::array();
        if (!n.contains("meta"))
            n["meta"] = Json::object();
        nodes.push_back(n);
        auto &children = n.at("children");
        for (auto i = children.rbegin(); i != children.rend(); ++i)
            stack.push_back({i->get<std::string>(), entry.id, n.at("fen")});
    }
    if (visited.size() != source.size())
        throw std::runtime_error("Orphan tree node");
    return nodes;
}
} // namespace native
} // namespace xqn
