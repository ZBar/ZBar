Scanning a User-Selected Image
==============================

Some applications may need the full resolution offered by camera snapshots, or
need to scan an image or document from the user's photo library.  In these
cases you use a :class:`ZBarReaderController`.  This reader is a *subclass* of
:class:`UIImagePickerController`, and you use it the same way.  See the
documentation for :class:`UIImagePickerController` for more detais.

1. Create the reader.

   This is as simple as creating a new :class:`ZBarReaderController`::

      ZBarReaderController *reader = [[ZBarReaderController alloc] init];

2. Setup a delegate to receive the results.

   The delegate should implement the :class:`ZBarReaderDelegate` protocol,
   which inherits from :class:`UIImagePickerControllerDelegate`::

      reader.readerDelegate = self;

3. Configure the reader.

   You will need to set the :member:`~ZBarReaderController::sourceType`
   appropriately.  Aside from the properties of the reader itself, you can
   configure the decoder via the :member:`~ZBarReaderController::scanner`
   property::

      if([ZBarReaderController isSourceTypeAvailable:
                                   UIImagePickerControllerSourceTypeCamera])
          reader.sourceType = UIImagePickerControllerSourceTypeCamera;
      [reader.scanner setSymbology: ZBAR_I25
                      config: ZBAR_CFG_ENABLE
                      to: 0];

   See :doc:`custom` and :doc:`optimizing` for more details.

4. Present the reader to the user.

   As the reader is a UIImagePickerController, it must be presented modally::

      [self presentModalViewController: reader
            animated: YES];

5. Process the results.

   The controller will call the
   ``imagePickerController:didFinishPickingMediaWithInfo:`` method of
   your delegate for a successful decode (NB *not* every time the user takes a
   picture or selects an image).  The barcode data can be obtained using the
   :c:data:`ZBarReaderControllerResults` key of the info dictionary.  This key
   will return "something enumerable"; keep in mind that there may be multiple
   results.  You may also retrieve the corresponding image with
   :c:data:`UIImagePickerControllerOriginalImage` as usual::

      - (void) imagePickerController: (UIImagePickerController*) reader
       didFinishPickingMediaWithInfo: (NSDictionary*) info
      {
          id<NSFastEnumeration> results =
              [info objectForKey: ZBarReaderControllerResults];
          UIImage *image =
              [info objectForKey: UIImagePickerControllerOriginalImage];
          ...

   The ``reader`` parameter will be the actual :class:`ZBarReaderController`
   (again, a subclass :class:`UIImagePickerController`).

   .. note::

      The delegate method should dismiss the reader and return as soon as
      possible; any processing of the results should be deferred until later,
      otherwise the user will experience unacceptable latency between the
      actual scan completion and the visual interface feedback.

6. Dismiss the reader.

   Once you have the results you should dismiss the reader::

      [reader dismissModalViewControllerAnimated: YES];

   .. warning::

      It is very important to dismiss from the *reader* (not the presenting
      controller) to avoid corrupting the interface.


Handling Failure
----------------

It is always possible the user selects/takes an image that does not contain
barcodes, or that the image quality is not sufficient for the ZBar library to
scan successfully.

In this case, and if :member:`~ZBarReaderController::showsHelpOnFail` is
``YES``, the integrated help controller will automatically be displayed with
reason set to ``"FAIL"``.

Your delegate may also choose to implement the optional
``readerControllerDidFailToRead:withRetry:`` method to explicitly handle
failures.  If the ``retry`` parameter is ``NO``, you *must* dismiss the reader
before returning, otherwise you may continue and allow the user to retry the
operation.  Note that, if it is enabled, the integrated help will be displayed
when this delegate method is invoked.
