#include "execpath.h"
#include "singleton.h"

#include <filesystem>

#include <unistd.h>

static std::string GetExecPathImpl() {
  std::string path = "/proc/" + std::to_string(getpid()) + "/exe";
  return std::filesystem::read_symlink(path).string();
}

struct ExecPathHolder {
  inline ExecPathHolder()
      : exec_path(GetExecPathImpl()) {}

  static inline auto instance() { return core::SingletonWithPriority<ExecPathHolder, 1>(); }

  std::string exec_path;
};

const std::string& core::GetExecPath() { return ExecPathHolder::instance()->exec_path; }