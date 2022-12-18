#include "user_groups.h"

#include <algorithm>

#include <grp.h>

namespace detail {

auto GetUserGroups(std::string_view name) noexcept {
  int groups_num = 0;
  getgrouplist(name.data(), 0, nullptr, &groups_num);

  std::vector<gid_t> groups(groups_num);
  std::vector<std::string> group_names;
  getgrouplist(name.data(), 0, groups.data(), &groups_num);
  std::transform(groups.begin(), groups.end(), std::back_inserter(group_names), [](gid_t g) {
    struct group* gr = getgrgid(g);
    return "@" + std::string(gr->gr_name);
  });
  return group_names;
}

}  // namespace detail

namespace backend {

LoginType GetUserType(std::string_view user_name) noexcept {
  return user_name[0] == '@' ? LoginType::kGroupName : LoginType::kUserName;
}

std::vector<std::string> GetUserGroups(std::string_view name) noexcept {
  if (GetUserType(name) == LoginType::kGroupName) {
    return {std::string(name)};
  } else {
    return ::detail::GetUserGroups(name);
  }
}

}  // namespace backend