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

static inline PyObject*
exc_get_message (zbarException *self,
                 void *closure)
{
    PyBaseExceptionObject *super = (PyBaseExceptionObject*)self;
    if(!PyString_Size(super->message)) {
        Py_CLEAR(super->message);
        if(!self->obj || !zbarProcessor_Check(self->obj))
            super->message = PyString_FromString("unknown zbar error");
        else {
            const void *zobj = ((zbarProcessor*)self->obj)->zproc;
            super->message = PyString_FromString(_zbar_error_string(zobj, 1));
        }
    }
    Py_INCREF(super->message);
    return(super->message);
}

static int
exc_init (zbarException *self,
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
exc_traverse (zbarException *self,
              visitproc visit,
              void *arg)
{
    Py_VISIT(self->obj);
    PyTypeObject *base = (PyTypeObject*)PyExc_Exception;
    return(base->tp_traverse((PyObject*)self, visit, arg));
}

static int
exc_clear (zbarException *self)
{
    Py_CLEAR(self->obj);
    ((PyTypeObject*)PyExc_Exception)->tp_clear((PyObject*)self);
    return(0);
}

static void
exc_dealloc (zbarException *self)
{
    exc_clear(self);
    ((PyTypeObject*)PyExc_Exception)->tp_dealloc((PyObject*)self);
}

static PyObject*
exc_str (zbarException *self)
{
    return(exc_get_message(self, NULL));
}

static int
exc_set_message (zbarException *self,
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

PyTypeObject zbarException_Type = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "zbar.Exception",
    .tp_basicsize   = sizeof(zbarException),
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
zbarErr_Set (PyObject *self)
{
    const void *zobj = ((zbarProcessor*)self)->zproc;
    zbar_error_t err = _zbar_get_error_code(zobj);

    if(err == ZBAR_ERR_NOMEM)
        PyErr_NoMemory();
    else if(err < ZBAR_ERR_NUM) {
        PyObject *type = zbar_exc[err];
        assert(type);
        PyErr_SetObject(type, self);
    }
    else
        PyErr_SetObject(zbar_exc[0], self);
    return(NULL);
}
