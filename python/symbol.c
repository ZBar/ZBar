#include "zebramodule.h"

static char symbol_doc[] = PyDoc_STR(
    "symbol result object.\n"
    "\n"
    "data and associated information about a successful decode.");

static int
symbol_traverse (zebraSymbol *self,
                 visitproc visit,
                 void *arg)
{
    Py_VISIT(self->img);
    return(0);
}

static int
symbol_clear (zebraSymbol *self)
{
    Py_CLEAR(self->img);
    Py_CLEAR(self->data);
    Py_CLEAR(self->loc);
    return(0);
}

static void
symbol_dealloc (zebraSymbol *self)
{
    symbol_clear(self);
    ((PyObject*)self)->ob_type->tp_free((PyObject*)self);
}

static zebraEnumItem*
symbol_get_type (zebraSymbol *self,
                 void *closure)
{
    return(zebraSymbol_LookupEnum(zebra_symbol_get_type(self->zsym)));
}

static PyObject*
symbol_get_count (zebraSymbol *self,
                  void *closure)
{
    return(PyInt_FromLong(zebra_symbol_get_count(self->zsym)));
}

static PyObject*
symbol_get_data (zebraSymbol *self,
                 void *closure)
{
    if(!self->data) {
        self->data = PyString_FromString(zebra_symbol_get_data(self->zsym));
        if(!self->data)
            return(NULL);
    }
    Py_INCREF(self->data);
    return(self->data);
}

static PyObject*
symbol_get_location (zebraSymbol *self,
                     void *closure)
{
    if(!self->loc) {
        /* build tuple of 2-tuples representing location polygon */
        unsigned int n = zebra_symbol_get_loc_size(self->zsym);
        self->loc = PyTuple_New(n);
        unsigned int i;
        for(i = 0; i < n; i++) {
            PyObject *x, *y;
            x = PyInt_FromLong(zebra_symbol_get_loc_x(self->zsym, i));
            y = PyInt_FromLong(zebra_symbol_get_loc_y(self->zsym, i));
            PyTuple_SET_ITEM(self->loc, i, PyTuple_Pack(2, x, y));
        }
    }
    Py_INCREF(self->loc);
    return(self->loc);
}

static PyGetSetDef symbol_getset[] = {
    { "type",     (getter)symbol_get_type, },
    { "count",    (getter)symbol_get_count, },
    { "data",     (getter)symbol_get_data, },
    { "location", (getter)symbol_get_location, },
    { NULL, },
};

PyTypeObject zebraSymbol_Type = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "zebra.Symbol",
    .tp_doc         = symbol_doc,
    .tp_basicsize   = sizeof(zebraSymbol),
    .tp_flags       = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                      Py_TPFLAGS_HAVE_GC,
    .tp_traverse    = (traverseproc)symbol_traverse,
    .tp_clear       = (inquiry)symbol_clear,
    .tp_dealloc     = (destructor)symbol_dealloc,
    .tp_getset      = symbol_getset,
};

zebraSymbol*
zebraSymbol_FromSymbol (zebraImage *img,
                        const zebra_symbol_t *zsym)
{
    /* FIXME symbol object recycle cache */
    zebraSymbol *self = PyObject_GC_New(zebraSymbol, &zebraSymbol_Type);
    if(!self)
        return(NULL);
    assert(img);
    assert(zsym);
    Py_INCREF(img);
    self->zsym = zsym;
    self->img = img;
    self->data = NULL;
    self->loc = NULL;
    return(self);
}

zebraEnumItem*
zebraSymbol_LookupEnum (zebra_symbol_type_t type)
{
    PyObject *key = PyInt_FromLong(type);
    zebraEnumItem *e = (zebraEnumItem*)PyDict_GetItem(symbol_enum, key);
    if(!e) 
        return((zebraEnumItem*)key);
    Py_INCREF((PyObject*)e);
    Py_DECREF(key);
    return(e);
}
