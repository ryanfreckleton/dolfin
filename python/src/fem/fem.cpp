// Copyright (C) 2017 Chris Richardson and Garth N. Wells
//
// This file is part of DOLFIN.
//
// DOLFIN is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DOLFIN is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with DOLFIN. If not, see <http://www.gnu.org/licenses/>.

#include <iostream>
#include <memory>
#include <Eigen/Dense>
#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>
#include <pybind11/cast.h>

#include <ufc.h>

#include <dolfin/fem/fem_utils.h>
#include <dolfin/fem/assemble.h>
#include <dolfin/fem/assemble_local.h>
#include <dolfin/fem/Assembler.h>
#include <dolfin/fem/DirichletBC.h>
#include <dolfin/fem/DiscreteOperators.h>
#include <dolfin/fem/DofMap.h>
#include <dolfin/fem/FiniteElement.h>
#include <dolfin/fem/Form.h>
#include <dolfin/fem/PointSource.h>
#include <dolfin/fem/SystemAssembler.h>
#include <dolfin/fem/PETScDMCollection.h>
#include <dolfin/function/FunctionSpace.h>
#include <dolfin/function/GenericFunction.h>
#include <dolfin/mesh/SubDomain.h>
#include <dolfin/la/GenericTensor.h>
#include <dolfin/la/GenericMatrix.h>
#include <dolfin/la/GenericVector.h>


namespace py = pybind11;

namespace dolfin_wrappers
{
  void fem(py::module& m)
  {
    // Delcare UFC objects
    py::class_<ufc::finite_element, std::shared_ptr<ufc::finite_element>>
      (m, "ufc_finite_element", "UFC finite element object");
    py::class_<ufc::dofmap, std::shared_ptr<ufc::dofmap>>
      (m, "ufc_dofmap", "UFC dofmap object");
    py::class_<ufc::form, std::shared_ptr<ufc::form>>
      (m, "ufc_form", "UFC form object");

    // Function to convert pointers (from JIT usually) to UFC objects
    m.def("make_ufc_finite_element",
          [](std::uintptr_t e)
          {
            ufc::finite_element * p = reinterpret_cast<ufc::finite_element *>(e);
            return std::shared_ptr<const ufc::finite_element>(p);
          });

    m.def("make_ufc_dofmap",
          [](std::uintptr_t e)
          {
            ufc::dofmap * p = reinterpret_cast<ufc::dofmap *>(e);
            return std::shared_ptr<const ufc::dofmap>(p);
          });

    m.def("make_ufc_form",
          [](std::uintptr_t e)
          {
            ufc::form * p = reinterpret_cast<ufc::form *>(e);
            return std::shared_ptr<const ufc::form>(p);
          });

    // dolfin::FiniteElement class
    py::class_<dolfin::FiniteElement, std::shared_ptr<dolfin::FiniteElement>>
      (m, "FiniteElement", "DOLFIN FiniteElement object")
      .def(py::init<std::shared_ptr<const ufc::finite_element>>())
      .def("num_sub_elements", &dolfin::FiniteElement::num_sub_elements)
      .def("evaluate_dofs", [](const dolfin::FiniteElement& self, py::object f,
                               py::array_t<double> coordinate_dofs,
                               int cell_orientation, const dolfin::Cell& c)
           {
             const ufc::function* _f = nullptr;
             if (py::hasattr(f, "_cpp_expression"))
               _f = f.attr("_cpp_expression").cast<ufc::function*>();
             else
               _f = f.cast<ufc::function*>();

             ufc::cell ufc_cell;
             c.get_cell_data(ufc_cell);
             py::array_t<double, py::array::c_style> dofs(self.space_dimension());
             self.evaluate_dofs(dofs.mutable_data(), *_f,
                                coordinate_dofs.data(), cell_orientation, ufc_cell);
             return dofs;
           }, "Evaluate degrees of freedom on element for a given function")
      .def("tabulate_dof_coordinates", [](const dolfin::FiniteElement& self, const dolfin::Cell& cell)
           {
             // Get cell vertex coordinates
             std::vector<double> coordinate_dofs;
             cell.get_coordinate_dofs(coordinate_dofs);

             // Tabulate the coordinates
             boost::multi_array<double, 2> _dof_coords;
             self.tabulate_dof_coordinates(_dof_coords, coordinate_dofs, cell);

             // Copy data and return
             typedef Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> EigenArray;
             EigenArray dof_coords = Eigen::Map<EigenArray>(_dof_coords.data(),
                                                            _dof_coords.shape()[0],
                                                            _dof_coords.shape()[1]);
             return dof_coords;
           }, "Tabulate coordinates of dofs on cell")
      .def("space_dimension", &dolfin::FiniteElement::space_dimension)
      .def("signature", &dolfin::FiniteElement::signature);

    // dolfin::GenericDofMap class
    py::class_<dolfin::GenericDofMap, std::shared_ptr<dolfin::GenericDofMap>>
      (m, "GenericDofMap", "DOLFIN DofMap object");

    // dolfin::DofMap class
    py::class_<dolfin::DofMap, std::shared_ptr<dolfin::DofMap>, dolfin::GenericDofMap>
      (m, "DofMap", "DOLFIN DofMap object")
      .def(py::init<std::shared_ptr<const ufc::dofmap>, const dolfin::Mesh&>())
      .def("ownership_range", &dolfin::DofMap::ownership_range)
      .def("cell_dofs", &dolfin::DofMap::cell_dofs);

    // dolfin::DirichletBC class
    py::class_<dolfin::DirichletBC, std::shared_ptr<dolfin::DirichletBC>>
      (m, "DirichletBC", "DOLFIN DirichletBC object")
      .def(py::init<std::shared_ptr<const dolfin::FunctionSpace>,
           std::shared_ptr<const dolfin::GenericFunction>,
           std::shared_ptr<const dolfin::SubDomain>>())
      .def("apply", (void (dolfin::DirichletBC::*)(dolfin::GenericVector&) const)
           &dolfin::DirichletBC::apply)
      .def("apply", (void (dolfin::DirichletBC::*)(dolfin::GenericMatrix&) const)
           &dolfin::DirichletBC::apply)
      .def("user_subdomain", &dolfin::DirichletBC::user_sub_domain);


    // dolfin::Assembler class
    py::class_<dolfin::Assembler, std::shared_ptr<dolfin::Assembler>>
      (m, "Assembler", "DOLFIN Assembler object")
      .def(py::init<>())
      .def("assemble", &dolfin::Assembler::assemble)
      .def_readwrite("add_values", &dolfin::Assembler::add_values)
      .def_readwrite("keep_diagonal", &dolfin::Assembler::keep_diagonal)
      .def_readwrite("finalize_tensor", &dolfin::Assembler::finalize_tensor);

    // dolfin::SystemAssembler class
    py::class_<dolfin::SystemAssembler, std::shared_ptr<dolfin::SystemAssembler>>
      (m, "SystemAssembler", "DOLFIN SystemAssembler object")
      .def(py::init<std::shared_ptr<const dolfin::Form>, std::shared_ptr<const dolfin::Form>,
           std::vector<std::shared_ptr<const dolfin::DirichletBC>>>())
      .def("assemble", (void (dolfin::SystemAssembler::*)(dolfin::GenericMatrix&, dolfin::GenericVector&))
           &dolfin::SystemAssembler::assemble);

    // dolfin::DiscreteOperators
    py::class_<dolfin::DiscreteOperators> (m, "DiscreteOperators")
      .def_static("build_gradient", &dolfin::DiscreteOperators::build_gradient);

    // dolfin::Form class
    py::class_<dolfin::Form, std::shared_ptr<dolfin::Form>>
      (m, "Form", "DOLFIN Form object")
      .def(py::init<std::shared_ptr<const ufc::form>,
                    std::vector<std::shared_ptr<const dolfin::FunctionSpace>>>())
      .def("num_coefficients", &dolfin::Form::num_coefficients, "Return number of coefficients in form")
      .def("original_coefficient_position", &dolfin::Form::original_coefficient_position)
      .def("set_coefficient", (void (dolfin::Form::*)(std::size_t, std::shared_ptr<const dolfin::GenericFunction>))
           &dolfin::Form::set_coefficient, "Doc")
      .def("set_coefficient", (void (dolfin::Form::*)(std::string, std::shared_ptr<const dolfin::GenericFunction>))
           &dolfin::Form::set_coefficient, "Doc")
      .def("rank", &dolfin::Form::rank)
      .def("mesh", &dolfin::Form::mesh);

    // dolfin::PointSource class
    py::class_<dolfin::PointSource, std::shared_ptr<dolfin::PointSource>>
      (m, "PointSource")
      .def(py::init<std::shared_ptr<const dolfin::FunctionSpace>, const dolfin::Point&, double>(),
           py::arg("V"), py::arg("p"), py::arg("magnitude") = 1.0);

#ifdef HAS_PETSC
    // dolfin::PETScDMCollection
    py::class_<dolfin::PETScDMCollection, std::shared_ptr<dolfin::PETScDMCollection>>
      (m, "PETScDMCollection")
      .def_static("create_transfer_matrix", &dolfin::PETScDMCollection::create_transfer_matrix);
#endif

    // Assemble functions

    m.def("assemble", (void (*)(dolfin::GenericTensor&, const dolfin::Form&)) &dolfin::assemble);
    m.def("assemble", (double (*)(const dolfin::Form&)) &dolfin::assemble);

    m.def("assemble_system", (void (*)(dolfin::GenericMatrix&, dolfin::GenericVector&,
                                       const dolfin::Form&, const dolfin::Form&,
                                       std::vector<std::shared_ptr<const dolfin::DirichletBC>>))
          &dolfin::assemble_system);

    m.def("assemble_system", (void (*)(dolfin::GenericMatrix&, dolfin::GenericVector&,
                                       const dolfin::Form&, const dolfin::Form&,
                                       std::vector<std::shared_ptr<const dolfin::DirichletBC>>,
                                       const dolfin::GenericVector&))
          &dolfin::assemble_system);

    m.def("assemble_local",
          (Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>(*)(const dolfin::Form&, const dolfin::Cell&))
          &dolfin::assemble_local);

    // FEM utils functions

    m.def("set_coordinates", &dolfin::set_coordinates);
    m.def("get_coordinates", &dolfin::get_coordinates);
    m.def("vertex_to_dof_map", &dolfin::vertex_to_dof_map);
    m.def("dof_to_vertex_map", &dolfin::dof_to_vertex_map);
  }

}
