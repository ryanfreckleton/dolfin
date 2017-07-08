import dolfin.cpp as cpp

from dolfin.function.functionspace import FunctionSpace
from dolfin.function.expression import CompiledExpression
from dolfin.function.constant import Constant
from dolfin.fem.form import Form
from dolfin.fem.dirichletbc import DirichletBC, CompiledSubDomain

from dolfin.cpp.generation import UnitSquareMesh
from dolfin.cpp.fem import Assembler
from dolfin.cpp.function import Function
from dolfin.cpp.mesh import SubDomain, Vertex, Cell

from dolfin.cpp.la import LUSolver, KrylovSolver, Matrix, Vector
if cpp.common.has_petsc():
    from dolfin.cpp.la import PETScMatrix, PETScVector

from dolfin.cpp import MPI

from dolfin.cpp.io import XDMFFile

from dolfin.cpp.parameter import parameters
from dolfin.cpp.refinement import refine
from ufl import TestFunction, TrialFunction, inner, grad, dx, ds

if cpp.common.has_petsc():
    parameters['linear_algebra_backend'] = 'PETSc'

# Create mesh and refine
mesh = UnitSquareMesh(12, 12)
mesh = refine(mesh)

# Create function space
V = FunctionSpace(mesh, "Lagrange", 1)

# Create a function
w = Function(V)

#xdmf = XDMFFile("a.xdmf")
#xdmf.write(mesh,  XDMFFile.Encoding.ASCII)
#xdmf.write(w, XDMFFile.Encoding.ASCII)

DOLFIN_EPS = 1e-9

class Boundary(SubDomain):
    def inside(self, x, on_boundary):
        result = (x[0] < DOLFIN_EPS or x[0] > 1.0 - DOLFIN_EPS)
        return bool(result)
# boundary = CompiledSubDomain("x[0] < DOLFIN_EPS or x[0] > 1.0 - DOLFIN_EPS")

u0 = Constant(0.0)
bc = DirichletBC(V, u0.cpp_object(), "x[0] < DOLFIN_EPS or x[0] > 1.0 - DOLFIN_EPS")

u = TrialFunction(V)
v = TestFunction(V)
f = CompiledExpression("10*exp(-(pow(x[0] - 0.5, 2) + pow(x[1] - 0.5, 2)) / 0.02)", degree=2)
g = CompiledExpression("sin(5*x[0])", degree=2)
a = inner(grad(u), grad(v))*dx
L = f*v*dx + g*v*ds

assembler = Assembler()

A = Matrix()
assembler.assemble(A, Form(a, [V, V]))

b = Vector()
myform = Form(L, [V])
assembler.assemble(b, myform)

bc.apply(b)
bc.apply(A)

solver = KrylovSolver(A)
solver.solve(w.vector(), b)

file = XDMFFile("poisson.xdmf")

if cpp.common.has_hdf5():
    file.write(w, XDMFFile.Encoding.HDF5)
else:
    file.write(w, XDMFFile.Encoding.ASCII)

# plot(w, interactive=True)
