#!/bin/bash

NDK_DIR=/home/petya/download/android-ndk-r6

$NDK_DIR/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-gcc -Wno-write-strings -mthumb -Wl,-rpath-link=$NDK_DIR/platforms/android-3/arch-arm/usr/lib/,-dynamic-linker=/system/bin/linker -I $NDK_DIR/platforms/android-3/arch-arm/usr/include/ -fno-short-enums -L $NDK_DIR/platforms/android-3/arch-arm/usr/lib/ -lc -c sqlite/sqlite3.c -O2 \
	&& $NDK_DIR/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-ar q sqlite3.a sqlite3.o
