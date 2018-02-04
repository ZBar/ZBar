ZBarReaderDelegate Protocol Reference
=====================================

.. class:: ZBarReaderDelegate

   :Inherits from: :class:`UIImagePickerControllerDelegate`

   This protocol must be implemented by the
   :member:`~ZBarReaderViewController::readerDelegate` provided to a
   :class:`ZBarReaderViewController` or :class:`ZBarReaderController`.  It is
   used to notify the delegate of new decode results, when an image fails to
   decode, or when the user dismisses the reader with the built-in controls.


Instance Methods
----------------

   .. describe:: - (void) imagePickerController:(UIImagePickerController*)picker didFinishPickingMediaWithInfo:(NSDictionary*)info

      This inherited delegate method is called when a barcode is successfully
      decoded.  The decoded symbols are available from the dictionary as a
      :class:`ZBarSymbolSet` using the :c:data:`ZBarReaderControllerResults`
      key.  The image from which the barcodes were scanned is available using
      the :c:data:`UIImagePickerControllerOriginalImage` key.  No other keys
      are guaranteed to be valid.

      .. note::

         The ``picker`` parameter will be the reader controller instance that
         read the barcodes - not necessarily a
         :class:`UIImagePickerController` instance.  You should cast it to the
         correct type for anything other than basic view controller access.

      :picker: The reader controller that scanned the barcode(s).
      :info: A dictionary containing the image and decode results.

   .. describe:: - (void) imagePickerControllerDidCancel:(UIImagePickerController*)picker

      Called when the user taps the "Cancel" button provided by the built-in
      controls (when :member:`showsZBarControls`\ ``=YES``).  The default
      implementation dismisses the reader.  If this method is implemented, it
      should do the same.

      .. note::

         The ``picker`` parameter will be the reader controller instance that
         read the barcodes - not necessarily a
         :class:`UIImagePickerController` instance.  You should cast it to the
         correct type for anything other than basic view controller access.

      :picker: The reader controller that scanned the barcode(s).

   .. describe:: - (void) readerControllerDidFailToRead:(ZBarReaderController*)reader withRetry:(BOOL)retry

      Called when an image, manually captured or selected from the photo
      library, is scanned and no barcodes were detected.

      If the ``retry`` parameter is ``NO``, the controller must be dismissed
      before this method returns.  Otherwise, another scan may be attempted
      without re-presenting the controller.

      If the :member:`~ZBarReaderController::showsHelpOnFail` is ``YES`` *and*
      ``retry`` is ``YES``, the integrated help viewer will already be
      presenting.

      If this method is not implemented, the controller will be dismissed iff
      ``retry`` is ``NO``.

      :reader: The :class:`ZBarReaderController` that scanned the barcode(s).
      :retry: Whether another scan may be attempted.
