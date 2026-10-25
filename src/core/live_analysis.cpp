#include <xiangqi/live_analysis.hpp>
namespace xqn {
LiveAnalysis::LiveAnalysis(Handler handler) : handler_(std::move(handler)), worker_([this] { run(); }) {}
LiveAnalysis::~LiveAnalysis() {
    closing_ = true;
    stop();
    if (worker_.joinable())
        worker_.join();
}
std::uint64_t LiveAnalysis::request(std::filesystem::path executable, std::string fen) {
    std::shared_ptr<UciEngine> engine;
    std::uint64_t revision;
    {
        std::lock_guard lock(mutex_);
        revision = ++revision_;
        pending_ = Job{revision, std::move(executable), std::move(fen),
                       std::chrono::steady_clock::now() + std::chrono::milliseconds(250)};
        engine = engine_;
    }
    if (engine)
        engine->stop();
    changed_.notify_one();
    return revision;
}
void LiveAnalysis::stop() {
    std::shared_ptr<UciEngine> engine;
    {
        std::lock_guard lock(mutex_);
        ++revision_;
        pending_.reset();
        engine = engine_;
    }
    if (engine)
        engine->stop();
    changed_.notify_one();
}
void LiveAnalysis::run() {
    std::shared_ptr<UciEngine> engine;
    std::filesystem::path executable;
    while (!closing_) {
        Job job;
        {
            std::unique_lock lock(mutex_);
            changed_.wait(lock, [&] { return closing_ || pending_.has_value(); });
            if (closing_)
                break;
            job = *pending_;
            if (changed_.wait_until(lock, job.ready, [&] {
                    return closing_ || !pending_ || pending_->revision != job.revision;
                }))
                continue;
            pending_.reset();
        }
        auto cancelled = [this, revision = job.revision] { return closing_ || revision_ != revision; };
        auto emit = [&](LiveAnalysisEvent::Kind kind, Json value = {}, std::string message = {}) {
            if (!cancelled()) {
                try {
                    handler_(LiveAnalysisEvent{job.revision, kind, std::move(value), std::move(message)});
                } catch (...) {
                }
            }
        };
        if (cancelled())
            continue;
        try {
            if (!engine || executable != job.executable) {
                {
                    std::lock_guard lock(mutex_);
                    engine_.reset();
                }
                engine.reset();
                engine = std::make_shared<UciEngine>(job.executable, cancelled);
                executable = job.executable;
                {
                    std::lock_guard lock(mutex_);
                    engine_ = engine;
                }
            }
            if (cancelled())
                continue;
            emit(LiveAnalysisEvent::Kind::Started);
            auto last = std::chrono::steady_clock::time_point::min();
            auto result = engine->analyze_live(
                job.fen,
                [&](Json const &value) {
                    auto now = std::chrono::steady_clock::now();
                    if (last == std::chrono::steady_clock::time_point::min() ||
                        now - last >= std::chrono::milliseconds(150)) {
                        last = now;
                        emit(LiveAnalysisEvent::Kind::Update, red_view(value, job.fen));
                    }
                },
                cancelled);
            emit(LiveAnalysisEvent::Kind::Finished, red_view(result, job.fen));
        } catch (const std::exception &error) {
            if (std::string(error.what()) != "Analysis cancelled") {
                emit(LiveAnalysisEvent::Kind::Error, {}, error.what());
                {
                    std::lock_guard lock(mutex_);
                    engine_.reset();
                }
                engine.reset();
                executable.clear();
            }
        }
    }
    {
        std::lock_guard lock(mutex_);
        engine_.reset();
    }
}
} // namespace xqn
