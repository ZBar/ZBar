#include "zebramodule.h"

static inline PyObject*
exc_get_message (zebraException *self,
                 void *closure)
{
    PyBaseExceptionObject *super = (PyBaseExceptionObject*)self;
    if(!PyString_Size(super->message)) {
        Py_CLEAR(super->message);
        if(!self->obj || !zebraProcessor_Check(self->obj))
            super->message = PyString_FromString("unknown zebra error");
        else {
            const void *zobj = ((zebraProcessor*)self->obj)->zproc;
            super->message = PyString_FromString(_zebra_error_string(zobj, 1));
        }
    }
    Py_INCREF(super->message);
    return(super->message);
}

static int
exc_init (zebraException *self,
          PyObject *args,
          PyObject *kwds)
{
    if(!_PyArg_NoKeywords(self->base.ob_type->tp_name, kwds))
        return(-1);
    PyBaseExceptionObject *super = (PyBaseExceptionObject*)self;
    Py_CLEAR(super->args);
    Py_INCREF(args);
    super->args = args;

    if(PyTuple_GET_SIZE(args) == 1) {
        Py_CLEAR(self->obj);
        self->obj = PyTuple_GET_ITEM(args, 0);
        Py_INCREF(self->obj);
    }
    return(0);
}

static int
exc_traverse (zebraException *self,
              visitproc visit,
              void *arg)
{
    Py_VISIT(self->obj);
    PyTypeObject *base = (PyTypeObject*)PyExc_Exception;
    return(base->tp_traverse((PyObject*)self, visit, arg));
}

static int
exc_clear (zebraException *self)
{
    Py_CLEAR(self->obj);
    ((PyTypeObject*)PyExc_Exception)->tp_clear((PyObject*)self);
    return(0);
}

static void
exc_dealloc (zebraException *self)
{
    exc_clear(self);
    ((PyTypeObject*)PyExc_Exception)->tp_dealloc((PyObject*)self);
}

static PyObject*
exc_str (zebraException *self)
{
    return(exc_get_message(self, NULL));
}

static int
exc_set_message (zebraException *self,
                 PyObject *value,
                 void *closure)
{
    PyBaseExceptionObject *super = (PyBaseExceptionObject*)self;
    Py_CLEAR(super->message);
    if(!value)
        value = PyString_FromString("");
    else
        Py_INCREF(value);
    super->message = value;
    return(0);
}

static PyGetSetDef exc_getset[] = {
    { "message", (getter)exc_get_message, (setter)exc_set_message, },
    { NULL, },
};

PyTypeObject zebraException_Type = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "zebra.Exception",
    .tp_basicsize   = sizeof(zebraException),
    .tp_flags       = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                      Py_TPFLAGS_HAVE_GC,
    .tp_init        = (initproc)exc_init,
    .tp_traverse    = (traverseproc)exc_traverse,
    .tp_clear       = (inquiry)exc_clear,
    .tp_dealloc     = (destructor)exc_dealloc,
    .tp_str         = (reprfunc)exc_str,
    .tp_getset      = exc_getset,
};

PyObject*
zebraErr_Set (PyObject *self)
{
    const void *zobj = ((zebraProcessor*)self)->zproc;
    zebra_error_t err = _zebra_get_error_code(zobj);

    if(err == ZEBRA_ERR_NOMEM)
        PyErr_NoMemory();
    else if(err < ZEBRA_ERR_NUM) {
        PyObject *type = zebra_exc[err];
        assert(type);
        PyErr_SetObject(type, self);
    }
    else
        PyErr_SetObject(zebra_exc[0], self);
    return(NULL);
}
