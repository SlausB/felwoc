package(features = ["-default_compile_flags"])

config_setting(
    name = "dev_build",
    values = {
        "compilation_mode": "dbg",
    },
)
config_setting(
    name = "prod_build",
    values = {
        "compilation_mode": "opt",
    },
)

gcc_options = [
    "-std=gnu++20",
    "-fconcepts",
    "-fconcepts-diagnostics-depth=20",
    "-fdiagnostics-color=always",
    "-fmax-errors=2",
    #"-Wall",
    #"-Werror",
] + select({
    "//conditions:default" : [ "-g" ],
    ":prod_build" : [ "-Ofast", "-mfpmath=both", "-march=native", "-m64", "-mavx2", "-ffast-math" ],
})
linkopts = [
    #"-lOpenCL",
]

cc_library(
    name = "excel_format",
    srcs = [ "excel_format/BasicExcel.cpp", "excel_format/ExcelFormat.cpp" ],
    hdrs = [ "excel_format/BasicExcel.hpp", "excel_format/ExcelFormat.h" ],
)

cc_library(
    name = "output",
    srcs = [
        "output/messenger.cpp",
        "output/outputs/console_output.cpp",
        "output/outputs/file_output.cpp",
        "output/outputs/string_output.cpp",
    ],
    hdrs = [
        "output/messenger.h",
        "output/outputs/output.h",
        "output/outputs/console_output.h",
        "output/outputs/file_output.h",
        "output/outputs/string_output.h",
    ],
    deps = [
        "@boost//:format",
        "@boost//:thread",
        "@boost//:filesystem",
    ],
)

cc_library(
    name = "targets",
    srcs = [
        "targets/utils.cpp",
        "targets/as3/as3_target.cpp",
        "targets/ts/ts_target.cpp",
        "targets/haxe/haxe_target.cpp",
    ],
    hdrs = [
        "targets/utils.hpp",

        "targets/target_platform.h",

        "targets/as3/as3_target.h",

        "targets/ts/ts_target.h",

        "targets/haxe/haxe_target.h",
        "targets/haxe/Infos.hx",
        "targets/haxe/Info.hx",
        "targets/haxe/Bound.hx",
        "targets/haxe/Count.hx",
        "targets/haxe/Link.hx",
        "targets/haxe/FosUtils.hx",
    ],
    deps = [
        "@boost//:property_tree",
        ":ast",
        ":output",
    ],
    copts = gcc_options,
)

cc_library(
    name = "ast",
    srcs = glob(["ast/*.cpp"]),
    hdrs = glob(["ast/*.h"]),
)

cc_library(
    name = "pugixml",
    srcs = [
        "xml/pugixml.cpp",
    ],
    hdrs = [
        "xml/pugixml.hpp",
        "xml/pugiconfig.hpp",
    ],
)

cc_library(
    name = "data_source",
    srcs = [
        "data_source/data_source.cpp",
        "data_source/ods_as_xml/ods_as_xml.cpp",
    #] + glob(["data_source/ods_as_xml/zip/minizip/*.c"], exclude = ["**/iowin32.c"]),
    ],
    hdrs = [
        "data_source/data_source.h",
        "data_source/ods_as_xml/ods_as_xml.h",

        #minizip:
    #] + glob(["data_source/ods_as_xml/zip/minizip/*.h"], exclude = ["**/iowin32.h"]),
    ],
    deps = [
        "@boost//:shared_ptr",
        "@boost//:smart_ptr",
        ":output",
        ":pugixml",
        "@net_zlib_zlib//:zlib",
    ],
)


cc_binary(
    name = "xls2xj",
    srcs = [
        "main.cpp",
        "parsing.cpp",
        "parsing.h",
    ],
    copts = gcc_options,
    features = ["-default_compile_flags"],
    deps = [
        "@fmt",
        "@boost//:property_tree",
        "@boost//:program_options",
        ":excel_format",
        ":output",
        ":targets",
        ":data_source",
    ],
    linkopts = linkopts,
)
