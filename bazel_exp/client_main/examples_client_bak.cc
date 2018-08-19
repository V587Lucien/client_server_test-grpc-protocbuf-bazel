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
 
  std::string Search(SearchRequest& request) {
 
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
 
 printf("client start!\n请输入操作类型：\n  l/login:用户登录   s/signup:注册新用户");
  std::string strAddr = "";
  std::string strUser = "";
  std::string strPasswd = "";
  std::string strMode = ""; 

  SearchRequest request;
  request.set_request(user);
  
  
  printf("client start!\n");
  while(true)
  {
    printf("client~:请输入服务器ip端口,格式：ip:port\nservice \"ip:port\" is:");
    char szIpPort[24] = {"\0"};
    scanf("%s",szIpPort);
    if(strlen(szIpPort) < 1)
    {
      printf("client~:输入内容错误，请参照以下格式： ip:port\n   如：127.0.0.1:50051\n");
      continue;
    }
    strAddr = szIpPort;
    break;
  }  
    // Create a default SSL ChannelCredentials object.
    auto channel_creds = grpc::SslCredentials(grpc::SslCredentialsOptions());
    ExampleClient client(grpc::CreateChannel(
        strAddr.c_str(), channel_creds));
  
  while(true)
  {
    printf("client~:\n  登录请输入: l（login）\n  新用户注册请输入: c （create）\n  查看登录状态请输入：s （status）");
    char szMode[8] = {"\0"};
    scanf("%s",szMode);
    if(strlen(szMode) < 1)
      continue;
    strMode = szMode;
    break;
  }
  
  
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

