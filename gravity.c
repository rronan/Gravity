#include <Python.h>
#define NPY_NO_DEPRECATED_API NPY_1_8_API_VERSION
#define PY_ARRAY_UNIQUE_SYMBOL NP_ARRAY_API
#include <numpy/arrayobject.h>

struct Body {
    double* x;
    double* y;
    double* z;
    double* vx;
    double* vy;
    double* vz;
};

void setSpace(struct Body *bodies[], PyArrayObject *space_arr, unsigned long nbodies){
    for (unsigned long i=0;i<nbodies;i++){
        bodies[i] = (struct Body*)malloc(sizeof(struct Body));
        bodies[i]->x = (double*)PyArray_GETPTR3(space_arr,i,0,0);
        bodies[i]->y = (double*)PyArray_GETPTR3(space_arr,i,1,0);
        bodies[i]->z = (double*)PyArray_GETPTR3(space_arr,i,2,0);
        bodies[i]->vx = (double*)PyArray_GETPTR3(space_arr,i,0,1);
        bodies[i]->vy = (double*)PyArray_GETPTR3(space_arr,i,1,1);
        bodies[i]->vz = (double*)PyArray_GETPTR3(space_arr,i,2,1);
    }
}

void forwardGravitation(struct Body *a, struct Body *b, double G, double DT, double DAMPING, double SOFTENING) {
    double px = pow(*a->x - *b->x, 2);
    double py = pow(*a->y - *b->y, 2);
    double pz = pow(*a->z - *b->z, 2);
    double r = pow(px + py + pz + SOFTENING, .5);
    double f = G / (r * r);
    *a->vx = (*a->vx + DT * f * (*b->x - *a->x) / r) * DAMPING;
    *a->vy = (*a->vy + DT * f * (*b->y - *a->y) / r) * DAMPING;
    *a->vz = (*a->vz + DT * f * (*b->z - *a->z) / r) * DAMPING;
}

void forwardPhysics(struct Body *bodies[], unsigned long nbodies, double G, double DT, double DAMPING, double SOFTENING) {
    for (unsigned long i = 0; i < nbodies; i++) {
        for (unsigned long j = 0; j < nbodies; j++) {
            if (i != j) forwardGravitation(bodies[i], bodies[j], G, DT, DAMPING, SOFTENING);
        }
        *bodies[i]->x += DT * (*bodies[i]->vx);
        *bodies[i]->y += DT * (*bodies[i]->vy);
        *bodies[i]->z += DT * (*bodies[i]->vz);
    };
}

static PyObject * run(PyObject* Py_UNUSED(self), PyObject* args) {
    PyObject *space_object;
    unsigned long NSTEPS;
    double G, DT, DAMPING, SOFTENING;
    if (!PyArg_ParseTuple(args, "Oldddd", &space_object, &NSTEPS, &G, &DT, &DAMPING, &SOFTENING)) return NULL;
    PyArrayObject *space_arr;
    space_arr = (PyArrayObject *) PyArray_ContiguousFromObject(space_object, NPY_DOUBLE, 0, 0);
    unsigned long nbodies = PyArray_DIMS(space_arr)[0];
    struct Body *bodies[nbodies];
    setSpace(bodies, space_arr, nbodies);
    clock_t t;
    t = clock();
    for (unsigned long i = 0; i < NSTEPS; i++) {
        forwardPhysics(bodies, nbodies, G, DT, DAMPING, SOFTENING);
        printf("%ld\n", i);
    }
    t = clock() - t;
    double time_taken = ((double)t)/CLOCKS_PER_SEC;
    printf("C: %f seconds to execute", time_taken);
    return Py_None;
}

static PyMethodDef module_methods[] = {{"run", run, METH_VARARGS, NULL}, {0, 0}};

static struct PyModuleDef gravity = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_gravity",
    .m_methods = module_methods
};

PyMODINIT_FUNC PyInit__gravity(void) {
    Py_Initialize();
    import_array();
    return PyModule_Create(&gravity);
}
