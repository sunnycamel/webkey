#!/bin/bash

NDK_DIR=/home/petya/download/android-ndk-r6b
APK_DIR=/home/petya/work/Webkey/webkey

$NDK_DIR/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-g++ -Wno-write-strings -mthumb -DANDROID -DNOCRYPT -DUSE_FILE32API -Wl,-rpath-link=$NDK_DIR/platforms/android-3/arch-arm/usr/lib/,-dynamic-linker=/system/bin/linker -I $NDK_DIR/platforms/android-3/arch-arm/usr/include/ -I $NDK_DIR/sources/cxx-stl/stlport/stlport/ -fno-short-enums -L $NDK_DIR/platforms/android-3/arch-arm/usr/lib/ -lc -llog minizip/zip.c minizip/ioapi.c shellinabox/service.c shellinabox/launcher.c shellinabox/logging.c shellinabox/hashmap.c webkey.c suinput.c mongoose.c base64.c -fno-exceptions -fno-rtti $NDK_DIR/tmp/ndk-digit/build/install/sources/cxx-stl/stlport/libs/armeabi/libstlport_static.a libpng12.a libjpeg.a sqlite3.a  -lz -o webkey -O2 \
       	&& $NDK_DIR/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-strip webkey \
       	&& echo OK \
	&& cp -r -f css keycodes.txt language_??.txt sqlite3 openssl ssleay.cnf ShellInABox.js js beep.wav styles.css enabled.gif reganim.gif *.html favicon.ico fast_keys.txt *.png spec_keys.txt notify.txt ae_* client plugins $APK_DIR/assets/webkey/ \
	&& cp webkey $APK_DIR/assets/bin/ \
	&& cd $APK_DIR \
	&& rm -rf bin gen \
	&& ant release \
	&& mv bin/webkey-release.apk ../webkey.apk \
	&& cd -
