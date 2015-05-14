#!/bin/bash
egrep -o "\`[^\`]*\`" ../java/assets/webkey/*.html ../java/assets/webkey/js/webkey.js | cut -f 2- -d ':' | tr -d '\`' | sed -r 's/$/ -> /g' > language_base2.txt
egrep -o "lang\([^,]*,\".*\"\)" *.cpp *.h | sed -r 's/[^,]*,"(.*)"\)$/\1 -> /g' >> language_base2.txt
rm language_base.txt
touch language_base.txt
cat language_base2.txt | while read; do
	a=$REPLY
	if [[ $a != "BEFOREDATE" ]]; then
		if ! egrep "^$a" language_base.txt &> /dev/null; then
			echo "$a" >> language_base.txt
		fi;
	fi;
done

rm language_base2.txt

rm -rf /tmp/lang
mkdir /tmp/lang
find ../java/assets/webkey/language_??.txt | while read; do
	f=${REPLY}
	cat language_base.txt | while read; do s="${REPLY}"; if ! grep "^$a" ${f} &>/dev/null && ! grep "^\#$a" ${f} &> /dev/null; then echo "#"$REPLY; fi; done >> /tmp/lang/${f##*/}
	cat ${f} >> /tmp/lang/${f##*/}
done

mv language_base.txt ../java/assets/webkey/

find ../java/res/values-??/strings.xml | while read; do
	f=${REPLY}
	d=${f%/strings.xml}
	d=${d##*/}
	mkdir /tmp/lang/${d}
	cp ${f} /tmp/lang/${d}/strings.xml
	grep -o 'name="[^"]*"' ../java/res/values/strings.xml | while read; do l="${REPLY}"; if ! grep "${l}" ${f} &> /dev/null; then grep "${l}" ../java/res/values/strings.xml >> /tmp/lang/${d}/strings.xml; fi; done
done

#echo "To add:"
#cat language_base.txt | while read; do if ! grep "^$REPLY" ../java/assets/webkey/language_hu.txt &> /dev/null ; then echo $REPLY; fi; done
#echo
#echo "To remove:"
#cat ../java/assets/webkey/language_hu.txt | while read; do if ! grep "^${REPLY% ->*}" language_base.txt &> /dev/null ; then echo $REPLY; fi; done
#echo

