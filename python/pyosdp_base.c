#include "pyosdp.h"

#define TAG "pyosdp_base"

int pyosdp_log_fn(const char *fmt, ...)
{
	int len;
	va_list args;

	va_start(args, fmt);
	len = vfprintf(stderr, fmt, args);
	va_end(args);

	return len;
}

#define pyosdp_set_loglevel_doc                                                \
	"Set OSDP logging level\n"                                             \
	"\n"                                                                   \
	"@param log_level OSDP log level (0 to 7)\n"                           \
	"\n"                                                                   \
	"@return None"
static PyObject *pyosdp_set_loglevel(pyosdp_base_t *self, PyObject *args)
{
	int log_level;

	if (!PyArg_ParseTuple(args, "I", &log_level))
		return NULL;

	if (log_level < OSDP_LOG_EMERG ||
	    log_level > OSDP_LOG_MAX_LEVEL) {
		PyErr_SetString(PyExc_KeyError, "invalid log level");
		return NULL;
	}

	osdp_logger_init(log_level, pyosdp_log_fn);

	Py_RETURN_NONE;
}

PyObject *pyosdp_get_version(pyosdp_base_t *self, PyObject *args)
{
	const char *version;
	PyObject *obj;

	version = osdp_get_version();
	obj = Py_BuildValue("s", version);
	if (obj == NULL)
		return NULL;
	Py_INCREF(obj);
	return obj;
}

PyObject *pyosdp_get_source_info(pyosdp_base_t *self, PyObject *args)
{
	const char *info;
	PyObject *obj;

	info = osdp_get_source_info();
	obj = Py_BuildValue("s", info);
	if (obj == NULL)
		return NULL;
	Py_INCREF(obj);
	return obj;
}

static void pyosdp_base_tp_dealloc(pyosdp_base_t *self)
{
	channel_manager_teardown(&self->channel_manager);
}

static int pyosdp_base_tp_init(pyosdp_base_t *self, PyObject *args, PyObject *kwds)
{
	channel_manager_init(&self->channel_manager);
	osdp_logger_init(OSDP_LOG_INFO, pyosdp_log_fn);
	return 0;
}

static PyMethodDef pyosdp_base_methods[] = {
	{ "get_version", (PyCFunction)pyosdp_get_version, METH_NOARGS,
	  "Get OSDP version as string" },
	{ "get_source_info", (PyCFunction)pyosdp_get_source_info, METH_NOARGS,
	  "Get LibOSDP source info string" },
	{ "set_loglevel", (PyCFunction)pyosdp_set_loglevel, METH_VARARGS,
	  pyosdp_set_loglevel_doc },
	{ NULL } /* Sentinel */
};

PyTypeObject OSDPBaseType = {
	PyVarObject_HEAD_INIT(NULL, 0).tp_name = "osdp.Base",
	.tp_doc = "OSDP Base Class",
	.tp_basicsize = sizeof(pyosdp_base_t),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
	.tp_init = (initproc)pyosdp_base_tp_init,
	.tp_dealloc = (destructor)pyosdp_base_tp_dealloc,
	.tp_methods = pyosdp_base_methods,
};

int pyosdp_add_type_osdp_base(PyObject *module)
{
	return pyosdp_module_add_type(module, "_OSDPBaseClass", &OSDPBaseType);
}