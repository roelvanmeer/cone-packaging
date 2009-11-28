#!/bin/sh -e

# called by uscan with '--upstream-version' <version> <file>
TAR=../$(basename $3 .bz2)

bunzip2 $3
gzip -9 $TAR

# move to directory 'tarballs'
if [ -r .svn/deb-layout ]; then
  . .svn/deb-layout
  mv $TAR.gz $origDir
  echo "moved $TAR.gz to $origDir"
fi
