ZBarImage Class Reference
=========================

.. class:: ZBarImage

   :Inherits from: :class:`NSObject`

   A :class:`ZBarImage` is a wrapper for images passed to the barcode reader.
   It encapsulates raw image data with the format and size metadata necessary
   to interpret it.

   An image must be wrapped in a :class:`ZBarImage` in order to be scanned by
   the library.  At least the format, size and data must be set.  There are
   also initialization methods for automatically extracting the data and
   format from a `CGImage`.

   This class is a wrapper around a :type:`zbar_image_t` C object (q.v.)


Properties
----------

   .. member:: unsigned long format

      The image format four-charcter code (fourcc) as a 4-byte integer.  Use
      :ref:`fourcc:<fourcc:>` to create a fourcc value from a string.

   .. member:: unsigned sequence

      A "sequence number" associated with the image.  This reference value is
      unused by the library.

   .. member:: CGSize size

      The size of the image in pixels.

      .. note::

         There is no separate "bytesPerLine" property, the width must match
         the image data (which is not always the logical image width).

   .. member:: CGRect crop

      Optionally limit the scan region to this rectangle without having to
      generate a cropped image.

   .. member:: const void *data

      Obtain a pointer to the raw image data.  This property is read-only, use
      :ref:`setData:withLength:<setData:withLength:>` to set the image data.

   .. member:: unsigned long dataLength

      Byte length of the raw image data.  This property is read-only, use 
      :ref:`setData:withLength:<setData:withLength:>` to set the image data.

   .. member:: ZBarSymbolSet *symbols

      Barcode results from the last scan.

   .. member:: zbar_image_t *zbarImage

      Retrieve the underlying C object instance. (read-only)

   .. member:: UIImage *UIImage

      Convert the image to a UIImage.  Only certain image formats are
      supported for conversion (read-only)

      :See also: :ref:`UIImageWithOrientation:<UIImageWithOrientation:>`


Class Methods
-------------

   .. _`fourcc:`:
   .. describe:: + (unsigned long) fourcc:(NSString*)format

      Parse the integer four-character code from a string.  Alternatively use
      the :func:`zbar_fourcc` macro to create a constant expression.

      :format: A four character string representing an image format.
      :Returns: The corresponding 4-byte integer format code.


Instance Methods
----------------

   .. _`initWithImage:`:
   .. describe:: - (id) initWithImage:(zbar_image_t*)image

      Initialize an image wrapper, given the C object to wrap.

      :image: The C object to wrap.
      :Returns: The initialized :class:`ZBarImage`.

   .. _`initWithCGImage:`:
   .. describe:: - (id) initWithCGImage:(CGImageRef)image

      Initialize a :class:`ZBarImage` from the data and metadata extracted
      from a `CGImage`.  The image is converted to `Y800` (grayscale) format.

      :image: A `CGImage` to source the data and metadata.
      :Returns: The initialized :class:`ZBarImage`.
      :See also: :ref:`initWithCGImage:size:<initWithCGImage:size:>`

   .. _`initWithCGImage:size:`:
   .. describe:: - (id) initWithCGImage:(CGImageRef)image size:(CGSize)size

      Initialize a :class:`ZBarImage` from the data and metadata extracted
      from a `CGImage`.  The image is converted to `Y800` (grayscale) format
      and scaled to the specified size.

      :image: A `CGImage` to source the data and metadata.
      :size: The pixel size of the resulting ZBarImage.
      :Returns: The initialized :class:`ZBarImage`.
      :See also: :ref:`initWithCGImage:crop:size:<initWithCGImage:crop:size:>`

   .. _`initWithCGImage:crop:size:`:
   .. describe:: - (id) initWithCGImage:(CGImageRef)image crop:(CGRect)crop size:(CGSize)size

      Initialize a :class:`ZBarImage` from the data and metadata extracted
      from a `CGImage`.  The image is simultaneously converted to `Y800`
      (grayscale) format, cropped and scaled to the specified size.

      :image: A `CGImage` to source the data and metadata.
      :crop: The region to convert, in image coordinates.
      :size: The pixel size of the resulting ZBarImage.
      :Returns: The initialized :class:`ZBarImage`.

   .. _`setData:withLength:`:
   .. describe:: - (void) setData:(const void*)data withLength:(unsigned long)length

      Specify a pointer to the raw image data, for the image format and size.
      The length of the data must also be provided.  Note that the data must
      remain valid as long as the image has a reference to it.  Set data to
      ``NULL`` to clear a previous reference.

      :data: A pointer to a raw image data buffer.
      :length: The size of the image data buffer.

   .. _`UIImageWithOrientation:`:
   .. describe:: - (UIImage*) UIImageWithOrientation:(UIImageOrientation)orient

      Convert the image to a UIImage with the specified orientation.  Only
      certain image formats are supported for conversion.  (currently
      ``RGB3``, ``RGB4``, ``RGBQ``)

      :orient: Desired orientation of the image.
      :Returns: A new :class:`UIImage`, or ``nil`` in case of error.
