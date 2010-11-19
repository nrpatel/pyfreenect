#include "pyfreenect.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <numpy/arrayobject.h>
#include "freenect_internal.h"

typedef struct {
    PyObject_HEAD
    freenect_context *f_ctx;
    freenect_device *f_dev;
    pthread_mutex_t *rgb_lock;
    pthread_mutex_t *depth_lock;
    uint32_t rgb_time;
    uint32_t depth_time;
    freenect_pixel *rgb;
    freenect_depth *depth;
} Freenect;

void rgb_cb(freenect_device *dev, freenect_pixel *rgb, uint32_t timestamp)
{
    Freenect *self = (Freenect *)dev->user_data;
    pthread_mutex_lock(self->rgb_lock);
    self->rgb_time = timestamp;
    memcpy(self->rgb, rgb, FREENECT_RGB_SIZE);
    pthread_mutex_unlock(self->rgb_lock);
}

void depth_cb(freenect_device *dev, freenect_depth *depth, uint32_t timestamp)
{
    Freenect *self = (Freenect *)dev->user_data;
    pthread_mutex_lock(self->depth_lock);
    self->depth_time = timestamp;
    memcpy(self->depth, depth, FREENECT_DEPTH_SIZE);
    pthread_mutex_unlock(self->depth_lock);
}

static void pyfreenect_dealloc(Freenect* self)
{
    pthread_mutex_destroy(self->rgb_lock);
    pthread_mutex_destroy(self->depth_lock);
    free(self->rgb);
    free(self->depth);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *pyfreenect_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Freenect *self;
    
    self = (Freenect *)type->tp_alloc(type, 0);
    if (self != NULL) {
        
    }
    return (PyObject *)self;
}

static int pyfreenect_init(Freenect *self, PyObject *args, PyObject *kwds)
{
	printf("Kinect camera test\n");

	if (freenect_init(&self->f_ctx, NULL) < 0) {
		printf("freenect_init() failed\n");
		return -1;
	}

	if (freenect_open_device(self->f_ctx, &self->f_dev, 0) < 0) {
		printf("Could not open device\n");
		return -1;
	}
	
	self->rgb = (freenect_pixel *)malloc(FREENECT_RGB_SIZE);
	if (!self->rgb) {
	    printf("Could not allocate buffer\n");
	    return -1;
	}
	
	self->depth = (freenect_depth *)malloc(FREENECT_DEPTH_SIZE);
	if (!self->depth) {
	    printf("Could not allocate buffer\n");
	    return -1;
	}
	
	self->rgb_time = self->depth_time = 0;
	
	pthread_mutex_init(self->rgb_lock, NULL);
	pthread_mutex_init(self->depth_lock, NULL);
	
	freenect_set_user(self->f_dev, self);
	freenect_set_depth_callback(self->f_dev, depth_cb);
	freenect_set_rgb_callback(self->f_dev, rgb_cb);
	freenect_set_rgb_format(self->f_dev, FREENECT_FORMAT_RGB);
	freenect_start_depth(self->f_dev);
	freenect_start_rgb(self->f_dev);

    return 0;
}

PyObject* freenect_rgbimage(Freenect *self)
{
    npy_intp dims[3] = {FREENECT_FRAME_W, FREENECT_FRAME_H, 3};
    PyArrayObject* rgb_array = PyArray_SimpleNew(3, dims, NPY_UBYTE);
    pthread_mutex_lock(self->rgb_lock);
    memcpy(PyArray_BYTES(rgb_array), self->rgb, FREENECT_RGB_SIZE);
    pthread_mutex_unlock(self->rgb_lock);
    return PyArray_Return(rgb_array);
}

PyObject* freenect_depthimage(Freenect *self)
{
    npy_intp dims[2] = {FREENECT_FRAME_W, FREENECT_FRAME_H};
    PyArrayObject* depth_array = PyArray_SimpleNew(2, dims, NPY_USHORT);
    pthread_mutex_lock(self->depth_lock);
    memcpy(PyArray_BYTES(depth_array), self->depth, FREENECT_DEPTH_SIZE);
    pthread_mutex_unlock(self->depth_lock);
    return PyArray_Return(depth_array);
}

static PyMethodDef pyfreenect_methods[] = {
    {"rgb", (PyCFunction)freenect_rgbimage, METH_NOARGS, "Returns RGB Numpy array."},
    {"depth", (PyCFunction)freenect_depthimage, METH_NOARGS, "Returns Depth Numpy array."},
    {NULL}  /* Sentinel */
};

static PyTypeObject PyFreenectType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pyfreenect.Freenect",     /*tp_name*/
    sizeof(Freenect),          /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)pyfreenect_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "PyFreenect",              /* tp_doc */
    0,		                   /* tp_traverse */
    0,		                   /* tp_clear */
    0,		                   /* tp_richcompare */
    0,		                   /* tp_weaklistoffset */
    0,		                   /* tp_iter */
    0,		                   /* tp_iternext */
    pyfreenect_methods,        /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)pyfreenect_init, /* tp_init */
    0,                         /* tp_alloc */
    pyfreenect_new,            /* tp_new */
};

static PyMethodDef module_methods[] = {
    {NULL}  /* Sentinel */
};

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initpyfreenect(void) 
{
    PyObject* m;

    if (PyType_Ready(&PyFreenectType) < 0)
        return;

    m = Py_InitModule3("pyfreenect", module_methods,
                       "Python wrapper for libfreenect.");

    if (m == NULL)
      return;

    Py_INCREF(&PyFreenectType);
    PyModule_AddObject(m, "Freenect", (PyObject *)&PyFreenectType);
}

