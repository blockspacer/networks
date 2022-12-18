#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace backend {

enum class LoginType { kUserName, kGroupName };

LoginType GetUserType(std::string_view user_name) noexcept;

std::vector<std::string> GetUserGroups(std::string_view name) noexcept;

}  // namespace backend
