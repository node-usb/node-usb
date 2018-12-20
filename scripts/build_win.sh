#!/bin/bash -l

set -u #Do not allow unset variables

git submodule update --init --recursive

nvm install 10.15.0
nvm use 10.15.0

source activate py27

npm i
npm run build

if [ "$RELEASE" = "true" ]
then
  npm run release-current-platform
fi
