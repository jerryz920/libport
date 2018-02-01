
#60 
curl http://10.10.1.39:7777/postAttesterImage -XPOST -d '{"principal":"152.3.145.38:444","otherValues":["vmimg","*"]}'

#61 
curl http://10.10.1.39:7777/postAttesterImage -XPOST -d '{"principal":"152.3.145.38:444","otherValues":["containerimg","*"]}'

#62 
curl http://10.10.1.39:7777/postImageProperty -XPOST -d '{"principal":"152.3.145.38:444","otherValues":["vmimg","*","git://docker"]}'

#63 
curl http://10.10.1.39:7777/postImageProperty -XPOST -d '{"principal":"152.3.145.38:444","otherValues":["containerimg","*","git://spark"]}'

#64 
curl http://10.10.1.39:7777/postImageProperty -XPOST -d '{"principal":"152.3.145.38:444","otherValues":["sparkimg","*","git://pio"]}'

#74 
curl http://10.10.1.39:7777/postObjectAcl -XPOST -d '{"principal":"alice","otherValues":["alice:object_pio","git://pio"]}'



#56 
curl http://10.10.1.39:7777/postInstanceSet -XPOST -d '{"principal":"152.3.145.38:444","otherValues":["1:12","vmimg","image","192.1.1.3:1-65535","*"]}' | tee key
inst_id=`python id.py <key`
curl http://10.10.1.39:7777/updateSubjectSet -XPOST -d '{"principal":"192.1.1.3:1-65535","otherValues":["'$inst_id'"]}'

#65 
curl http://10.10.1.39:7777/postInstanceSet -XPOST -d '{"principal":"192.1.1.3:1-65535","otherValues":["2:14","containerimg","image","192.1.1.3:10000-20000","*"]}' | tee key
inst_id=`python id.py <key`
curl http://10.10.1.39:7777/updateSubjectSet -XPOST -d '{"principal":"192.1.1.3:10000-20000","otherValues":["'$inst_id'"]}'

#69 
curl http://10.10.1.39:7777/postInstanceSet -XPOST -d '{"principal":"192.1.2.3:10000-20000","otherValues":["30:16","sparkimg","image","192.1.2.3:10001-10010","*"]}' | tee key
inst_id=`python id.py <key`
curl http://10.10.1.39:7777/updateSubjectSet -XPOST -d '{"principal":"192.1.2.3:10001-10010","otherValues":["'$inst_id'"]}'

#58 
curl http://10.10.1.39:7777/postInstanceSet -XPOST -d '{"principal":"152.3.145.38:444","otherValues":["10:13","vmimg","image","192.1.2.3:1-65535","*"]}' | tee key
inst_id=`python id.py <key`
curl http://10.10.1.39:7777/updateSubjectSet -XPOST -d '{"principal":"192.1.2.3:1-65535","otherValues":["'$inst_id'"]}'

curl http://10.10.1.39:7777/postInstanceSet -XPOST -d '{"principal":"192.1.2.3:1-65535","otherValues":["20:15","containerimg","image","192.1.2.3:10000-20000","*"]}' | tee key
inst_id=`python id.py <key`
curl http://10.10.1.39:7777/updateSubjectSet -XPOST -d '{"principal":"192.1.2.3:10000-20000","otherValues":["'$inst_id'"]}'

curl http://10.10.1.39:7777/postInstanceSet -XPOST -d '{"principal":"192.1.1.3:10000-20000","otherValues":["3:17","sparkimg","image","192.1.1.3:10001-10010","*"]}' | tee key
inst_id=`python id.py <key`
curl http://10.10.1.39:7777/updateSubjectSet -XPOST -d '{"principal":"192.1.1.3:10001-10010","otherValues":["'$inst_id'"]}'

#73 
curl http://10.10.1.39:7777/postWorkerSet -XPOST -d '{"principal":"192.1.1.3:10001-10010","otherValues":["clustersp","192.1.2.3:10005","*"]}' |  tee key
inst_id=`python id.py <key`
curl http://10.10.1.39:7777/updateSubjectSet -XPOST -d '{"principal":"192.1.2.3:10001-10010","otherValues":["'$inst_id'"]}' | tee key
inst_id=`python id.py <key`

#75 
curl http://10.10.1.39:7777/workerAccessesObject -XPOST -d '{"principal":"152.3.145.138:4144", "bearerRef": "'$inst_id'", "otherValues":["192.1.2.3:10005","alice:object_pio"]}'

