#!/usr/bin/env py.test

"""Unit tests for Dirichlet boundary conditions"""

# Copyright (C) 2011-2012 Garth N. Wells
#
# This file is part of DOLFIN.
#
# DOLFIN is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# DOLFIN is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with DOLFIN. If not, see <http://www.gnu.org/licenses/>.
#
# Modified by Kent-Andre Mardal 2011
# Modified by Anders Logg 2011
# Modified by Martin Alnaes 2012

import os

import pytest
import numpy
import six

from dolfin import *
from dolfin_utils.test import skip_in_parallel, datadir


@pytest.mark.xfail
def test_instantiation():
    """ A rudimentary test for instantiation"""
    # FIXME: Needs to be expanded
    mesh = UnitCubeMesh(8, 8, 8)
    V = FunctionSpace(mesh, "CG", 1)

    bc0 = DirichletBC(V, 1, "x[0]<0")
    bc1 = DirichletBC(bc0)
    assert bc0.function_space() == bc1.function_space()


@pytest.mark.xfail
def test_director_lifetime():
    """Test for any problems with objects with directors going out
    of scope"""

    class Boundary(SubDomain):
        def inside(self, x, on_boundary): return on_boundary

    class BoundaryFunction(Expression):
        def eval(self, values, x): values[0] = 1.0

    mesh = UnitSquareMesh(8, 8)
    V = FunctionSpace(mesh, "Lagrange", 1)
    v, u = TestFunction(V), TrialFunction(V)

    A0 = assemble(v*u*dx)
    bc0 = DirichletBC(V, BoundaryFunction(degree=1), Boundary())
    bc0.apply(A0)

    bc1 = DirichletBC(V, Expression("1.0", degree=0), CompiledSubDomain("on_boundary"))
    A1 = assemble(v*u*dx)
    bc1.apply(A1)

    assert round(A1.norm("frobenius") - A0.norm("frobenius"), 7) == 0


@pytest.mark.xfail
def xtest_get_values():
    mesh = UnitSquareMesh(8, 8)
    dofs = numpy.zeros(3, dtype="I")

    def upper(x, on_boundary):
        return x[1] > 0.5 + DOLFIN_EPS

    V = FunctionSpace(mesh, "CG", 1)
    bc = DirichletBC(V, 0.0, upper)
    bc_values = bc.get_boundary_values()

@pytest.mark.skip
def xtest_meshdomain_bcs(datadir):
    """Test application of Dirichlet boundary conditions stored as
    part of the mesh. This test is also a compatibility test for
    VMTK."""

    mesh = Mesh(os.path.join(datadir, "aneurysm.xml"))
    V = FunctionSpace(mesh, "CG", 1)
    v = TestFunction(V)

    f = Constant(0)
    u1 = Constant(1)
    u2 = Constant(2)
    u3 = Constant(3)

    bc1 = DirichletBC(V, u1, 1)
    bc2 = DirichletBC(V, u2, 2)
    bc3 = DirichletBC(V, u3, 3)
    bcs = [bc1, bc2, bc3]

    L = f*v*dx
    b = assemble(L)
    [bc.apply(b) for bc in bcs]
    assert round(norm(b) - 16.55294535724685, 7) == 0


@pytest.mark.xfail
def xtest_user_meshfunction_domains():
    mesh0 = UnitSquareMesh(12, 12)
    mesh1 = UnitSquareMesh(12, 12)
    V = FunctionSpace(mesh0, "CG", 1)

    DirichletBC(V, Constant(0.0), EdgeFunction("size_t", mesh0), 0)
    DirichletBC(V, Constant(0.0), FacetFunction("size_t", mesh0), 0)
    with pytest.raises(RuntimeError):
        DirichletBC(V, 0.0, CellFunction("size_t", mesh0), 0)
        DirichletBC(V, 0.0, VertexFunction("size_t", mesh0), 0)
        DirichletBC(V, 0.0, FacetFunction("size_t", mesh1), 0)


@skip_in_parallel
@pytest.mark.xfail
def test_bc_for_piola_on_manifolds():
    "Testing DirichletBC for piolas over standard domains vs manifolds."
    n = 4
    side = CompiledSubDomain("near(x[2], 0.0)")
    mesh = SubMesh(BoundaryMesh(UnitCubeMesh(n, n, n), "exterior"), side)
    square = UnitSquareMesh(n, n)
    mesh.init_cell_orientations(Expression(("0.0", "0.0", "1.0"), degree=0))

    RT1 = lambda mesh: FunctionSpace(mesh, "RT", 1)
    RT2 = lambda mesh: FunctionSpace(mesh, "RT", 2)
    DRT1 = lambda mesh: FunctionSpace(mesh, "DRT", 1)
    DRT2 = lambda mesh: FunctionSpace(mesh, "DRT", 2)
    BDM1 = lambda mesh: FunctionSpace(mesh, "BDM", 1)
    BDM2 = lambda mesh: FunctionSpace(mesh, "BDM", 2)
    N1curl1 = lambda mesh: FunctionSpace(mesh, "N1curl", 1)
    N2curl1 = lambda mesh: FunctionSpace(mesh, "N2curl", 1)
    N1curl2 = lambda mesh: FunctionSpace(mesh, "N1curl", 2)
    N2curl2 = lambda mesh: FunctionSpace(mesh, "N2curl", 2)
    elements = [N1curl1, N2curl1,  N1curl2, N2curl2, RT1, RT2, BDM1,
                BDM2, DRT1, DRT2]

    for element in elements:
        V = element(mesh)
        bc = DirichletBC(V, (1.0, 0.0, 0.0), "on_boundary")
        u = Function(V)
        bc.apply(u.vector())
        b0 = assemble(inner(u, u)*dx)

        V = element(square)
        bc = DirichletBC(V, (1.0, 0.0), "on_boundary")
        u = Function(V)
        bc.apply(u.vector())
        b1 = assemble(inner(u, u)*dx)
        assert round(b0 - b1, 7) == 0


@pytest.mark.xfail
def test_zero():
    mesh = UnitSquareMesh(4, 4)
    V = FunctionSpace(mesh, "CG", 1)
    u1 = interpolate(Constant(1.0), V)

    bc = DirichletBC(V, 0, "on_boundary")

    # Create arbitrary matrix of size V.dim()
    #
    # Note: Identity matrix would suffice, but there doesn't seem
    # an easy way to construct it in dolfin

    v, u = TestFunction(V), TrialFunction(V)
    A0 = assemble(u*v*dx)

    # Zero rows at boundary dofs
    bc.zero(A0)

    u1_zero = Function(V)
    u1_zero.vector()[:] = A0 * u1.vector()

    boundaryIntegral = assemble(u1_zero * ds)
    assert near(boundaryIntegral, 0.0)


@skip_in_parallel
@pytest.mark.xfail
def test_zero_columns_offdiag():
    """Test zero_columns applied to offdiagonal block"""
    mesh = UnitSquareMesh(20, 20)
    V = VectorFunctionSpace(mesh, "P", 2)
    Q = FunctionSpace(mesh, "P", 1)
    u = TrialFunction(V)
    q = TestFunction(Q)
    a = inner(div(u), q)*dx
    L = inner(Constant(0), q)*dx
    A = assemble(a)
    b = assemble(L)

    bc = DirichletBC(V, Constant((-32.23333, 43243.1)), 'on_boundary')

    # Compute residual with x satisfying bc before zero_columns
    u = Function(V)
    x = u.vector()
    bc.apply(x)
    r0 = A*x - b

    bc.zero_columns(A, b)

    # Test that A gets zero columns
    bc_dict = bc.get_boundary_values()
    for i in six.moves.xrange(*A.local_range(0)):
        cols, vals = A.getrow(i)
        for j, v in six.moves.zip(cols, vals):
            if j in bc_dict:
                assert v == 0.0

    # Compute residual with x satisfying bc after zero_columns
    # and check that it is preserved
    r1 = A*x - b
    assert numpy.isclose((r1-r0).norm('linf'), 0.0)


@skip_in_parallel
@pytest.mark.xfail
def test_zero_columns_square():
    """Test zero_columns applied to square matrix"""
    mesh = UnitSquareMesh(20, 20)
    V = FunctionSpace(mesh, "P", 1)
    u, v = TrialFunction(V), TestFunction(V)
    a = inner(grad(u), grad(v))*dx
    L = Constant(0)*v*dx
    A = assemble(a)
    b = assemble(L)
    u = Function(V)
    x = u.vector()

    bc = DirichletBC(V, 666.0, 'on_boundary')
    bc.zero_columns(A, b, 42.0)

    # Check that A gets zeros in bc rows and bc columns and 42 on diagonal
    bc_dict = bc.get_boundary_values()
    for i in six.moves.xrange(*A.local_range(0)):
        cols, vals = A.getrow(i)
        for j, v in six.moves.zip(cols, vals):
            if i in bc_dict or j in bc_dict:
                if i == j:
                    assert numpy.isclose(v, 42.0)
                else:
                    assert v == 0.0

    # Check that solution of linear system works
    solve(A, x, b)
    assert numpy.isclose((b-A*x).norm('linf'), 0.0)
    x1 = x.copy()
    bc.apply(x1)
    x1 -= x
    assert numpy.isclose(x1.norm('linf'), 0.0)


@pytest.mark.xfail
def test_homogenize_consistency():
    mesh = UnitIntervalMesh(10)
    V = FunctionSpace(mesh, "CG", 1)

    for method in ['topological', 'geometric', 'pointwise']:
        bc = DirichletBC(V, Constant(0), "on_boundary", method=method)
        bc_new = DirichletBC(bc)
        bc_new.homogenize()
        assert bc_new.method() == bc.method()


@pytest.mark.xfail
def test_nocaching_values():
    """There might be caching of dof indices in DirichletBC.
    But caching of values is _not_ allowed."""
    mesh = UnitSquareMesh(4, 4)
    V = FunctionSpace(mesh, "P", 1)
    u = Function(V)
    x = u.vector()

    for method in ["topological", "geometric", "pointwise"]:
        bc = DirichletBC(V, 0.0, lambda x, b: True, method=method)

        x.zero()
        bc.set_value(Constant(1.0))
        bc.apply(x)
        assert numpy.allclose(x.array(), 1.0)

        x.zero()
        bc.set_value(Constant(2.0))
        bc.apply(x)
        assert numpy.allclose(x.array(), 2.0)
