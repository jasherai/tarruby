#!/bin/sh
VERSION=0.1.2

rm *.gem *.tar.bz2
rm -rf doc
rdoc README.txt ext/tarruby.c --title 'TAR/Ruby - Ruby bindings for libtar.'
tar jcvf tarruby-${VERSION}.tar.bz2 --exclude=.svn README.txt *.gemspec ext doc
gem build tarruby.gemspec
gem build tarruby-mswin32.gemspec
cp tarruby-${VERSION}-x86-mswin32.gem tarruby-${VERSION}-mswin32.gem
