#include <algorithm>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <xiangqi/uci.hpp>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
namespace xqn {
Json red_view(Json evaluation, const std::string &fen) {
    bool black = fen.find(" b") != std::string::npos;
    auto normalize = [black](Json &value) {
        if (!value.contains("score"))
            return;
        int score = value.at("score");
        if (black)
            score = -score;
        if (value.value("scoreType", std::string("cp")) == "mate" && score == 0)
            score = black ? 1 : -1;
        value["score"] = score;
    };
    normalize(evaluation);
    if (evaluation.contains("pvs"))
        for (auto &pv : evaluation["pvs"])
            normalize(pv);
    return evaluation;
}
Json parse_uci_info(const std::string &line) {
    std::istringstream in(line);
    std::string token;
    Json j = Json::object();
    in >> token;
    if (token != "info")
        return j;
    bool bound = false;
    while (in >> token) {
        if (token == "depth") {
            int n;
            if (in >> n)
                j["depth"] = n;
        } else if (token == "multipv") {
            int n;
            if (in >> n)
                j["multipv"] = n;
        } else if (token == "score") {
            std::string type;
            int n;
            if (in >> type >> n && (type == "cp" || type == "mate")) {
                j["scoreType"] = type;
                j["score"] = n;
            }
        } else if (token == "lowerbound" || token == "upperbound")
            bound = true;
        else if (token == "pv") {
            Json moves = Json::array();
            while (in >> token)
                moves.push_back(token);
            j["moves"] = moves;
            break;
        }
    }
    if (bound) {
        j.erase("score");
        j.erase("scoreType");
        j.erase("moves");
    }
    return j;
}
struct UciEngine::Impl {
    std::mutex operation, write;
    std::atomic<bool> cancelled{false};
    std::string pending;
#ifdef _WIN32
    HANDLE process{}, input{}, output{};
    bool active = false;
    void send(const std::string &command) {
        if (command.find_first_of("\r\n") != std::string::npos)
            throw std::runtime_error("Invalid UCI command");
        std::lock_guard lock(write);
        std::string text = command + "\n";
        DWORD n = 0;
        if (!WriteFile(input, text.data(), static_cast<DWORD>(text.size()), &n, nullptr) || n != text.size())
            throw std::runtime_error("Engine input closed");
    }
    std::string line(std::chrono::steady_clock::time_point deadline) {
        while (true) {
            auto pos = pending.find('\n');
            if (pos != std::string::npos) {
                auto s = pending.substr(0, pos);
                pending.erase(0, pos + 1);
                if (!s.empty() && s.back() == '\r')
                    s.pop_back();
                return s;
            }
            if (std::chrono::steady_clock::now() >= deadline)
                throw std::runtime_error("Engine response timeout");
            DWORD available = 0;
            if (!PeekNamedPipe(output, nullptr, 0, nullptr, &available, nullptr))
                throw std::runtime_error("Engine output closed");
            if (available) {
                char buf[4096];
                DWORD n = 0;
                if (!ReadFile(output, buf, (std::min)(available, DWORD(sizeof(buf))), &n, nullptr) || !n)
                    throw std::runtime_error("Engine output closed");
                pending.append(buf, n);
                if (pending.size() > 1024 * 1024)
                    throw std::runtime_error("Engine output line exceeds limit");
            } else {
                if (WaitForSingleObject(process, 0) == WAIT_OBJECT_0)
                    throw std::runtime_error("Engine exited");
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
    }
    void wait(const std::string &wanted) {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        while (line(deadline) != wanted) {
        }
    }
    void close() noexcept {
        if (input) {
            CloseHandle(input);
            input = nullptr;
        }
        if (process) {
            if (WaitForSingleObject(process, 1000) == WAIT_TIMEOUT)
                TerminateProcess(process, 1);
            WaitForSingleObject(process, 1000);
            CloseHandle(process);
            process = nullptr;
        }
        if (output) {
            CloseHandle(output);
            output = nullptr;
        }
        active = false;
    }
#endif
};
UciEngine::UciEngine(const std::filesystem::path &executable) : impl_(std::make_unique<Impl>()) {
#ifdef _WIN32
    auto &p = *impl_;
    auto full = std::filesystem::absolute(executable);
    if (!std::filesystem::is_regular_file(full))
        throw std::runtime_error("Engine executable does not exist");
    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    HANDLE childInput{}, childOutput{};
    if (!CreatePipe(&childInput, &p.input, &sa, 0))
        throw std::runtime_error("Cannot create engine input");
    if (!CreatePipe(&p.output, &childOutput, &sa, 0)) {
        CloseHandle(childInput);
        p.close();
        throw std::runtime_error("Cannot create engine output");
    }
    SetHandleInformation(p.input, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(p.output, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    startup.StartupInfo.hStdInput = childInput;
    startup.StartupInfo.hStdOutput = childOutput;
    startup.StartupInfo.hStdError = childOutput;
    SIZE_T bytes = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &bytes);
    std::vector<std::byte> attributes(bytes);
    startup.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
    if (!InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &bytes)) {
        CloseHandle(childInput);
        CloseHandle(childOutput);
        p.close();
        throw std::runtime_error("Cannot initialize engine process attributes");
    }
    HANDLE handles[] = {childInput, childOutput};
    bool attrs = UpdateProcThreadAttribute(startup.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                           handles, sizeof(handles), nullptr, nullptr) != 0;
    PROCESS_INFORMATION process{};
    std::wstring command = L"\"" + full.wstring() + L"\"";
    auto directory = full.parent_path().wstring();
    bool created = attrs && CreateProcessW(full.c_str(), command.data(), nullptr, nullptr, TRUE,
                                           CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT, nullptr,
                                           directory.c_str(), &startup.StartupInfo, &process);
    DeleteProcThreadAttributeList(startup.lpAttributeList);
    CloseHandle(childInput);
    CloseHandle(childOutput);
    if (!created) {
        p.close();
        throw std::runtime_error("Cannot launch engine");
    }
    p.process = process.hProcess;
    CloseHandle(process.hThread);
    try {
        p.send("uci");
        p.wait("uciok");
        p.send("isready");
        p.wait("readyok");
        p.active = true;
    } catch (...) {
        p.close();
        throw;
    }
#else
    (void)executable;
    throw std::runtime_error("Native engine service currently requires Windows");
#endif
}
UciEngine::~UciEngine() {
#ifdef _WIN32
    if (impl_) {
        try {
            impl_->send("quit");
        } catch (...) {
        }
        impl_->close();
    }
#endif
}
void UciEngine::stop() noexcept { impl_->cancelled = true; }
Json UciEngine::analyze(const std::string &fen, int depth, int skill, int multipv,
                        std::chrono::milliseconds timeout) {
#ifdef _WIN32
    auto &p = *impl_;
    std::lock_guard operation(p.operation);
    if (!p.active)
        throw std::runtime_error("Engine is not running");
    if (depth < 1 || depth > 30 || skill < 0 || skill > 20 || multipv < 1 || multipv > 10)
        throw std::runtime_error("Engine parameter out of range");
    if (fen.find_first_of("\r\n") != std::string::npos)
        throw std::runtime_error("Invalid engine position");
    p.cancelled = false;
    bool sentStop = false;
    auto deadline = std::chrono::steady_clock::now() + timeout;
    try {
        p.send("setoption name Skill Level value " + std::to_string(skill));
        p.send("setoption name MultiPV value " + std::to_string(multipv));
        p.send("isready");
        p.wait("readyok");
        p.send("position fen " + fen);
        p.send("go depth " + std::to_string(depth));
        Json result = Json::object();
        std::map<int, Json> pv;
        while (true) {
            if (p.cancelled && !sentStop) {
                p.send("stop");
                sentStop = true;
                deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
            }
            std::string line;
            try {
                line = p.line(
                    (std::min)(deadline, std::chrono::steady_clock::now() + std::chrono::milliseconds(50)));
            } catch (const std::runtime_error &e) {
                if (std::string(e.what()) == "Engine response timeout" &&
                    std::chrono::steady_clock::now() < deadline)
                    continue;
                throw;
            }
            if (line.starts_with("info ")) {
                auto info = parse_uci_info(line);
                int index = info.value("multipv", 1);
                if (index == 1) {
                    if (info.contains("depth"))
                        result["depth"] = info["depth"];
                    if (info.contains("score")) {
                        result["score"] = info["score"];
                        result["scoreType"] = info["scoreType"];
                    }
                }
                if (info.contains("score") && info.contains("moves"))
                    pv[index] = info;
            } else if (line.starts_with("bestmove ")) {
                if (sentStop)
                    throw std::runtime_error("Analysis cancelled");
                std::istringstream input(line);
                std::string token, best;
                input >> token >> best;
                if (best.empty() || best == "(none)" || best == "0000")
                    throw std::runtime_error("Engine returned no legal best move");
                result["bestmove"] = best;
                while (input >> token) {
                    if (token == "ponder" && input >> token) {
                        result["ponder"] = token;
                        break;
                    }
                }
                result["pvs"] = Json::array();
                for (auto &[i, v] : pv)
                    result["pvs"].push_back(v);
                return result;
            }
        }
    } catch (const std::exception &e) {
        if (std::string(e.what()) != "Analysis cancelled")
            p.close();
        throw;
    }
#else
    (void)fen;
    (void)depth;
    (void)skill;
    (void)multipv;
    (void)timeout;
    throw std::runtime_error("Native engine requires Windows");
#endif
}
} // namespace xqn
