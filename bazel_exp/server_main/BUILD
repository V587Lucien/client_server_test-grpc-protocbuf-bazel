cc_binary(
    name = "server",
    srcs = ["examples_server.cc"],
    deps = [
        "//lib:examples-grpc","//lib:examples-pb","//lib:Base64"
    ],
    copts = ["-std=c++11 -I/usr/local/include -pthread"],
    linkopts = ["-L/usr/local/lib -lgrpc++ -lgrpc -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -lprotobuf -pthread -lpthread -ldl"],
)

