#!/bin/sh

set -ex
echo $PWD
rm -r dist
mkdir dist
cp -RL $HOME/project/NanoVNA-Web-Client/www/* dist/
mv dist/lib/material-design-icons-iconfont dist/lib/mdii
sed -i "" s/material-design-icons-iconfont/mdii/ dist/index.html
sed -i "" 's!<link rel="stylesheet" href="https://fonts.googleapis.com/css?family=Roboto:400,500,700,400italic">.*!!' dist/index.html
sed -i "" 's!<script src="cordova.js" defer></script>!!' dist/index.html
find dist -type f | xargs gzip
find dist -print
du -h dist
