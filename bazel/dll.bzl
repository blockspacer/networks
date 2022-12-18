load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_import", "cc_library")

def cc_shared_library(
        name,
        srcs = [],
        deps = [],
        hdrs = [],
        visibility = None,
        **kwargs):
    dll_name = "lib" + name + ".so"
    import_target_name = name + "_so_import"

    cc_binary(
        name = dll_name,
        srcs = srcs + hdrs,
        deps = deps,
        linkshared = True,
        visibility = visibility,
        **kwargs
    )

    cc_import(
        name = import_target_name,
        shared_library = ":" + dll_name,
    )

    cc_library(
        name = name,
        hdrs = hdrs,
        visibility = visibility,
        deps = deps + [":" + import_target_name],
    )
