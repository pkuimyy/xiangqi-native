#include <iostream>
#include <xiangqi/document.hpp>
#include <xiangqi/uci.hpp>
int main(int argc, char **argv) {
    try {
        xqn::Kernel kernel;
        if (argc == 1) {
            xqn::Document d(kernel);
            std::cout << d.transport().dump(2) << "\n";
            return 0;
        }
        std::string command = argv[1];
        if (command == "position" && argc == 3) {
            std::cout << kernel.call("position", {{"fen", argv[2]}}).dump() << "\n";
            return 0;
        }
        if (command == "inspect" && argc == 3) {
            auto docs = xqn::open_documents(kernel, std::filesystem::u8path(argv[2]));
            xqn::Json result = xqn::Json::array();
            for (auto &d : docs)
                result.push_back(d.transport());
            std::cout << result.dump(2) << "\n";
            return 0;
        }
        if (command == "convert" && argc == 4) {
            auto docs = xqn::open_documents(kernel, std::filesystem::u8path(argv[2]));
            if (docs.size() != 1)
                throw std::runtime_error("Conversion to one OXQ requires exactly one game");
            xqn::save_atomic(std::filesystem::u8path(argv[3]), docs.front().oxq());
            return 0;
        }
        if (command == "analyze" && argc == 4) {
            xqn::UciEngine engine(std::filesystem::u8path(argv[2]));
            std::cout << engine.analyze(argv[3], 6).dump(2) << "\n";
            return 0;
        }
        std::cerr << "Usage: xiangqi_cli position FEN | inspect FILE | convert INPUT OUTPUT.oxq | analyze "
                     "ENGINE FEN\n";
        return 2;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
