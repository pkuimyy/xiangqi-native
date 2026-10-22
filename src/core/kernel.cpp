#include <chrono>
#include <quickjs.h>
#include <stdexcept>
#include <xiangqi/kernel.hpp>
namespace xqn {
struct Kernel::Impl {
    JSRuntime *runtime{};
    JSContext *context{};
    std::chrono::steady_clock::time_point deadline;
    static int interrupt(JSRuntime *, void *opaque) {
        return std::chrono::steady_clock::now() > static_cast<Impl *>(opaque)->deadline;
    }
    std::string exception() {
        JSValue e = JS_GetException(context);
        const char *s = JS_ToCString(context, e);
        std::string result = s ? s : "JavaScript exception";
        JS_FreeCString(context, s);
        JS_FreeValue(context, e);
        return result;
    }
};
Kernel::Kernel() : impl_(std::make_unique<Impl>()) {
    auto &p = *impl_;
    p.runtime = JS_NewRuntime();
    if (!p.runtime)
        throw std::runtime_error("Cannot create rules runtime");
    JS_SetMemoryLimit(p.runtime, 256U * 1024U * 1024U);
    JS_SetMaxStackSize(p.runtime, 2U * 1024U * 1024U);
    p.context = JS_NewContext(p.runtime);
    if (!p.context) {
        JS_FreeRuntime(p.runtime);
        throw std::runtime_error("Cannot create rules context");
    }
    p.deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    JS_SetInterruptHandler(p.runtime, Impl::interrupt, &p);
    static constexpr unsigned char source[] = {
#include "kernel.inc"
    };
    JSValue v = JS_Eval(p.context, reinterpret_cast<const char *>(source), sizeof(source) - 1,
                        "xiangqi-kernel.js", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(v)) {
        auto message = p.exception();
        JS_FreeContext(p.context);
        JS_FreeRuntime(p.runtime);
        throw std::runtime_error(message);
    }
    JS_FreeValue(p.context, v);
}
Kernel::~Kernel() {
    if (impl_) {
        JS_FreeContext(impl_->context);
        JS_FreeRuntime(impl_->runtime);
    }
}
Json Kernel::call(const std::string &operation, const Json &data) {
    auto &p = *impl_;
    p.deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    auto text = Json{{"op", operation}, {"data", data}}.dump();
    JSValue global = JS_GetGlobalObject(p.context);
    JSValue kernel = JS_GetPropertyStr(p.context, global, "XiangqiKernel");
    JSValue fn = JS_GetPropertyStr(p.context, kernel, "dispatch");
    JSValue arg = JS_NewStringLen(p.context, text.data(), text.size());
    JSValue result = JS_Call(p.context, fn, kernel, 1, &arg);
    JS_FreeValue(p.context, arg);
    JS_FreeValue(p.context, fn);
    JS_FreeValue(p.context, kernel);
    JS_FreeValue(p.context, global);
    if (JS_IsException(result))
        throw std::runtime_error(p.exception());
    const char *encoded = JS_ToCString(p.context, result);
    if (!encoded) {
        JS_FreeValue(p.context, result);
        throw std::runtime_error(p.exception());
    }
    std::string json = encoded;
    JS_FreeCString(p.context, encoded);
    JS_FreeValue(p.context, result);
    return Json::parse(json);
}
} // namespace xqn
