#!/bin/bash -l

git submodule update --init --recursive

export NODEJS_ORG_MIRROR=http://nodejs.org/dist

nvm install v10.15.0
nvm use v10.15.0

node --version

npm i
npm run build

if [ "$RELEASE" = "true" ]
then
  npm run release-current-platform
fi
