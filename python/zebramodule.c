#include "zebramodule.h"

static char *exc_names[] = {
    "zebra.Exception",
    NULL,
    "zebra.InternalError",
    "zebra.UnsupportedError",
    "zebra.InvalidRequestError",
    "zebra.SystemError",
    "zebra.LockingError",
    "zebra.BusyError",
    "zebra.X11DisplayError",
    "zebra.X11ProtocolError",
    "zebra.WindowClosed",
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

PyObject *zebra_exc[ZEBRA_ERR_NUM];
zebraEnumItem *color_enum[2];
zebraEnum *config_enum;
PyObject *symbol_enum;
zebraEnumItem *symbol_NONE;

static PyObject*
version (PyObject *self,
         PyObject *args)
{
    if(!PyArg_ParseTuple(args, ""))
        return(NULL);

    unsigned int major, minor;
    zebra_version(&major, &minor);

    return(Py_BuildValue("II", major, minor));
}

static PyObject*
set_verbosity (PyObject *self,
               PyObject *args)
{
    int verbosity;
    if(!PyArg_ParseTuple(args, "i", &verbosity))
        return(NULL);

    zebra_set_verbosity(verbosity);

    Py_INCREF(Py_None);
    return(Py_None);
}

static PyObject*
increase_verbosity (PyObject *self,
                    PyObject *args)
{
    if(!PyArg_ParseTuple(args, ""))
        return(NULL);

    zebra_increase_verbosity();

    Py_INCREF(Py_None);
    return(Py_None);
}

static PyMethodDef zebra_functions[] = {
    { "version",            version,            METH_VARARGS, NULL },
    { "set_verbosity",      set_verbosity,      METH_VARARGS, NULL },
    { "increase_verbosity", increase_verbosity, METH_VARARGS, NULL },
    { NULL, },
};

PyMODINIT_FUNC
initzebra ()
{
    /* initialize constant containers */
    config_enum = zebraEnum_New();
    symbol_enum = PyDict_New();
    if(!config_enum || !symbol_enum)
        return;

    /* initialize types */
    zebraEnumItem_Type.tp_base = &PyInt_Type;
    zebraException_Type.tp_base = (PyTypeObject*)PyExc_Exception;

    if(PyType_Ready(&zebraException_Type) < 0 ||
       PyType_Ready(&zebraEnumItem_Type) < 0 ||
       PyType_Ready(&zebraEnum_Type) < 0 ||
       PyType_Ready(&zebraImage_Type) < 0 ||
       PyType_Ready(&zebraSymbol_Type) < 0 ||
       PyType_Ready(&zebraSymbolIter_Type) < 0 ||
       PyType_Ready(&zebraProcessor_Type) < 0 ||
       PyType_Ready(&zebraImageScanner_Type) < 0 ||
       PyType_Ready(&zebraDecoder_Type) < 0 ||
       PyType_Ready(&zebraScanner_Type) < 0)
        return;

    zebra_exc[0] = (PyObject*)&zebraException_Type;
    zebra_exc[ZEBRA_ERR_NOMEM] = NULL;
    zebra_error_t ei;
    for(ei = ZEBRA_ERR_INTERNAL; ei < ZEBRA_ERR_NUM; ei++) {
        zebra_exc[ei] = PyErr_NewException(exc_names[ei], zebra_exc[0], NULL);
        if(!zebra_exc[ei])
            return;
    }

    /* internally created/read-only type overrides */
    zebraEnum_Type.tp_new = NULL;
    zebraEnum_Type.tp_setattr = NULL;
    zebraEnum_Type.tp_setattro = NULL;

    /* initialize module */
    PyObject *mod = Py_InitModule("zebra", zebra_functions);
    if(!mod)
        return;

    /* add types to module */
    PyModule_AddObject(mod, "EnumItem", (PyObject*)&zebraEnumItem_Type);
    PyModule_AddObject(mod, "Image", (PyObject*)&zebraImage_Type);
    PyModule_AddObject(mod, "Config", (PyObject*)config_enum);
    PyModule_AddObject(mod, "Symbol", (PyObject*)&zebraSymbol_Type);
    PyModule_AddObject(mod, "SymbolIter", (PyObject*)&zebraSymbolIter_Type);
    PyModule_AddObject(mod, "Processor", (PyObject*)&zebraProcessor_Type);
    PyModule_AddObject(mod, "ImageScanner", (PyObject*)&zebraImageScanner_Type);
    PyModule_AddObject(mod, "Decoder", (PyObject*)&zebraDecoder_Type);
    PyModule_AddObject(mod, "Scanner", (PyObject*)&zebraScanner_Type);

    for(ei = 0; ei < ZEBRA_ERR_NUM; ei++)
        if(zebra_exc[ei])
            PyModule_AddObject(mod, exc_names[ei] + 6, zebra_exc[ei]);

    /* add constants */
    PyObject *dict = PyModule_GetDict(mod);
    color_enum[ZEBRA_SPACE] =
        zebraEnumItem_New(dict, NULL, ZEBRA_SPACE, "SPACE");
    color_enum[ZEBRA_BAR] =
        zebraEnumItem_New(dict, NULL, ZEBRA_BAR, "BAR");

    zebraEnum_Add(config_enum, ZEBRA_CFG_ENABLE,     "ENABLE");
    zebraEnum_Add(config_enum, ZEBRA_CFG_ADD_CHECK,  "ADD_CHECK");
    zebraEnum_Add(config_enum, ZEBRA_CFG_EMIT_CHECK, "EMIT_CHECK");
    zebraEnum_Add(config_enum, ZEBRA_CFG_ASCII,      "ASCII");
    zebraEnum_Add(config_enum, ZEBRA_CFG_MIN_LEN,    "MIN_LEN");
    zebraEnum_Add(config_enum, ZEBRA_CFG_MAX_LEN,    "MAX_LEN");
    zebraEnum_Add(config_enum, ZEBRA_CFG_X_DENSITY,  "X_DENSITY");
    zebraEnum_Add(config_enum, ZEBRA_CFG_Y_DENSITY,  "Y_DENSITY");

    PyObject *tp_dict = zebraSymbol_Type.tp_dict;
    symbol_NONE =
        zebraEnumItem_New(tp_dict, symbol_enum, ZEBRA_NONE,    "NONE");
    zebraEnumItem_New(tp_dict, symbol_enum, ZEBRA_PARTIAL, "PARTIAL");
    zebraEnumItem_New(tp_dict, symbol_enum, ZEBRA_EAN8,    "EAN8");
    zebraEnumItem_New(tp_dict, symbol_enum, ZEBRA_UPCE,    "UPCE");
    zebraEnumItem_New(tp_dict, symbol_enum, ZEBRA_ISBN10,  "ISBN10");
    zebraEnumItem_New(tp_dict, symbol_enum, ZEBRA_UPCA,    "UPCA");
    zebraEnumItem_New(tp_dict, symbol_enum, ZEBRA_EAN13,   "EAN13");
    zebraEnumItem_New(tp_dict, symbol_enum, ZEBRA_ISBN13,  "ISBN13");
    zebraEnumItem_New(tp_dict, symbol_enum, ZEBRA_I25,     "I25");
    zebraEnumItem_New(tp_dict, symbol_enum, ZEBRA_CODE39,  "CODE39");
    zebraEnumItem_New(tp_dict, symbol_enum, ZEBRA_PDF417,  "PDF417");
    zebraEnumItem_New(tp_dict, symbol_enum, ZEBRA_CODE128, "CODE128");
}
