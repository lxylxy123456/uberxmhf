#!/bin/bash
set -xe
mkdir tmp
cd tmp
git init
git config user.email "you@example.com"
git config user.name "Your Name"
cat > .gitignore <<< asdf
git add -A
git commit -m .
mkdir a
touch a/asdf
git clean -Xdf
cd ..
rm -rf tmp

