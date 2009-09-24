/*------------------------------------------------------------------------
 *  Copyright 2009 (c) Jeff Brown <spadix@users.sourceforge.net>
 *
 *  This file is part of the ZBar Bar Code Reader.
 *
 *  The ZBar Bar Code Reader is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The ZBar Bar Code Reader is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with the ZBar Bar Code Reader; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301  USA
 *
 *  http://sourceforge.net/projects/zbar
 *------------------------------------------------------------------------*/

#include "zbarmodule.h"

static char symbolset_doc[] = PyDoc_STR(
    "symbol result container.\n"
    "\n"
    "collection of symbols.");

static int
symbolset_clear (zbarSymbolSet *self)
{
    if(self->zsyms) {
        zbar_symbol_set_t *zsyms = (zbar_symbol_set_t*)self->zsyms;
        self->zsyms = NULL;
        zbar_symbol_set_ref(zsyms, -1);
    }
    return(0);
}

static void
symbolset_dealloc (zbarSymbolSet *self)
{
    symbolset_clear(self);
    ((PyObject*)self)->ob_type->tp_free((PyObject*)self);
}

static zbarSymbolIter*
symbolset_iter (zbarSymbolSet *self)
{
    return(zbarSymbolIter_FromSymbolSet(self));
}

Py_ssize_t
symbolset_length (zbarSymbolSet *self)
{
    if(self->zsyms)
        return(zbar_symbol_set_get_size(self->zsyms));
    return(0);
}

static PySequenceMethods symbolset_as_sequence = {
    .sq_length      = (lenfunc)symbolset_length,
};

PyTypeObject zbarSymbolSet_Type = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "zbar.SymbolSet",
    .tp_doc         = symbolset_doc,
    .tp_basicsize   = sizeof(zbarSymbolSet),
    .tp_flags       = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_dealloc     = (destructor)symbolset_dealloc,
    .tp_iter        = (getiterfunc)symbolset_iter,
    .tp_as_sequence = &symbolset_as_sequence,
};

zbarSymbolSet*
zbarSymbolSet_FromSymbolSet(const zbar_symbol_set_t *zsyms)
{
    zbarSymbolSet *self = PyObject_New(zbarSymbolSet, &zbarSymbolSet_Type);
    if(!self)
        return(NULL);
    if(zsyms) {
        zbar_symbol_set_t *ncsyms = (zbar_symbol_set_t*)zsyms;
        zbar_symbol_set_ref(ncsyms, 1);
    }
    self->zsyms = zsyms;
    return(self);
}
