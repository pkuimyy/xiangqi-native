#include <iostream>
#include <string>
int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "uci")
            std::cout << "id name Native test engine\nuciok\n" << std::flush;
        else if (line == "isready")
            std::cout << "readyok\n" << std::flush;
        else if (line == "go depth 30" || line == "go depth 29")
            continue;
        else if (line == "stop")
            std::cout << "bestmove h2e2\n" << std::flush;
        else if (line.starts_with("go "))
            std::cout << "info depth 8 multipv 2 score mate -3 pv b0c2\n"
                         "info depth 9 multipv 1 score cp 42 pv h2e2 h7e7\n"
                         "info depth 10 multipv 1 score cp 999 lowerbound pv a0a1\n"
                         "bestmove h2e2 ponder h7e7\n"
                      << std::flush;
        else if (line == "quit")
            return 0;
    }
    return 0;
}
