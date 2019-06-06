#!/bin/bash

version="1.0.01"
context="-dev"
channel="weekly"
path=`pwd`
distributions=(disco cosmic bionic)

for i in "${!distributions[@]}"
do
	echo "Packaging for ${distributions[$i]}..."

	if [ $i -eq 0 ]
	then
		dch -v "${version}${context}-1~${distributions[$i]}~ppa1" "New upstream release" && dch -r --distribution ${distributions[$i]} ""
		debuild -S -sa
	else
		cd ${path}
		sed -i.bak 1,2s/${distributions[$i - 1]}/${distributions[$i]}/g debian/changelog
		debuild -S -sd
	fi

	cd ../
	dput ppa:otter-browser/${channel} *.changes
	rm *.build
	rm *.changes
	rm *.dsc
	rm *ppa1.tar.gz
done
