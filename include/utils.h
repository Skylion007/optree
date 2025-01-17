/*
Copyright 2022-2023 MetaOPT Team. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
================================================================================
*/

// Caution: this code uses exceptions. The exception use is local to the binding
// code and the idiomatic way to emit Python exceptions.

#pragma once

#include <Python.h>
#include <absl/strings/str_format.h>
#include <absl/strings/str_join.h>
#include <pybind11/pybind11.h>
#include <structmember.h>  // PyMemberDef

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

constexpr bool NONE_IS_LEAF = true;
constexpr bool NONE_IS_NODE = false;

namespace py = pybind11;
using size_t = py::size_t;
using ssize_t = py::ssize_t;

#define PyCollectionsModule (ImportCollections())
#define PyOrderedDictTypeObject (ImportOrderedDict())
#define PyDefaultDictTypeObject (ImportDefaultDict())
#define PyDequeTypeObject (ImportDeque())
#define PyOrderedDict_Type (reinterpret_cast<PyTypeObject*>(PyOrderedDictTypeObject.ptr()))
#define PyDefaultDict_Type (reinterpret_cast<PyTypeObject*>(PyDefaultDictTypeObject.ptr()))
#define PyDeque_Type (reinterpret_cast<PyTypeObject*>(PyDequeTypeObject.ptr()))

inline const py::module& ImportCollections() {
    // NOTE: Use raw pointers to leak the memory intentionally to avoid py::object deallocation and
    //       garbage collection.
    static const py::module* ptr = new py::module{py::module::import("collections")};
    return *ptr;
}
inline const py::object& ImportOrderedDict() {
    // NOTE: Use raw pointers to leak the memory intentionally to avoid py::object deallocation and
    //       garbage collection.
    static const py::object* ptr = new py::object{py::getattr(PyCollectionsModule, "OrderedDict")};
    return *ptr;
}
inline const py::object& ImportDefaultDict() {
    // NOTE: Use raw pointers to leak the memory intentionally to avoid py::object deallocation and
    //       garbage collection.
    static const py::object* ptr = new py::object{py::getattr(PyCollectionsModule, "defaultdict")};
    return *ptr;
}
inline const py::object& ImportDeque() {
    // NOTE: Use raw pointers to leak the memory intentionally to avoid py::object deallocation and
    //       garbage collection.
    static const py::object* ptr = new py::object{py::getattr(PyCollectionsModule, "deque")};
    return *ptr;
}

template <typename T>
inline std::vector<T> reserved_vector(const size_t& size) {
    std::vector<T> v;
    v.reserve(size);
    return v;
}

inline void TotalOrderSort(py::list& list) {  // NOLINT[runtime/references]
    try {
        // Sort directly if possible.
        // NOLINTNEXTLINE[readability-implicit-bool-conversion]
        if (PyList_Sort(list.ptr())) [[unlikely]] {
            throw py::error_already_set();
        }
    } catch (py::error_already_set& ex1) {
        if (ex1.matches(PyExc_TypeError)) [[likely]] {
            // Found incomparable keys (e.g. `int` vs. `str`, or user-defined types).
            try {
                // Sort with `(f'{o.__class__.__module__}.{o.__class__.__qualname__}', o)`
                auto sort_key_fn = py::cpp_function([](const py::object& o) {
                    py::handle t = o.get_type();
                    py::str qualname{absl::StrFormat(
                        "%s.%s",
                        static_cast<std::string>(py::getattr(t, "__module__").cast<py::str>()),
                        static_cast<std::string>(py::getattr(t, "__qualname__").cast<py::str>()))};
                    return py::make_tuple(qualname, o);
                });
                list.attr("sort")(py::arg("key") = sort_key_fn);
            } catch (py::error_already_set& ex2) {
                if (ex2.matches(PyExc_TypeError)) [[likely]] {
                    // Found incomparable user-defined key types.
                    // The keys remain in the insertion order.
                    PyErr_Clear();
                } else [[unlikely]] {
                    throw;
                }
            }
        } else [[unlikely]] {
            throw;
        }
    }
}

inline py::list DictKeys(const py::dict& dict) {
    return py::reinterpret_steal<py::list>(PyDict_Keys(dict.ptr()));
}

inline py::list SortedDictKeys(const py::dict& dict) {
    py::list keys = DictKeys(dict);
    TotalOrderSort(keys);
    return keys;
}

template <typename Sized = py::object>
inline ssize_t GetSize(const py::handle& sized) {
    return py::ssize_t_cast(py::len(sized));
}
template <>
inline ssize_t GetSize<py::tuple>(const py::handle& sized) {
    return PyTuple_Size(sized.ptr());
}
template <>
inline ssize_t GetSize<py::list>(const py::handle& sized) {
    return PyList_Size(sized.ptr());
}
template <>
inline ssize_t GetSize<py::dict>(const py::handle& sized) {
    return PyDict_Size(sized.ptr());
}

template <typename Sized = py::object>
inline ssize_t GET_SIZE(const py::handle& sized) {
    return py::ssize_t_cast(py::len(sized));
}
template <>
inline ssize_t GET_SIZE<py::tuple>(const py::handle& sized) {
    return PyTuple_GET_SIZE(sized.ptr());
}
template <>
inline ssize_t GET_SIZE<py::list>(const py::handle& sized) {
    return PyList_GET_SIZE(sized.ptr());
}
#ifdef PyDict_GET_SIZE
template <>
inline ssize_t GET_SIZE<py::dict>(const py::handle& sized) {
    return PyDict_GET_SIZE(sized.ptr());
}
#else
template <>
inline ssize_t GET_SIZE<py::dict>(const py::handle& sized) {
    return PyDict_Size(sized.ptr());
}
#endif

template <typename Container, typename Item>
inline py::handle GET_ITEM_HANDLE(const py::handle& container, const Item& item) {
    return container[item];
}
template <>
inline py::handle GET_ITEM_HANDLE<py::tuple>(const py::handle& container, const ssize_t& item) {
    return PyTuple_GET_ITEM(container.ptr(), item);
}
template <>
inline py::handle GET_ITEM_HANDLE<py::tuple>(const py::handle& container, const size_t& item) {
    return PyTuple_GET_ITEM(container.ptr(), py::ssize_t_cast(item));
}
template <>
inline py::handle GET_ITEM_HANDLE<py::tuple>(const py::handle& container, const int& item) {
    return PyTuple_GET_ITEM(container.ptr(), py::ssize_t_cast(item));
}
template <>
inline py::handle GET_ITEM_HANDLE<py::list>(const py::handle& container, const ssize_t& item) {
    return PyList_GET_ITEM(container.ptr(), item);
}
template <>
inline py::handle GET_ITEM_HANDLE<py::list>(const py::handle& container, const size_t& item) {
    return PyList_GET_ITEM(container.ptr(), py::ssize_t_cast(item));
}
template <>
inline py::handle GET_ITEM_HANDLE<py::list>(const py::handle& container, const int& item) {
    return PyList_GET_ITEM(container.ptr(), py::ssize_t_cast(item));
}

template <typename Container, typename Item>
inline py::object GET_ITEM_BORROW(const py::handle& container, const Item& item) {
    return container[item];
}
template <>
inline py::object GET_ITEM_BORROW<py::tuple>(const py::handle& container, const ssize_t& item) {
    return py::reinterpret_borrow<py::object>(PyTuple_GET_ITEM(container.ptr(), item));
}
template <>
inline py::object GET_ITEM_BORROW<py::tuple>(const py::handle& container, const size_t& item) {
    return py::reinterpret_borrow<py::object>(
        PyTuple_GET_ITEM(container.ptr(), py::ssize_t_cast(item)));
}
template <>
inline py::object GET_ITEM_BORROW<py::tuple>(const py::handle& container, const int& item) {
    return py::reinterpret_borrow<py::object>(
        PyTuple_GET_ITEM(container.ptr(), py::ssize_t_cast(item)));
}
template <>
inline py::object GET_ITEM_BORROW<py::list>(const py::handle& container, const ssize_t& item) {
    return py::reinterpret_borrow<py::object>(PyList_GET_ITEM(container.ptr(), item));
}
template <>
inline py::object GET_ITEM_BORROW<py::list>(const py::handle& container, const size_t& item) {
    return py::reinterpret_borrow<py::object>(
        PyList_GET_ITEM(container.ptr(), py::ssize_t_cast(item)));
}
template <>
inline py::object GET_ITEM_BORROW<py::list>(const py::handle& container, const int& item) {
    return py::reinterpret_borrow<py::object>(
        PyList_GET_ITEM(container.ptr(), py::ssize_t_cast(item)));
}

template <typename Container, typename Item>
inline py::object GET_ITEM_STEAL(const py::handle& container, const Item& item) {
    return container[item];
}
template <>
inline py::object GET_ITEM_STEAL<py::tuple>(const py::handle& container, const ssize_t& item) {
    return py::reinterpret_steal<py::object>(PyTuple_GET_ITEM(container.ptr(), item));
}
template <>
inline py::object GET_ITEM_STEAL<py::tuple>(const py::handle& container, const size_t& item) {
    return py::reinterpret_steal<py::object>(
        PyTuple_GET_ITEM(container.ptr(), py::ssize_t_cast(item)));
}
template <>
inline py::object GET_ITEM_STEAL<py::tuple>(const py::handle& container, const int& item) {
    return py::reinterpret_steal<py::object>(
        PyTuple_GET_ITEM(container.ptr(), py::ssize_t_cast(item)));
}
template <>
inline py::object GET_ITEM_STEAL<py::list>(const py::handle& container, const ssize_t& item) {
    return py::reinterpret_steal<py::object>(PyList_GET_ITEM(container.ptr(), item));
}
template <>
inline py::object GET_ITEM_STEAL<py::list>(const py::handle& container, const size_t& item) {
    return py::reinterpret_steal<py::object>(
        PyList_GET_ITEM(container.ptr(), py::ssize_t_cast(item)));
}
template <>
inline py::object GET_ITEM_STEAL<py::list>(const py::handle& container, const int& item) {
    return py::reinterpret_steal<py::object>(
        PyList_GET_ITEM(container.ptr(), py::ssize_t_cast(item)));
}

template <typename Container, typename Item>
inline void SET_ITEM(const py::handle& container, const Item& item, const py::handle& value) {
    container[item] = value;
}
template <>
inline void SET_ITEM<py::tuple>(const py::handle& container,
                                const ssize_t& item,
                                const py::handle& value) {
    PyTuple_SET_ITEM(container.ptr(), item, value.inc_ref().ptr());
}
template <>
inline void SET_ITEM<py::tuple>(const py::handle& container,
                                const size_t& item,
                                const py::handle& value) {
    PyTuple_SET_ITEM(container.ptr(), py::ssize_t_cast(item), value.inc_ref().ptr());
}
template <>
inline void SET_ITEM<py::tuple>(const py::handle& container,
                                const int& item,
                                const py::handle& value) {
    PyTuple_SET_ITEM(container.ptr(), py::ssize_t_cast(item), value.inc_ref().ptr());
}
template <>
inline void SET_ITEM<py::list>(const py::handle& container,
                               const ssize_t& item,
                               const py::handle& value) {
    PyList_SET_ITEM(container.ptr(), item, value.inc_ref().ptr());
}
template <>
inline void SET_ITEM<py::list>(const py::handle& container,
                               const size_t& item,
                               const py::handle& value) {
    PyList_SET_ITEM(container.ptr(), py::ssize_t_cast(item), value.inc_ref().ptr());
}
template <>
inline void SET_ITEM<py::list>(const py::handle& container,
                               const int& item,
                               const py::handle& value) {
    PyList_SET_ITEM(container.ptr(), py::ssize_t_cast(item), value.inc_ref().ptr());
}

template <typename PyType>
inline void AssertExact(const py::handle& object) {
    if (!py::isinstance<PyType>(object)) [[unlikely]] {
        throw std::invalid_argument(absl::StrFormat(
            "Expected an instance of %s, got %s.", typeid(PyType).name(), py::repr(object)));
    }
}
template <>
inline void AssertExact<py::list>(const py::handle& object) {
    if (!PyList_CheckExact(object.ptr())) [[unlikely]] {
        throw std::invalid_argument(absl::StrFormat("Expected list, got %s.", py::repr(object)));
    }
}
template <>
inline void AssertExact<py::tuple>(const py::handle& object) {
    if (!PyTuple_CheckExact(object.ptr())) [[unlikely]] {
        throw std::invalid_argument(absl::StrFormat("Expected tuple, got %s.", py::repr(object)));
    }
}
template <>
inline void AssertExact<py::dict>(const py::handle& object) {
    if (!PyDict_CheckExact(object.ptr())) [[unlikely]] {
        throw std::invalid_argument(absl::StrFormat("Expected dict, got %s.", py::repr(object)));
    }
}

inline bool IsNamedTupleClassImpl(const py::handle& type) {
    // We can only identify namedtuples heuristically, here by the presence of a _fields attribute.
    if (PyObject_IsSubclass(type.ptr(), reinterpret_cast<PyObject*>(&PyTuple_Type)) == 1)
        [[unlikely]] {
        if (PyObject* _fields = PyObject_GetAttrString(type.ptr(), "_fields")) [[unlikely]] {
            bool result = static_cast<bool>(PyTuple_CheckExact(_fields));
            if (result) [[likely]] {
                for (const auto& field : py::reinterpret_borrow<py::tuple>(_fields)) {
                    if (!static_cast<bool>(PyUnicode_CheckExact(field.ptr()))) [[unlikely]] {
                        result = false;
                        break;
                    }
                }
            }
            Py_DECREF(_fields);
            return result;
        }
        PyErr_Clear();
    }
    return false;
}
inline bool IsNamedTupleClass(const py::handle& type) {
    return PyType_Check(type.ptr()) && IsNamedTupleClassImpl(type);
}
inline bool IsNamedTuple(const py::handle& object) { return IsNamedTupleClass(object.get_type()); }
inline void AssertExactNamedTuple(const py::handle& object) {
    if (!IsNamedTuple(object)) [[unlikely]] {
        throw std::invalid_argument(
            absl::StrFormat("Expected collections.namedtuple, got %s.", py::repr(object)));
    }
}

inline void AssertExactOrderedDict(const py::handle& object) {
    if (!object.get_type().is(PyOrderedDictTypeObject)) [[unlikely]] {
        throw std::invalid_argument(
            absl::StrFormat("Expected collections.OrderedDict, got %s.", py::repr(object)));
    }
}

inline void AssertExactDefaultDict(const py::handle& object) {
    if (!object.get_type().is(PyDefaultDictTypeObject)) [[unlikely]] {
        throw std::invalid_argument(
            absl::StrFormat("Expected collections.defaultdict, got %s.", py::repr(object)));
    }
}

inline void AssertExactDeque(const py::handle& object) {
    if (!object.get_type().is(PyDequeTypeObject)) [[unlikely]] {
        throw std::invalid_argument(
            absl::StrFormat("Expected collections.deque, got %s.", py::repr(object)));
    }
}

inline bool IsStructSequenceClassImpl(const py::handle& type) {
    // We can only identify StructSequences heuristically, here by the presence of
    // n_sequence_fields, n_fields, n_unnamed_fields attributes.
    auto* type_object = reinterpret_cast<PyTypeObject*>(type.ptr());
    if (type_object->tp_base == &PyTuple_Type &&
        !static_cast<bool>(PyType_HasFeature(type_object, Py_TPFLAGS_BASETYPE))) [[unlikely]] {
        // NOLINTNEXTLINE[readability-use-anyofallof]
        for (const char* name : {"n_sequence_fields", "n_fields", "n_unnamed_fields"}) {
            if (PyObject* attr = PyObject_GetAttrString(type.ptr(), name)) [[unlikely]] {
                bool result = static_cast<bool>(PyLong_CheckExact(attr));
                Py_DECREF(attr);
                if (!result) [[unlikely]] {
                    return false;
                }
            } else [[likely]] {
                PyErr_Clear();
                return false;
            }
        }
        return true;
    }
    return false;
}
inline bool IsStructSequenceClass(const py::handle& type) {
    return PyType_Check(type.ptr()) && IsStructSequenceClassImpl(type);
}
inline bool IsStructSequence(const py::handle& object) {
    return IsStructSequenceClass(object.get_type());
}
inline void AssertExactStructSequence(const py::handle& object) {
    if (!IsStructSequence(object)) [[unlikely]] {
        throw std::invalid_argument(
            absl::StrFormat("Expected StructSequence, got %s.", py::repr(object)));
    }
}
inline py::tuple StructSequenceGetFields(const py::handle& object) {
    py::handle type;
    if (PyType_Check(object.ptr())) {
        type = object;
        if (!IsStructSequenceClass(type)) [[unlikely]] {
            throw std::invalid_argument(
                absl::StrFormat("Expected StructSequence type, got %s.", py::repr(object)));
        }
    } else {
        type = object.get_type();
        AssertExactStructSequence(object);
    }

    const auto n_sequence_fields = getattr(type, "n_sequence_fields").cast<ssize_t>();
    auto* members = reinterpret_cast<PyTypeObject*>(type.ptr())->tp_members;
    py::tuple fields{n_sequence_fields};
    for (ssize_t i = 0; i < n_sequence_fields; ++i) {
        // NOLINTNEXTLINE[cppcoreguidelines-pro-bounds-pointer-arithmetic]
        SET_ITEM<py::tuple>(fields, i, py::str(members[i].name));
    }
    return fields;
}
