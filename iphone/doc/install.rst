Installing the SDK
==================

These are the basic instructions for obtaining the SDK and adding it to an
Xcode project.

You may want to try things out with the :doc:`tutorial` before hacking at your
own project.


Requirements
------------

You will need *all* of the following to develop iPhone applications
using this SDK:

* Mac OS X >= 10.6.x (Snow Leopard)
* Xcode >= 4.5.1
* iPhone SDK >= 4.0
* An iPhone 3GS, iPhone 4 or newer iOS device with an auto-focus camera
* iOS >= 4.0 running on the device

.. warning::

   *Only* the iPhone 3GS, iPhone 4 and newer models are supported, as they
   have a camera with auto-focus.  The iPad 2 and iPad 3 will also work, *iff*
   the barcode is printed large enough to achieve good focus.  The ZBar
   library does not support the iPhone 3G and is unlikely to ever support it.


Downloading
-----------

Download the latest binary release of the ZBar SDK from

http://zbar.sourceforge.net/iphone


Integration
-----------

The recommended installation method is to simply copy the SDK into your
Xcode project:

1. Open ZBarSDK-|version|.dmg in the Finder.

2. Drag the :file:`ZBarSDK` folder into your Xcode project.  In the dialog
   that appears, you should choose to **copy** the SDK into your project by
   checking the box.  The target that you want to link with the library should
   also be selected in the target list.

3. Link the following additional frameworks to any targets that link with the
   ZBarSDK.  You should set the first three to use weak references and
   configure an appropriate deployment target if you still need to support
   iOS 3:

   * :file:`AVFoundation.framework` (weak)
   * :file:`CoreMedia.framework` (weak)
   * :file:`CoreVideo.framework` (weak)
   * :file:`QuartzCore.framework`
   * :file:`libiconv.dylib`

   If you check "Link Binary With Libraries" for the target(s), you should see
   all of these frameworks followed by :file:`libzbar.a`.

   .. note::

      Link order may be important for some versions of Xcode; the referenced
      libraries should be listed *before* :file:`libzbar.a` in the link order.

4. Import the SDK header from your prefix header to make the barcode reader
   APIs available::

      #import "ZBarSDK.h"

Proceed to :doc:`camera` or :doc:`picker` to learn about using the reader APIs
to scan barcodes.  Use the :doc:`apiref` for specific interface details.


Upgrading from an Older Version
-------------------------------

If you are using an older version of the *SDK* (NB, skip to the next section
if you are currently using Mercurial), upgrading is straightforward:

1. Delete the current ZBarSDK group from your project.  You should choose
   to delete the files if you copied them into your project.
2. Drag the new ZBarSDK from the DMG into your project.

Clean out and rebuild your project with the new version.


Upgrading a Pre-SDK Integration
-------------------------------

If your project was using the library directly from the Mercurial repository,
before the SDK was introduced, there are a few incompatibilities that you must
resolve in order to upgrade.  Don't worry - all of your source stays the same,
you just need to update how the library is included in the project and how the
headers are imported.

Switching to the Binary Distribution
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This approach is recommended - the binary releases provide you with a stable
development platform, isolating you from temporary instability and transient
problems that may occur at the bleeding edge.

The first task is to reverse the previous ZBar integration:

1. Remove the reference to zbar.xcodeproj from your project.
2. Remove any :file:`zbar-*` files from your Resources.
3. In the target build settings, remove any "Header Search Paths" that
   reference zbar.
4. Remove any references to zbar headers in your :file:`prefix.pch` or source
   files.

Now just continue with the `integration`_ instructions above and you should be
back up and running!

Continuing with Mercurial
^^^^^^^^^^^^^^^^^^^^^^^^^

Alternatively, you may still prefer to select Mercurial revisions.  You have a
few choices for this:

* You may build your own ZBarSDK and copy/link it into your project.  This is
  the same as `Switching to the Binary Distribution`_, except that you use
  your own version of the SDK.  In this case you need to manually rebuild the
  SDK when you update it.
* You may leave zbar.xcodeproj as a project dependency and pull the SDK into
  your project.  This is not well tested, so ymmv.
* You may leave zbar.xcodeproj as a project dependency and just link libzbar.a
  into your project, as before.  You will need to update the target dependency
  (the library target changed names to libzbar) and add the
  :file:`iphone/include/ZBarSDK` directory to "Header Search Paths"

In any case, you should remove the references to the zbar headers from
:file:`prefix.pch` (or your source files) and replace them with::

   #import "ZBarSDK.h"
