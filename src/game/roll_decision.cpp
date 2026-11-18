#include "game/roll_decision.hpp"
#include <cassert>
#include <cstdlib>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#else
#include <ucontext.h>
#endif
namespace blobber {
struct RollResolver::Stack {
    void (*operation)(void*) noexcept = nullptr;
    void* owner = nullptr;
    static constexpr std::size_t capacity = 1024 * 1024;
#ifdef _WIN32
    void* simulation = nullptr;
    void* caller = nullptr;
    static VOID WINAPI entry(void* self) { static_cast<Stack*>(self)->loop(); }
    // Convert a simulation host thread once. Sessions prepared on a loading
    // thread can be entered later by the gameplay thread.
    static void* hostFiber() noexcept {
        struct Host {
            bool owns = false;
            ~Host() {
                if (owns && IsThreadAFiber())
                    ConvertFiberToThread();
            }
        };
        static thread_local Host host;
        if (IsThreadAFiber())
            return GetCurrentFiber();
        auto* fiber = ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
        if (!fiber)
            std::abort();
        host.owns = true;
        return fiber;
    }
    Stack() : simulation(CreateFiberEx(0, capacity, FIBER_FLAG_FLOAT_SWITCH, entry, this)) {
        if (!simulation)
            throw std::runtime_error("Cannot allocate roll decision stack");
    }
    ~Stack() { DeleteFiber(simulation); }
    void resume() noexcept {
        caller = hostFiber();
        SwitchToFiber(simulation);
    }
    void yield() noexcept { SwitchToFiber(caller); }
#else
    ucontext_t caller{}, simulation{};
    alignas(16) std::array<unsigned char, capacity> storage;
    // makecontext's portable argument types cannot carry a pointer. Bootstrap
    // synchronously on first entry and retain the instance on its own stack.
    static thread_local Stack* entering;
    static void entry() noexcept { entering->loop(); }
    Stack() {
        if (getcontext(&simulation))
            throw std::runtime_error("Cannot create roll decision context");
        simulation.uc_link = nullptr;
        simulation.uc_stack.ss_sp = storage.data();
        simulation.uc_stack.ss_size = storage.size();
        makecontext(&simulation, entry, 0);
    }
    void resume() noexcept {
        entering = this;
        if (swapcontext(&caller, &simulation))
            std::abort();
    }
    void yield() noexcept {
        if (swapcontext(&simulation, &caller))
            std::abort();
    }
#endif
    void loop() noexcept {
        for (;;) {
            operation(owner);
            operation = nullptr;
            yield();
        }
    }
};
#ifndef _WIN32
thread_local RollResolver::Stack* RollResolver::Stack::entering = nullptr;
#endif
RollResolver::RollResolver() : stack_(std::make_unique<Stack>()) {}
RollResolver::~RollResolver() {
    close();
}
void RollResolver::run(void (*operation)(void*) noexcept, void* owner) noexcept {
    assert(!stack_->operation && !decision_.serial);
    stack_->operation = operation;
    stack_->owner = owner;
    executing_ = true;
    stack_->resume();
    executing_ = false;
}
RollAnswer RollResolver::ask(RollDecision d) noexcept {
    if (!executing_ || closing_ || serial_ == UINT64_MAX)
        return {};
    d.serial = ++serial_;
    decision_ = d;
    answer_ = {};
    stack_->yield();
    return answer_;
}
bool RollResolver::answer(std::uint64_t world, std::uint64_t serial, RollAnswer a) noexcept {
    if (!decision_.accepts(world, serial, a))
        return false;
    decision_ = {};
    answer_ = a;
    executing_ = true;
    stack_->resume();
    executing_ = false;
    return true;
}
void RollResolver::close() noexcept {
    closing_ = true;
    if (decision_.serial)
        answer(decision_.world, decision_.serial, {});
}
} // namespace blobber
