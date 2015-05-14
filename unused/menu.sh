#!/bin/bash
menu=`grep MENU mongoose.h |  sed -r 's/.*MENU \\"//g' | head -c -3`
rm -rf html
mkdir html
#echo $menu
VER=`grep android:versionName= /home/petya/work/Webkey/webkey/AndroidManifest.xml | grep -o \".*\" | cut -d "\"" -f 2`
for i in *.html; do echo ${i}; cat ${i} | sed -r "s^MENU^${menu}^" | sed -r "s^WEBVERSION^${VER}^" > html/${i}; done
