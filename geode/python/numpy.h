//#####################################################################
// Numpy interface functions
//#####################################################################
#pragma once

#include <geode/python/config.h>
#include <geode/utility/config.h>

#ifdef GEODE_PYTHON
#define PY_ARRAY_UNIQUE_SYMBOL _try_python_array_api
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#ifndef GEODE_IMPORT_NUMPY
#define NO_IMPORT_ARRAY
#endif
#include <numpy/arrayobject.h>
#endif

#include <geode/python/numpy-types.h>

#include <geode/array/Array.h>
#include <geode/array/IndirectArray.h>
#include <geode/python/exceptions.h>
#include <geode/structure/Tuple.h>
#include <geode/utility/const_cast.h>
namespace geode {

typedef Py_intptr_t npy_intp;

#ifdef GEODE_PYTHON
GEODE_CORE_EXPORT void GEODE_NORETURN(throw_dimension_mismatch());
GEODE_CORE_EXPORT void GEODE_NORETURN(throw_not_owned());
GEODE_CORE_EXPORT void GEODE_NORETURN(throw_array_conversion_error(PyObject* object, int flags, int rank_range, PyArray_Descr* descr));
GEODE_CORE_EXPORT void check_numpy_conversion(PyObject* object, int flags, int rank_range, PyArray_Descr* descr);
#endif
GEODE_CORE_EXPORT Tuple<Array<uint8_t>,size_t> fill_numpy_header(int rank, const npy_intp* dimensions, int type_num); // Returns total data size in bytes
GEODE_CORE_EXPORT void write_numpy(const string& filename, int rank, const npy_intp* dimensions, int type_num, void* data);

// Export wrappers around numpy api functions so that other libraries don't need the PY_ARRAY_UNIQUE_SYMBOL, which can't be portably exported.
#ifdef GEODE_PYTHON
GEODE_CORE_EXPORT bool is_numpy_array(PyObject* o);
GEODE_CORE_EXPORT PyArray_Descr* numpy_descr_from_type(int type_num);
GEODE_CORE_EXPORT Ref<> numpy_from_any(PyObject* op, PyArray_Descr* dtype, int min_rank, int max_rank, int requirements);
GEODE_CORE_EXPORT PyObject* numpy_new_from_descr(PyTypeObject* subtype, PyArray_Descr* descr, int nd, npy_intp* dims, npy_intp* strides, void* data, int flags, PyObject* obj);
GEODE_CORE_EXPORT PyTypeObject* numpy_array_type();
GEODE_CORE_EXPORT PyTypeObject* numpy_recarray_type();
#endif

// Stay compatible with old versions of numpy
#ifndef NPY_ARRAY_WRITEABLE
#define NPY_ARRAY_WRITEABLE NPY_WRITEABLE
#define NPY_ARRAY_FORCECAST NPY_FORCECAST
#define NPY_ARRAY_CARRAY_RO NPY_CARRAY_RO
#define NPY_ARRAY_CARRAY NPY_CARRAY
#define NPY_ARRAY_C_CONTIGUOUS NPY_C_CONTIGUOUS
#endif

// The numpy API changed incompatibly without incrementing the NPY_VERSION define.  Therefore, we use the
// fact that PyArray_BASE switched from being a macro in the old version to an inline function in the new.
#if defined(PyArray_BASE) && !defined(PyArray_CLEARFLAGS)
static inline void PyArray_CLEARFLAGS(PyArrayObject* array,int flags) {
  PyArray_FLAGS(array) &= ~flags;
}
static inline void PyArray_ENABLEFLAGS(PyArrayObject* array,int flags) {
  PyArray_FLAGS(array) |= flags;
}
static inline void PyArray_SetBaseObject(PyArrayObject* array,PyObject* base) {
  PyArray_BASE(array) = base;
}
#endif

// Use an unnamed namespace since a given instantiation of these functions should appear in only one object file
namespace {

// NumpyDescr
#ifdef GEODE_PYTHON
template<class T> struct NumpyDescr{static PyArray_Descr* descr(){return numpy_descr_from_type(NumpyScalar<T>::value);}};
template<class T> struct NumpyDescr<const T>:public NumpyDescr<T>{};
template<class T,int d> struct NumpyDescr<Vector<T,d> >:public NumpyDescr<T>{};
template<class T,int m,int n> struct NumpyDescr<Matrix<T,m,n> >:public NumpyDescr<T>{};
template<class T,int d> struct NumpyDescr<Array<T,d> >:public NumpyDescr<T>{};
template<class T,int d> struct NumpyDescr<RawArray<T,d> >:public NumpyDescr<T>{};
template<class T> struct NumpyDescr<NdArray<T> >:public NumpyDescr<T>{};
#endif

// NumpyArrayType
#ifdef GEODE_PYTHON
template<class T> struct NumpyArrayType{static PyTypeObject* type(){return numpy_array_type();}};
template<class T> struct NumpyArrayType<const T>:public NumpyArrayType<T>{};
template<class T,int d> struct NumpyArrayType<Vector<T,d> >:public NumpyArrayType<T>{};
template<class T,int d> struct NumpyArrayType<Array<T,d> >:public NumpyArrayType<T>{};
template<class T,int d> struct NumpyArrayType<RawArray<T,d> >:public NumpyArrayType<T>{};
template<class T> struct NumpyArrayType<NdArray<T> >:public NumpyArrayType<T>{};
#endif

// Struct NumpyMinRank/NumpyMaxRank: Extract rank of an array type
template<class T,class Enable=void> struct NumpyRank;
template<class T> struct NumpyRank<T,typename enable_if<mpl::and_<NumpyIsScalar<T>,mpl::not_<is_const<T>>>>::type>:public mpl::int_<0>{};
template<class T> struct NumpyRank<const T>:public NumpyRank<T>{};

template<class T,int d> struct NumpyRank<Vector<T,d> >:public mpl::int_<1+NumpyRank<T>::value>{};
template<class T,int m,int n> struct NumpyRank<Matrix<T,m,n> >:public mpl::int_<2+NumpyRank<T>::value>{};
template<class T,int d> struct NumpyRank<Array<T,d> >:public mpl::int_<d+NumpyRank<T>::value>{};
template<class T,int d> struct NumpyRank<RawArray<T,d> >:public mpl::int_<d+NumpyRank<T>::value>{};
template<class T> struct NumpyRank<NdArray<T> >:public mpl::int_<-1-NumpyRank<T>::value>{}; // -r-1 means r or higher

// Extract possibly runtime-variable rank
template<class T> int numpy_rank(const T&) {
  return NumpyRank<T>::value;
}
template<class T> int numpy_rank(const NdArray<T>& array) {
  return array.rank()+NumpyRank<T>::value;
}

// Struct NumpyInfo: Recursively shape information from statically sized types

template<class T> struct NumpyInfo { static void dimensions(npy_intp* dimensions) {
  static_assert(NumpyRank<T>::value==0,"");
}};

template<class T> struct NumpyInfo<const T> : public NumpyInfo<T>{};

template<class T,int d> struct NumpyInfo<Vector<T,d> > { static void dimensions(npy_intp* dimensions) {
  dimensions[0] = d;
  NumpyInfo<T>::dimensions(dimensions+1);
}};

template<class T,int m,int n> struct NumpyInfo<Matrix<T,m,n> > { static void dimensions(npy_intp* dimensions) {
  dimensions[0] = m;
  dimensions[1] = n;
  NumpyInfo<T>::dimensions(dimensions+2);
}};

// Function Numpy_Info: Recursively extract type and shape information from dynamically sized types

template<class TV> typename enable_if<NumpyIsStatic<TV> >::type
numpy_info(const TV& block, void*& data, npy_intp* dimensions) {
  data = const_cast_(&block);
  NumpyInfo<TV>::dimensions(dimensions);
}

template<class T,int d> void
numpy_info(const Array<T,d>& array, void*& data, npy_intp* dimensions) {
  data = (void*)array.data();
  const Vector<npy_intp,d> sizes(array.sizes());
  for (int i=0;i<d;i++) dimensions[i] = sizes[i];
  NumpyInfo<T>::dimensions(dimensions+d);
}

template<class T,int d> void
numpy_info(const RawArray<T,d>& array, void*& data, npy_intp* dimensions) {
  data = (void*)array.data();
  const Vector<npy_intp,d> sizes(array.sizes());
  for (int i=0;i<d;i++) dimensions[i] = sizes[i];
  NumpyInfo<T>::dimensions(dimensions+d);
}

template<class T> void
numpy_info(const NdArray<T>& array, void*& data, npy_intp* dimensions) {
  data = (void*)array.data();
  for (int i=0;i<array.rank();i++) dimensions[i] = array.shape[i];
  NumpyInfo<T>::dimensions(dimensions+array.rank());
}

// Numpy_Shape_Match: Check whether dynamic type can be resized to fit a given numpy array

template<class T> typename enable_if<NumpyIsScalar<T>,bool>::type
numpy_shape_match(Types<T>,int rank,const npy_intp* dimensions) {
  return true;
}

template<class TV> typename enable_if<mpl::and_<NumpyIsStatic<TV>,mpl::not_<NumpyIsScalar<TV> > >,bool>::type
numpy_shape_match(Types<TV>, int rank, const npy_intp* dimensions) {
  if (rank!=NumpyRank<TV>::value) return false;
  npy_intp subdimensions[NumpyRank<TV>::value?NumpyRank<TV>::value:1];
  NumpyInfo<TV>::dimensions(subdimensions);
  for (int i=0;i<NumpyRank<TV>::value;i++) if(dimensions[i]!=subdimensions[i]) return false;
  return true;
}

template<class T> bool
numpy_shape_match(Types<const T>, int rank, const npy_intp* dimensions) {
  return numpy_shape_match(Types<T>(),rank,dimensions);
}

template<class T,int d> bool
numpy_shape_match(Types<Array<T,d> >, int rank, const npy_intp* dimensions) {
  return numpy_shape_match(Types<T>(),rank-d,dimensions+d);
}

template<class T,int d> bool
numpy_shape_match(Types<RawArray<T,d> >, int rank, const npy_intp* dimensions) {
  return numpy_shape_match(Types<T>(),rank-d,dimensions+d);
}

template<class T> bool
numpy_shape_match(Types<NdArray<T> >, int rank, const npy_intp* dimensions) {
  return numpy_shape_match(Types<T>(),NumpyRank<T>::value,dimensions+rank-NumpyRank<T>::value);
}

#ifdef GEODE_PYTHON

// to_numpy for static types
template<class TV> typename enable_if<NumpyIsStatic<TV>,PyObject*>::type
to_numpy(const TV& x) {
  // Extract memory layout information
  const int rank = numpy_rank(x);
  void* data;
  Array<npy_intp> dimensions(rank,uninit);
  numpy_info(x,data,dimensions.data());

  // Make a new numpy array and copy the vector into it
  PyObject* numpy = numpy_new_from_descr(NumpyArrayType<TV>::type(),NumpyDescr<TV>::descr(),rank,dimensions.data(),0,0,0,0);
  if (!numpy) return 0;
  *(TV*)PyArray_DATA((PyArrayObject*)numpy) = x;

  // Mark the array nonconst so users don't expect to be changing the original
  PyArray_CLEARFLAGS((PyArrayObject*)numpy, NPY_ARRAY_WRITEABLE);
  return numpy;
}

// to_numpy for shareable array types
template<class TArray> typename enable_if<IsShareable<TArray>,PyObject*>::type
to_numpy(TArray& array) {
  // Extract memory layout information
  const int rank = numpy_rank(array);
  void* data;
  Array<npy_intp> dimensions(rank,uninit);
  numpy_info(array,data,dimensions.data());

  // Verify ownership
  PyObject* owner = array.owner();
  if (!owner && data)
    throw_not_owned();

  // Wrap the existing array as a numpy array without copying data
  PyObject* numpy = numpy_new_from_descr(NumpyArrayType<TArray>::type(),NumpyDescr<TArray>::descr(),rank,dimensions.data(),0,data,NPY_ARRAY_CARRAY,0);
  if (!numpy) return 0;
  PyArray_ENABLEFLAGS((PyArrayObject*)numpy,NPY_ARRAY_C_CONTIGUOUS);
  if (TArray::is_const) PyArray_CLEARFLAGS((PyArrayObject*)numpy, NPY_ARRAY_WRITEABLE);

  // Let numpy array share ownership with array
  if (owner)
    PyArray_SetBaseObject((PyArrayObject*)numpy, owner);
  return numpy;
}

// from_numpy for static types
template<class TV> typename enable_if<NumpyIsStatic<TV>,TV>::type
from_numpy(PyObject* object) { // Borrows reference to object
  // Allow conversion from 0 to static vector/matrix types
  if(PyInt_Check(object) && !PyInt_AS_LONG(object))
    return TV();

  // Convert object to an array with the correct type and rank
  static const int rank = NumpyRank<TV>::value;
  const auto array = numpy_from_any(object,NumpyDescr<TV>::descr(),rank,rank,NPY_ARRAY_CARRAY_RO|NPY_ARRAY_FORCECAST);

  // Ensure appropriate dimensions
  if (!numpy_shape_match(Types<TV>(),rank,PyArray_DIMS((PyArrayObject*)&*array)))
    throw_dimension_mismatch();

  return *(const TV*)(PyArray_DATA((PyArrayObject*)&*array));
}

// Build an Array<T,d> from a compatible numpy array
template<class T,int d> inline Array<T,d>
from_numpy_helper(Types<Array<T,d>>, PyObject& array) {
  PyObject* base = PyArray_BASE((PyArrayObject*)&array);
  Vector<int,d> counts;
  for (int i=0;i<d;i++){
    counts[i] = (int)PyArray_DIMS((PyArrayObject*)&array)[i];
    GEODE_ASSERT(counts[i]==PyArray_DIMS((PyArrayObject*)&array)[i]);}
  return Array<T,d>(counts,(T*)PyArray_DATA((PyArrayObject*)&array),base?base:&array);
}

// Build an NdArray<T,d> from a compatible numpy array
template<class T> inline NdArray<T>
from_numpy_helper(Types<NdArray<T>>, PyObject& array) {
  PyObject* base = PyArray_BASE((PyArrayObject*)&array);
  Array<int> shape(PyArray_NDIM((PyArrayObject*)&array)-NumpyRank<T>::value,uninit);
  for (int i=0;i<shape.size();i++){
    shape[i] = (int)PyArray_DIMS((PyArrayObject*)&array)[i];
    GEODE_ASSERT(shape[i]==PyArray_DIMS((PyArrayObject*)&array)[i]);}
  return NdArray<T>(shape,(T*)PyArray_DATA((PyArrayObject*)&array),base?base:&array);
}

// from_numpy for shareable arrays
template<class TArray> typename enable_if<IsShareable<TArray>,TArray>::type
from_numpy(PyObject* object) { // Borrows reference to object
  const int flags = TArray::is_const?NPY_ARRAY_CARRAY_RO:NPY_ARRAY_CARRAY;
  const int rank_range = NumpyRank<TArray>::value;
  PyArray_Descr* const descr = NumpyDescr<TArray>::descr();
  const int min_rank = rank_range<0?-rank_range-1:rank_range,max_rank=rank_range<0?100:rank_range;

  if (is_numpy_array(object)) {
    // Already a numpy array: require an exact match to avoid hidden performance issues
    check_numpy_conversion(object,flags,rank_range,descr);
    if (!numpy_shape_match(Types<TArray>(),PyArray_NDIM((PyArrayObject*)object),PyArray_DIMS((PyArrayObject*)object)))
      throw_dimension_mismatch();
    return from_numpy_helper(Types<TArray>(),*object);
  } else if (!TArray::is_const)
    throw_type_error(object,numpy_array_type());

  // If we're converting to a const array, and the input isn't already numpy, allow any matching nested sequence
  const auto array = numpy_from_any(object,descr,min_rank,max_rank,flags);

  // Ensure appropriate dimension
  const int rank = PyArray_NDIM((PyArrayObject*)&*array);
  if (!numpy_shape_match(Types<TArray>(),rank,PyArray_DIMS((PyArrayObject*)&*array)))
    throw_dimension_mismatch();

  return from_numpy_helper(Types<TArray>(),array);
}

#endif

// Write a numpy-convertible array to an .npy file
// Unlike other functions in this file, this is safe to call without initializing either Python or Numpy.
template<class TArray> void
write_numpy(const string& filename, const TArray& array) {
  // Extract memory layout information
  const int rank = numpy_rank(array);
  void* data;
  Array<npy_intp> dimensions(rank,uninit);
  numpy_info(array,data,dimensions.data());
  // Write
  geode::write_numpy(filename,rank,dimensions.data(),NumpyScalar<TArray>::value,data);
}

// Generate an .npy file header for a numpy-convertible array.  Returns header,data_size.
// Unlike other functions in this file, this is safe to call without initializing either Python or Numpy.
template<class TArray> Tuple<Array<uint8_t>,size_t> fill_numpy_header(const TArray& array) {
  // Extract memory layout information
  const int rank = numpy_rank(array);
  void* data;
  Array<npy_intp> dimensions(rank,uninit);
  numpy_info(array,data,dimensions.data());
  // Fill header
  return geode::fill_numpy_header(rank,dimensions.data(),NumpyScalar<TArray>::value);
}

}
}
