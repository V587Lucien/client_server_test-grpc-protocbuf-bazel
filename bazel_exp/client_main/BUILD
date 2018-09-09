cc_binary(
    name = "client",
    srcs = ["client.cc"],
    deps = [
        "//lib:CSLibs-grpc","//lib:CSLibs-pb","//lib:Base64"
    ],
    copts = ["-std=c++11 -I include -pthread"],
    linkopts = ["-L/home/sheny/new_bazel_exp/grpclib -lgrpc++ -lgrpc -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -lprotobuf -pthread -lpthread -ldl"],
)

