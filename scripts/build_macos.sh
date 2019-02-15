#!/bin/bash -l

set -u #Do not allow unset variables
set -x #Print commands
set -e #Exit on error

git submodule update --init --recursive

nvm install 11.9.0
nvm use 11.9.0

source activate py27

yarn install --python=/usr/bin/python2.7

yarn run build

if [ "$RELEASE" = "true" ]
then
  source activate py27
  yarn run release-current-platform
fi
