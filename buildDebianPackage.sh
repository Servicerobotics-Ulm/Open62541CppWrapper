#!/bin/bash

CURRENT_DIR=${PWD##*/}

# package name and version are parsed from the changelog automatically
PACKAGE_NAME=`dpkg-parsechangelog --show-field Source`
# strip the revision number from the package version (revision is thenumber coming after the '-' character)
PACKAGE_VERSION=`dpkg-parsechangelog --show-field Version | sed 's/-[0-9]//g'`

echo "latest package version: $PACKAGE_VERSION"

echo "clean-up previous build (if available)"
rm -rf debian/build

cd ..
echo "create orig source archive for debain package"
tar -czf ${PACKAGE_NAME}_${PACKAGE_VERSION}.orig.tar.gz ${CURRENT_DIR}

echo "build debain packages"
cd ${CURRENT_DIR}
dpkg-buildpackage -us -uc -tc

cd ..
rm ${PACKAGE_NAME}_${PACKAGE_VERSION}.orig.tar.gz

mkdir ${CURRENT_DIR}/debian/build
mv ${PACKAGE_NAME}_${PACKAGE_VERSION}* ${CURRENT_DIR}/debian/build/
mv ${PACKAGE_NAME}*.deb ${CURRENT_DIR}/debian/build/

cd ${CURRENT_DIR}
echo "Debian package built into the folder: $PWD/debian/build/"
ls debian/build/
