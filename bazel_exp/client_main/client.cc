#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>
 
#include <grpc++/grpc++.h>
#include <grpc/support/log.h>
#include <fstream>  
#include <streambuf> 
#include <grpcpp/impl/codegen/sync_stream.h>

#include "lib/CSLibs.grpc.pb.h"
#include "lib/Base64.h" 

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

 
class ClientImpl {
 public:
  explicit ClientImpl(std::shared_ptr<Channel> channel)
      : stub_(ServerService::NewStub(channel)) {}
 
  std::string userLogin(ClientRequestParams request) {
 
    
    ServerResponse reply;
   
    ClientContext context;
 
    CompletionQueue cq;
 
    Status status;
 
    std::unique_ptr<ClientAsyncResponseReader<ServerResponse> > rpc(
        stub_->AsyncuserLogin(&context, request, &cq));
 
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
  
  std::string userSignUp(ClientRequestParams request) {
 
    
    ServerResponse reply;
   
    ClientContext context;
 
    CompletionQueue cq;
 
    Status status;
 
    std::unique_ptr<ClientAsyncResponseReader<ServerResponse> > rpc(
        stub_->AsyncuserSignUp(&context, request, &cq));
 
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
  
  std::string getUserLoginStatus(ClientRequestParams request) {
 
    
    ServerResponse reply;
   
    ClientContext context;
 
    CompletionQueue cq;
 
    Status status;
 
    std::unique_ptr<ClientAsyncResponseReader<ServerResponse> > rpc(
        stub_->AsyncgetUserLoginStatus(&context, request, &cq));
 
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

  std::string userLoginOut(ClientRequestParams request) {
 
    
    ServerResponse reply;
   
    ClientContext context;
 
    CompletionQueue cq;
 
    Status status;
 
    std::unique_ptr<ClientAsyncResponseReader<ServerResponse> > rpc(
        stub_->AsyncuserLoginOut(&context, request, &cq));
 
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

  std::string waitForOffLineSignal(ClientRequestParams request)
  {
    ServerResponse reply;
   
    ClientContext context;
 
    Status status;
    std::unique_ptr<grpc::ClientReaderWriter<ClientRequestParams,ServerResponse> > stream(stub_->waitForOffLine(&context));
    stream->Write(request);
    while (stream->Read(&reply)) {
      if( reply.response() == "0")
      {
        request.set_strmode("-1");
        stream->Write(request);  
        stream->WritesDone();
      }
      else
      {
        stream->Write(request); 
      }
      //std::cout << "Found reply called " <<  reply.response()<< std::endl;
    }
    status = stream->Finish();
    if(status.ok())
      return reply.response();
    else {
      return "RPC failed";
    }
  }
 
 private:
 
  std::unique_ptr<ServerService::Stub> stub_;
};

typedef struct threadParams
{
  ClientImpl* client;
  //ClientRequestParams request;
  std::string strUser;
  std::string strPasswd;
  std::string strMode;
  std::string strTime;

}TParams,*PTParams; 

void* waitForSignalFromServer(void* param) 
{
  PTParams ptparams = (PTParams)param;
  ClientRequestParams request;
  request.set_struser(ptparams->strUser);
  request.set_strpasswd(ptparams->strPasswd);
  request.set_strmode(ptparams->strMode);
  request.set_strltime(ptparams->strTime);
  std::string strRes = ptparams->client->waitForOffLineSignal(request);
  if(strRes == "0")
    printf("Connect refused by server! Reason: This user had logined on another machion!\n");
  else
    printf("%s\n",strRes.c_str());
  return NULL;
}

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
  ClientImpl client(grpc::CreateChannel(strAddr.c_str(), creds));
  char szTime[13];
  time_t cTime = time(NULL);
  sprintf(szTime,"%d",(int)cTime);
  
  ClientRequestParams request;
  //request.set_request(user);
  request.set_struser(strUser);
  request.set_strpasswd(strPasswd);
  request.set_strmode(strMode);
  request.set_strltime(szTime);
  std::string reply;
  if( request.strmode() == "1")
  {  
    reply = client.userSignUp(request);  // The actual RPC call!
    //request.set_strmode("0");
  }
  else if( request.strmode() == "0" )
    reply = client.userLogin(request);
  else
  {
    printf("unknown mode , please check your mode param again:\n  mode = 1 :new user sign up\n  mode = 0 : user login\n");
    return 0;
  }
  
  while ( true )
  {
    if(reply == "RPC failed")
    {
      std::cout << reply << std::endl;
      //break;
      return 0;
    }
    
    if( reply == "0" )
    {
      std::cout << "Connect refused by server! Reason: This user had logined on another machion!" << std::endl;
      //break;
      return 0;
    }
    else if( reply == "1" )
    {
      std::cout << "user: " << strUser << " sign up susseced ! Now login ..." << std::endl;
      reply = client.userLogin(request);
      continue; //must continue , reply may be 0
    }
    else if( reply == "2" )
    {
      std::cout << "sign up faild! Please try again!" << std::endl;
      //break;
      return 0;
    }
    else if(reply == "3")
    {
      std::cout << "login faild ! User or Password is wrong !" << std::endl;
      //break;
      return 0;
    }
    std::cout << "login susseced! Now keep on login ..." << std::endl;
    break;
    //sleep(2);
    //reply = client.getUserLoginStatus(request);
  }
  //create thread wait for kickoff signal
  TParams tparams;
  tparams.client = &client;
  tparams.strUser = strUser;
  tparams.strPasswd = strPasswd;
  tparams.strMode = strMode;
  tparams.strTime = szTime;
  waitForSignalFromServer((void*) &tparams );
  /*pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED );
  printf("===[%d]==\n",__LINE__);  
  pthread_t tThreadT;
	if (pthread_create(&tThreadT, &attr, waitForSignalFromServer,(void*) &tparams ))
	{
		printf("开启强制下线消息接收线程失败\n");
		return 0;
	}
	printf("===[%d]==\n",__LINE__);
	pthread_join(tThreadT,NULL);
	printf("===[%d]==\n",__LINE__);
  pthread_attr_destroy(&attr);*/
  return 0;
}

