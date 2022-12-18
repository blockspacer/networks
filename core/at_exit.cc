#include "at_exit.h"
#include "assert.h"
#include "atomic.h"
#include "spinlock.h"

#include <cstdlib>
#include <deque>
#include <limits>
#include <queue>
#include <tuple>

namespace {

class AtExitManager {
  struct Function {
    core::AtExitFunc func;
    void* ctx;
    size_t priority;
    size_t num;
  };

  struct Cmp {
    inline auto operator()(const Function* l, const Function* r) const noexcept {
      return std::tie(l->priority, l->num) < std::tie(r->priority, r->num);
    }
  };

 public:
  inline AtExitManager() noexcept = default;

  inline void finish() noexcept {
    auto guard = core::Guard(lock_);

    while (!items_.empty()) {
      auto c = items_.top();
      core_assert(c);
      items_.pop();
      {
        auto unguard = core::Unguard(guard);
        try {
          c->func(c->ctx);
        } catch (...) {
        }
      }
    }
  }

  inline void registerFunction(core::AtExitFunc func, void* ctx, size_t priority) noexcept {
    core_with_lock(lock_) {
      store_.push_back({func, ctx, priority, store_.size()});
      items_.push(&store_.back());
    }
  }

 private:
  core::AdaptiveLock lock_;
  std::deque<Function> store_;
  std::priority_queue<Function*, std::vector<Function*>, Cmp> items_;
};

static core::Atomic at_exit_lock = 0;
static AtExitManager* volatile at_exit_manager = nullptr;
std::aligned_storage_t<sizeof(AtExitManager), alignof(AtExitManager)> at_exit_mem;

static void OnExit() {
  if (AtExitManager* const at_exit = core::atomics::Load(at_exit_manager)) {
    at_exit->finish();
    at_exit->~AtExitManager();
    core::atomics::Store(at_exit_manager, nullptr);
  }
}

static inline auto Instance() {
  if (AtExitManager* const at_exit = core::atomics::Load(at_exit_manager)) {
    return at_exit;
  }
  core_with_lock(at_exit_lock) {
    if (AtExitManager* const at_exit = core::atomics::Load(at_exit_manager)) {
      return at_exit;
    }
    atexit(OnExit);
    auto* const at_exit = new (&at_exit_mem) AtExitManager;
    core::atomics::Store(at_exit_manager, at_exit);
    return at_exit;
  }
}

void Wrapper(void* ctx) { reinterpret_cast<core::TraditionalAtExitFunc>(ctx)(); }

}  // namespace

void core::AtExit(AtExitFunc func, void* ctx) { AtExit(func, ctx, std::numeric_limits<size_t>::max()); }

void core::AtExit(AtExitFunc func, void* ctx, size_t priority) { Instance()->registerFunction(func, ctx, priority); }

void core::AtExit(TraditionalAtExitFunc func) { AtExit(Wrapper, reinterpret_cast<void*>(func)); }

void core::AtExit(TraditionalAtExitFunc func, size_t priority) {
  AtExit(Wrapper, reinterpret_cast<void*>(func), priority);
}