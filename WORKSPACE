load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "com_google_absl",
    sha256 = "cfb1f22164808eb0a233ad91287df84c2af2084cfc8b429eca1be1e57511065d",
    strip_prefix = "abseil-cpp-20210324.1",
    urls = ["https://github.com/abseil/abseil-cpp/archive/refs/tags/20210324.1.zip"],
)

http_archive(
    name = "com_google_googletest",
    sha256 = "b63b397efd4b7a89266482a8ba5e2f2622a9e5aaedf4605696f210536601e7e6",
    strip_prefix = "googletest-9dce5e5d878176dc0054ef381f5c6e705f43ef99",
    url = "https://github.com/google/googletest/archive/9dce5e5d878176dc0054ef381f5c6e705f43ef99.zip",
)

http_archive(
    name = "inicpp",
    build_file = "//contrib:BUILD.inicpp",
    patch_args = ["-p1"],
    patches = ["//contrib:inicpp.patch"],
    sha256 = "70249842cddbc5ba7796e886a77d07336914ec1ce8071a814769f5546a7ece7a",
    strip_prefix = "inicpp-dc6907704ac9e082761fa3900babf8e1b8a0aa1f",
    urls = [
        "https://github.com/SemaiCZE/inicpp/archive/dc6907704ac9e082761fa3900babf8e1b8a0aa1f.zip",
    ],
)

http_archive(
    name = "pqxx",
    build_file = "//contrib:BUILD.pqxx",
    patch_args = ["-p1"],
    patches = ["//contrib:libpqxx.patch"],
    sha256 = "45005fabe3b8e55dbc9b2733bf14f7620e8f6fa51acb03de1ccc660c88a7e846",
    strip_prefix = "libpqxx-7.4.1",
    urls = ["https://github.com/jtv/libpqxx/archive/refs/tags/7.4.1.zip"],
)

http_archive(
    name = "mysqlxx",
    build_file = "//contrib:BUILD.mysqlxx",
    patch_args = ["-p1"],
    patches = ["//contrib:libmysqlxx.patch"],
    sha256 = "839cfbf71d50a04057970b8c31f4609901f5d3936eaa86dab3ede4905c4db7a8",
    strip_prefix = "mysql++-3.2.5",
    urls = ["https://tangentsoft.com/mysqlpp/releases/mysql++-3.2.5.tar.gz"],
)

http_archive(
    name = "spdlog",
    build_file = "//contrib:BUILD.spdlog",
    sha256 = "f0410b12b526065802b40db01304783550d3d20b4b6fe2f8da55f9d08ed2035d",
    strip_prefix = "spdlog-1.8.2",
    urls = ["https://github.com/gabime/spdlog/archive/v1.8.2.zip"],
)

http_archive(
    name = "com_github_grpc_grpc",
    sha256 = "2a0aef1d60660d4c4ff2bc7f43708e5df561e41c9f98d0351f9672f965a8461f",
    strip_prefix = "grpc-1.37.1",
    url = "https://github.com/grpc/grpc/archive/v1.37.1.zip",
)

load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")

grpc_deps()

load("@com_github_grpc_grpc//bazel:grpc_extra_deps.bzl", "grpc_extra_deps")

grpc_extra_deps()
