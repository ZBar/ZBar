ZBarSymbol Class Reference
==========================

.. class:: ZBarSymbol

   :Inherits from: :class:`NSObject`

   A symbol wraps all of the information the library has about a decoded
   barcode.  Use the available properties to retrieve the barcode data, the
   symbology (type of barcode), location and more.

   This class is a simple wrapper around a :type:`zbar_symbol_t` C object
   (q.v.)


Properties
----------

   .. member:: zbar_symbol_type_t type

      The type of symbology that was decoded. (read-only)

   .. member:: NSString *typeName

      The canonical name used by the library to represent the symbology.
      (read-only)

   .. member:: NSUInteger configMask

      Bitmask of symbology config settings used during decode.

   .. member:: NSUInteger modifierMask

      Bitmask of symbology characteristics detected during decode.  See
      :type:`zbar_modifier_t` for the currently defined modifier bits.

   .. member:: NSString *data

      The raw decoded barcode data. (read-only)

   .. member:: int quality

      A relative metric indicating rough confidence in the decoded value.
      Larger values are better than smaller values. (read-only)

   .. member:: zbar_orientation_t orientation

      The general, axis-aligned orientation of the symbol, or
      ZBAR_ORIENT_UNKNOWN if unknown. (read-only)

   .. member:: ZBarSymbolSet *components

      The components of a composite symbol. (read-only)

   .. member:: const zbar_symbol_t *zbarSymbol

      Retrieve the underlying C object instance. (read-only)

   .. member:: CGRect bounds

      Calculate a rough bounding box for the symbol. (read-only)

      .. note::

         Coordinates are relative to the image *data*, which may not match a
         displayed UIImage.  Make sure to account for the UIImage orientation
         when using these values.


Class Methods
-------------

   .. _`nameForType:`:
   .. describe:: + (NSString*) nameForType:(zbar_symbol_type_t)type

      Retrieve the canonical name for a symbology used by the library, given
      its enumerated value.

      :type: The :type:`zbar_symbol_type_t` enumerated symbology value.
      :Returns: A short string name for the symbology.


Instance Methods
----------------

   .. _`initWithSymbol:`:
   .. describe:: - (id) initWithSymbol:(const zbar_symbol_t*)symbol

      Initialize a symbol wrapper, given the C object to wrap.

      :symbol: The C object to wrap.
      :Returns: The initialized symbol, or nil if an error occurred.


Constants
---------

.. type:: zbar_symbol_type_t

   Symbology identifiers.

   ZBAR_NONE
      No symbol was decoded.

   ZBAR_PARTIAL
      Intermediate status.

   ZBAR_EAN8
      EAN-8

   ZBAR_UPCE
      UPC-E

   ZBAR_ISBN10
      ISBN-10, converted from EAN-13

   ZBAR_UPCA
      UPC-A

   ZBAR_EAN13
      EAN-13

   ZBAR_ISBN13
      ISBN-13, converted from EAN-13

   ZBAR_I25
      Interleaved 2 of 5

   ZBAR_DATABAR
      GS1 DataBar (RSS)

   ZBAR_DATABAR_EXP
      GS1 DataBar Expanded

   ZBAR_CODABAR
      Codabar

   ZBAR_CODE39
      Code 39 (3 of 9)

   ZBAR_QRCODE
      QR Code

   ZBAR_CODE128
      Code 128

.. type:: zbar_orientation_t

   The coarse orientation of a symbol.

   .. note::

      Orientation is relative to the image *data*, which may not match a
      displayed UIImage.  Make sure to account for the UIImage orientation
      when using these values.

   ZBAR_ORIENT_UNKNOWN
      Unable to determine orientation.

   ZBAR_ORIENT_UP
      Upright, read left to right

   ZBAR_ORIENT_RIGHT
      Sideways, read top to bottom

   ZBAR_ORIENT_DOWN
      Upside-down, read right to left

   ZBAR_ORIENT_LEFT
      Sideways, read bottom to top

.. type:: zbar_modifier_t

   Decoder symbology modifier flags.

   .. note::

      These are bit indices, use eg, (1 << ZBAR_MOD_GS1) to test the
      modifierMask property.

   ZBAR_MOD_GS1
      Barcode tagged as GS1 (EAN.UCC) reserved (eg, FNC1 before first data
      character).  Data may be parsed as a sequence of GS1 AIs.

   ZBAR_MOD_AIM
      Barcode tagged as AIM reserved.
