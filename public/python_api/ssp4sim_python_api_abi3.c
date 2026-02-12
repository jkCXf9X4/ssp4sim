#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "simulator_c_api.h"

typedef struct
{
    PyObject_HEAD
    ssp4sim_simulator_handle *handle;
} PySsp4simSimulator;

static int raise_status_error(const int status)
{
    const char *msg = ssp4sim_last_error();
    if (msg == NULL || msg[0] == '\0')
    {
        msg = "Unknown simulator error";
    }

    if (status == SSP4SIM_STATUS_INVALID_ARGUMENT)
    {
        PyErr_SetString(PyExc_ValueError, msg);
        return -1;
    }

    if (status == SSP4SIM_STATUS_OUT_OF_MEMORY)
    {
        PyErr_SetString(PyExc_MemoryError, msg);
        return -1;
    }

    PyErr_SetString(PyExc_RuntimeError, msg);
    return -1;
}

static int PySsp4simSimulator_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"config_path", NULL};
    const char *config_path = NULL;
    PySsp4simSimulator *sim = (PySsp4simSimulator *)self;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &config_path))
    {
        return -1;
    }

    if (sim->handle != NULL)
    {
        ssp4sim_simulator_destroy(sim->handle);
        sim->handle = NULL;
    }

    const int status = ssp4sim_simulator_create(config_path, &sim->handle);
    if (status != SSP4SIM_STATUS_OK)
    {
        return raise_status_error(status);
    }

    return 0;
}

static void PySsp4simSimulator_dealloc(PyObject *self)
{
    PySsp4simSimulator *sim = (PySsp4simSimulator *)self;
    if (sim->handle != NULL)
    {
        ssp4sim_simulator_destroy(sim->handle);
        sim->handle = NULL;
    }
    freefunc tp_free = (freefunc)PyType_GetSlot(Py_TYPE(self), Py_tp_free);
    if (tp_free != NULL)
    {
        tp_free(self);
    }
    else
    {
        PyObject_Free(self);
    }
}

static PyObject *PySsp4simSimulator_init_method(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PySsp4simSimulator *sim = (PySsp4simSimulator *)self;
    const int status = ssp4sim_simulator_init(sim->handle);
    if (status != SSP4SIM_STATUS_OK)
    {
        return raise_status_error(status), NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *PySsp4simSimulator_simulate_method(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PySsp4simSimulator *sim = (PySsp4simSimulator *)self;
    const int status = ssp4sim_simulator_simulate(sim->handle);
    if (status != SSP4SIM_STATUS_OK)
    {
        return raise_status_error(status), NULL;
    }

    Py_RETURN_NONE;
}

static PyMethodDef PySsp4simSimulator_methods[] = {
    {
        "init",
        (PyCFunction)PySsp4simSimulator_init_method,
        METH_NOARGS,
        PyDoc_STR("Initialize the simulator.")
    },
    {
        "simulate",
        (PyCFunction)PySsp4simSimulator_simulate_method,
        METH_NOARGS,
        PyDoc_STR("Run the simulation.")
    },
    {NULL, NULL, 0, NULL}
};

static PyType_Slot PySsp4simSimulator_slots[] = {
    {Py_tp_doc, "SSP4SIM simulator wrapper."},
    {Py_tp_new, (void *)PyType_GenericNew},
    {Py_tp_init, (void *)PySsp4simSimulator_init},
    {Py_tp_dealloc, (void *)PySsp4simSimulator_dealloc},
    {Py_tp_methods, (void *)PySsp4simSimulator_methods},
    {0, NULL}
};

static PyType_Spec PySsp4simSimulator_spec = {
    "py_ssp4sim.Simulator",
    sizeof(PySsp4simSimulator),
    0,
    Py_TPFLAGS_DEFAULT,
    PySsp4simSimulator_slots
};

static PyModuleDef module_def = {
    PyModuleDef_HEAD_INIT,
    "py_ssp4sim",
    "Python Limited API bindings for ssp4sim.",
    -1,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC PyInit_py_ssp4sim(void)
{
    PyObject *module = PyModule_Create(&module_def);
    if (module == NULL)
    {
        return NULL;
    }

    PyObject *sim_type = PyType_FromSpec(&PySsp4simSimulator_spec);
    if (sim_type == NULL)
    {
        Py_DECREF(module);
        return NULL;
    }

    if (PyModule_AddObject(module, "Simulator", sim_type) < 0)
    {
        Py_DECREF(sim_type);
        Py_DECREF(module);
        return NULL;
    }

    return module;
}
