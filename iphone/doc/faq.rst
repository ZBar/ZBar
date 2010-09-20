Frequently Asked Questions (FAQ)
================================

This is the ever-growing list of answers to commonly asked questions.  Please
feel free to post you question in our `iPhone Developers forum`_ if you do not
find the information you need in this documentation.

.. _`iPhone Developers Forum`:
   http://sourceforge.net/projects/zbar/forums/forum/1072195


General
-------

This looks great...  Where can I get it?
   You can download the latest version of the SDK from
   http://zbar.sf.net/iphone


Compatibility
-------------

Which iPhone devices does this library support?
   The library works *only* with the iPhone 3GS and iPhone 4.

Will you make it work with the iPhone 3G?
   *No* - the 3G it is not supported and is unlikely to ever be supported.

   To be fair, you *can* use the 3G to scan image files, as long as they're in
   focus (ie, *not* images taken by the built-in fixed-focus camera).  There
   is at least one application that found a use for this...

What target iOS versions does this library work with?
   iOS 4 is fully supported, including the latest video streaming interfaces.
   Since Apple has dropped support for earlier versions of iOS on the App
   Store, we recommend that you target only iOS 4 for reading barcodes.

   iOS 3.1 is also supported, but you will have to resort to manual capture if
   you intend to distribute on the App Store.

   The library should work all the way back to iOS 3.0, although we no longer
   test with it.  For most cases we recommend you use at least iOS 3.1, which
   introduced several must-have enhancements for the UIImagePickerController
   (ie, :member:`~ZBarReaderController::cameraOverlayView` and ``takePicture``)

   In all cases you will need at least a 4.0 SDK to build.

   See :doc:`compat` for details about iOS version fallbacks.

Are any private APIs in use?
   No - the binary release of the SDK does not use any private APIs.

   OTOH, if you do not care about private APIs (for instance, you are
   targeting AdHoc or enterprise distribution) and you want to enable
   automatic capture with iOS 3.1, you can build a version that uses
   UIGetScreenImage (which is again a private API).

   See :doc:`compat` for details.

Does this support "automatic" barcode capture?
   Yes - if you use iOS 4, the default configuration will capture barcodes
   automatically from the video stream.

   Again, automatic capture is no longer supported with iOS 3.1 for apps
   distributed on the App Store.


Building
--------

I get "Undefined symbols" errors when I try to build?
   Most likely you did not add all of the necessary framework dependencies.
   See :doc:`tutorial` or :doc:`install` for the list of frameworks you need
   to link against.


Licensing
---------

Please refer to :doc:`licensing` for questions about licensing.


Barcodes
--------

Why do my UPC barcodes have an extra 0 at the front?
   The UPC-A_ symbology is the subset of EAN-13_ that starts with a leading 0.
   The ZBar decoder enables only EAN-13_ by default, so GTIN-13_ product codes
   are consistently reported.  You can choose to receive the 12-digit results
   instead by explicitly enabling UPC-A_.

   The :member:`~ZBarSymbol::type` property of the symbol can be used to see
   which type of barcode is reported.

   See EAN-13_ and UPC-A_ for more information.

Why does my UPC-E (short version) barcode data look completely wrong?
   UPC-E_ is a "zero compressed" version of UPC-A_; certain of the zeros are
   removed from the UPC-A_ data to generate the UPC-E_ barcode.  The ZBar
   decoder *expands* this compression by default, again to consistently report
   GTIN-13_ product codes.  You can choose to receive the compressed 8-digit
   results instead by explicitly enabling UPC-E_.

   The :member:`~ZBarSymbol::type` property of the symbol can be used to see
   which type of barcode is reported.

   See UPC-E_ for more information.

.. _GTIN-13:
.. _GTIN: http://wikipedia.org/wiki/GTIN
.. _EAN-13: http://wikipedia.org/wiki/EAN-13
.. _UPC-A: http://wikipedia.org/wiki/UPC-A
.. _UPC-E: http://wikipedia.org/wiki/UPC-E#Zero-compressed_UPC-E
