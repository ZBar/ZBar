Customizing the Interface
=========================

The reader supports customization of the camera overlay and the integrated
help that is displayed.


Customizing the Overlay
-----------------------

If you are scanning with the camera, whether using a
:class:`ZBarReaderViewController` for automatic capture or manually with
:class:`ZBarReaderController`, you may want to customize the appearance of the
reader.  You do this mainly by setting a
:member:`~ZBarReaderViewController::cameraOverlayView`.

Note that if you are scanning images from the photo library, there is no
customization - you are limited to the system picker interface provided by the
:class:`UIImagePickerController`.

If you are using a :class:`ZBarReaderViewController` and just want to add to
the existing controls, you can simply set your overlay to include the
additional view hierarchy::

   reader.cameraOverlayView = myLogoImageView;

Otherwise, if you are using a :class:`ZBarReaderController` or prefer to
completely replace the default controls, you should disable those first.  Note
that you will need to provide your own controls, which should at least include
a way to dismiss the reader::

   reader.showsCameraControls = NO;  // for UIImagePickerController
   reader.showsZBarControls = NO;
   reader.cameraOverlayView = myControlView;

For manual capture with :class:`ZBarReaderController`, you should also include
a control connected to :member:`~ZBarReaderController::takePicture`.

In either case, the overlay view may be loaded from a NIB, or simply created
programmatically.

You can also disable the tracking rectangle that highlights barcodes with
:member:`~ZBarReaderViewController::tracksSymbols`.


Presenting Help
---------------

If you have set ``showsZBarControls = NO`` and replaced the default controls,
you may still present the built-in help viewer.  Just hook your custom control
to the ``showsHelpWithReason:`` method of the controller.  You should only
call this method when the reader is actually presented.

The default reader controls invoke ``showsHelpWithReason:`` with a reason
parameter of ``"INFO"`` when the info button is tapped.


Customizing the Help Content
----------------------------

Whether you use the default controls or provide your own, you can still
customize the content of the help that is displayed.  The integrated viewer
uses a UIWebView to display the contents of :file:`zbar-help.html` that we
copied into your Resources.  You should hack this up as you see fit to give
your users the best help experience.

To allow for runtime customization based on the reason for presenting help,
the javascript function ``onZBarHelp`` will be called just before the page is
displayed, with the ``reason`` argument set as provided to
``showsHelpWithReason:``.
