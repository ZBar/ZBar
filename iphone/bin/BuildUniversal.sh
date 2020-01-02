#!/bin/sh
set -ux
SUBTARGET=${1:?}
OUTDIR=${2:-$TARGET_BUILD_DIR}

# build library for device and simulator
xcodebuild -target $SUBTARGET -configuration $CONFIGURATION -sdk iphoneos BUILD_DIR=$BUILD_DIR BUILD_ROOT=$BUILD_ROOT\
    || exit 1
xcodebuild -target $SUBTARGET -configuration $CONFIGURATION -sdk iphonesimulator BUILD_DIR=$BUILD_DIR BUILD_ROOT=$BUILD_ROOT\
    || exit 1

mkdir -p $OUTDIR

# combine device and simulator libs into single fat lib.
# others have indicated that this approach is "wrong", but for us
# the ease of including the universal lib in a project without complicated
# changes to build settings outweighs any lack of purity in the approach
# ...we can always fix things later, if necessary
lipo -create \
    $BUILD_ROOT/$CONFIGURATION-iphoneos/$SUBTARGET.a \
    $BUILD_ROOT/$CONFIGURATION-iphonesimulator/$SUBTARGET.a \
    -output $OUTDIR/$SUBTARGET.a \
    || exit 1
