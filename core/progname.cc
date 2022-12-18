#include "progname.h"
#include "execpath.h"
#include "singleton.h"

#include <filesystem>

namespace core {

static const char* name;

struct ProgramNameHolder {
  inline ProgramNameHolder()
      : program_name(std::filesystem::path(name ? name : GetExecPath()).filename().string()) {}
  std::string program_name;
};

const std::string& GetProgramName() { return Singleton<ProgramNameHolder>()->program_name; }

void SetProgramName(const char* n) { name = n; }

}  // namespace core