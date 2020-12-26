#!/bin/bash

bucket='devices'
filepath=$1
path=$2
filename=$3

host=minio.crudus.no
s3_key='tbRRXTRY7Lhc6WCyg0GIFEu2a2oNBItqAN3lqUCjbwE7Rd67Q7glMGq6Tv3nLFTH'
s3_secret='54gNtcfGqs15faBE6AmVrsuAmd1QTGAIhvdC4aXgHINnL9cTxLo934d7F6wsDXdA#'

resource="/${bucket}/${path}/${filename}"
content_type="application/octet-stream"
date=$(date -R)
_signature="PUT\n\n${content_type}\n${date}\n${resource}"
signature=$(echo -en ${_signature} | openssl sha1 -hmac ${s3_secret} -binary | base64)

curl -v -X PUT -T "${filepath}" \
	-H "Host: $host" \
	-H "Date: ${date}" \
	-H "Content-Type: ${content_type}" \
	-H "Authorization: AWS ${s3_key}:${signature}" \
	https://$host${resource}
