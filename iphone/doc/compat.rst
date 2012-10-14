Backward Compatibility
======================

Generally speaking, we take great care to ensure that each release of the
library is backward compatible with previous versions - upgrading the library
should not require any changes to your code and will continue to provide
equivalent functionality.  The notable exception to this is the iOS 4 upgrade
and associated "deprecation" of the former automatic capture method by our
vendor.


.. warning::

   Versions before iOS 4 are no longer supported by the library.  We are no
   longer able to test anything in this section, so you're on your own if you
   try to make use of it.


The Private API
---------------

The API that we use for automatic capture with iOS 3.x (namely
:func:`UIGetScreenImage`) has an interesting history.  It has changed status
several times, starting with "Private, unless we like you" moving to
"reluctantly Public but undocumeted" by popular demand and reverting to
"strictly Private" as of iOS 4.  The current story: if you want to distribute
on the App Store, you had better not be using it - IOW, no automatic capture
for you with iOS 3.x.

Since App Store distribution is the most common use for the library, the
default configuration, and thus the binary SDK, does *not* use any private
APIs.

Users targeting ad-hoc or enterprise distribution may not care about the
status of the API and may prefer to continue supporting automatic capture for
iOS 3.x.  To do this you will need to rebuild the library with the following
define set for all configurations:

.. sourcecode:: sh

   USE_PRIVATE_APIS=1

For reference, you can check whether your app refers to the offensive function
with this command:

.. sourcecode:: sh

   $ otool -vI MyApp.app/MyApp | grep UIGetScreenImage

If there is any output, then the executable includes the private API and is
bound to be rejected if submitted for review.  Otherwise it is "clean" as far
as this library is concerned.


Upgrading to iOS 4
------------------

If you were using the reader before iOS 4 was introduced, you will want to
upgrade to the new reader controller.  The performance has improved quite a
bit, and you can continue to support automatic capture on the App Store.

.. note::

   This discussion only applies to automatic capture from the camera.  If you
   are only scanning image files, or prefer/need to use manual capture, you
   should not change anything.

Basically just replace your old :class:`ZBarReaderController` with a new
:class:`ZBarReaderViewController` and you're done!  See the reference and the
next section for compatibility between the two classes.

Also see the :doc:`install` instructions for details about upgrading the
header references to use the SDK.


Supporting iOS 3.x
------------------

The new :class:`ZBarReaderViewController` is intentionally designed to be
compatible with the old :class:`ZBarReaderController` in most aspects that
relate to reading barcodes.  When a :class:`ZBarReaderViewController` is
initialized under iOS 3.x, it will *replace* itself with a
:class:`ZBarReaderController`.  You can leverage the compatibility of these
controllers to continue supporting iOS 3.x.

The following properties and methods should be equivalent across
implementations.  You may use them without regard for the actual instance
type.

========================================================  ====
Equivalent Members
========================================================  ====
:member:`~ZBarReaderViewController::cameraOverlayView`
:member:`~ZBarReaderViewController::cameraViewTransform`
:member:`~ZBarReaderViewController::enableCache`
:member:`~ZBarReaderViewController::scanner`
:member:`~ZBarReaderViewController::readerDelegate`
:member:`~ZBarReaderViewController::scanCrop`
``showHelpWithReason:``
:member:`~ZBarReaderViewController::showsZBarControls`
:member:`~ZBarReaderViewController::tracksSymbols`
========================================================  ====

Some properties are available with :class:`ZBarReaderViewController` only for
backward compatibility.  If these are configured, they must be set as
indicated; attempts to set another value will raise an exception.

====================================================  =======================================
:class:`ZBarReaderController` Property                :class:`ZBarReaderViewController` Value
====================================================  =======================================
:member:`~ZBarReaderController::allowsEditing`        ``NO``
:member:`~ZBarReaderController::cameraMode`           ``Sampling``
:member:`~ZBarReaderController::maxScanDimension`     (ignored)
:member:`~ZBarReaderController::showsCameraControls`  ``NO``
:member:`~ZBarReaderController::showsHelpOnFail`      (ignored)
:member:`~ZBarReaderController::sourceType`           ``Camera``
:member:`~ZBarReaderController::takesPicture`         ``NO``
====================================================  =======================================

Also, the ``isSourceTypeAvailable:`` class method of
:class:`ZBarReaderViewController` will return ``YES`` only for the ``Camera``
source.

All other members of :class:`ZBarReaderController`, including those inherited
from :class:`UIImagePickerController` are not supported by
:class:`ZBarReaderViewController`.  This includes ``takePicture`` and
``scanImage:``, among others.

Remaining members of :class:`ZBarReaderViewController`: are only available
with the new implementation.  At the moment this is only
:member:`~ZBarReaderViewController::readerView`, but any new properties or
methods not listed here will also fall in this category.

To access settings that may not be available in a potential fallback
environment, you must verify that they exist and may be set as desired - eg,
by testing the specific reader subtype.

Weak Linking
^^^^^^^^^^^^

When leveraging fallbacks to iOS 3.x, it is important that features introduced
in iOS 4 are referenced using *weak* links.  You must configure your project
correctly to support this:

* Make sure the iOS 4 frameworks are set to *Weak*.  Specifically, these are
  AVCapture, CoreMedia and CoreVideo.

* Build with the latest SDK - do *not* use the "Base SDK" setting to target
  earlier devices.

* Set the correct iOS 3.x version for the "iPhone OS Deployment Target"
  build setting.


Example: Fallback to Manual Capture
-----------------------------------

This code example will configure the reader for automatic capture from the
camera for iOS 4 and fall back to manual or automatic capture for iOS 3.x,
depending on whether the library was compiled to use private APIs::

   if(![ZBarReaderController isSourceTypeAvailable:
                                 UIImagePickerControllerSourceTypeCamera]) {
       // camera unavailable: display warning and abort
       // or resort to keypad entry, etc...
       return;
   }
   
   ZBarReaderViewController *reader = [ZBarReaderViewController new];
   // reader will be a ZBarReaderController for iOS 3.x
   // or a ZBarReaderViewController for iOS 4
   
   reader.readerDelegate = self;
   reader.sourceType = UIImagePickerControllerSourceTypeCamera;
   reader.showsZBarControls = YES;
   
   if(reader.cameraMode == ZBarReaderControllerCameraModeSampling) {
       // additional automatic capture configuration here
   }
   else {
       // additional manual capture configuration here
   }
   
   [self presentModalViewController: reader
         animated: YES];

If you are using a custom control set
(:member:`~ZBarReaderViewController::showsZBarControls`\ ``=NO``), you will
want to provide a button attached to ``takePicture`` for the manual capture
case.  The built-in controls do this automatically.
