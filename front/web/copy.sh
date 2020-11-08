#!/bin/sh

set -ex

cd `git rev-parse --show-toplevel`
cd front/web
echo $PWD

if [ ! -e NanoVNA-Web-Client ]; then
	git clone https://github.com/cho45/NanoVNA-Web-Client.git
	cd NanoVNA-Web-Client
	git checkout websocket
	cd ..
fi

cd NanoVNA-Web-Client
git pull
./copy-minimum.sh
cd ..

rm -rf dist
mkdir dist
cp -pRL NanoVNA-Web-Client/www/* dist/
mv dist/lib/material-design-icons-iconfont dist/lib/mdii
sed -i "" s/material-design-icons-iconfont/mdii/ dist/index.html
sed -i "" 's!<link rel="stylesheet" href="https://fonts.googleapis.com/css?family=Roboto:400,500,700,400italic">.*!!' dist/index.html
sed -i "" 's!<script src="cordova.js" defer></script>!!' dist/index.html
find dist -type f | xargs gzip -n
find dist -print
du -h dist

