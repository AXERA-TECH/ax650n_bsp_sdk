#!/usr/bin/env bash

set -e

PWD=`pwd`

PROJECT=$1
VERSION=$2
VERSION_EXT=${VERSION}_$(date "+%Y%m%d%H%M%S")
OTA_PKG=${PROJECT}_${VERSION_EXT}.swu

echo "PROJECT=${PROJECT}"
OUT_PATH=${PWD}/out/${PROJECT}/images

cp -rf ${PWD}/../tools/mkswu/swugenerator.sh ${OUT_PATH}

cd ${OUT_PATH}
sh swugenerator.sh ${OTA_PKG}
mv ${OTA_PKG} ../../
rm -rf swugenerator.sh
cd -

