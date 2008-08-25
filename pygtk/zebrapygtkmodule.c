/*------------------------------------------------------------------------
 *  Copyright 2008 (c) Jeff Brown <spadix@users.sourceforge.net>
 *
 *  This file is part of the Zebra Barcode Library.
 *
 *  The Zebra Barcode Library is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The Zebra Barcode Library is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with the Zebra Barcode Library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301  USA
 *
 *  http://sourceforge.net/projects/zebra
 *------------------------------------------------------------------------*/

/* avoid "multiple definition" darwin link errors
 * for symbols defined in pygobject.h (bug #2052681)
 */
#define NO_IMPORT_PYGOBJECT

#include <pygobject.h>

void zebrapygtk_register_classes(PyObject*);
extern PyMethodDef zebrapygtk_functions[];

DL_EXPORT(void)
initzebrapygtk(void)
{
    init_pygobject();

    PyObject *mod = Py_InitModule("zebrapygtk", zebrapygtk_functions);
    PyObject *dict = PyModule_GetDict(mod);

    zebrapygtk_register_classes(dict);

    if(PyErr_Occurred())
        Py_FatalError("unable to initialise module zebrapygtk");
}
