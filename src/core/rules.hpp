#pragma once
#include <array>
#include <vector>
#include <xiangqi/kernel.hpp>
namespace xqn::native {
inline constexpr auto initial_fen = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1";
struct Move {
    int from{}, to{};
};
class Position {
  public:
    std::array<char, 90> board{};
    bool red = true;
    int halfmove{}, fullmove = 1;
    explicit Position(const std::string &fen = initial_fen);
    std::string fen() const;
    std::vector<Move> moves() const;
    bool check(bool side) const;
    bool facing() const;
    void apply(Move m);
    Move parse(const std::string &text) const;
    Json describe(Move m) const;
    Json view() const;
    std::string wxf(Move m) const;

  private:
    bool reaches(int from, int to, bool attack) const;
};
std::string square(int index);
std::string trim(std::string text);
Json hydrate(const Json &data);
void layout(Json &nodes);
Json parse_pgn(const std::string &input);
std::string stringify_pgn(const Json &data);
} // namespace xqn::native
