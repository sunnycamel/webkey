#!/bin/bash
cd ../java/assets/webkey/; zip /tmp/language.zip language*; cd ../../res; zip -r /tmp/language.zip values*  -x \*.svn\* -x \*userpermissions.xml
scp /tmp/language.zip petya@hostv:language/
