#pragma once

#if defined(__clang__)
#if __has_feature(address_sanitizer)
#define core_address_sanitizer
#endif
#if __has_feature(memory_sanitizer)
#define core_memory_sanitizer
#endif
#if __has_feature(thread_sanitizer)
#define core_thread_sanitizer
#endif
#else
#if defined(__SANITIZE_ADDRESS__)
#define core_address_sanitizer
#endif
#if defined(__SANITIZE_THREAD__)
#define core_thread_sanitizer
#endif
#endif
