#!/bin/bash

while IFS= read -r line
do
case $line in
  /*)
	# turn into a windows path
	CYGPATH=`echo $line | sed -r -e "s/([^:]+)(.+)/\1/"`
	WINPATH=`cygpath -aw $CYGPATH`
	WINPATH=`echo $WINPATH | awk -F"\\\\" '{$1=$1}1' OFS="\\\\\\\/"`
	FINAL=`echo $line | sed -r -e "s/(.+):(.[0-9]+):/"$WINPATH"(\2) : /"`
	echo $FINAL
	;;
  *)
	echo $line
	;;
esac
done
