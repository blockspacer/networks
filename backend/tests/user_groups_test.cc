#include "backend/user_groups.h"

#include <iostream>

int main(int, char** argv) {
  if (backend::GetUserType(argv[1]) == backend::LoginType::kUserName) {
    std::cout << argv[1] << " - user\n";
  } else {
    std::cout << argv[1] << " - group\n";
  }

  if (backend::GetUserType(argv[1]) == backend::LoginType::kUserName) {
    std::cout << "User " << argv[1] << " groups:\n";
    for (const auto& g : backend::GetUserGroups(argv[1])) {
      std::cout << g << '\n';
    }
  }

  return 0;
}
