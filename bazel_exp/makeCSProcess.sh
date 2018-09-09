#!/bin/sh

cpath=`pwd`
export CPLUS_INCLUDE_PATH=$cpath/include:$CPLUS_INCLUDE_PATH
rm -f /etc/ld.so.conf.d/grpc.conf
echo $cpath/grpclib >> /etc/ld.so.conf.d/grpc.conf
ldconfig
#export LIBRARY_PATH=$cpath/grpclib:$LIBRARY_PATH
cpath=${cpath//\//\\/}
sed -i "s/@grpclibpath@/$cpath\/grpclib/" client_main/BUILD
sed -i "s/@grpclibpath@/$cpath\/grpclib/" server_main/BUILD
bazel build //server_main:server
cp -f new_certs/* bazel-bin/server_main/
bazel build //client_main:client
cp -f new_certs/* bazel-bin/client_main/
