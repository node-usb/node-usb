#!/bin/bash -l

set -u #Do not allow unset variables
set -x #Print commands
set -e #Exit on error

git submodule update --init --recursive

nvm install 11.9.0
nvm use 11.9.0

source activate py27

yarn global add windows-build-tools

yarn install
ARCH=x64 yarn run build
ARCH=ia32 yarn run build

if [ "$RELEASE" = "true" ]
then
  yarn run release-current-platform
fi
