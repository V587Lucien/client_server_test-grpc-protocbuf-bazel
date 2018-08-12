subdir = ./
 
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
 
SOURCES = $(wildcard $(subdir)*.cc)
SRCOBJS = $(patsubst %.cc,%.o,$(SOURCES))
CC = g++
 
%.o:%.cc
	$(CC) -std=c++11 -I/usr/local/include -pthread -c $< -o $@
 
all: client server
 
client:	examples.grpc.pb.o examples.pb.o examples_client.o Base64.o
	$(CC) $^ -L/usr/local/lib `pkg-config --libs grpc++ grpc` -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -lprotobuf -lpthread -ldl -o $@
 
server:	examples.grpc.pb.o examples.pb.o examples_server.o Base64.o
	$(CC) $^ -L/usr/local/lib `pkg-config --libs grpc++ grpc` -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -lprotobuf -lpthread -ldl -o $@
#chmod 777 $@
 
clean:
	sudo rm *.o

