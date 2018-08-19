#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>
 
#include <grpc++/grpc++.h>
#include <grpc/support/log.h>
#include <fstream>  
#include <streambuf> 
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
 
  std::string Search(SearchRequest request) {
 
    
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
  
  // Create a default SSL ChannelCredentials object.
  std::ifstream skey("server.key");
  std::string strClientKey((std::istreambuf_iterator<char>(skey)),std::istreambuf_iterator<char>());
  //std::cout << "key: " <<strClientKey << std::endl;
  std::ifstream sCrt("server.crt");
  std::string strClientCert((std::istreambuf_iterator<char>(sCrt)),std::istreambuf_iterator<char>());
  //std::cout << "crt: " << strClientCert << std::endl;
  std::ifstream sCaCrt("ca.crt");  
  std::string strCaCert((std::istreambuf_iterator<char>(sCaCrt)),std::istreambuf_iterator<char>());
  //std::cout << "ca: " << strClientCert << std::endl;
  
  grpc::SslCredentialsOptions ssl_opts;
  ssl_opts.pem_private_key = strClientKey;
  ssl_opts.pem_cert_chain = strClientCert;
  ssl_opts.pem_root_certs=strCaCert;
  //auto creds = grpc::SslCredentials(grpc::SslCredentialsOptions());
  auto creds = grpc::SslCredentials(ssl_opts);
  //auto creds = grpc::GoogleDefaultCredentials();
  ExampleClient client(grpc::CreateChannel(strAddr.c_str(), creds));
  char szTime[13];
  time_t cTime = time(NULL);
  sprintf(szTime,"%d",(int)cTime);
  
  SearchRequest request;
  //request.set_request(user);
  request.set_struser(strUser);
  request.set_strpasswd(strPasswd);
  request.set_strmode(strMode);
  request.set_strltime(szTime);
  //CBase64 myBase64;
  //char sz64PostStr[2048] = {'\0'};
  //std::string strPostStr = strUser + strSplit + strPasswd + strSplit + strMode + strSplit + szTime;
  //std::cout << "Post string : " << strPostStr << std::endl;
  //myBase64.Encode_turn(strPostStr.c_str(),(unsigned char*)sz64PostStr,2048);
  std::string reply = client.Search(request);  // The actual RPC call!
  //strMode = "0";
  request.set_strmode("0");
  //strPostStr = strUser + strSplit + strPasswd + strSplit + strMode  + strSplit + szTime;
  //std::cout << "Post string : " << strPostStr << std::endl;
  //memset(sz64PostStr,0x00,2048);
  //myBase64.Encode_turn(strPostStr.c_str(),(unsigned char*)sz64PostStr,2048);
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
    reply = client.Search(request);
  }
 
  return 0;
}

