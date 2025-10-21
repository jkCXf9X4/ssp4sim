#include <pybind11/pybind11.h>
#include "simulator.hpp"

namespace py = pybind11;

PYBIND11_MODULE(py_ssp4cpp, m) {
    py::class_<ssp4sim::Simulator>(m, "Simulator")
        .def(py::init<const std::string &, const std::string &>())
        .def("init", &ssp4sim::Simulator::init)
        .def("simulate", &ssp4sim::Simulator::simulate);
}
