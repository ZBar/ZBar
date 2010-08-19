ZBarImageScanner Class Reference
================================

.. class:: ZBarImageScanner

   :Inherits from: :class:`NSObject`

   This is a low-level interface for programmatically scanning images without
   a user interface.  If you want to scan images manually selected by the user
   (from the photo library or using the camera), you may prefer to use a
   :class:`ZBarReaderController` instead.

   This class is a wrapper around a :type:`zbar_image_scanner_t` C object
   (q.v.)


Properties
----------

   .. member:: BOOL enableCache

      Enable the inter-frame consistency cache.  Set to ``YES`` for scanning
      video or ``NO`` for scanning images.

   .. member:: ZBarSymbolSet results

      Decoded symbols resulting from the last scan.


Instance Methods
----------------

   .. _`parseConfig:`:
   .. describe:: - (void) parseConfig:(NSString*)config

      Apply scanner/decoder configuration parsed from a string.

      :config: A configuration setting of the form: `symbology.config[=value]`.

   .. _`setSymbology:config:to:`:
   .. describe:: - (void) setSymbology:(zbar_symbol_type_t)symbology config:(zbar_config_t)config to:(int)value

      Apply generic scanner/decoder configuration.

      :symbology: The symbology to effect, or 0 for all.
      :config: The configuration setting to adjust.
      :value: The value to set for the specific configuration/symbology.

   .. _`scanImage:`:
   .. describe:: - (NSInteger) scanImage:(ZBarImage*)image

      Scan an image for barcodes using the current configuration.  The image
      must be in ``Y800`` format (8-bpp graysale).

      :image: The :class:`ZBarImage` to scan.
      :Returns: The number of barcode symbols decoded in the image.


Constants
---------

.. type:: zbar_config_t

   ZBAR_CFG_ENABLE
      Control whether specific symbologies will be recognized.  Disabling
      unused symbologies improves performance and prevents bad scans.

   ZBAR_CFG_EMIT_CHECK
      Whether to include the check digit in the result data string.  This
      value may be set individually for symbologies where it makes sense.

   ZBAR_CFG_MIN_LEN
      The minimum data length for a symbol to be valid, set to 0 to disable.
      Use with eg, I2/5 to avoid short scans.  This value may be set
      individually for variable-length symbologies.

   ZBAR_CFG_MAX_LEN
      The maximum data length for which a symbol is valid, set to 0 to
      disable.  Use with eg, I2/5 to enforce a specific range of data lengths.
      This value may be set individually for variable-length symbologies.

   ZBAR_CFG_UNCERTAINTY
      Number of "nearby" frames that must contain a symbol before it will be
      considered valid.  This value may be set for individual symbologies.

   ZBAR_CFG_POSITION
      Whether to track position information.  

   ZBAR_CFG_X_DENSITY
      The stride to use for scanning vertical columns of the image.  This many
      pixel columns will be skipped between vertical scan passes.  Useful for
      trading off between resolution and performance.  This is a scanner
      setting (use 0 for the symbology).

   ZBAR_CFG_Y_DENSITY
      The stride to use for scanning horizontal columns of the image.  This
      many pixel rows will be skipped between horizontal scan passes.  Useful
      for trading off between resolution and performance.  This is a scanner
      setting (use 0 for the symbology).
