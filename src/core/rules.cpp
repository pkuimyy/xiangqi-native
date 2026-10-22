// C++ port of @weshell/xiangqi.js 1.0.3 notation and rules.
// Copyright (c) 2025, Jeff Hlywa (jhlywa@gmail.com).
// BSD-2-Clause; see licenses/xiangqi.js-LICENSE.
#include "rules.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>
namespace xqn::native {
namespace {
bool isred(char c) { return c >= 'A' && c <= 'Z'; }
char type(char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
bool palace(bool red, int x, int y) { return x >= 3 && x <= 5 && (red ? y <= 2 : y >= 7); }
const std::array<std::string, 9> cn = {"一", "二", "三", "四", "五", "六", "七", "八", "九"};
const std::array<std::string, 9> fw = {"１", "２", "３", "４", "５", "６", "７", "８", "９"};
std::string replace(std::string s, const std::string &from, const std::string &to) {
    size_t p = 0;
    while ((p = s.find(from, p)) != std::string::npos) {
        s.replace(p, from.size(), to);
        p += to.size();
    }
    return s;
}
std::string normalized(std::string s) {
    const std::pair<const char *, const char *> aliases[] = {{"車", "车"}, {"俥", "车"}, {"馬", "马"},
                                                             {"傌", "马"}, {"砲", "炮"}, {"將", "将"},
                                                             {"帥", "帅"}, {"進", "进"}, {"後", "后"}};
    for (auto [a, b] : aliases)
        s = replace(s, a, b);
    for (int i = 0; i < 9; i++) {
        s = replace(s, cn[i], std::to_string(i + 1));
        s = replace(s, fw[i], std::to_string(i + 1));
    }
    for (char &c : s)
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(std::tolower(c));
    // Common WXF front/back ordering R+.5, as well as upstream +R.5.
    if (s.size() == 4 && std::isalpha(static_cast<unsigned char>(s[0])) &&
        (s[1] == '+' || s[1] == '-' || s[1] == '.'))
        std::swap(s[0], s[1]);
    return s;
}
} // namespace
std::string square(int s) {
    if (s < 0 || s >= 90)
        throw std::runtime_error("Square outside board");
    return std::string(1, char('a' + s % 9)) + char('0' + s / 9);
}
std::string trim(std::string s) {
    auto a = s.find_first_not_of(" \r\n\t");
    if (a == std::string::npos)
        return {};
    return s.substr(a, s.find_last_not_of(" \r\n\t") - a + 1);
}
Position::Position(const std::string &fen) {
    std::istringstream in(fen);
    std::vector<std::string> fields;
    std::string word;
    while (in >> word)
        fields.push_back(word);
    if (fields.empty() || fields.size() > 6)
        throw std::runtime_error("Invalid FEN fields");
    const std::array<std::string, 6> defaults = {"", "w", "-", "-", "0", "1"};
    while (fields.size() < 6)
        fields.push_back(defaults[fields.size()]);
    if (fields[1] != "w" && fields[1] != "b")
        throw std::runtime_error("Invalid FEN turn");
    if (fields[2] != "-" || fields[3] != "-")
        throw std::runtime_error("Invalid Xiangqi FEN flags");
    auto number = [](const std::string &s) {
        size_t used{};
        int n = std::stoi(s, &used);
        if (used != s.size())
            throw std::runtime_error("Invalid FEN counter");
        return n;
    };
    halfmove = number(fields[4]);
    fullmove = number(fields[5]);
    if (halfmove < 0 || fullmove < 1 || halfmove >= 2147483647 || fullmove >= 2147483647)
        throw std::runtime_error("Invalid FEN counters");
    red = fields[1] == "w";
    int r = 9, f = 0;
    bool digit = false;
    for (char c : fields[0]) {
        if (c == '/') {
            if (f != 9 || r == 0)
                throw std::runtime_error("Invalid FEN rank");
            --r;
            f = 0;
            digit = false;
            continue;
        }
        if (c >= '1' && c <= '9') {
            if (digit)
                throw std::runtime_error("Consecutive FEN digits");
            f += c - '0';
            digit = true;
        } else {
            digit = false;
            if (f >= 9 || std::string("pnrbckahePNRBCKAHE").find(c) == std::string::npos)
                throw std::runtime_error("Invalid FEN piece");
            if (c == 'h')
                c = 'n';
            if (c == 'H')
                c = 'N';
            if (c == 'e')
                c = 'b';
            if (c == 'E')
                c = 'B';
            board[r * 9 + f++] = c;
        }
        if (f > 9)
            throw std::runtime_error("FEN rank overflow");
    }
    if (r != 0 || f != 9)
        throw std::runtime_error("FEN requires ten ranks");
    if (std::count(board.begin(), board.end(), 'K') != 1 || std::count(board.begin(), board.end(), 'k') != 1)
        throw std::runtime_error("FEN requires one king per side");
    for (int s = 0; s < 90; s++) {
        char c = type(board[s]);
        int x = s % 9, y = isred(board[s]) ? s / 9 : 9 - s / 9;
        if (c == 'a' && !((y == 0 || y == 2) && (x == 3 || x == 5) || y == 1 && x == 4))
            throw std::runtime_error("Invalid advisor position");
        if (c == 'b' && !((y == 0 || y == 4) && (x == 2 || x == 6) || y == 2 && (x == 0 || x == 4 || x == 8)))
            throw std::runtime_error("Invalid elephant position");
    }
    if (facing())
        throw std::runtime_error("Flying generals in FEN");
}
std::string Position::fen() const {
    std::string s;
    for (int r = 9; r >= 0; r--) {
        int empty = 0;
        for (int f = 0; f < 9; f++) {
            char c = board[r * 9 + f];
            if (!c) {
                ++empty;
                continue;
            }
            if (empty) {
                s += char('0' + empty);
                empty = 0;
            }
            s += c;
        }
        if (empty)
            s += char('0' + empty);
        if (r)
            s += '/';
    }
    return s + (red ? " w - - " : " b - - ") + std::to_string(halfmove) + " " + std::to_string(fullmove);
}
bool Position::facing() const {
    auto a = std::find(board.begin(), board.end(), 'K'), b = std::find(board.begin(), board.end(), 'k');
    if (a == board.end() || b == board.end())
        return false;
    int s = int(a - board.begin()), t = int(b - board.begin());
    if (s % 9 != t % 9)
        return false;
    for (int n = std::min(s, t) + 9; n < std::max(s, t); n += 9)
        if (board[n])
            return false;
    return true;
}
bool Position::reaches(int from, int to, bool attack) const {
    if (from == to)
        return false;
    char p = board[from];
    if (!p)
        return false;
    int x = from % 9, y = from / 9, u = to % 9, v = to / 9, dx = u - x, dy = v - y, ax = std::abs(dx),
        ay = std::abs(dy);
    bool side = isred(p);
    switch (type(p)) {
    case 'k':
        return palace(side, u, v) && ax + ay == 1;
    case 'a':
        return palace(side, u, v) && ax == 1 && ay == 1;
    case 'b':
        return ax == 2 && ay == 2 && (attack || (side ? v <= 4 : v >= 5)) &&
               !board[(y + dy / 2) * 9 + x + dx / 2];
    case 'n':
        return ((ax == 2 && ay == 1) || (ax == 1 && ay == 2)) &&
               !board[(y + (ay == 2 ? dy / 2 : 0)) * 9 + x + (ax == 2 ? dx / 2 : 0)];
    case 'p':
        return dx == 0 && dy == (side ? 1 : -1) || dy == 0 && ax == 1 && (side ? y >= 5 : y <= 4);
    case 'r':
    case 'c': {
        if (dx && dy)
            return false;
        int step = dx ? (dx > 0 ? 1 : -1) : (dy > 0 ? 9 : -9), count = 0;
        for (int n = from + step; n != to; n += step)
            if (board[n])
                ++count;
        return type(p) == 'r' ? count == 0 : (attack || board[to] ? count == 1 : count == 0);
    }
    }
    return false;
}
bool Position::check(bool side) const {
    auto it = std::find(board.begin(), board.end(), side ? 'K' : 'k');
    if (it == board.end())
        return false;
    int king = int(it - board.begin());
    for (int from = 0; from < 90; from++)
        if (board[from] && isred(board[from]) != side && reaches(from, king, true))
            return true;
    return false;
}
std::vector<Move> Position::moves() const {
    std::vector<Move> result;
    for (int from = 0; from < 90; from++)
        if (board[from] && isred(board[from]) == red)
            for (int to = 0; to < 90; to++)
                if ((!board[to] || isred(board[to]) != red) && type(board[to]) != 'k' &&
                    reaches(from, to, false)) {
                    auto next = *this;
                    next.board[to] = next.board[from];
                    next.board[from] = 0;
                    if (!next.check(red) && !next.facing())
                        result.push_back({from, to});
                }
    return result;
}
void Position::apply(Move m) {
    if (m.from < 0 || m.from >= 90 || m.to < 0 || m.to >= 90)
        throw std::runtime_error("Invalid move square");
    halfmove = (board[m.to] || type(board[m.from]) == 'p') ? 0 : halfmove + 1;
    board[m.to] = board[m.from];
    board[m.from] = 0;
    if (!red)
        ++fullmove;
    red = !red;
}
std::string Position::wxf(Move m) const {
    const char p = board[m.from], kind = type(p);
    bool side = isred(p);
    int x = side ? 8 - m.from % 9 : m.from % 9, y = side ? 9 - m.from / 9 : m.from / 9,
        tx = side ? 8 - m.to % 9 : m.to % 9, ty = side ? 9 - m.to / 9 : m.to / 9;
    std::map<int, std::vector<int>> columns;
    for (int s = 0; s < 90; s++)
        if (board[s] == p)
            columns[side ? 8 - s % 9 : s % 9].push_back(side ? 9 - s / 9 : s / 9);
    for (auto &[col, rows] : columns)
        std::sort(rows.begin(), rows.end());
    std::string prefix = std::string(1, p) + char('1' + x);
    auto &rows = columns[x];
    if (kind != 'a' && kind != 'b' && rows.size() > 1) {
        int index = int(std::find(rows.begin(), rows.end(), y) - rows.begin());
        size_t multi = 0;
        for (auto &[col, rr] : columns)
            if (rr.size() > 1)
                ++multi;
        if (kind == 'p' && multi > 1) {
            int global = 0;
            for (auto &[col, rr] : columns)
                if (rr.size() > 1) {
                    if (col == x) {
                        global += index;
                        break;
                    }
                    global += int(rr.size());
                }
            prefix = std::string(1, p) + char('a' + global);
        } else if (rows.size() == 2)
            prefix = std::string(index == 0 ? "+" : "-") + p;
        else if (rows.size() == 3)
            prefix = std::string(index == 0 ? "+" : index == 1 ? "." : "-") + p;
        else
            prefix = std::string(1, p) + char('a' + index);
    }
    char action = y == ty ? '.' : ty < y ? '+' : '-';
    int target =
        y == ty ? tx + 1 : (std::string("rckp").find(kind) != std::string::npos ? std::abs(ty - y) : tx + 1);
    return prefix + action + char('0' + target);
}
Json Position::describe(Move m) const {
    auto w = wxf(m);
    std::string zh = w;
    std::smatch match;
    static const std::regex pattern(R"(^([+\-.a-e])?([A-Za-z])([a-e])?(\d)?([+\-.])(\d)$)");
    if (std::regex_match(w, match, pattern)) {
        const std::map<char, std::string> names = {
            {'K', "帅"}, {'k', "将"}, {'A', "仕"}, {'a', "士"}, {'B', "相"}, {'b', "象"}, {'N', "马"},
            {'n', "马"}, {'R', "车"}, {'r', "车"}, {'C', "炮"}, {'c', "炮"}, {'P', "兵"}, {'p', "卒"}};
        auto number = [&](char n) {
            int i = n - '1';
            return i >= 0 && i < 9 ? (red ? cn[i] : fw[i]) : std::string{};
        };
        std::string before = match[1], label = match[3], pref;
        char piece = match[2].str()[0];
        if (before == "+")
            pref = "前";
        else if (before == ".")
            pref = "中";
        else if (before == "-")
            pref = "后";
        else if (!before.empty())
            pref = cn[before[0] - 'a'];
        if (pref.empty() && !label.empty())
            pref = cn[label[0] - 'a'];
        zh = pref.empty() ? names.at(piece) + number(match[4].str()[0]) : pref + names.at(piece);
        char action = match[5].str()[0];
        zh += (action == '.' ? "平" : action == '+' ? "进" : "退");
        zh += number(match[6].str()[0]);
    }
    auto lan = square(m.from) + square(m.to);
    auto iccs = square(m.from) + "-" + square(m.to);
    for (char &c : iccs)
        c = char(std::toupper(static_cast<unsigned char>(c)));
    return {{"from", m.from}, {"to", m.to}, {"zh", zh}, {"lan", lan}, {"iccs", iccs}};
}
Move Position::parse(const std::string &input) const {
    auto s = trim(input);
    while (!s.empty() && (s.back() == '!' || s.back() == '?' || s.back() == '#'))
        s.pop_back();
    auto key = normalized(s);
    std::string coordinate;
    for (char c : key)
        if (c != '-')
            coordinate += c;
    for (auto m : moves()) {
        if (coordinate == square(m.from) + square(m.to))
            return m;
        auto d = describe(m);
        if (normalized(d.at("zh").get<std::string>()) == key || normalized(wxf(m)) == key)
            return m;
    }
    throw std::runtime_error("Illegal move: " + input);
}
Json Position::view() const {
    Json rows = Json::array(), legal = Json::array();
    for (int r = 9; r >= 0; r--) {
        Json row = Json::array();
        for (int f = 0; f < 9; f++) {
            int s = r * 9 + f;
            char c = board[s];
            if (!c)
                row.push_back(nullptr);
            else
                row.push_back({{"square", square(s)},
                               {"type", std::string(1, type(c))},
                               {"color", isred(c) ? "w" : "b"}});
        }
        rows.push_back(row);
    }
    for (auto m : moves())
        legal.push_back(describe(m));
    return {{"fen", fen()},
            {"board", rows},
            {"turn", red ? "w" : "b"},
            {"check", check(red)},
            {"checkmate", legal.empty()},
            {"stalemate", false},
            {"moves", legal}};
}
} // namespace xqn::native
