ZBarReaderController Class Reference
====================================

.. class:: ZBarReaderController

   :Inherits from: :class:`UIImagePickerController`

   This is the controller to use for scanning images selected by a
   :class:`UIImagePickerController` either captured manually using the camera,
   or selected from the Photo Library.  For more information, see
   :doc:`picker`.

   It can support automatic capture from the camera only if the library is
   re-built to use private APIs (see :doc:`compat`).


Properties
----------

   .. member:: ZBarImageScanner *scanner

      Access to the image scanner for configuration. (read-only)

   .. member:: id<ZBarReaderDelegate> readerDelegate

      The delegate that will be notified when new barcode results are
      available.

   .. member:: BOOL showsZBarControls

      Whether to display a default control set consisting of cancel, scan and
      info buttons.  Disable these if you provide your own controls using the
      :member:`cameraOverlayView`.  Enabling this automatically disables the
      system controls :member:`showsCameraControls`.  (Default ``YES``).

   .. member:: BOOL showsHelpOnFail

      Whether to automatically display the integrated help viewer when an
      image fails to decode.  Even if this is disabled, the integrated help
      may still be presented manually using ``showHelpWithReason:``.
      (Default ``YES``)

   .. member:: ZBarReaderControllerCameraMode cameraMode

      Scanning mode to use with the camera.  It is generally appropriate to
      leave this at the default.

   .. member:: BOOL tracksSymbols

      Whether to display the tracking rectangle around detected barcodes.

   .. member:: BOOL takesPicture

      Whether to take a full picture (with ``takePicture``) when a barcode
      is detected with ``ZBarReaderControllerCameraModeSampling``.  The
      resulting image will be delayed from the actual decode.

   .. member:: BOOL enableCache

      This property is deprecated and should not be modified.

   .. member:: CGRect scanCrop

      Crop images before scanning.  The original image will be cropped to this
      rectangle, which should be in normalized image coordinates, x-axis
      major.  Defaults to the full image ``{{0, 0}, {1, 1}}``.

   .. member:: NSInteger maxScanDimension

      Scale image to scan.  After cropping, the image will be scaled if
      necessary, such that neither of its dimensions exceed this value.
      Defaults to 640.

   .. note::

      The remaining properties are inherited from
      :class:`UIImagePickerController`.

   .. member:: UIImagePickerControllerSourceType sourceType

      Image source.  Use to select between the camera and photo library.

   .. member:: BOOL showsCameraControls

      Whether to display the system camera controls.  Overridden to ``NO``
      when :member:`showsZBarControls` is ``YES``.

   .. member:: UIView *cameraOverlayView

      A custom view to display over the camera preview.  The tracking layer
      and default controls will be added to this view if they are enabled.

   .. member:: CGAffineTransform cameraViewTransform

      A transform to apply to the camera preview.  Ignored by the reader.
      Possibly useful for eg, a digital zoom effect.

   .. member:: BOOL allowsEditing

      Whether to enable the system image editing dialog after a picture is
      taken.  Possibly useful to improve reader results in some cases using
      manual intervention.


Instance Methods
----------------

   .. _`showHelpWithReason:`:
   .. describe:: - (void) showHelpWithReason:(NSString*)reason

      Display the integrated help browser.  Use this with custom overlays if
      you don't also want to create your own help view.  Should only be called
      when the reader is displayed.  The ``reason`` argument will be passed to
      the :func:`onZBarHelp` javascript function.

      :reason: A string parameter passed to javascript.

   .. _`scanImage:`:
   .. describe:: - (id <NSFastEnumeration>) scanImage:(CGImageRef)image

      Scan an image for barcodes.  This is a wrapper around
      ``scanner.scanImage`` that applies scanCrop and maxScanDimension.  Some
      additional result filtering is also performed.

      :image: A :class:`CGImage` to scan.
      :Returns: The result set containing :class:`ZBarSymbol` objects.


Constants
---------

.. type:: ZBarReaderControllerCameraMode

   The scanning mode to use with the camera.

   ZBarReaderControllerCameraModeDefault
      The standard mode provided by UIImagePickerController - the user
      manually captures an image by tapping a control.  This is the default
      unless private APIs are enabled.

   ZBarReaderControllerCameraModeSampling
      Automatically capture by taking screenshots with
      :func:`UIGetScreenImage`.  Resolution is limited to the screen
      resolution, so this mode is inappropriate for longer codes.  Only
      available when private APIs are enabled, and becomes the default mode in
      that case.

   ZBarReaderControllerCameraModeSequence
      Experimental mode that automatically scans by "rapidly" scanning
      pictures captured with ``takePicture``.  Not recommended for serious
      use.

.. c:var:: NSString *ZBarReaderControllerResults

   The info dictionary key used to return decode results to
   ``imagePickerController:didFinishPickingMediaWithInfo:``
