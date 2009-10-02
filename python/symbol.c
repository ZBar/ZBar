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

static char symbol_doc[] = PyDoc_STR(
    "symbol result object.\n"
    "\n"
    "data and associated information about a successful decode.");

static int
symbol_traverse (zbarSymbol *self,
                 visitproc visit,
                 void *arg)
{
    return(0);
}

static int
symbol_clear (zbarSymbol *self)
{
    if(self->zsym) {
        zbar_symbol_t *zsym = (zbar_symbol_t*)self->zsym;
        self->zsym = NULL;
        zbar_symbol_ref(zsym, -1);
    }
    Py_CLEAR(self->data);
    Py_CLEAR(self->loc);
    return(0);
}

static void
symbol_dealloc (zbarSymbol *self)
{
    symbol_clear(self);
    ((PyObject*)self)->ob_type->tp_free((PyObject*)self);
}

static zbarSymbolSet*
symbol_get_components (zbarSymbol *self,
                       void *closure)
{
    const zbar_symbol_set_t *zsyms = zbar_symbol_get_components(self->zsym);
    return(zbarSymbolSet_FromSymbolSet(zsyms));
}

static zbarSymbolIter*
symbol_iter (zbarSymbol *self)
{
    zbarSymbolSet *syms = symbol_get_components(self, NULL);
    zbarSymbolIter *iter = zbarSymbolIter_FromSymbolSet(syms);
    Py_XDECREF(syms);
    return(iter);
}

static zbarEnumItem*
symbol_get_type (zbarSymbol *self,
                 void *closure)
{
    return(zbarSymbol_LookupEnum(zbar_symbol_get_type(self->zsym)));
}

static PyObject*
symbol_get_long (zbarSymbol *self,
                 void *closure)
{
    int val;
    if(!closure)
        val = zbar_symbol_get_quality(self->zsym);
    else
        val = zbar_symbol_get_count(self->zsym);
    return(PyInt_FromLong(val));
}

static PyObject*
symbol_get_data (zbarSymbol *self,
                 void *closure)
{
    if(!self->data) {
        /* FIXME this could be a buffer now */
        self->data =
            PyString_FromStringAndSize(zbar_symbol_get_data(self->zsym),
                                       zbar_symbol_get_data_length(self->zsym));
        if(!self->data)
            return(NULL);
    }
    Py_INCREF(self->data);
    return(self->data);
}

static PyObject*
symbol_get_location (zbarSymbol *self,
                     void *closure)
{
    if(!self->loc) {
        /* build tuple of 2-tuples representing location polygon */
        unsigned int n = zbar_symbol_get_loc_size(self->zsym);
        self->loc = PyTuple_New(n);
        unsigned int i;
        for(i = 0; i < n; i++) {
            PyObject *x, *y;
            x = PyInt_FromLong(zbar_symbol_get_loc_x(self->zsym, i));
            y = PyInt_FromLong(zbar_symbol_get_loc_y(self->zsym, i));
            PyTuple_SET_ITEM(self->loc, i, PyTuple_Pack(2, x, y));
        }
    }
    Py_INCREF(self->loc);
    return(self->loc);
}

static PyGetSetDef symbol_getset[] = {
    { "type",       (getter)symbol_get_type, },
    { "quality",    (getter)symbol_get_long, NULL, NULL, (void*)0 },
    { "count",      (getter)symbol_get_long, NULL, NULL, (void*)1 },
    { "data",       (getter)symbol_get_data, },
    { "location",   (getter)symbol_get_location, },
    { "components", (getter)symbol_get_components,  },
    { NULL, },
};

PyTypeObject zbarSymbol_Type = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "zbar.Symbol",
    .tp_doc         = symbol_doc,
    .tp_basicsize   = sizeof(zbarSymbol),
    .tp_flags       = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                      Py_TPFLAGS_HAVE_GC,
    .tp_traverse    = (traverseproc)symbol_traverse,
    .tp_clear       = (inquiry)symbol_clear,
    .tp_dealloc     = (destructor)symbol_dealloc,
    .tp_iter        = (getiterfunc)symbol_iter,
    .tp_getset      = symbol_getset,
};

zbarSymbol*
zbarSymbol_FromSymbol (const zbar_symbol_t *zsym)
{
    /* FIXME symbol object recycle cache */
    zbarSymbol *self = PyObject_GC_New(zbarSymbol, &zbarSymbol_Type);
    if(!self)
        return(NULL);
    assert(zsym);
    zbar_symbol_t *zs = (zbar_symbol_t*)zsym;
    zbar_symbol_ref(zs, 1);
    self->zsym = zsym;
    self->data = NULL;
    self->loc = NULL;
    return(self);
}

zbarEnumItem*
zbarSymbol_LookupEnum (zbar_symbol_type_t type)
{
    PyObject *key = PyInt_FromLong(type);
    zbarEnumItem *e = (zbarEnumItem*)PyDict_GetItem(symbol_enum, key);
    if(!e) 
        return((zbarEnumItem*)key);
    Py_INCREF((PyObject*)e);
    Py_DECREF(key);
    return(e);
}
