#include "zebramodule.h"

static char enumitem_doc[] = PyDoc_STR(
    "simple enumeration item.\n"
    "\n"
    "associates an int value with a name for printing.");

static zebraEnumItem*
enumitem_new (PyTypeObject *type,
              PyObject *args,
              PyObject *kwds)
{
    int val = 0;
    PyObject *name = NULL;
    static char *kwlist[] = { "value", "name", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "iS", kwlist, &val, &name))
        return(NULL);

    zebraEnumItem *self = (zebraEnumItem*)type->tp_alloc(type, 0);
    if(!self)
        return(NULL);

    self->val.ob_ival = val;
    self->name = name;
    return(self);
}

static void
enumitem_dealloc (zebraEnumItem *self)
{
    Py_CLEAR(self->name);
    ((PyObject*)self)->ob_type->tp_free((PyObject*)self);
}

static PyObject*
enumitem_str (zebraEnumItem *self)
{
    Py_INCREF(self->name);
    return(self->name);
}

static int
enumitem_print (zebraEnumItem *self,
                FILE *fp,
                int flags)
{
    return(self->name->ob_type->tp_print(self->name, fp, flags));
}

static PyObject*
enumitem_repr (zebraEnumItem *self)
{
    PyObject *name = PyObject_Repr(self->name);
    if(!name)
        return(NULL);
    char *namestr = PyString_AsString(name);
    PyObject *repr =
        PyString_FromFormat("%s(%ld, %s)",
                            ((PyObject*)self)->ob_type->tp_name,
                            self->val.ob_ival, namestr);
    Py_DECREF(name);
    return((PyObject*)repr);
}

PyTypeObject zebraEnumItem_Type = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "zebra.EnumItem",
    .tp_doc         = enumitem_doc,
    .tp_basicsize   = sizeof(zebraEnumItem),
    .tp_flags       = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new         = (newfunc)enumitem_new,
    .tp_dealloc     = (destructor)enumitem_dealloc,
    .tp_str         = (reprfunc)enumitem_str,
    .tp_print       = (printfunc)enumitem_print,
    .tp_repr        = (reprfunc)enumitem_repr,
};


zebraEnumItem*
zebraEnumItem_New (PyObject *byname,
                   PyObject *byvalue,
                   int val,
                   const char *name)
{
    zebraEnumItem *self = PyObject_New(zebraEnumItem, &zebraEnumItem_Type);
    if(!self)
        return(NULL);
    self->val.ob_ival = val;
    self->name = PyString_FromString(name);
    if(!self->name ||
       (byname && PyDict_SetItem(byname, self->name, (PyObject*)self)) ||
       (byvalue && PyDict_SetItem(byvalue, (PyObject*)self, (PyObject*)self))) {
        Py_DECREF((PyObject*)self);
        return(NULL);
    }
    return(self);
}


static char enum_doc[] = PyDoc_STR(
    "enumeration container for EnumItems.\n"
    "\n"
    "exposes items as read-only attributes");

/* FIXME add iteration */

static int
enum_traverse (zebraEnum *self,
               visitproc visit,
               void *arg)
{
    Py_VISIT(self->byname);
    Py_VISIT(self->byvalue);
    return(0);
}

static int
enum_clear (zebraEnum *self)
{
    Py_CLEAR(self->byname);
    Py_CLEAR(self->byvalue);
    return(0);
}

static void
enum_dealloc (zebraEnum *self)
{
    enum_clear(self);
    ((PyObject*)self)->ob_type->tp_free((PyObject*)self);
}

PyTypeObject zebraEnum_Type = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "zebra.Enum",
    .tp_doc         = enum_doc,
    .tp_basicsize   = sizeof(zebraEnum),
    .tp_flags       = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                      Py_TPFLAGS_HAVE_GC,
    .tp_dictoffset  = offsetof(zebraEnum, byname),
    .tp_traverse    = (traverseproc)enum_traverse,
    .tp_clear       = (inquiry)enum_clear,
    .tp_dealloc     = (destructor)enum_dealloc,
};


zebraEnum*
zebraEnum_New ()
{
    zebraEnum *self = PyObject_GC_New(zebraEnum, &zebraEnum_Type);
    if(!self)
        return(NULL);
    self->byname = PyDict_New();
    self->byvalue = PyDict_New();
    if(!self->byname || !self->byvalue) {
        Py_DECREF(self);
        return(NULL);
    }
    return(self);
}

int
zebraEnum_Add (zebraEnum *self,
               int val,
               const char *name)
{
    zebraEnumItem *item;
    item = zebraEnumItem_New(self->byname, self->byvalue, val, name);
    if(!item)
        return(-1);
    return(0);
}
