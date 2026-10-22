// Native port of obsidian-xiangqi c6f2197 PGN and comment metadata behavior.
#include "rules.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <variant>
namespace xqn::native {
namespace {
const std::vector<std::string> symbols = {"!", "?", "!!", "??", "!?", "?!"};
const std::vector<std::string> names = {"Good move", "Mistake",     "Brilliant",
                                        "Blunder",   "Interesting", "Inaccuracy"};
const std::vector<std::string> colors = {"#22ac38", "#e69f00", "#168226", "#df5353", "#ea45d8", "#56b4e9"};
const std::map<std::string, std::string> position_symbols = {
    {"10", "="}, {"13", "∞"}, {"16", "±"}, {"17", "∓"}};
Json glyph(std::string s) {
    if (s.size() > 1 && s[0] == '$') {
        if (s.size() != 2 || s[1] < '1' || s[1] > '6')
            return nullptr;
        s = symbols[s[1] - '1'];
    }
    auto i = std::find(symbols.begin(), symbols.end(), s);
    if (i == symbols.end())
        return nullptr;
    size_t n = i - symbols.begin();
    return {{"symbol", s}, {"name", names[n]}, {"color", colors[n]}};
}
void annotation(Json &meta, std::string key) {
    if (key == "+")
        key = "16";
    else if (key == "-")
        key = "17";
    else if (key == "=")
        key = "10";
    if (key != "bm" && key != "st" && !position_symbols.contains(key))
        return;
    if (!meta.contains("annotations"))
        meta["annotations"] = Json::array();
    auto &a = meta["annotations"];
    if (std::find(a.begin(), a.end(), key) == a.end())
        a.push_back(key);
}
Json eval_value(std::string s, bool privateForm = false) {
    s = trim(s);
    std::smatch m;
    if (std::regex_match(s, m, std::regex(privateForm ? R"(^m([+-]?\d+)$)" : R"(^[#M]([+-]?\d+)$)")))
        return {{"score", std::stoi(m[1])}, {"scoreType", "mate"}, {"depth", 0}};
    if (std::regex_match(s, std::regex(R"(^[+-]?\d+(?:\.\d+)?$)")))
        return {{"score", int(std::floor(std::stod(s) * 100 + .5))}, {"scoreType", "cp"}, {"depth", 0}};
    return nullptr;
}
std::string consume(std::string text, const std::regex &re,
                    const std::function<std::string(const std::smatch &)> &fn) {
    std::string out;
    std::smatch m;
    while (std::regex_search(text, m, re)) {
        out += m.prefix();
        out += fn(m);
        text = m.suffix();
    }
    return out + text;
}
void comment(Json &node, std::string raw) {
    raw = trim(raw);
    auto &meta = node["meta"];
    std::smatch m;
    if (std::regex_match(
            raw, m,
            std::regex(
                R"(^%e:([m+-]?\d+(?:\.\d+)?|[m+-]?[+-]\d+(?:\.\d+)?),?([a-i0-9]+)?,?([a-i0-9]+)?,?(!\?|\?!|\?\?|[?!]|!!)?$)"))) {
        auto ev = eval_value(m[1], true);
        if (!ev.is_null()) {
            if (m[2].matched)
                ev["bestmove"] = m[2].str();
            if (m[3].matched)
                ev["ponder"] = m[3].str();
            meta["eval"] = ev;
            auto g = glyph(m[4]);
            if (!g.is_null())
                meta["glyph"] = g;
            return;
        }
    }
    std::string payload;
    if (raw.starts_with("[#a:") && raw.ends_with("]"))
        payload = raw.substr(4, raw.size() - 5);
    else if (raw.starts_with("#a:"))
        payload = raw.substr(3);
    if (!payload.empty()) {
        auto g = glyph(payload);
        if (!g.is_null()) {
            meta["glyph"] = g;
            return;
        }
        for (auto &[key, symbol] : position_symbols)
            if (payload == symbol) {
                annotation(meta, key);
                return;
            }
        if (payload == "bm" || payload == "st" || payload == "+" || payload == "-" || payload == "=") {
            annotation(meta, payload);
            return;
        }
    }
    if (raw.starts_with("&s:")) {
        Json shapes = Json::array();
        std::istringstream input(raw.substr(3));
        std::string part;
        while (std::getline(input, part, ','))
            if (std::regex_match(part, m, std::regex(R"(^([a-i][0-9])([a-i][0-9])?:([gryb])$)"))) {
                Json shape = {{"orig", m[1].str()}, {"brush", m[3].str()}};
                if (m[2].matched)
                    shape["dest"] = m[2].str();
                shapes.push_back(shape);
            }
        if (!shapes.empty()) {
            meta["shapes"] = shapes;
            return;
        }
    }
    bool stripped = false;
    Json author = nullptr;
    for (auto tag : {"csl", "cal"}) {
        int count = std::string(tag) == "csl" ? 1 : 2;
        std::regex re("\\[\\s*%" + std::string(tag) + "\\s+((?:\\w[a-i][0-9]" +
                      (count == 2 ? "[a-i][0-9]" : "") + "[\\s,]*)+)\\]");
        raw = consume(raw, re, [&](const std::smatch &match) {
            stripped = true;
            if (!meta.contains("shapes"))
                meta["shapes"] = Json::array();
            auto group = match[1].str();
            std::regex piece(count == 1 ? R"(\w[a-i][0-9])" : R"(\w[a-i][0-9][a-i][0-9])");
            for (auto i = std::sregex_iterator(group.begin(), group.end(), piece);
                 i != std::sregex_iterator(); ++i) {
                auto s = i->str();
                char color = s[0];
                std::string brush = color == 'G' ? "g" : color == 'R' ? "r" : color == 'Y' ? "y" : "b";
                Json shape = {{"orig", s.substr(1, 2)}, {"brush", brush}};
                if (count == 2)
                    shape["dest"] = s.substr(3, 2);
                meta["shapes"].push_back(shape);
            }
            return std::string{};
        });
    }
    raw = consume(raw, std::regex(R"(\[\s*%eval\s+([^\]]*?)\])"), [&](const std::smatch &match) {
        auto e = eval_value(match[1]);
        if (e.is_null())
            return match[0].str();
        meta["eval"] = e;
        stripped = true;
        return std::string{};
    });
    raw = consume(raw, std::regex(R"(\[\s*%clk\s+(\d+(?::\d{1,2}){1,2})\])"), [&](const std::smatch &match) {
        meta["clock"] = match[1].str();
        stripped = true;
        return std::string{};
    });
    raw = consume(raw, std::regex(R"re(\[\s*%anno\s+(?:"([^"]*)"|([^\],]+))\s*(?:,\s*([^\]\s]+))?\s*\])re"),
                  [&](const std::smatch &match) {
                      auto name = trim(match[1].matched ? match[1].str() : match[2].str());
                      auto user = match[3].str();
                      if (name.empty() && user.empty())
                          return match[0].str();
                      author = {{"name", name}, {"user", user}};
                      stripped = true;
                      return std::string{};
                  });
    if (stripped)
        raw = trim(std::regex_replace(raw, std::regex(R"(\s+)"), " "));
    if (!raw.empty()) {
        auto &comments = node["comments"];
        comments.push_back(raw);
        if (!author.is_null()) {
            if (!meta.contains("commentAuthors"))
                meta["commentAuthors"] = Json::array();
            auto &a = meta["commentAuthors"];
            while (a.size() < comments.size())
                a.push_back(nullptr);
            a[comments.size() - 1] = author;
        }
    }
}
struct Token {
    std::string kind, text;
    int line, column;
};
std::vector<Token> tokenize(const std::string &s) {
    std::vector<Token> out;
    size_t pos = 0;
    int line = 1, column = 1;
    auto advance = [&](size_t n) {
        for (size_t i = 0; i < n; i++) {
            unsigned char c = s[pos++];
            if (c == '\n') {
                ++line;
                column = 1;
            } else if ((c & 0xC0) != 0x80)
                ++column;
        }
    };
    static const std::regex number(R"(^\d+\s*(?:\.\s*)+)"), nag(R"(^\$\d+)"),
        move(R"(^[a-i][0-9]-?[a-i][0-9])", std::regex::icase), result(R"(^(1-0|0-1|1/2-1/2|\*))"),
        suffix(R"(^(!!|\?\?|!\?|\?!|[!?]))");
    while (pos < s.size()) {
        if (pos == 0 && s.substr(0, 3) == "\xef\xbb\xbf") {
            advance(3);
            continue;
        }
        unsigned char c = s[pos];
        if (std::isspace(c)) {
            advance(1);
            continue;
        }
        int l = line, col = column;
        auto begin = s.cbegin() + static_cast<std::ptrdiff_t>(pos);
        std::smatch m;
        std::string kind, text;
        if (std::regex_search(begin, s.cend(), m, number)) {
            advance(m.length());
            continue;
        }
        if (c == '%') {
            size_t end = s.find('\n', pos);
            advance((end == std::string::npos ? s.size() : end) - pos);
            continue;
        }
        if (std::regex_search(begin, s.cend(), m, nag))
            kind = "nag", text = m.str();
        else if (std::regex_search(begin, s.cend(), m, move))
            kind = "move", text = m.str();
        else if (c == '{') {
            int depth = 1;
            size_t end = pos + 1;
            while (end < s.size() && depth) {
                if (s[end] == '{')
                    ++depth;
                if (s[end] == '}')
                    --depth;
                ++end;
            }
            kind = "comment";
            text = s.substr(pos + 1, end - pos - 1 - (depth == 0 ? 1 : 0));
            out.push_back({kind, text, l, col});
            advance(end - pos);
            continue;
        } else if (c == ';') {
            size_t end = s.find('\n', pos);
            if (end == std::string::npos)
                end = s.size();
            kind = "comment";
            text = s.substr(pos + 1, end - pos - 1);
            out.push_back({kind, text, l, col});
            advance(end - pos);
            continue;
        } else if (c == '[') {
            size_t end = pos + 1;
            bool quoted = false, escaped = false;
            for (; end < s.size(); ++end) {
                char ch = s[end];
                if (escaped) {
                    escaped = false;
                    continue;
                }
                if (ch == '\\') {
                    escaped = true;
                    continue;
                }
                if (ch == '"')
                    quoted = !quoted;
                if (ch == ']' && !quoted) {
                    ++end;
                    break;
                }
            }
            kind = "tag";
            text = s.substr(pos, end - pos);
        } else if (std::regex_search(begin, s.cend(), m, result))
            kind = "result", text = m.str();
        else if (c == '(' || c == ')')
            kind = std::string(1, c), text = kind;
        else if (std::regex_search(begin, s.cend(), m, suffix))
            kind = "nag", text = m.str();
        else {
            static const std::regex bareFen(
                R"(^[pnrbckahePNRBCKAHE1-9]+(?:/[pnrbckahePNRBCKAHE1-9]+){9}(?:[ \t]+[wb](?=[ \t\r\n]|$)(?:[ \t]+-[ \t]+-(?:[ \t]+\d+(?:[ \t]+\d+)?)?)?)?)");
            if (std::regex_search(begin, s.cend(), m, bareFen)) {
                auto fen = m.str();
                out.push_back({"tag", "[FEN \"" + fen + "\"]", l, col});
                advance(fen.size());
                continue;
            }
            // Chinese/WXF input is accepted as an additional native notation.
            size_t end = pos;
            while (end < s.size() && !std::isspace(static_cast<unsigned char>(s[end])) &&
                   std::string("{}()[]!?").find(s[end]) == std::string::npos)
                ++end;
            if (end == pos)
                ++end;
            kind = "candidate";
            text = s.substr(pos, end - pos);
        }
        out.push_back({kind, text, l, col});
        advance(text.size());
    }
    return out;
}
std::string unescape(const std::string &s) {
    std::string out;
    bool escaped = false;
    for (char c : s) {
        if (escaped) {
            out += c;
            escaped = false;
        } else if (c == '\\')
            escaped = true;
        else
            out += c;
    }
    if (escaped)
        out += '\\';
    return out;
}
std::vector<std::string> split(const std::string &input) {
    std::vector<std::string> out;
    std::istringstream stream(input);
    std::string line, current;
    bool moves = false, tags = false, blank = false;
    int depth = 0;
    static const std::regex tag(R"re(^\[\w+\s+"(?:[^"\\]|\\.)*"\]$)re");
    while (std::getline(stream, line)) {
        auto t = trim(line);
        for (char c : t) {
            if (c == '{')
                ++depth;
            else if (c == '}')
                depth = std::max(0, depth - 1);
        }
        bool in = depth > 0, isTag = std::regex_match(t, tag);
        if (isTag && !in) {
            if (moves || blank && tags) {
                out.push_back(current);
                current.clear();
                moves = tags = blank = false;
            }
            tags = true;
            blank = false;
        } else if (t.empty()) {
            if (tags && !moves)
                blank = true;
        } else if (!in) {
            moves = true;
            blank = false;
        }
        current += line + "\n";
    }
    if (!trim(current).empty())
        out.push_back(current);
    if (out.empty())
        out.push_back("");
    return out;
}
Json node(std::string id, std::string fen, Json parent = nullptr, Json move = nullptr) {
    return {{"id", id},
            {"x", 0},
            {"y", 0},
            {"parent", parent},
            {"children", Json::array()},
            {"fen", fen},
            {"move", move},
            {"comments", Json::array()},
            {"meta", Json::object()}};
}
Json parse_game(const std::string &input) {
    auto tokens = tokenize(input);
    Json headers = Json::object(), warnings = Json::array();
    std::vector<Json> nodes = {
        node("node-root", std::string(initial_fen).substr(0, std::string(initial_fen).find(" - -")))};
    size_t current = 0;
    int counter = 1;
    bool aborted = false;
    std::function<void(size_t &, int)> walk = [&](size_t &i, int depth) {
        if (depth > 512)
            throw std::runtime_error("PGN variation nesting limit");
        while (i < tokens.size() && !aborted) {
            auto tok = tokens[i++];
            auto &n = nodes[current];
            if (tok.kind == ")")
                return;
            if (tok.kind == "(") {
                if (n["parent"].is_null()) {
                    while (i < tokens.size() && tokens[i].kind != ")")
                        ++i;
                    if (i < tokens.size())
                        ++i;
                    continue;
                }
                size_t save = current;
                auto parent = n["parent"].get<std::string>();
                current = 0;
                while (nodes[current]["id"] != parent)
                    ++current;
                walk(i, depth + 1);
                current = save;
                continue;
            }
            if (tok.kind == "tag") {
                std::smatch m;
                if (std::regex_match(tok.text, m, std::regex(R"re(^\[(\w+)\s+"((?:[^"\\]|\\.)*)"\]$)re"))) {
                    auto key = m[1].str(), value = unescape(m[2]);
                    headers[key] = value;
                    auto upper = key;
                    for (char &c : upper)
                        c = char(std::toupper(static_cast<unsigned char>(c)));
                    if (upper == "FEN")
                        try {
                            Position p(value);
                            nodes[0]["fen"] = value;
                        } catch (...) {
                            warnings.push_back({{"text", tok.text},
                                                {"line", tok.line},
                                                {"column", tok.column},
                                                {"kind", "fen"}});
                            aborted = true;
                        }
                }
                continue;
            }
            if (tok.kind == "comment") {
                comment(n, tok.text);
                continue;
            }
            if (tok.kind == "nag") {
                if (n["move"].is_null())
                    continue;
                auto g = glyph(tok.text);
                auto &meta = n["meta"];
                if (!g.is_null()) {
                    if (!meta.contains("glyph"))
                        meta["glyph"] = g;
                } else if (tok.text == "$24")
                    annotation(meta, "st");
                else if (tok.text.size() > 1 && tok.text[0] == '$')
                    annotation(meta, tok.text.substr(1));
                continue;
            }
            if (tok.kind == "result") {
                if (tok.text != "*")
                    n["meta"]["result"] = tok.text;
                if (depth == 0)
                    headers["Result"] = tok.text;
                else {
                    while (i < tokens.size() && tokens[i].kind != ")")
                        ++i;
                    if (i < tokens.size())
                        ++i;
                    return;
                }
                continue;
            }
            try {
                Position p(n.at("fen"));
                auto move = p.parse(tok.text);
                auto detail = p.describe(move);
                p.apply(move);
                auto id = "node-" + std::to_string(counter++);
                auto parent = n["id"];
                n["children"].push_back(id);
                nodes.push_back(node(id, p.fen(), parent, detail));
                current = nodes.size() - 1;
                if (nodes.size() > 100000)
                    throw std::runtime_error("PGN node limit");
            } catch (const std::exception &) {
                warnings.push_back({{"text", tok.text},
                                    {"line", tok.line},
                                    {"column", tok.column},
                                    {"kind", tok.kind == "move" ? "move" : "unknown"}});
                aborted = true;
            }
        }
    };
    size_t index = 0;
    walk(index, 0);
    if (headers.contains("Result") &&
        (headers["Result"] == "1-0" || headers["Result"] == "0-1" || headers["Result"] == "1/2-1/2")) {
        size_t tip = 0;
        while (!nodes[tip]["children"].empty()) {
            auto id = nodes[tip]["children"][0];
            tip = 0;
            while (nodes[tip]["id"] != id)
                ++tip;
        }
        if (!nodes[tip]["meta"].contains("result"))
            nodes[tip]["meta"]["result"] = headers["Result"];
    }
    Json flat = Json::array();
    std::unordered_map<std::string, size_t> map;
    for (size_t i = 0; i < nodes.size(); i++)
        map[nodes[i]["id"]] = i;
    std::vector<size_t> stack = {0};
    while (!stack.empty()) {
        auto idx = stack.back();
        stack.pop_back();
        flat.push_back(nodes[idx]);
        for (auto c = nodes[idx]["children"].rbegin(); c != nodes[idx]["children"].rend(); ++c)
            stack.push_back(map.at(c->get<std::string>()));
    }
    return {{"headers", headers},
            {"fen", nodes[0]["fen"]},
            {"root", "node-root"},
            {"nodes", flat},
            {"warnings", warnings}};
}
std::string score_string(const Json &ev, bool privateForm) {
    int score = ev.value("score", 0);
    if (ev.value("scoreType", std::string("cp")) == "mate")
        return std::string(privateForm ? "m" : "#") + (score >= 0 ? (privateForm ? "+" : "") : "-") +
               std::to_string(std::abs(score));
    std::ostringstream out;
    out << (score >= 0 ? "+" : "-") << std::fixed << std::setprecision(2) << std::abs(score) / 100.0;
    return out.str();
}
std::string serialize_meta(const Json &node, const std::string &format) {
    const auto &m = node.at("meta");
    std::string out;
    if (m.contains("glyph")) {
        auto s = m["glyph"].value("symbol", std::string{});
        auto i = std::find(symbols.begin(), symbols.end(), s);
        if (format == "nag" && i != symbols.end())
            out += " $" + std::to_string(i - symbols.begin() + 1);
        else if (format == "comment")
            out += " {[#a:" + s + "]}";
        else
            out += s;
    }
    if (m.contains("annotations"))
        for (auto &a : m["annotations"]) {
            auto key = a.get<std::string>();
            if (std::regex_match(key, std::regex(R"(\d+)"))) {
                if (format == "comment" && position_symbols.contains(key))
                    out += " {[#a:" + position_symbols.at(key) + "]}";
                else
                    out += " $" + key;
            } else
                out += " {#a:" + key + "}";
        }
    size_t index = 0;
    for (auto &c : node.at("comments")) {
        out += " {";
        if (m.contains("commentAuthors") && index < m["commentAuthors"].size() &&
            !m["commentAuthors"][index].is_null()) {
            auto &a = m["commentAuthors"][index];
            out += "[%anno \"" + a.value("name", std::string{}) + "\"";
            auto user = a.value("user", std::string{});
            if (!user.empty())
                out += ", " + user;
            out += "] ";
        }
        out += c.get<std::string>() + "}";
        ++index;
    }
    if (m.contains("clock"))
        out += " {[%clk " + m["clock"].get<std::string>() + "]}";
    if (m.contains("shapes")) {
        std::string highlights, arrows;
        for (auto &s : m["shapes"]) {
            auto brush = s.value("brush", std::string("b"));
            char color = brush == "g" ? 'G' : brush == "r" ? 'R' : brush == "y" ? 'Y' : 'B';
            auto &v = s.contains("dest") ? arrows : highlights;
            if (!v.empty())
                v += ",";
            v += color;
            v += s.at("orig").get<std::string>();
            if (s.contains("dest"))
                v += s["dest"].get<std::string>();
        }
        std::string block;
        if (!highlights.empty())
            block += "[%csl " + highlights + "]";
        if (!arrows.empty())
            block += "[%cal " + arrows + "]";
        if (!block.empty())
            out += " { " + block + " }";
    }
    if (m.contains("eval")) {
        auto &ev = m["eval"];
        out += " { [%eval " + score_string(ev, false) + "] }";
        auto best = ev.value("bestmove", std::string{});
        if (!best.empty()) {
            out += " {%e:" + score_string(ev, true) + "," + best;
            auto ponder = ev.value("ponder", std::string{});
            if (!ponder.empty())
                out += "," + ponder;
            out += "}";
        }
    }
    return out;
}
std::string escape(const std::string &s) {
    std::string out;
    for (char c : s) {
        if (c == '"' || c == '\\')
            out += '\\';
        out += c;
    }
    return out;
}
} // namespace
Json parse_pgn(const std::string &input) {
    Json games = Json::array();
    for (auto &game : split(input))
        games.push_back(parse_game(game));
    return games;
}
std::string stringify_pgn(const Json &data) {
    auto nodes = hydrate(data);
    std::unordered_map<std::string, size_t> map;
    for (size_t i = 0; i < nodes.size(); i++)
        map[nodes[i]["id"]] = i;
    const auto format = data.value("annotationFormat", std::string("suffix"));
    Position root(nodes[0].at("fen"));
    struct Visit {
        size_t index;
        int step;
        bool lineStart, variation;
        int level;
    };
    using Action = std::variant<Visit, std::string>;
    std::vector<Action> pending = {Visit{0, root.fullmove - (root.red ? 1 : 0), false, false, 0}};
    std::string result;
    while (!pending.empty()) {
        auto action = std::move(pending.back());
        pending.pop_back();
        if (auto text = std::get_if<std::string>(&action)) {
            result += *text;
            continue;
        }
        auto v = std::get<Visit>(action);
        if (v.level > 2048)
            throw std::runtime_error("PGN exceeds export depth limit");
        auto &n = nodes[v.index];
        bool redMove = false;
        if (!n["move"].is_null()) {
            Position before(nodes[map.at(n["parent"].get<std::string>())].at("fen"));
            redMove = before.red;
            if (redMove)
                result += std::to_string(v.step) + ". ";
            else if (v.lineStart)
                result += std::to_string(v.step) + "... ";
            result += n["move"]["iccs"].get<std::string>();
        }
        auto meta = serialize_meta(n, format);
        result += meta;
        std::vector<size_t> brothers;
        if (!n["parent"].is_null()) {
            auto &siblings = nodes[map.at(n["parent"].get<std::string>())]["children"];
            if (siblings.size() > 1 && siblings[0] == n["id"])
                for (size_t i = 1; i < siblings.size(); i++)
                    brothers.push_back(map.at(siblings[i].get<std::string>()));
        }
        if (!n["children"].empty()) {
            auto next = map.at(n["children"][0].get<std::string>());
            bool nextRed = Position(n.at("fen")).red;
            bool blackStart =
                !nextRed && (v.index == 0 || meta.find('{') != std::string::npos || !brothers.empty());
            pending.push_back(Visit{next, v.step + (nextRed ? 1 : 0), blackStart, v.variation, v.level + 1});
            pending.push_back(std::string(" "));
        } else if (n["meta"].contains("result"))
            pending.push_back(" " + n["meta"]["result"].get<std::string>());
        else if (!v.variation)
            pending.push_back(std::string(" *"));
        for (auto i = brothers.rbegin(); i != brothers.rend(); ++i) {
            pending.push_back(std::string(")"));
            pending.push_back(Visit{*i, v.step, !redMove, true, v.level + 1});
            pending.push_back(std::string(" ("));
        }
    }
    std::string tags;
    auto headers = data.value("headers", Json::object());
    for (auto &[key, value] : headers.items()) {
        if (!tags.empty())
            tags += '\n';
        tags += "[" + key + " \"" + escape(value.get<std::string>()) + "\"]";
    }
    return tags + "\n\n" + trim(result);
}

} // namespace xqn::native
