load("@bazel_skylib//rules:expand_template.bzl", "expand_template")
load("@rules_cc//cc:defs.bzl", "cc_library")
load("@opencascade_bottle_demo//third_party:occt.bzl", "occt_headers", "occt_library")

package(default_visibility = ["//visibility:public"])

# The Visualization / ApplicationFramework / DataExchange / Draw modules are not
# needed by the bottle demo and are therefore not built.
ALL_TOOLKITS = {
    "FoundationClasses": [
        "TKernel",
        "TKMath",
    ],
    "ModelingData": [
        "TKG2d",
        "TKG3d",
        "TKGeomBase",
        "TKBRep",
    ],
    "ModelingAlgorithms": [
        "TKGeomAlgo",
        "TKTopAlgo",
        "TKPrim",
        "TKBO",
        "TKBool",
        "TKHLR",
        "TKFillet",
        "TKOffset",
        "TKFeat",
        "TKMesh",
        "TKShHealing",
    ],
}

# Standard_Version.hxx is the only header the upstream build generates.
expand_template(
    name = "standard_version",
    out = "generated/Standard_Version.hxx",
    substitutions = {
        "@OCCT_VERSION_DATE@": "2025",
        "@OCC_VERSION_MAJOR@": "8",
        "@OCC_VERSION_MINOR@": "0",
        "@OCC_VERSION_MAINTENANCE@": "1",
        "@SET_OCC_VERSION_DEVELOPMENT@": "",
    },
    template = "adm/templates/Standard_Version.hxx.in",
)

occt_headers(
    name = "headers",
    generated_hdrs = [":standard_version"],
    toolkits = ALL_TOOLKITS,
)

occt_library(
    name = "foundation_classes",
    toolkits = {"FoundationClasses": ALL_TOOLKITS["FoundationClasses"]},
    deps = [":headers"],
)

occt_library(
    name = "modeling_data",
    toolkits = {"ModelingData": ALL_TOOLKITS["ModelingData"]},
    deps = [
        ":foundation_classes",
        ":headers",
    ],
)

occt_library(
    name = "modeling_algorithms",
    toolkits = {"ModelingAlgorithms": ALL_TOOLKITS["ModelingAlgorithms"]},
    deps = [
        ":headers",
        ":modeling_data",
    ],
)

cc_library(
    name = "occt",
    deps = [
        ":headers",
        ":modeling_algorithms",
    ],
)
