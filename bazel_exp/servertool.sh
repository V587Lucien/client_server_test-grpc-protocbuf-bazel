#!/bin/sh
db_url=$1
ordercode=$2
username=$3

if [ "$ordercode" = "list" ]
then
  echo "online user is:"
  curl -s -XGET $db_url/clientinfo/user_log_info/_search?pretty -d '{"query":{"match":{"status":"1"}}}' | grep '"user"' | cut -d '"' -f 4
elif [ "$ordercode" = "kickoff" ]
then
  res=`curl -s -XGET $db_url/clientinfo/user_log_info/_search?pretty -d "{\"query\":{\"bool\":{\"must\":[{\"match\":{\"status\":\"1\"}},{\"match\":{\"user\":\"$username\"}}]}}}"`
  userid=${res#*'_id" : "'}
  userid=${userid%%'"'*}
  sourcedata=${res#*'"_source" : '}
  sourcedata=${sourcedata%':'*}
  sourcedata=$sourcedata':"0"}'
  
  for subsource in $sourcedata
  do
    finalsource=$finalsource$subsource
  done
  curl -s -XPUT $db_url/clientinfo/user_log_info/$userid -d "$finalsource"
  echo "kick user $username off !"
fi
exit
