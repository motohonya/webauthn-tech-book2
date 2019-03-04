#!/bin/bash

set -eux

rm -rf node_modules
# --unsafe-perm はrootでの実行時(= docker環境)で必要 非root時の挙動に影響なし
# Windows x docker-machine 環境下でシンボリックリンクが作れないため --no-bin-links オプションを追加
npm install --unsafe-perm --no-bin-links
git submodule init && git submodule update
