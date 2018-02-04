#!/bin/sh
set -ux
VOLNAME=${1:?}
shift
RES=$SOURCE_ROOT/res
BUDDY=/usr/libexec/PlistBuddy
if [ ! -x "$BUDDY" ]; then
    BUDDY=$(xcrun -find PlistBuddy) \
        || exit 1
fi
VERSION=$($BUDDY -c 'Print :CFBundleVersion' $RES/$VOLNAME-Info.plist) \
    || exit 1
DMG=$VOLNAME-$VERSION

mkdir -p $TARGET_BUILD_DIR/.background \
    || exit 1
cp -af $RES/$VOLNAME.DS_Store $TARGET_BUILD_DIR/.DS_Store
cp -af $RES/$VOLNAME-bg.png $TARGET_BUILD_DIR/.background/

# copy remaining arguments to image directly
for content
do
    cp -af $content $TARGET_BUILD_DIR/ \
        || exit 1
done

# prepare examples for distribution
for example in $(find $TARGET_BUILD_DIR/Examples -depth 1 -not -name '.*')
do
    rm -rf $example/{build,*.xcodeproj/{*.{mode1v3,pbxuser},project.xcworkspace,xcuserdata},ZBarSDK}
    cp -af $BUILT_PRODUCTS_DIR/ZBarSDK $example/
done

# override subdir .DS_Stores
for dir in $(find $TARGET_BUILD_DIR -type d -depth 1)
do
    cp -af $RES/Columns.DS_Store $dir/.DS_Store
done

hdiutil create -ov -fs HFS+ -format UDZO -imagekey zlib-level=9 \
    -volname $VOLNAME \
    -srcdir $TARGET_BUILD_DIR \
    $BUILT_PRODUCTS_DIR/$DMG.dmg \
    || exit 1
