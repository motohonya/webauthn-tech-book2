#!/bin/bash

rm -rf /book_tmp/
mkdir /book_tmp/
cp -aR /book/. /book_tmp/

cd /book_tmp/ && npm install && npm run pdf
d=`date "+%H%M%S"`
cp /book_tmp/articles/WebAuthn-Tech-Book2.pdf /book/WebAuthn-$d.pdf