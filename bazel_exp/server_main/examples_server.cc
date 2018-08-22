#include <iostream>
#include <memory>
#include <string>
#include <unistd.h> 
#include <grpc++/grpc++.h>
#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <fstream>
#include <streambuf>
#include <vector>
#include <rpc/des_crypt.h>

#include "lib/examples.grpc.pb.h"
#include "lib/Base64.h" 
#include "lib/Lock.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

#define CRYPT_KEY "heigagar"
#define ES_USER_PWD "admin:admin@"
static int kick_time_conf = 0;

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

typedef struct EsdocInfo
{
  std::string strId;
  std::string strUser;
  std::string strSource;
  EsdocInfo()
  {
    strId = "";
    strUser = "";
    strSource = "";
  }
}docinfo;

typedef std::vector<docinfo> vct_docinfo;
typedef vct_docinfo::iterator vct_docinfo_iter;
  
//创建一个互斥锁
CMutex g_Lock;  //for db
CMutex g_sLock; //for static

class curltool
{
public:
  curltool(){};
  ~curltool(){};
  std::string dealCurlOrder(std::string strCode);
};
 
class SearchRequestImpl final : public SearchService::Service {
  Status Search(ServerContext* context, const SearchRequest* request,
                  SearchResponse* reply) override {
    //对被保护资源自动加锁
	  //函数结束前，自动解锁
	  //printf("===[ lock ]===\n");
	  CMyLock lock(g_Lock);

    //char szDecodeData[2048] = {"\0"};
    //m_Base64.Decode( (request->request()).c_str(),(unsigned char*)szDecodeData,2048);
    //std::cout << "get string : " << szDecodeData << std::endl;
    std::string strClientAddr = context->peer();
    //std::cout << "connect : " << strClientAddr << std::endl;
    int res = dealPostString(request,strClientAddr);
    char szResponse[2];
    sprintf(szResponse,"%d",res);
    reply->set_response(szResponse);
    //printf("===[ unlock ]===\n");
    return Status::OK;
  }

//new functions for server  
public:
  int setDBip(std::string strAddr);
  
private:
  std::string m_es_url;
  CBase64 m_Base64;
  curltool tools;
  int dealPostString(const SearchRequest* request,std::string strClientAddr);
  int getClientInfo(const SearchRequest* request,std::string strClientAddr,stClient& clientInfo);
  int deal(stClient clientInfo);
  std::string getStatus(stClient clientInfo);
  std::string getUserPasswd(stClient clientInfo);
  std::string kickOffOldUser(stClient clientInfo);
  std::string addNewLoginInfo(stClient clientInfo);
  std::string addNewUser(stClient clientInfo);
};
 
int SearchRequestImpl::setDBip(std::string strAddr)
{
  m_es_url = strAddr;
  return 0;
}
int SearchRequestImpl::dealPostString(const SearchRequest* request,std::string strClientAddr)
{
  int res = 0;
  stClient clientInfo;
  res = getClientInfo(request,strClientAddr,clientInfo);
  res = deal(clientInfo);
  /*if( res == 1 )
    std::cout << "user : " << clientInfo.szUser << "  sign up susseced!" << std::endl;
  else if ( res == 2 )
    std::cout << "user : " << clientInfo.szUser << "  sign up failed!"  << std::endl;
  else if ( res == 3 )
    std::cout << "user : " << clientInfo.szUser << "  sign in failed!"  << std::endl;
  else if ( res == 4 )
    std::cout << "user : " << clientInfo.szUser << "  connect !"  << std::endl;*/
  
  return res;
}

int SearchRequestImpl::getClientInfo(const SearchRequest* request,std::string strClientAddr,stClient& clientInfo)
{
  if(request == NULL)
    return -1;
  //sscanf(szPostString,"%[^|]|%[^|]|%[^|]|%[^|]",clientInfo.szUser,clientInfo.szPasswd,clientInfo.szMode,clientInfo.szCTime);
  sprintf(clientInfo.szUser,"%s",(request->struser()).c_str());
  sprintf(clientInfo.szPasswd,"%s",(request->strpasswd()).c_str());
  sprintf(clientInfo.szMode,"%s",(request->strmode()).c_str());
  sprintf(clientInfo.szCTime,"%s",(request->strltime()).c_str());
  
  strncpy(clientInfo.szClientIpv4or6,strClientAddr.c_str(),4);
  if( strcmp(clientInfo.szClientIpv4or6,"ipv4") == 0 )
    sscanf(strClientAddr.c_str(),"%*[^:]:%[^:]:%s",clientInfo.szClientIp,clientInfo.szClientPort);
  else
    sscanf(strClientAddr.c_str(),"%*[^[][%[^]]]:%s",clientInfo.szClientIp,clientInfo.szClientPort);
  
  return 0;
}

/*int SearchRequestImpl::getClientInfo(const char* szPostString,std::string strClientAddr,stClient& clientInfo)
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
}*/

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
  std::string strCode = "curl -s -k -XGET 'https://";
  strCode += ES_USER_PWD;
  strCode += m_es_url;
  strCode += "/clientinfo/user_log_info/_search' -d '{\"query\":{\"bool\":{\"must\":[{\"match\":{\"user\":\"";
  strCode += clientInfo.szUser;
  strCode += "\"}},{\"match\":{\"ip\":\"";
  strCode += clientInfo.szClientIp;
  strCode += "\"}},{\"match\":{\"ltime\":\"";
  strCode += clientInfo.szCTime;
  strCode += "\"}}]}}}'";
  
  std::string strRes = tools.dealCurlOrder(strCode);
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
  std::string strCode = "curl -s -k -XGET 'https://";
  strCode += ES_USER_PWD;
  strCode += m_es_url;
  strCode += "/clientinfo/userinfo/";
  strCode += clientInfo.szUser;
  strCode += "'";
  
  std::string strRes = tools.dealCurlOrder(strCode);
  char* pBegin = strstr((char*)strRes.c_str(),"\"passwd\":\"");
  if(pBegin)
  {
    pBegin += 10;
    char* pDot = strstr(pBegin,"\"");
    if(!pDot)
      return "";
    int iLen = (pDot - pBegin) + 8 - ((pDot - pBegin)%8);
    char szPasswd[iLen];
    memset(szPasswd,0x00,iLen);
    strncpy(szPasswd,pBegin,pDot-pBegin);
    char szDecodeData[2048] = {"\0"};
    m_Base64.Decode( szPasswd,(unsigned char*)szDecodeData,2048);
    char szTmp[2048] = {"\0"};
    int res = cbc_crypt(CRYPT_KEY, szDecodeData, iLen, DES_DECRYPT,szTmp);
    //printf("==[%s]==[%s]==\n",szTmp,szDecodeData);
    //return szDecodeData;
    if(res == DESERR_NONE || res == DESERR_NOHWDEVICE )
      return szDecodeData;
    else
      return "";
  }
  else
    return "";
}

std::string SearchRequestImpl::kickOffOldUser(stClient clientInfo)
{
  std::string strCode = "curl -s -k -XGET 'https://";
  strCode += ES_USER_PWD;
  strCode += m_es_url;
  strCode += "/clientinfo/user_log_info/_search' -d '{\"query\":{\"bool\":{\"must\":[{\"match\":{\"user\":\"";
  strCode += clientInfo.szUser;
  strCode += "\"}},{\"match\":{\"status\":\"1\"}}]}}}'";
  
  std::string strRes = tools.dealCurlOrder(strCode);
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
    std::string strCode = "curl -s -k -XPUT 'https://";
    strCode += ES_USER_PWD;
    strCode += m_es_url;
    strCode += "/clientinfo/user_log_info/";
    strCode += strId;
    strCode += "' -d '";
    strCode += strSource;
    strCode += "'";
    strRes = tools.dealCurlOrder(strCode);
    return "2";
  }
  else
    return "3";
  
}

std::string SearchRequestImpl::addNewLoginInfo(stClient clientInfo)
{
  std::string strCode = "curl -s -k -XPOST 'https://";
  strCode += ES_USER_PWD;
  strCode += m_es_url;
  strCode += "/clientinfo/user_log_info' -d '{\"user\":\"";
  strCode += clientInfo.szUser;
  strCode += "\",\"ip\":\"";
  strCode += clientInfo.szClientIp;
  strCode += "\",\"ltime\":\"";
  strCode += clientInfo.szCTime;
  strCode += "\",\"status\":\"1\"}'";
  std::string strRes = tools.dealCurlOrder(strCode);
  if(strstr(strRes.c_str(),"successful\":1"))
    strRes = "1";
  else
    strRes = "5"; //connect db failed
  return strRes;
}

std::string SearchRequestImpl::addNewUser(stClient clientInfo)
{
  std::string strCode = "curl -s -k -XPUT 'https://";
  strCode += ES_USER_PWD;
  strCode += m_es_url;
  strCode += "/clientinfo/userinfo/";
  strCode += clientInfo.szUser;
  strCode += "' -d '{\"passwd\":\"";
  char szEncodeData[2048] = {"\0"};
  //m_Base64.Encode_turn(clientInfo.szPasswd,(unsigned char*)szEncodeData,2048);
  int iLen = strlen(clientInfo.szPasswd) + 8 - (strlen(clientInfo.szPasswd)%8);
  int res = cbc_crypt(CRYPT_KEY, clientInfo.szPasswd, iLen, DES_ENCRYPT,szEncodeData);
  //printf("==[%s]==[%s]==\n",clientInfo.szPasswd,szEncodeData);
  memset(szEncodeData,0x00,2048);
  m_Base64.Encode_turn(clientInfo.szPasswd,(unsigned char*)szEncodeData,2048);
  strCode += szEncodeData;
  strCode += "\"}'";
  std::string strRes = tools.dealCurlOrder(strCode);
  if(strstr(strRes.c_str(),"successful\":1"))
    strRes = "1";
  else
    strRes = "2"; //sign up failed
  return strRes;
}

std::string curltool::dealCurlOrder(std::string strCode)
{
  //printf("==[%s]==\n",strCode.c_str());
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
  std::string strCode = "curl -s -k -XPUT 'https://";
  strCode += ES_USER_PWD;
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
  
  strCode = "curl -s -k -XPOST 'https://";
  strCode += ES_USER_PWD;
  strCode += strAddr + "/clientinfo/userinfo/_mapping' -d '{\"userinfo\":{\"properties\": {\"user\" :{\"type\":\"string\",\"index\":\"not_analyzed\"},\"passwd\":{\"type\":\"string\",\"index\":\"not_analyzed\"},\"ip\":{\"type\":\"string\",\"index\":\"not_analyzed\"},\"ltime\":{\"type\":\"string\",\"index\":\"not_analyzed\"},\"status\":{\"type\":\"string\",\"index\":\"not_analyzed\"}}}}'";

  system(strCode.c_str());
  
  strCode = "curl -s -k -XPOST 'https://";
  strCode += ES_USER_PWD;
  strCode += strAddr + "/clientinfo/user_log_info//_mapping' -d '{\"user_log_info\":{\"properties\": {\"user\" :{\"type\":\"string\",\"index\":\"not_analyzed\"},\"passwd\":{\"type\":\"string\",\"index\":\"not_analyzed\"},\"ip\":{\"type\":\"string\",\"index\":\"not_analyzed\"},\"ltime\":{\"type\":\"string\",\"index\":\"not_analyzed\"},\"status\":{\"type\":\"string\",\"index\":\"not_analyzed\"}}}}'";
  
  system(strCode.c_str());
  
  return true;
}

bool getAllUser(std::string strRes,vct_docinfo& doc_vct)
{
  if(strRes.empty())
    return false;
  char* pBegin = strstr((char*)strRes.c_str(),"\"_id\":\"");
  while(pBegin!= NULL)
  {
    pBegin += 7;
    char* pEnd = strstr(pBegin,"\"");
    if(pEnd == NULL)
      break;
    char szId[pEnd-pBegin+1];
    memset(szId,0x00,pEnd-pBegin+1);
    memcpy(szId,pBegin,pEnd - pBegin);
    pBegin = strstr(pEnd,"\"_source\":{");
    if(pBegin == NULL)
      break;
    pBegin+= 10;
    pEnd = strstr(pBegin,"\"status\":\"1\"");
    if(pEnd == NULL)
      break;
    char szSource[pEnd-pBegin+1];
    memset(szSource,0x00,pEnd - pBegin + 1);
    memcpy(szSource,pBegin,pEnd - pBegin);
    
    pBegin = strstr(pBegin,"\"user\":\"");
    if(pBegin == NULL)
      break;
    pBegin += 8;
    pEnd = strstr(pBegin,"\"");
    if(pEnd == NULL)
      break;
    char szUser[pEnd-pBegin+1];
    memset(szUser,0x00,pEnd-pBegin+1);
    memcpy(szUser,pBegin,pEnd-pBegin);  
    
    pBegin = strstr(pEnd,"\"_id\":\"");
      
    docinfo dInfo;
    dInfo.strId = szId;
    dInfo.strUser = szUser;
    dInfo.strSource = szSource;
    doc_vct.push_back(dInfo);
  }
  return true;
}

vct_docinfo getUserOnline(std::string strAddr,std::string strUser = "",std::string strLTime = "")
{
  std::string strCode = "curl -s -k -XGET 'https://";
  strCode += ES_USER_PWD;
  strCode += strAddr;
  strCode += "/clientinfo/user_log_info/_search' -d '{\"query\":{\"bool\":{\"must\":[{\"match\":{\"status\":\"1\"}}";
  if(!strUser.empty())
  {
    strCode += ",{\"wildcard\":{\"user\":\"*";
    strCode += strUser;
    strCode += "*\"}}";
  }
  if(!strLTime.empty())
  {
    strCode += ",{\"range\":{\"ltime\":{\"lte\":\"";
    strCode += strLTime;
    strCode += "\"}}}";
  }
  strCode += "]}}}'";
  //printf("strcode = [%s]\n",strCode.c_str());
  curltool tools;
  std::string strRes = tools.dealCurlOrder(strCode);
  //printf("strRes = [%s]\n",strRes.c_str());
  
  vct_docinfo doc_vct;
  getAllUser(strRes,doc_vct);
  return doc_vct;
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
  
  
  std::ifstream skey("server.key");
  std::string strServerKey((std::istreambuf_iterator<char>(skey)),std::istreambuf_iterator<char>());
  //std::cout << "key: " <<strServerKey << std::endl;
  std::ifstream sCrt("server.crt");  
  std::string strServerCert((std::istreambuf_iterator<char>(sCrt)),std::istreambuf_iterator<char>());
  //std::cout << "crt: " << strServerCert << std::endl;
  std::ifstream sCaCrt("ca.crt");  
  std::string strCaCert((std::istreambuf_iterator<char>(sCaCrt)),std::istreambuf_iterator<char>());
  //std::cout << "ca: " << strClientCert << std::endl;
  grpc::SslServerCredentialsOptions::PemKeyCertPair pkcp ={strServerKey.c_str(),strServerCert.c_str()};
  //grpc::SslServerCredentialsOptions ssl_opts(GRPC_SSL_DONT_REQUEST_CLIENT_CERTIFICATE);
  grpc::SslServerCredentialsOptions ssl_opts;
  ssl_opts.pem_root_certs=strCaCert;
  ssl_opts.pem_key_cert_pairs.push_back(pkcp);
  std::shared_ptr<grpc::ServerCredentials> creds = grpc::SslServerCredentials(ssl_opts);
  //auto creds = grpc::SslServerCredentials(grpc::SslServerCredentialsOptions());
  builder.AddListeningPort(server_address,creds );
  //builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  //builder.AddListeningPort(server_address, grpc::SslServerCredentials(grpc::SslServerCredentialsOptions()));
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout <<  std::endl << "Server listening on " << server_address << std::endl;
 
  //server->Wait();
  while(true)
  {
    printf("server-code:");
    char szCode[1024] = {"\0"};
    //scanf("%s",szCode);
    fgets(szCode,1024,stdin);
    int len = strlen(szCode);
    szCode[--len] = '\0';
    if( len == 0 )
      continue;
    char* p = NULL;
    int i = 0;
    for( ; i< len ; i++)
    {
      if(*(szCode+i) != ' ' && *(szCode+i) != '\t')
      {
        p=szCode+i;
        break;
      }
    }
    if( i == len)
      continue;
    len -= i;
    while(char* pT = strchr(p,'\t'))
    {
      *pT = ' ';
    }
    
    char* szCodes[4] = {"\0","\0","\0","\0"};
    char* s = strsep(&p," ");
    for(int l = 0 ; l < 4 ; l++)
    {
      if(s == NULL)
       break;
      if(strlen(s) == 0)
      {
        l--;
        s = strsep(&p," ");
        continue;
      }
      szCodes[l] = s;
      s = strsep(&p," ");
    }
    CMyLock lock(g_Lock);
    if(strcmp(szCodes[0],"exit") ==0 )
      break;
    else if(strcmp(szCodes[0],"help") == 0)
      printf("mode list:\n  list : 显示所有在线用户\n  list -t m : 显示上线时间超过 m 分钟，且仍然在线的用户\n  list -u username : 显示用户名包含 username 的所有在线用户\n  kickoff -u username :强制用户username下线\n  kickoff -t m ：强制登录时间超过 m 分钟的所有用户下线\n  kickoff -a : 强制所有当前在线用户下线\n  set -t m : 设置策略：自动强制在线时间超过m分钟的用户下线。使用set -t 或set -t 0 取消策略。重新配置会自动生效。\n  exit : 退出并关闭服务\n");
    else if(strcmp(szCodes[0],"list") == 0)
    {
      if(strcmp(szCodes[1],"-u") == 0)
      {
        vct_docinfo user_vct = getUserOnline(strAddr,szCodes[2]);
        vct_docinfo_iter iter = user_vct.begin();
        for(;iter!= user_vct.end();iter++)
        {
          printf("  %s\n",(iter->strUser).c_str());
        }
      }
      else if(strcmp(szCodes[1],"-t") == 0)
      {
        int c_time = (int)time(NULL);
        c_time -= atoi(szCodes[2])*60;
        char szTmp[24];
        sprintf(szTmp,"%d",c_time);
        vct_docinfo user_vct = getUserOnline(strAddr,"",szTmp);
        vct_docinfo_iter iter = user_vct.begin();
        for(;iter!= user_vct.end();iter++)
        {
          printf("  %s\n",(iter->strUser).c_str());
        }
      }
      else
      {
        vct_docinfo user_vct = getUserOnline(strAddr);
        vct_docinfo_iter iter = user_vct.begin();
        for(;iter!= user_vct.end();iter++)
        {
          printf("  %s\n",(iter->strUser).c_str());
        }
      }
    }
    else if(strcmp(szCodes[0],"kickoff") == 0)
    {
      if(strcmp(szCodes[1],"-u") == 0)
      {
        if(strlen(szCodes[2]) < 1 )
        {
          printf("请输入正确的用户名\n");
          
        }
        else
        {
          vct_docinfo user_vct = getUserOnline(strAddr,szCodes[2]);
          vct_docinfo_iter iter = user_vct.begin();
          for(;iter!= user_vct.end();iter++)
          {
            if(iter->strUser == szCodes[2])
            {
              std::string strCode = "curl -s -k -XPUT 'https://";
              strCode += ES_USER_PWD;
              strCode += strAddr;
              strCode += "/clientinfo/user_log_info/";
              strCode += iter->strId;
              strCode += "' -d '";
              strCode += iter->strSource;
              strCode += "\"status\":\"0\"}'";
              curltool tools;
              tools.dealCurlOrder(strCode);
            }
          }
        }
      }
      else if(strcmp(szCodes[1],"-t") == 0)
      {
        int c_time = (int)time(NULL);
        c_time -= atoi(szCodes[2])*60;
        char szTmp[24];
        sprintf(szTmp,"%d",c_time);
        vct_docinfo user_vct = getUserOnline(strAddr,"",szTmp);
        vct_docinfo_iter iter = user_vct.begin();
        for(;iter!= user_vct.end();iter++)
        {
          std::string strCode = "curl -s -k -XPUT 'https://";
          strCode += ES_USER_PWD;
          strCode += strAddr;
          strCode += "/clientinfo/user_log_info/";
          strCode += iter->strId;
          strCode += "' -d '";
          strCode += iter->strSource;
          strCode += "\"status\":\"0\"}'";
          curltool tools;
          tools.dealCurlOrder(strCode);
        }
      }
      else if(strcmp(szCodes[1],"-a") == 0)
      {
        vct_docinfo user_vct = getUserOnline(strAddr);
        vct_docinfo_iter iter = user_vct.begin();
        for(;iter!= user_vct.end();iter++)
        {
          std::string strCode = "curl -s -k -XPUT 'https://";
          strCode += ES_USER_PWD;
          strCode += strAddr;
          strCode += "/clientinfo/user_log_info/";
          strCode += iter->strId;
          strCode += "' -d '";
          strCode += iter->strSource;
          strCode += "\"status\":\"0\"}'";
          curltool tools;
          tools.dealCurlOrder(strCode);
        }
      }
      else
      {
        
      }
    }
    else if(strcmp(szCodes[0],"set") == 0)
    {
      if (strcmp(szCodes[1],"-t") == 0)
      {
        kick_time_conf = atoi(szCodes[2]);
      }
    }
  }
}

void* backround(void* param)
{
  std::string strAddr = (const char*)param;
  while(true)
  {
    if( kick_time_conf != 0 )
    {
      int c_time = (int)time(NULL);
      c_time -= kick_time_conf*60;
      char szTmp[24];
      sprintf(szTmp,"%d",c_time);
      CMyLock lock(g_Lock);
      vct_docinfo user_vct = getUserOnline(strAddr,"",szTmp);
      vct_docinfo_iter iter = user_vct.begin();
      for(;iter!= user_vct.end();iter++)
      {
        std::string strCode = "curl -s -k -XPUT 'https://";
        strCode += ES_USER_PWD;
        strCode += strAddr;
        strCode += "/clientinfo/user_log_info/";
        strCode += iter->strId;
        strCode += "' -d '";
        strCode += iter->strSource;
        strCode += "\"status\":\"0\"}'";
        curltool tools;
        tools.dealCurlOrder(strCode);
      }
    }
    sleep(10);
  }
  return (void*)NULL;
}
bool CreateServiceThread(std::string strAddr)
{
	//设置线程状态
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED );
    
  pthread_t tThreadT;
	if (pthread_create(&tThreadT, &attr, backround,(void*)strAddr.c_str() ))
	{
		//写错误日志
		printf("开启策略线程失败\n");
		return false;
	}
  pthread_attr_destroy(&attr);
  return true;
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
  if(!CreateServiceThread(strAddr))
    return 0;
  
  RunServer(strAddr);
 
  return 0;
}
