ZBarReaderViewController Class Reference
========================================

.. class:: ZBarReaderViewController

   :Inherits from: :class:`UIViewController`

   This is the controller to use for live scanning from the camera feed with
   automatic capture.  For scanning from image files or with manual capture,
   see :class:`ZBarReaderController`.


Properties
----------

   .. member:: ZBarImageScanner *scanner

      Access to the image scanner for configuration. (read-only)

   .. member:: id <ZBarReaderDelegate> readerDelegate

      The delegate that will be notified when new barcode results are
      available.

   .. member:: BOOL showsZBarControls

      Whether to display a default control set consisting of cancel, scan and
      info buttons.  Disable these if you provide your own controls using the
      :member:`cameraOverlayView`.  (Default ``YES``).

   .. member:: BOOL tracksSymbols

      Whether to display the tracking rectangle around detected barcodes.

   .. member:: CGRect scanCrop

      Crop images before scanning.  The original image will be cropped to this
      rectangle, which should be in normalized image coordinates (NB the
      camera image x-axis is *vertical* on the screen).  Defaults to the full
      image ``{{0, 0}, {1, 1}}``.

   .. member:: UIView *cameraOverlayView

      A custom view to display over the camera preview.

   .. member:: CGAffineTransform cameraViewTransform

      A transform to apply to the camera preview.  Ignored by the reader.

   .. member:: ZBarReaderView *readerView

      View that presents the camera preview and performs the scanning.  This
      view has other properties you may use to control the appearance and
      behavior of the reader.

      Note that this view may be released when it is not displayed (eg, under
      low memory conditions).  You should apply any configuration just before
      you present the reader.

   .. member:: BOOL enableCache

      This property is deprecated and should not be modified.

   .. warning::

      The remaining properties are deprecated, they are only present for
      backward compatibility with :class:`ZBarReaderController` and will raise
      an exception if inappropriate/unsupported values are set.

   .. member:: UIImagePickerControllerSourceType sourceType

      Raises an exception if anything other than
      ``UIImagePickerControllerSourceTypeCamera`` is set.  If you want to scan
      images, use a :class:`ZBarReaderController` instead of this class.

   .. member:: BOOL allowsEditing

      Raises an exception if anything other than ``NO`` is set.

   .. member:: BOOL showsCameraControls

      Raises an exception if anything other than ``NO`` is set.  Use
      :member:`showsZBarControls` to disable the buit-in overlay.

   .. member:: BOOL showsHelpOnFail

      Any value set to this property is ignored.  It is only useful for
      scanning images, for which you should use :class:`ZBarReaderController`.

   .. member:: ZBarReaderControllerCameraMode cameraMode

      This reader only supports scanning from the camera feed.  If you want to
      scan manually captured images, use a :class:`ZBarReaderController`
      instead of this class.

   .. member:: BOOL takesPicture

      Raises an exception if anything other than ``NO`` is set.  This
      controller automatically returns the scanned camera frame and does not
      support capturing a separate image.

   .. member:: NSInteger maxScanDimension

      Any value set to this property is ignored.  It is only useful for
      scanning images, for which you should use :class:`ZBarReaderController`.


Class Methods
-------------

   .. describe:: + (BOOL) isSourceTypeAvailable:(UIImagePickerControllerSourceType)source

      Returns ``YES`` only if ``source`` is ``Camera`` and the
      :class:`UImagePickerController` method of the same name also returns
      ``YES``.

Instance Methods
----------------

   .. _`showHelpWithReason:`:
   .. describe:: - (void) showHelpWithReason:(NSString*)reason

      Display the integrated help browser.  Use this with custom overlays if
      you don't also want to create your own help view.  Should only be called
      when the reader is displayed.  The ``reason`` argument will be passed to
      the :func:`onZBarHelp` javascript function.

      :reason: A string parameter passed to javascript.
