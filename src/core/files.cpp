#include <algorithm>
#include <cwctype>
#include <fstream>
#include <oxq/convert/cbl_reader.hpp>
#include <oxq/core/reader.hpp>
#include <random>
#include <stdexcept>
#include <xiangqi/document.hpp>
#ifdef _WIN32
#include <windows.h>
#endif
namespace xqn {
std::vector<std::byte> read_binary(const std::filesystem::path &path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in)
        throw std::runtime_error("Cannot open file");
    auto size = in.tellg();
    if (size < 0 || size > 256 * 1024 * 1024)
        throw std::runtime_error("File exceeds application limit (256 MiB)");
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    in.seekg(0);
    if (!bytes.empty() && !in.read(reinterpret_cast<char *>(bytes.data()), bytes.size()))
        throw std::runtime_error("Cannot read complete file");
    return bytes;
}
void save_atomic(const std::filesystem::path &path, const std::vector<std::byte> &bytes) {
    if (path.empty())
        throw std::runtime_error("Empty save path");
    auto tmp = path;
    tmp += L".xqn-tmp-" + std::to_wstring(std::random_device{}());
#ifdef _WIN32
    HANDLE handle =
        CreateFileW(tmp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
        throw std::runtime_error("Cannot create temporary save file");
    bool success = true;
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        DWORD written = 0;
        DWORD count = static_cast<DWORD>((std::min)(bytes.size() - offset, std::size_t(1024 * 1024)));
        if (!WriteFile(handle, bytes.data() + offset, count, &written, nullptr) || written != count) {
            success = false;
            break;
        }
        offset += written;
    }
    if (success && !FlushFileBuffers(handle))
        success = false;
    CloseHandle(handle);
    if (success) {
        if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
            success = ReplaceFileW(path.c_str(), tmp.c_str(), nullptr, 0, nullptr, nullptr) != 0;
        } else {
            success = MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH) != 0;
        }
    }
    if (!success) {
        DeleteFileW(tmp.c_str());
        throw std::runtime_error("Save failed; previous file retained");
    }
#else
    {
        std::ofstream out(tmp, std::ios::binary);
        out.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
        out.flush();
        if (!out) {
            std::filesystem::remove(tmp);
            throw std::runtime_error("Temporary write failed");
        }
    }
    std::error_code error;
    std::filesystem::rename(tmp, path, error);
    if (error) {
        std::filesystem::remove(tmp);
        throw std::runtime_error("Save failed: " + error.message());
    }
#endif
}
std::vector<Document> open_documents(Kernel &kernel, const std::filesystem::path &path) {
    auto bytes = read_binary(path);
    auto ext = path.extension().wstring();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    if (ext == L".oxq") {
        auto r = oxq::core::read_oxq(bytes);
        if (auto *e = std::get_if<oxq::core::CodecError>(&r))
            throw std::runtime_error(e->message);
        std::vector<Document> result;
        result.push_back(Document::from_model(kernel, std::move(std::get<oxq::core::ReaderResult>(r).game)));
        return result;
    }
    if (ext == L".cbl") {
        auto r = oxq::convert::read_cbl(bytes);
        if (auto *e = std::get_if<oxq::convert::CblError>(&r))
            throw std::runtime_error(e->message);
        auto &parsed = std::get<oxq::convert::CblReadResult>(r);
        if (!parsed.report.diagnostics.empty())
            throw std::runtime_error(
                "CBL conversion reported diagnostics; use OXQ CLI to review conversion before opening");
        std::vector<Document> result;
        for (auto &game : parsed.games)
            result.push_back(Document::from_model(kernel, std::move(game)));
        return result;
    }
    if (ext != L".pgn")
        throw std::runtime_error("Supported formats: .oxq, .pgn, .cbl");
    return Document::parse_pgn(kernel, std::string(reinterpret_cast<char *>(bytes.data()), bytes.size()));
}
} // namespace xqn
