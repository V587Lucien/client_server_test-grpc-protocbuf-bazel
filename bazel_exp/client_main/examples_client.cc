#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>
 
#include <grpc++/grpc++.h>
#include <grpc/support/log.h>
 
#include "lib/examples.grpc.pb.h"
#include "lib/Base64.h" 
using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
 
 
class ExampleClient {
 public:
  explicit ExampleClient(std::shared_ptr<Channel> channel)
      : stub_(SearchService::NewStub(channel)) {}
 
  std::string Search(const std::string& user) {
 
    SearchRequest request;
    request.set_request(user);
 
    SearchResponse reply;
   
    ClientContext context;
 
    CompletionQueue cq;
 
    Status status;
 
    std::unique_ptr<ClientAsyncResponseReader<SearchResponse> > rpc(
        stub_->AsyncSearch(&context, request, &cq));
 
    rpc->Finish(&reply, &status, (void*)1);
    void* got_tag;
    bool ok = false;
  
    GPR_ASSERT(cq.Next(&got_tag, &ok));
 
   
    GPR_ASSERT(got_tag == (void*)1);
  
    GPR_ASSERT(ok);
 
    if (status.ok()) {
      return reply.response();
    } else {
      return "RPC failed";
    }
  }
 
 private:
 
  std::unique_ptr<SearchService::Stub> stub_;
};
 
int main(int argc, char** argv) {
 
  int ch;
  std::string strAddr = "";
  std::string strUser = "";
  std::string strPasswd = "";
  std::string strMode = ""; 

  while ((ch = getopt(argc, argv, "h:u:p:m:")) != -1)
  {
    switch (ch) 
    {
      case 'h':
        strAddr = optarg;
        break;
      case 'u':
        strUser = optarg;
        break;
      case 'p':
        strPasswd = optarg;
        break;
      case 'm':
        strMode = optarg;
        break;
      default:
      {
        //printf("Unknown option: %c\n",(char)optopt);
      }
      
    }
  }
  
  if(strAddr.empty() || strUser.empty() || strPasswd.empty() || strMode.empty() )
  {
    printf("请输入参数：\n"\
    "   -h ip:port\n"\
    "   -u user\n"\
    "   -p passwd\n"\
    "   -m mode : 0 login / 1 regin\n");
    return 0;
  }
  
  ExampleClient client(grpc::CreateChannel(
      strAddr.c_str(), grpc::InsecureChannelCredentials()));
  std::string strSplit = "|";
  char szTime[13];
  time_t cTime = time(NULL);
  sprintf(szTime,"%d",cTime);
  CBase64 myBase64;
  char sz64PostStr[2048] = {'\0'};
  std::string strPostStr = strUser + strSplit + strPasswd + strSplit + strMode + strSplit + szTime;
  //std::cout << "Post string : " << strPostStr << std::endl;
  myBase64.Encode_turn(strPostStr.c_str(),(unsigned char*)sz64PostStr,2048);
  std::string reply = client.Search(sz64PostStr);  // The actual RPC call!
  strMode = "0";
  strPostStr = strUser + strSplit + strPasswd + strSplit + strMode  + strSplit + szTime;
  //std::cout << "Post string : " << strPostStr << std::endl;
  memset(sz64PostStr,0x00,2048);
  myBase64.Encode_turn(strPostStr.c_str(),(unsigned char*)sz64PostStr,2048);
  while ( true )
  {
    if(reply == "RPC failed")
    {
      std::cout << reply << std::endl;
      break;
    }
    
    if( reply == "0" )
    {
      std::cout << "Connect refused by server! Reason: This user had logined on another machion!" << std::endl;
      break;
    }
    else if( reply == "1" )
    {
      std::cout << "user: " << strUser << " sign up susseced ! Now login ..." << std::endl;
    }
    else if( reply == "2" )
    {
      std::cout << "sign up faild! Please try again!" << std::endl;
      break;
    }
    else if(reply == "3")
    {
      std::cout << "login faild ! User or Password is wrong !" << std::endl;
        break;
    }
    std::cout << "login susseced! Now keep on login ..." << std::endl;
    sleep(2);
    reply = client.Search(sz64PostStr);
  }
 
  return 0;
}

