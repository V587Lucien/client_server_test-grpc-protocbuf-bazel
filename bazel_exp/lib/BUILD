cc_library(
    name = "CSLibs-grpc",
    srcs = ["CSLibs.grpc.pb.cc"],
    hdrs = ["CSLibs.grpc.pb.h","CSLibs.pb.h"],
    copts = ["-std=c++11 -I/usr/local/include -pthread"],
    linkopts = ["-L/usr/local/lib -lgrpc++ -lgrpc -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -lprotobuf -pthread -lpthread -ldl"],
    visibility = ["//server_main:__pkg__","//client_main:__pkg__"],
)
cc_library(
    name = "CSLibs-pb",
    srcs = ["CSLibs.pb.cc"],
    hdrs = ["CSLibs.pb.h"],
    copts = ["-std=c++11 -I/usr/local/include -pthread"],
    linkopts = ["-L/usr/local/lib -lgrpc++ -lgrpc -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -lprotobuf -pthread -lpthread -ldl"],
    visibility = ["//server_main:__pkg__","//client_main:__pkg__"],
)

cc_library(
    name = "Base64",
    srcs = ["Base64.cc"],
    hdrs = ["Base64.h"],
    copts = ["-std=c++11 -I/usr/local/include -pthread"],
    linkopts = ["-L/usr/local/lib -lgrpc++ -lgrpc -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -lprotobuf -pthread -lpthread -ldl"],
    visibility = ["//server_main:__pkg__","//client_main:__pkg__"],
)

cc_library(
    name = "Lock",
    srcs = ["Lock.cc"],
    hdrs = ["Lock.h"],
    copts = ["-std=c++11 -I/usr/local/include -pthread"],
    linkopts = ["-L/usr/local/lib -lgrpc++ -lgrpc -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -lprotobuf -pthread -lpthread -ldl"],
    visibility = ["//server_main:__pkg__","//client_main:__pkg__"],
)

