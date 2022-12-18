#include "yield.h"

#include <pthread.h>
#include <sched.h>

void core::SchedYield() noexcept { sched_yield(); }

void core::ThreadYield() noexcept { pthread_yield(); }