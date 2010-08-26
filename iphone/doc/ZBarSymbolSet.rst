ZBarSymbolSet Class Reference
=============================

.. class:: ZBarSymbolSet

   :Inherits from: :class:`NSObject`
   :Conforms to: :class:`NSFastEnumeration`

   A symbol set is a simple container for the symbols scanned from an image.
   It supports :class:`NSFastEnumeration`, and not much else...  Use it to
   iterate through the :class:`ZBarSymbol` objects in a decode result set::

      ZBarSymbolSet *symbols = image.symbols;
      for(ZBarSymbol *symbol in symbols) {
          // process result
      }

   This class is a simple wrapper around a :type:`zbar_symbol_set_t` C object
   (q.v.)


Properties
----------

   .. member:: int count

      The number of symbols in the set. (read-only)

   .. member:: const zbar_symbol_set_t *zbarSymbolSet

      Retrieve the underlying C object instance. (read-only)


Instance Methods
----------------

   .. _`initWithSymbolSet:`:
   .. describe:: - (id) initWithSymbolSet:(const zbar_symbol_set_t*)set

      Initialize a symbol set wrapper, given the C object to wrap.

      :set: The C object to wrap.
      :Returns: The initialized symbol set, or nil if an error occurred.
