#include <iostream>
#include <memory>
#include <string>
#include <unistd.h> 
#include <grpc++/grpc++.h>
#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
 
#include "lib/examples.grpc.pb.h"
#include "lib/Base64.h" 
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

#define CRYPT_KEY "heigaga"

typedef struct stClientInfo
{
  char szUser[64];
  char szPasswd[64];
  char szMode[64];
  char szCTime[64];
  char szClientIp[64];
  char szClientPort[64];
  char szClientIpv4or6[64];
  stClientInfo()
  {
    memset(szUser,0x00,64);
    memset(szPasswd,0x00,64);
    memset(szMode,0x00,64);
    memset(szCTime,0x00,64);
    memset(szClientIp,0x00,64);
    memset(szClientPort,0x00,64);
    memset(szClientIpv4or6,0x00,64);
  }
}stClient,*pClient;

 
class SearchRequestImpl final : public SearchService::Service {
  Status Search(ServerContext* context, const SearchRequest* request,
                  SearchResponse* reply) override {
    char szDecodeData[2048] = {"\0"};
    m_Base64.Decode( (request->request()).c_str(),(unsigned char*)szDecodeData,2048);
    //std::cout << "get string : " << szDecodeData << std::endl;
    std::string strClientAddr = context->peer();
    //std::cout << "connect : " << strClientAddr << std::endl;
    int res = dealPostString((const char*)szDecodeData,strClientAddr);
    char szResponse[2];
    sprintf(szResponse,"%d",res);
    reply->set_response(szResponse);
    return Status::OK;
  }

//new functions for server  
public:
  int setDBip(std::string strAddr);
private:
  std::string m_es_url;
  CBase64 m_Base64;
  int dealPostString(const char* szPostString,std::string strClientAddr);
  int getClientInfo(const char* szPostString,std::string strClientAddr,stClient& clientInfo);
  int deal(stClient clientInfo);
  std::string getStatus(stClient clientInfo);
  std::string getUserPasswd(stClient clientInfo);
  std::string kickOffOldUser(stClient clientInfo);
  std::string addNewLoginInfo(stClient clientInfo);
  std::string addNewUser(stClient clientInfo);
  std::string dealCurlOrder(std::string strCode);

  
};
 
int SearchRequestImpl::setDBip(std::string strAddr)
{
  m_es_url = strAddr;
  return 0;
}
int SearchRequestImpl::dealPostString(const char* szPostString,std::string strClientAddr)
{
  int res = 0;
  stClient clientInfo;
  res = getClientInfo((const char*)szPostString,strClientAddr,clientInfo);
  res = deal(clientInfo);
  if( res == 1 )
    std::cout << "user : " << clientInfo.szUser << "  sign up susseced!" << std::endl;
  else if ( res == 2 )
    std::cout << "user : " << clientInfo.szUser << "  sign up failed!"  << std::endl;
  else if ( res == 3 )
    std::cout << "user : " << clientInfo.szUser << "  sign in failed!"  << std::endl;
  else if ( res == 4 )
    std::cout << "user : " << clientInfo.szUser << "  connect !"  << std::endl;
  
  return res;
}

int SearchRequestImpl::getClientInfo(const char* szPostString,std::string strClientAddr,stClient& clientInfo)
{
  if(szPostString == NULL)
    return -1;
  sscanf(szPostString,"%[^|]|%[^|]|%[^|]|%[^|]",clientInfo.szUser,clientInfo.szPasswd,clientInfo.szMode,clientInfo.szCTime);
  strncpy(clientInfo.szClientIpv4or6,strClientAddr.c_str(),4);
  if( strcmp(clientInfo.szClientIpv4or6,"ipv4") == 0 )
    sscanf(strClientAddr.c_str(),"%*[^:]:%[^:]:%s",clientInfo.szClientIp,clientInfo.szClientPort);
  else
    sscanf(strClientAddr.c_str(),"%*[^[][%[^]]]:%s",clientInfo.szClientIp,clientInfo.szClientPort);
  
  return 0;
}

int SearchRequestImpl::deal(stClient clientInfo)
{
  
  //first check if user in table,if not and mode = 1 , create
  //if user had been ceate , mode = 1 , response sign up failed
  //if user  had been created,mode = 0 , check if passwd right: right go to next ,wrong reponse passwd wrong
  std::string strPasswd = getUserPasswd(clientInfo);
  if (!strPasswd.empty())
  {  
    if( atoi(clientInfo.szMode) == 0 )
    {
      if( strPasswd == clientInfo.szPasswd )
      {
        //check login info
        std::string strStatus = getStatus(clientInfo);
        if(!strStatus.empty())
        {
          if( strStatus  ==  "0" )
            return 0;
          else
          {
            return 4;
          }
        }
        else
        {
          kickOffOldUser(clientInfo);
          addNewLoginInfo(clientInfo);
          return 4;
        }
      }
      else
        return 3;
    }
    else
      return 2;
  }
  else
  {
    if(atoi(clientInfo.szMode) == 0)
    {
      return 3;  //sign up first,refused
    }
    else
    {
      // try sign up
      std::string strRes = addNewUser(clientInfo);
      if(strRes == "2")
        return 2;
      strRes = addNewLoginInfo(clientInfo);
      return atoi(strRes.c_str());  // 1 sign up susseced
    }
  }
}

std::string SearchRequestImpl::getStatus(stClient clientInfo)
{
  std::string strCode = "curl -s -XGET '";
  strCode += m_es_url;
  strCode += "/clientinfo/user_log_info/_search' -d '{\"query\":{\"bool\":{\"must\":[{\"match\":{\"user\":\"";
  strCode += clientInfo.szUser;
  strCode += "\"}},{\"match\":{\"ip\":\"";
  strCode += clientInfo.szClientIp;
  strCode += "\"}},{\"match\":{\"ltime\":\"";
  strCode += clientInfo.szCTime;
  strCode += "\"}}]}}}'";
  
  std::string strRes = dealCurlOrder(strCode);
  char* pBegin = strstr((char*)strRes.c_str(),"\"status\":\"");
  if(pBegin)
  {
    pBegin += 10;
    char* pDot = strstr(pBegin,"\"");
    if(!pDot)
      return "";
    char szTmp[pDot - pBegin + 1];
    memset(szTmp,0x00,pDot-pBegin+1);
    strncpy(szTmp,pBegin,pDot-pBegin);
    return szTmp;
  }
  else
    return "";
}
    
std::string SearchRequestImpl::getUserPasswd(stClient clientInfo)
{
  std::string strCode = "curl -s -XGET '";
  strCode += m_es_url;
  strCode += "/clientinfo/userinfo/";
  strCode += clientInfo.szUser;
  strCode += "'";
  
  std::string strRes = dealCurlOrder(strCode);
  char* pBegin = strstr((char*)strRes.c_str(),"\"passwd\":\"");
  if(pBegin)
  {
    pBegin += 10;
    char* pDot = strstr(pBegin,"\"");
    if(!pDot)
      return "";
    char szPasswd[pDot - pBegin + 1];
    memset(szPasswd,0x00,pDot-pBegin+1);
    strncpy(szPasswd,pBegin,pDot-pBegin);
    char szDecodeData[2048] = {"\0"};
    m_Base64.Decode( szPasswd,(unsigned char*)szDecodeData,2048);
    return szDecodeData;
  }
  else
    return "";
}

std::string SearchRequestImpl::kickOffOldUser(stClient clientInfo)
{
  std::string strCode = "curl -s -XGET '";
  strCode += m_es_url;
  strCode += "/clientinfo/user_log_info/_search' -d '{\"query\":{\"bool\":{\"must\":[{\"match\":{\"user\":\"";
  strCode += clientInfo.szUser;
  strCode += "\"}},{\"match\":{\"status\":\"1\"}}]}}}'";
  
  std::string strRes = dealCurlOrder(strCode);
  char* pBegin = strstr((char*)strRes.c_str(),"\"_id\":\"");
  if(pBegin)
  {
    pBegin += 7;
    char* pDot = strstr(pBegin,"\"");
    if(!pDot)
      return "3";
    char szTmp[pDot - pBegin + 1];
    memset(szTmp,0x00,pDot-pBegin+1);
    strncpy(szTmp,pBegin,pDot-pBegin);
    std::string strId = szTmp;
    pBegin = strstr(pDot,"\"_source\":");
    
    if(pBegin == NULL)
      return "3";
    pBegin += 10;
    pDot = strstr(pBegin,"}");
    if(!pDot)
      return "3";
    pDot++;
    char szTmpSource[pDot-pBegin+1];
    memset(szTmpSource,0x00,pDot-pBegin+1);
    strncpy(szTmpSource,pBegin,pDot-pBegin);
    pDot = strstr(szTmpSource,"\"status\":\"");
    if( pDot == NULL )
      return "3";
    // status 1 -> status 0
    pDot += 10;
    *pDot = '0';
    std::string strSource = szTmpSource;
    std::string strCode = "curl -s -XPUT '";
    strCode += m_es_url;
    strCode += "/clientinfo/user_log_info/";
    strCode += strId;
    strCode += "' -d '";
    strCode += strSource;
    strCode += "'";
    strRes = dealCurlOrder(strCode);
    return "2";
  }
  else
    return "3";
  
}

std::string SearchRequestImpl::addNewLoginInfo(stClient clientInfo)
{
  std::string strCode = "curl -s -XPOST '";
  strCode += m_es_url;
  strCode += "/clientinfo/user_log_info' -d '{\"user\":\"";
  strCode += clientInfo.szUser;
  strCode += "\",\"ip\":\"";
  strCode += clientInfo.szClientIp;
  strCode += "\",\"ltime\":\"";
  strCode += clientInfo.szCTime;
  strCode += "\",\"status\":\"1\"}'";
  std::string strRes = dealCurlOrder(strCode);
  if(strstr(strRes.c_str(),"successful\":1"))
    strRes = "1";
  else
    strRes = "5"; //connect db failed
  return strRes;
}

std::string SearchRequestImpl::addNewUser(stClient clientInfo)
{
  std::string strCode = "curl -s -XPUT '";
  strCode += m_es_url;
  strCode += "/clientinfo/userinfo/";
  strCode += clientInfo.szUser;
  strCode += "' -d '{\"passwd\":\"";
  char szEncodeData[2048] = {"\0"};
  m_Base64.Encode_turn(clientInfo.szPasswd,(unsigned char*)szEncodeData,2048);
  strCode += szEncodeData;
  strCode += "\"}'";
  std::string strRes = dealCurlOrder(strCode);
  if(strstr(strRes.c_str(),"successful\":1"))
    strRes = "1";
  else
    strRes = "2"; //sign up failed
  return strRes;
}

std::string SearchRequestImpl::dealCurlOrder(std::string strCode)
{
  FILE* fp = popen(strCode.c_str(),"r");
  if (NULL == fp)
  {
	  return "2";
  }        
  else
  {
    char* pData = NULL;
    size_t nLen = 0;
    ssize_t nRead;
    if ((nRead = getline(&pData, &nLen, fp)) < 1)  
    {
        if (pData)free(pData);
        pclose(fp);
        return "2";        
    }
    else
    {
        pData[nRead] = '\0';
    
        std::string strRes = pData;
        if (pData)
          free(pData);
        pclose(fp);         
        return strRes;
    }    
    
  }
}  

bool createESIndex(std::string strAddr)
{
  std::string strCode = "curl -s -XPUT '";
  strCode += strAddr + "/clientinfo'";
  
   FILE* fp = popen(strCode.c_str(),"r");
    if (NULL == fp)
    {
		  return false;
    }        
    else
    {
      char* pData = NULL;
      size_t nLen = 0;
      ssize_t nRead;
      if ((nRead = getline(&pData, &nLen, fp)) < 1)  
      {
          if (pData)free(pData);
          pclose(fp);
          return false;        
      }
      else
      {
          pData[nRead] = '\0';
      
          std::string strRes = pData;
          
          char* pCheck = strstr(pData,"{");
          if (pData)
            free(pData);
          pclose(fp);
          if(!pCheck)
            return false;         
      }    
      
    }
  
  strCode = "curl -XPOST '";
  strCode += strAddr + "/clientinfo/userinfo/_mapping' -d '{\"userinfo\":{\"properties\": {\"user\" :{\"type\":\"string\",\"index\":\"not_analyzed\"},\"passwd\":{\"type\":\"string\",\"index\":\"not_analyzed\"},\"ip\":{\"type\":\"string\",\"index\":\"not_analyzed\"},\"ltime\":{\"type\":\"string\",\"index\":\"not_analyzed\"},\"status\":{\"type\":\"string\",\"index\":\"not_analyzed\"}}}}'";

  system(strCode.c_str());
  
  strCode = "curl -XPOST '";
  strCode += strAddr + "/clientinfo/user_log_info//_mapping' -d '{\"user_log_info\":{\"properties\": {\"user\" :{\"type\":\"string\",\"index\":\"not_analyzed\"},\"passwd\":{\"type\":\"string\",\"index\":\"not_analyzed\"},\"ip\":{\"type\":\"string\",\"index\":\"not_analyzed\"},\"ltime\":{\"type\":\"string\",\"index\":\"not_analyzed\"},\"status\":{\"type\":\"string\",\"index\":\"not_analyzed\"}}}}'";
  
  system(strCode.c_str());
  
  return true;
}

void RunServer(std::string strAddr) {
  
  if( !createESIndex(strAddr))
  {
    std::cout << "can't connect to db ,please check db status !" << std::endl;
    return ;
  }
  std::string server_address("0.0.0.0:50051");
  SearchRequestImpl service;
  service.setDBip(strAddr);

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout <<  std::endl << "Server listening on " << server_address << std::endl;
 
  server->Wait();
}
 
int main(int argc, char** argv) {
  
  int ch;
  std::string strAddr;
  while ((ch = getopt(argc, argv, "H:")) != -1)
  {
    switch (ch) 
    {
      case 'H':
        strAddr = optarg;
        break;
      default:
      {
        //printf("Unknown option: %c\n",(char)optopt);
      }
      
    }
  }
  if(strAddr.empty())
  {
    printf("请输入参数：\n"\
    "   -H dbip:port\n");
    return 0;
  }
  RunServer(strAddr);
 
  return 0;
}
