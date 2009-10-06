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

static char *exc_names[] = {
    "zbar.Exception",
    NULL,
    "zbar.InternalError",
    "zbar.UnsupportedError",
    "zbar.InvalidRequestError",
    "zbar.SystemError",
    "zbar.LockingError",
    "zbar.BusyError",
    "zbar.X11DisplayError",
    "zbar.X11ProtocolError",
    "zbar.WindowClosed",
    "zbar.WinAPIError",
};

int
object_to_bool (PyObject *obj,
                int *val)
{
    int tmp = PyObject_IsTrue(obj);
    if(tmp < 0)
        return(0);
    *val = tmp;
    return(1);
}

PyObject *zbar_exc[ZBAR_ERR_NUM];
zbarEnumItem *color_enum[2];
zbarEnum *config_enum;
PyObject *symbol_enum;
zbarEnumItem *symbol_NONE;

static PyObject*
version (PyObject *self,
         PyObject *args)
{
    if(!PyArg_ParseTuple(args, ""))
        return(NULL);

    unsigned int major, minor;
    zbar_version(&major, &minor);

    return(Py_BuildValue("II", major, minor));
}

static PyObject*
set_verbosity (PyObject *self,
               PyObject *args)
{
    int verbosity;
    if(!PyArg_ParseTuple(args, "i", &verbosity))
        return(NULL);

    zbar_set_verbosity(verbosity);

    Py_INCREF(Py_None);
    return(Py_None);
}

static PyObject*
increase_verbosity (PyObject *self,
                    PyObject *args)
{
    if(!PyArg_ParseTuple(args, ""))
        return(NULL);

    zbar_increase_verbosity();

    Py_INCREF(Py_None);
    return(Py_None);
}

static PyMethodDef zbar_functions[] = {
    { "version",            version,            METH_VARARGS, NULL },
    { "set_verbosity",      set_verbosity,      METH_VARARGS, NULL },
    { "increase_verbosity", increase_verbosity, METH_VARARGS, NULL },
    { NULL, },
};

PyMODINIT_FUNC
initzbar (void)
{
    /* initialize constant containers */
    config_enum = zbarEnum_New();
    symbol_enum = PyDict_New();
    if(!config_enum || !symbol_enum)
        return;

    /* initialize types */
    zbarEnumItem_Type.tp_base = &PyInt_Type;
    zbarException_Type.tp_base = (PyTypeObject*)PyExc_Exception;

    if(PyType_Ready(&zbarException_Type) < 0 ||
       PyType_Ready(&zbarEnumItem_Type) < 0 ||
       PyType_Ready(&zbarEnum_Type) < 0 ||
       PyType_Ready(&zbarImage_Type) < 0 ||
       PyType_Ready(&zbarSymbol_Type) < 0 ||
       PyType_Ready(&zbarSymbolSet_Type) < 0 ||
       PyType_Ready(&zbarSymbolIter_Type) < 0 ||
       PyType_Ready(&zbarProcessor_Type) < 0 ||
       PyType_Ready(&zbarImageScanner_Type) < 0 ||
       PyType_Ready(&zbarDecoder_Type) < 0 ||
       PyType_Ready(&zbarScanner_Type) < 0)
        return;

    zbar_exc[0] = (PyObject*)&zbarException_Type;
    zbar_exc[ZBAR_ERR_NOMEM] = NULL;
    zbar_error_t ei;
    for(ei = ZBAR_ERR_INTERNAL; ei < ZBAR_ERR_NUM; ei++) {
        zbar_exc[ei] = PyErr_NewException(exc_names[ei], zbar_exc[0], NULL);
        if(!zbar_exc[ei])
            return;
    }

    /* internally created/read-only type overrides */
    zbarEnum_Type.tp_new = NULL;
    zbarEnum_Type.tp_setattr = NULL;
    zbarEnum_Type.tp_setattro = NULL;

    /* initialize module */
    PyObject *mod = Py_InitModule("zbar", zbar_functions);
    if(!mod)
        return;

    /* add types to module */
    PyModule_AddObject(mod, "EnumItem", (PyObject*)&zbarEnumItem_Type);
    PyModule_AddObject(mod, "Image", (PyObject*)&zbarImage_Type);
    PyModule_AddObject(mod, "Config", (PyObject*)config_enum);
    PyModule_AddObject(mod, "Symbol", (PyObject*)&zbarSymbol_Type);
    PyModule_AddObject(mod, "SymbolSet", (PyObject*)&zbarSymbolSet_Type);
    PyModule_AddObject(mod, "SymbolIter", (PyObject*)&zbarSymbolIter_Type);
    PyModule_AddObject(mod, "Processor", (PyObject*)&zbarProcessor_Type);
    PyModule_AddObject(mod, "ImageScanner", (PyObject*)&zbarImageScanner_Type);
    PyModule_AddObject(mod, "Decoder", (PyObject*)&zbarDecoder_Type);
    PyModule_AddObject(mod, "Scanner", (PyObject*)&zbarScanner_Type);

    for(ei = 0; ei < ZBAR_ERR_NUM; ei++)
        if(zbar_exc[ei])
            PyModule_AddObject(mod, exc_names[ei] + 5, zbar_exc[ei]);

    /* add constants */
    PyObject *dict = PyModule_GetDict(mod);
    color_enum[ZBAR_SPACE] =
        zbarEnumItem_New(dict, NULL, ZBAR_SPACE, "SPACE");
    color_enum[ZBAR_BAR] =
        zbarEnumItem_New(dict, NULL, ZBAR_BAR, "BAR");

    zbarEnum_Add(config_enum, ZBAR_CFG_ENABLE,     "ENABLE");
    zbarEnum_Add(config_enum, ZBAR_CFG_ADD_CHECK,  "ADD_CHECK");
    zbarEnum_Add(config_enum, ZBAR_CFG_EMIT_CHECK, "EMIT_CHECK");
    zbarEnum_Add(config_enum, ZBAR_CFG_ASCII,      "ASCII");
    zbarEnum_Add(config_enum, ZBAR_CFG_MIN_LEN,    "MIN_LEN");
    zbarEnum_Add(config_enum, ZBAR_CFG_MAX_LEN,    "MAX_LEN");
    zbarEnum_Add(config_enum, ZBAR_CFG_X_DENSITY,  "POSITION");
    zbarEnum_Add(config_enum, ZBAR_CFG_X_DENSITY,  "X_DENSITY");
    zbarEnum_Add(config_enum, ZBAR_CFG_Y_DENSITY,  "Y_DENSITY");

    PyObject *tp_dict = zbarSymbol_Type.tp_dict;
    symbol_NONE =
        zbarEnumItem_New(tp_dict, symbol_enum, ZBAR_NONE, "NONE");
    zbarEnumItem_New(tp_dict, symbol_enum, ZBAR_PARTIAL, "PARTIAL");
    zbarEnumItem_New(tp_dict, symbol_enum, ZBAR_EAN8,    "EAN8");
    zbarEnumItem_New(tp_dict, symbol_enum, ZBAR_UPCE,    "UPCE");
    zbarEnumItem_New(tp_dict, symbol_enum, ZBAR_ISBN10,  "ISBN10");
    zbarEnumItem_New(tp_dict, symbol_enum, ZBAR_UPCA,    "UPCA");
    zbarEnumItem_New(tp_dict, symbol_enum, ZBAR_EAN13,   "EAN13");
    zbarEnumItem_New(tp_dict, symbol_enum, ZBAR_ISBN13,  "ISBN13");
    zbarEnumItem_New(tp_dict, symbol_enum, ZBAR_I25,     "I25");
    zbarEnumItem_New(tp_dict, symbol_enum, ZBAR_CODE39,  "CODE39");
    zbarEnumItem_New(tp_dict, symbol_enum, ZBAR_PDF417,  "PDF417");
    zbarEnumItem_New(tp_dict, symbol_enum, ZBAR_QRCODE,  "QRCODE");
    zbarEnumItem_New(tp_dict, symbol_enum, ZBAR_CODE128, "CODE128");
}
