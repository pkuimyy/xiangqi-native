#include <algorithm>
#include <oxq/core/reader.hpp>
#include <oxq/core/validation.hpp>
#include <oxq/core/writer.hpp>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <xiangqi/document.hpp>
namespace xqn {
namespace {
constexpr auto meta_author = "org.xiangqi.native/v1";
oxq::core::Uuid uuid() {
    oxq::core::Uuid result;
    std::random_device r;
    for (auto &b : result.bytes)
        b = static_cast<std::uint8_t>(r());
    result.bytes[6] = (result.bytes[6] & 15) | 64;
    result.bytes[8] = (result.bytes[8] & 63) | 128;
    return result;
}
oxq::core::Position from_fen(const std::string &fen) {
    oxq::core::Position p;
    std::istringstream input(fen);
    std::string board, side, token;
    input >> board >> side;
    p.side_to_move = side == "b" ? oxq::core::Side::black : oxq::core::Side::red;
    int rank = 9, file = 0;
    const std::string types = "kabnrcp";
    for (char c : board) {
        if (c == '/') {
            --rank;
            file = 0;
        } else if (c >= '1' && c <= '9')
            file += c - '0';
        else {
            auto t = types.find(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            if (t == std::string::npos || rank < 0 || file > 8)
                throw std::runtime_error("Invalid FEN");
            p.pieces.push_back(
                {std::isupper(static_cast<unsigned char>(c)) ? oxq::core::Side::red : oxq::core::Side::black,
                 static_cast<oxq::core::PieceType>(t + 1), static_cast<std::uint8_t>(rank * 9 + file++)});
        }
    }
    // Fullmove is the sixth FEN field when present.
    if (input >> token >> token >> token >> token) {
        auto n = std::stoul(token);
        if (n == 0 || n > 65535)
            throw std::runtime_error("FEN fullmove out of range");
        p.fullmove_number = static_cast<std::uint16_t>(n);
    }
    return p;
}
std::string to_fen(const oxq::core::Position &p) {
    std::array<char, 90> b{};
    const std::string types = "kabnrcp";
    for (auto piece : p.pieces) {
        char c = types.at(static_cast<int>(piece.type) - 1);
        if (piece.side == oxq::core::Side::red)
            c = static_cast<char>(std::toupper(c));
        b.at(piece.square) = c;
    }
    std::string out;
    for (int rank = 9; rank >= 0; --rank) {
        int empty = 0;
        for (int file = 0; file < 9; ++file) {
            char c = b[rank * 9 + file];
            if (!c) {
                ++empty;
                continue;
            }
            if (empty) {
                out += static_cast<char>('0' + empty);
                empty = 0;
            }
            out += c;
        }
        if (empty)
            out += static_cast<char>('0' + empty);
        if (rank)
            out += '/';
    }
    return out + (p.side_to_move == oxq::core::Side::red ? " w - - 0 " : " b - - 0 ") +
           std::to_string(p.fullmove_number);
}
Json meta(const oxq::core::MoveNode &n) {
    for (auto &a : n.annotations)
        if (a.kind == oxq::core::AnnotationKind::source_note && a.author == meta_author) {
            auto j = Json::parse(a.text);
            if (j.value("version", 0) != 1)
                throw std::runtime_error("Unsupported node metadata version");
            return j.at("data");
        }
    return Json::object();
}
Json comments(const oxq::core::MoveNode &n) {
    Json a = Json::array();
    for (auto &x : n.annotations)
        if (x.kind == oxq::core::AnnotationKind::comment)
            a.push_back(x.text);
    return a;
}
} // namespace
Document::Document(Kernel &kernel) : kernel_(&kernel) {
    model_.uuid = uuid();
    model_.initial_position = from_fen(kernel.call("initial").at("fen"));
    rebuild();
}
const Json &Document::node(NodeId id) const { return cache_.at(id); }
void Document::rebuild() {
    auto data = kernel_->call("layout", transport());
    cache_.clear();
    for (auto &n : data.at("nodes"))
        cache_[std::stoull(n.at("id").get<std::string>())] = n;
}
Json Document::transport() const {
    Json headers = Json::object();
    auto ns = model_.metadata.extensions.find("org.xiangqi.native");
    if (ns != model_.metadata.extensions.end()) {
        auto it = ns->second.find("pgn_headers");
        if (it != ns->second.end())
            headers = Json::parse(std::get<std::string>(it->second));
    }
    if (model_.metadata.red_player.name)
        headers["Red"] = *model_.metadata.red_player.name;
    if (model_.metadata.black_player.name)
        headers["Black"] = *model_.metadata.black_player.name;
    if (model_.metadata.title)
        headers["Title"] = *model_.metadata.title;

    if (model_.metadata.event.name)
        headers["Event"] = *model_.metadata.event.name;
    if (model_.metadata.event.location)
        headers["Site"] = *model_.metadata.event.location;
    if (model_.metadata.event.round)
        headers["Round"] = *model_.metadata.event.round;
    if (model_.metadata.opening.code)
        headers["ECO"] = *model_.metadata.opening.code;
    if (model_.metadata.opening.name)
        headers["Opening"] = *model_.metadata.opening.name;
    if (model_.metadata.result) {
        switch (*model_.metadata.result) {
        case oxq::core::GameResult::red_win:
            headers["Result"] = "1-0";
            break;
        case oxq::core::GameResult::black_win:
            headers["Result"] = "0-1";
            break;
        case oxq::core::GameResult::draw:
            headers["Result"] = "1/2-1/2";
            break;
        default:
            if (!headers.contains("Result"))
                headers["Result"] = "*";
            break;
        }
    }

    auto fen = to_fen(model_.initial_position);
    headers["FEN"] = fen;
    Json nodes = Json::array();
    for (auto &n : model_.move_tree.nodes) {
        Json children = Json::array();
        for (auto c : n.children)
            children.push_back(std::to_string(c));
        Json move = nullptr;
        if (n.move)
            move = {{"from", n.move->from_square}, {"to", n.move->to_square}};
        nodes.push_back({{"id", std::to_string(n.id)},
                         {"parent", n.parent ? Json(std::to_string(*n.parent)) : Json(nullptr)},
                         {"children", children},
                         {"move", move},
                         {"comments", comments(n)},
                         {"meta", meta(n)}});
    }
    return {{"root", std::to_string(model_.move_tree.nodes.front().id)},
            {"fen", fen},
            {"nodes", nodes},
            {"headers", headers}};
}
Document Document::from_model(Kernel &kernel, oxq::core::GameModel model) {
    auto issues = oxq::core::validate(model);
    if (oxq::core::has_errors(issues))
        throw std::runtime_error("Invalid OXQ model");
    Document d(kernel);
    d.model_ = std::move(model);
    d.current_ = d.model_.move_tree.nodes.front().id;
    d.rebuild();
    return d;
}
std::vector<Document> Document::parse_pgn(Kernel &kernel, const std::string &text) {
    auto parsed = kernel.call("parse", {{"pgn", text}});
    std::vector<Document> result;
    for (auto &game : parsed) {
        if (!game.at("warnings").empty())
            throw std::runtime_error(
                "PGN contains unrecognized or illegal content; original document retained: " +
                game.at("warnings").dump());
        Document d(kernel);
        d.model_.initial_position = from_fen(game.at("fen"));
        auto headers = game.at("headers");
        d.model_.metadata.extensions["org.xiangqi.native"]["pgn_headers"] = headers.dump();
        if (headers.contains("Red"))
            d.model_.metadata.red_player.name = headers["Red"];
        if (headers.contains("Black"))
            d.model_.metadata.black_player.name = headers["Black"];
        if (headers.contains("Title"))
            d.model_.metadata.title = headers["Title"];

        if (headers.contains("White") && !headers.contains("Red"))
            d.model_.metadata.red_player.name = headers["White"];
        if (headers.contains("Event"))
            d.model_.metadata.event.name = headers["Event"];
        if (headers.contains("Site"))
            d.model_.metadata.event.location = headers["Site"];
        if (headers.contains("Round"))
            d.model_.metadata.event.round = headers["Round"];
        if (headers.contains("ECO"))
            d.model_.metadata.opening.code = headers["ECO"];
        if (headers.contains("Opening"))
            d.model_.metadata.opening.name = headers["Opening"];
        if (headers.contains("Result")) {
            const auto value = headers["Result"].get<std::string>();
            if (value == "1-0")
                d.model_.metadata.result = oxq::core::GameResult::red_win;
            else if (value == "0-1")
                d.model_.metadata.result = oxq::core::GameResult::black_win;
            else if (value == "1/2-1/2")
                d.model_.metadata.result = oxq::core::GameResult::draw;
            else if (value == "*")
                d.model_.metadata.result = oxq::core::GameResult::unfinished;
        }

        std::unordered_map<std::string, NodeId> ids;
        ids[game.at("root")] = 0;
        for (auto &n : game.at("nodes")) {
            std::string old = n.at("id");
            NodeId id = 0;
            if (!n.at("parent").is_null()) {
                auto &m = n.at("move");
                id = d.model_.move_tree.addNode(ids.at(n.at("parent")), {m.at("from").get<std::uint8_t>(),
                                                                         m.at("to").get<std::uint8_t>()});
            }
            ids[old] = id;
            auto *target = d.model_.move_tree.findNode(id);
            for (auto &c : n.at("comments"))
                target->annotations.push_back(
                    {oxq::core::AnnotationKind::comment, false, c.get<std::string>(), {}, {}});
            d.write_meta(id, n.at("meta"));
        }
        d.rebuild();
        result.push_back(std::move(d));
    }
    return result;
}
void Document::select(NodeId id) {
    if (!cache_.contains(id))
        throw std::runtime_error("Unknown node");
    current_ = id;
}
void Document::back() {
    auto *n = model_.move_tree.findNode(current_);
    if (n->parent)
        current_ = *n->parent;
}
void Document::next() {
    auto *n = model_.move_tree.findNode(current_);
    if (!n->children.empty())
        current_ = n->children.front();
}
void Document::start() { current_ = model_.move_tree.nodes.front().id; }
void Document::end() {
    while (true) {
        auto *n = model_.move_tree.findNode(current_);
        if (n->children.empty())
            return;
        current_ = n->children.front();
    }
}
Json Document::position() { return kernel_->call("position", {{"fen", node(current_).at("fen")}}); }
void Document::checkpoint() {
    undo_.push_back({model_, current_, revision_});
    if (undo_.size() > 100)
        undo_.erase(undo_.begin());
    redo_.clear();
    revision_ = next_revision_++;
}
NodeId Document::play(int from, int to) {
    auto valid = kernel_->call("move", {{"fen", node(current_).at("fen")}, {"from", from}, {"to", to}});
    auto *parent = model_.move_tree.findNode(current_);
    for (auto id : parent->children) {
        auto *n = model_.move_tree.findNode(id);
        if (n->move->from_square == from && n->move->to_square == to) {
            current_ = id;
            return id;
        }
    }
    checkpoint();
    bool extends = parent->children.empty();
    auto inherited = meta(*parent);
    auto id =
        model_.move_tree.addNode(current_, {static_cast<std::uint8_t>(from), static_cast<std::uint8_t>(to)});
    Json newmeta = Json::object();
    if (extends && inherited.contains("result")) {
        newmeta["result"] = inherited["result"];
        inherited.erase("result");
        write_meta(current_, inherited);
    }
    if (!valid.at("result").is_null())
        newmeta["result"] = valid.at("result");
    write_meta(id, newmeta);
    current_ = id;
    rebuild();
    return id;
}
void Document::write_meta(NodeId id, const Json &value) {
    auto *n = model_.move_tree.findNode(id);
    std::erase_if(n->annotations, [](auto &a) {
        return a.kind == oxq::core::AnnotationKind::source_note && a.author == meta_author;
    });
    if (!value.empty())
        n->annotations.push_back({oxq::core::AnnotationKind::source_note,
                                  false,
                                  Json{{"version", 1}, {"data", value}}.dump(),
                                  meta_author,
                                  {}});
}
void Document::set_comments(std::vector<std::string> lines) {
    if (comments(*model_.move_tree.findNode(current_)) == Json(lines))
        return;
    checkpoint();
    auto *n = model_.move_tree.findNode(current_);
    std::erase_if(n->annotations, [](auto &a) { return a.kind == oxq::core::AnnotationKind::comment; });
    for (auto &s : lines)
        n->annotations.push_back({oxq::core::AnnotationKind::comment, false, std::move(s), {}, {}});
    auto m = meta(*n);
    m.erase("commentAuthors");
    write_meta(current_, m);
    rebuild();
}
void Document::set_evaluation(const Json &evaluation) {
    checkpoint();
    auto m = meta(*model_.move_tree.findNode(current_));
    m["eval"] = evaluation;
    write_meta(current_, m);
    rebuild();
}
void Document::erase_current() {
    auto *n = model_.move_tree.findNode(current_);
    if (!n->parent)
        return;
    auto parent = *n->parent;
    checkpoint();
    if (!model_.move_tree.removeNode(current_))
        throw std::runtime_error("Cannot remove node");
    current_ = parent;
    rebuild();
}
void Document::promote_current() {
    auto *n = model_.move_tree.findNode(current_);
    if (!n->parent)
        return;
    auto parent = *n->parent;
    auto *p = model_.move_tree.findNode(parent);
    if (p->children.front() == current_)
        return;
    checkpoint();
    std::erase(p->children, current_);
    p->children.insert(p->children.begin(), current_);
    rebuild();
}
bool Document::undo() {
    if (undo_.empty())
        return false;
    redo_.push_back({model_, current_, revision_});
    auto s = std::move(undo_.back());
    undo_.pop_back();
    model_ = std::move(s.model);
    current_ = s.current;
    revision_ = s.revision;
    rebuild();
    return true;
}
bool Document::redo() {
    if (redo_.empty())
        return false;
    undo_.push_back({model_, current_, revision_});
    auto s = std::move(redo_.back());
    redo_.pop_back();
    model_ = std::move(s.model);
    current_ = s.current;
    revision_ = s.revision;
    rebuild();
    return true;
}
std::string Document::pgn() { return kernel_->call("stringify", transport()).get<std::string>(); }
std::vector<std::byte> Document::oxq() const {
    auto result = oxq::core::write_oxq(model_);
    if (auto *error = std::get_if<oxq::core::WriterError>(&result))
        throw std::runtime_error(error->message);
    return std::get<std::vector<std::byte>>(std::move(result));
}
} // namespace xqn
