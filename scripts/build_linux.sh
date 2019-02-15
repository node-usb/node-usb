#!/bin/bash -l

git submodule update --init --recursive

export NODEJS_ORG_MIRROR=http://nodejs.org/dist

nvm install v11.9.0
nvm use v11.9.0

node --version

yarn install
yarn run build

if [ "$RELEASE" = "true" ]
then
  yarn run release-current-platform
fi
