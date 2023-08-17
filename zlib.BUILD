load("@rules_cc//cc:defs.bzl", "cc_library")

licenses(["notice"])  # BSD/MIT-like license (for zlib)

_ZLIB_HEADERS = [
    "crc32.h",
    "deflate.h",
    "gzguts.h",
    "inffast.h",
    "inffixed.h",
    "inflate.h",
    "inftrees.h",
    "trees.h",
    "zconf.h",
    "zlib.h",
    "zutil.h",
]

minizip_sources = glob(["contrib/minizip/*.c"], exclude = ["**/iowin32.c"])
minizip_headers = glob(["contrib/minizip/*.h"], exclude = ["**/iowin32.h"])

cc_library(
    name = "zlib",
    srcs = [
        "adler32.c",
        "compress.c",
        "crc32.c",
        "deflate.c",
        "gzclose.c",
        "gzlib.c",
        "gzread.c",
        "gzwrite.c",
        "infback.c",
        "inffast.c",
        "inflate.c",
        "inftrees.c",
        "trees.c",
        "uncompr.c",
        "zutil.c",
        # Include the un-prefixed headers in srcs to work
        # around the fact that zlib isn't consistent in its
        # choice of <> or "" delimiter when including itself.
    ] + minizip_sources,
    hdrs = _ZLIB_HEADERS + minizip_headers,
    copts = select({
        "@bazel_tools//src/conditions:windows": [],
        "//conditions:default": [
            "-Wno-unused-variable",
            "-Wno-implicit-function-declaration",
        ],
    }),
    visibility = ["//visibility:public"],
)