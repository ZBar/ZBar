ZBarReaderViewDelegate Protocol Reference
=========================================

.. class:: ZBarReaderViewDelegate

   :Inherits from: :class:`NSObject`

   This protocol, which must be implemented by the `readerDelegate` provided
   to a :class:`ZBarReaderView`, is used to notify the delegate of new decode
   results.


Instance Methods
----------------

   .. describe:: - (void) readerView:(ZBarReaderView*)readerView didReadSymbols:(ZBarSymbolSet*)symbols fromImage:(UIImage*)image

      Called to notify the delegate of new decode results.

      Note that the referenced image is a proxy for a video buffer that is
      asynchronously being converted to a :class:`UIImage`, attempting to
      access the data will block until the conversion is complete.

      :readerView: :class:`ZBarReaderView` that scanned the barcode(s).
      :symbols: :class:`ZBarSymbolSet` containing the decode results.
      :image: :class:`UIImage` from which the barcode(s) were scanned.
