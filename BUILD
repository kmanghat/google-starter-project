# Copyright 2017 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

licenses(["notice"])  # 3-clause BSD

package(default_visibility = ["//visibility:public"])

load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_proto_library")
load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")

# The following three rules demonstrate the usage of the cc_grpc_library rule in
# in a mode compatible with the native proto_library and cc_proto_library rules.
proto_library(
    name = "foodvendor_proto",
    srcs = ["proto/foodvendor.proto"],
)

cc_proto_library(
    name = "foodvendor_cc_proto",
    deps = [":foodvendor_proto"],
)

cc_grpc_library(
    name = "foodvendor_cc_grpc",
    srcs = [":foodvendor_proto"],
    grpc_only = True,
    deps = [":foodvendor_cc_proto"],
)


cc_binary(
    name = "foodsupplier_server",
    srcs = ["foodsupplier_server.cc"],
    defines = ["BAZEL_BUILD"],
    data = ["database/data.txt"],
    deps = [
        ":foodvendor_cc_grpc",
        # http_archive made this label available for binding
        "@com_github_grpc_grpc//:grpc++",
    ],
)


cc_binary(
    name = "foodvendor_server",
    srcs = ["foodvendor_server.cc"],
    defines = ["BAZEL_BUILD"],
    data = ["database/data.txt","database/vendorA.txt","database/vendorB.txt","database/vendorC.txt","database/vendorD.txt",],
    deps = [
        ":foodvendor_cc_grpc",
        # http_archive made this label available for binding
        "@com_github_grpc_grpc//:grpc++",
    ],
)

cc_binary(
    name = "foodfinder_client",
    srcs = ["foodfinder_client.cc"],
    defines = ["BAZEL_BUILD"],
    deps = [
        ":foodvendor_cc_grpc",
        # http_archive made this label available for binding
        "@com_github_grpc_grpc//:grpc++",
    ],
)

cc_binary(
    name = "foodfinder_server",
    srcs = ["foodfinder_server.cc"],
    defines = ["BAZEL_BUILD"],
    deps = [
        ":foodvendor_cc_grpc",
        # http_archive made this label available for binding
        "@com_github_grpc_grpc//:grpc++",
    ],
)
