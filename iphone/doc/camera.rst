Scanning From the Camera Feed
=============================

Many iOS developers want their application to support automatic recognition of
barcodes from the camera feed in real-time.  ZBar makes this easy!

There are three levels that you may choose to integrate at, from least complex
(recommended) to most complex these are:

* Use the fully integrated view controller - this is very easy to implement
  and is the recommended approach.
* Use the reader view with your own controller - this more advanced approach
  allows you to embed the view directly in your view hierarchy.
* Use the capture component with your own AVCapture session - this is not
  supported and only provided for advanced developers with special needs who
  are already familiar with AVCapture.


Using a ZBarReaderViewController
--------------------------------

This is the fastest, easiest and recommend way to get the barcode reader into
your application.  The procedure is the same as using a
UIImagePickerController to take a picture with the camera, so it will help if
you are familiar with that.  Basically you:

1. Create the reader.

   This is as simple as creating a new :class:`ZBarReaderViewController`::

      ZBarReaderViewController *reader = [[ZBarReaderViewController alloc] init];

2. Setup a delegate to receive the results.

   The delegate should implement the :class:`ZBarReaderDelegate` protocol,
   which inherits from :class:`UIImagePickerControllerDelegate`::

      reader.readerDelegate = self;

3. Configure the reader.

   Aside from the properties of the reader itself, you can configure the
   decoder via the :member:`~ZBarReaderViewController::scanner` property and
   further customize the view via the
   :member:`~ZBarReaderViewController::readerView` property::

      // disable QR Code
      [reader.scanner setSymbology: ZBAR_QRCODE
                      config: ZBAR_CFG_ENABLE
                      to: 0];
      reader.readerView.zoom = 1.0;

   See :doc:`custom` and :doc:`optimizing` for more details.

4. Present the reader to the user.

   Typically the controller is presented modally::

      [self presentModalViewController: reader
            animated: YES];

   Alternatively, it may be added to a container controller.

5. Process the results.

   The controller will call the
   ``imagePickerController:didFinishPickingMediaWithInfo:`` method of
   your delegate every time new results become available.  The barcode data
   can be obtained using the :c:data:`ZBarReaderControllerResults` key of the
   info dictionary.  This key will return "something enumerable"; keep in mind
   that there may be multiple results.  You may also retrieve the
   corresponding image with :c:data:`UIImagePickerControllerOriginalImage` as
   usual::

      - (void) imagePickerController: (UIImagePickerController*) reader
       didFinishPickingMediaWithInfo: (NSDictionary*) info
      {
          id<NSFastEnumeration> results =
              [info objectForKey: ZBarReaderControllerResults];
          UIImage *image =
              [info objectForKey: UIImagePickerControllerOriginalImage];
          ...

   The ``reader`` parameter will be the actual type of the reader (not
   necessarily a :class:`UIImagePickerController`).

   .. note::

      The delegate method should queue the interface response and return as
      soon as possible; any processing of the results should be deferred until
      later, otherwise the user will experience unacceptable latency between
      the actual scan completion and the visual interface feedback.

6. Dismiss the reader (or not).

   Once you have the results you may dismiss the reader::

      [reader dismissModalViewControllerAnimated: YES];

   .. warning::

      It is very important to dismiss from the *reader* (not the presenting
      controller) to avoid corrupting the interface.

   Alternatively, you may choose to continue scanning and provide visual
   feedback another way (eg, maybe by updating your custom overlay with the
   results).  The "continuous" mode of the readertest example does this.


Using a ZBarReaderView
----------------------

:class:`ZBarReaderViewController` is a relatively thin wrapper around a
:class:`ZBarReaderView`; it is possible to use the view directly, even from
Interface Builder.  You lose only some of the simulator and rotation hooks.
The documentation is also less complete, so you need to be able to UTSL.  See
the :file:`EmbedReader` sample for a working example.


Using the ZBarCaptureReader
---------------------------

If you have special requirements for the capture session or just want to use
your own preview, you can add your own :class:`ZBarCaptureReader` to your
session.  You must have a solid understanding of the AVCapture infrastructure
if you plan to use this approach.

.. admonition:: TBD

   sorry, you're on your own here - UTSL  :)
