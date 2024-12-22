#!/bin/sh
###### -copyright-
#-endcopyright-
# create C code to get ui.xml in a string
# ISO C stipulates that the length of a string constant should
# not be larger than 4096, so we create a definition as in
# char xsnow_xml[] = {60,63,120,109,108,32,118,101,0};
#
root="${1:-..}"
in="ui.glade"
out="ui_xml.h"
echo "/* This file is generated from '$in' by '$0' */" > "$out"
echo "/* -copyright-" >> "$out"
#-endcopyright-
echo "*/" >> "$out"
echo "#pragma once" >> "$out"
echo "char xsnow_xml[] = {" >> "$out"
sed 's/^ *//' "$root/src/$in" | awk -v FS="" \
   'BEGIN{for(n=0;n<256;n++)ord[sprintf("%c",n)]=n;}
   {for (i=1;i<=NF;i++) printf "%d,", ord[$i];
      printf "%d,\n",ord["\n"];}' >> "$out"
rc1=$?
echo "0};">> "$out" 
if [ -x "$root/addcopyright.sh" ] ; then "$root/addcopyright.sh" "$out" ; fi
rc2=$?
if [ "$rc1" -eq 0 ]; then
   exit "$rc2"
fi
exit $rc1
