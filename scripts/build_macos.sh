#!/bin/bash -l

set -u #Do not allow unset variables
set -x #Print commands

git submodule update --init --recursive

nvm install 10.15.0
nvm use 10.15.0

source activate py27

npm i --python=/usr/bin/python2.7

npm run build

if [ "$RELEASE" = "true" ]
then
  source activate py27
  npm run release-current-platform
fi
