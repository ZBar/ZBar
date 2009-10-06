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

static char symboliter_doc[] = PyDoc_STR(
    "symbol iterator.\n"
    "\n"
    "iterates over decode results attached to an image.");

static int
symboliter_traverse (zbarSymbolIter *self,
                     visitproc visit,
                     void *arg)
{
    Py_VISIT(self->syms);
    return(0);
}

static int
symboliter_clear (zbarSymbolIter *self)
{
    if(self->zsym) {
        zbar_symbol_t *zsym = (zbar_symbol_t*)self->zsym;
        self->zsym = NULL;
        zbar_symbol_ref(zsym, -1);
    }
    Py_CLEAR(self->syms);
    return(0);
}

static void
symboliter_dealloc (zbarSymbolIter *self)
{
    symboliter_clear(self);
    ((PyObject*)self)->ob_type->tp_free((PyObject*)self);
}

static zbarSymbolIter*
symboliter_iter (zbarSymbolIter *self)
{
    Py_INCREF(self);
    return(self);
}

static zbarSymbol*
symboliter_iternext (zbarSymbolIter *self)
{
    if(self->zsym) {
        zbar_symbol_t *zsym = (zbar_symbol_t*)self->zsym;
        zbar_symbol_ref(zsym, -1);
        self->zsym = zbar_symbol_next(self->zsym);
    }
    else if(self->syms->zsyms)
        self->zsym = zbar_symbol_set_first_symbol(self->syms->zsyms);
    else
        self->zsym = NULL;

    zbar_symbol_t *zsym = (zbar_symbol_t*)self->zsym;
    if(!zsym)
        return(NULL);
    zbar_symbol_ref(zsym, 1);
    return(zbarSymbol_FromSymbol(self->zsym));
}

PyTypeObject zbarSymbolIter_Type = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "zbar.SymbolIter",
    .tp_doc         = symboliter_doc,
    .tp_basicsize   = sizeof(zbarSymbolIter),
    .tp_flags       = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                      Py_TPFLAGS_HAVE_GC,
    .tp_traverse    = (traverseproc)symboliter_traverse,
    .tp_clear       = (inquiry)symboliter_clear,
    .tp_dealloc     = (destructor)symboliter_dealloc,
    .tp_iter        = (getiterfunc)symboliter_iter,
    .tp_iternext    = (iternextfunc)symboliter_iternext,
};

zbarSymbolIter*
zbarSymbolIter_FromSymbolSet (zbarSymbolSet *syms)
{
    zbarSymbolIter *self;
    self = PyObject_GC_New(zbarSymbolIter, &zbarSymbolIter_Type);
    if(!self)
        return(NULL);

    Py_INCREF(syms);
    self->syms = syms;
    self->zsym = NULL;
    return(self);
}
