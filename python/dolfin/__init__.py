# -*- coding: utf-8 -*-
"""Main module for DOLFIN"""

import sys

# Store dl open flags to restore them after import
stored_dlopen_flags = sys.getdlopenflags()

# Fix dlopen flags (may need reorganising)
import sys
if "linux" in sys.platform:
    # FIXME: What with other platforms?
    try:
        from ctypes import RTLD_NOW, RTLD_GLOBAL
    except ImportError:
        RTLD_NOW = 2
        RTLD_GLOBAL = 256
    sys.setdlopenflags(RTLD_NOW | RTLD_GLOBAL)
del sys

#import dolfin.cpp

# Reset dl open flags
#import sys
#sys.setdlopenflags(stored_dlopen_flags)
#del sys

# cpp modules
from .cpp.common import (Variable, has_debug, has_hdf5, has_scotch,
                         has_hdf5_parallel, has_mpi, has_petsc, has_parmetis,
                         has_slepc, git_commit_hash, DOLFIN_EPS,
                         DOLFIN_PI, TimingClear, TimingType, timing, timings,
                         list_timings, dump_timings_to_xml)

if has_hdf5():
    from .cpp.adaptivity import TimeSeries
    from .cpp.io import HDF5File

#from .cpp.adaptivity import AdaptiveLinearVariationalSolver, AdaptiveNonlinearVariationalSolver

from .cpp.ale import ALE
from .cpp import MPI
from .cpp.function import (Expression, Constant, FunctionAXPY,
                           LagrangeInterpolator, FunctionAssigner, assign)
from .cpp.fem import (FiniteElement, DofMap, Assembler, get_coordinates,
                      create_mesh,
                      set_coordinates, vertex_to_dof_map, dof_to_vertex_map, PointSource,
                      DiscreteOperators, LinearVariationalSolver,
                      NonlinearVariationalSolver, SparsityPatternBuilder)

from .cpp.geometry import BoundingBoxTree, Point, MeshPointIntersection, intersect
from .cpp.generation import (IntervalMesh, BoxMesh, RectangleMesh, UnitDiscMesh,
                             UnitQuadMesh, UnitHexMesh, UnitTriangleMesh,
                             UnitCubeMesh, UnitSquareMesh, UnitIntervalMesh)
from .cpp.graph import GraphBuilder
from .cpp.io import File, XDMFFile, VTKFile
from .cpp.la import (has_linear_algebra_backend,
                     linear_algebra_backends, has_krylov_solver_method,
                     has_krylov_solver_preconditioner, normalize, VectorSpaceBasis,
                     in_nullspace)

if has_linear_algebra_backend('PETSc'):
    from .cpp.la import (PETScVector, PETScMatrix, PETScFactory, PETScOptions,
                         PETScLUSolver, PETScKrylovSolver, PETScPreconditioner)
    from .cpp.fem import PETScDMCollection
    from .cpp.nls import PETScSNESSolver, PETScTAOSolver, TAOLinearBoundSolver

if has_linear_algebra_backend('Tpetra'):
    from .cpp.la import (TpetraVector, TpetraMatrix, TpetraFactory, MueluPreconditioner, BelosKrylovSolver)

if has_slepc():
    from .cpp.la import SLEPcEigenSolver

from .cpp.la import (IndexMap, DefaultFactory, Matrix, Vector, Scalar, EigenMatrix,
                     EigenVector, EigenFactory, LUSolver, KrylovSolver, TensorLayout,
                     LinearOperator, BlockMatrix, BlockVector)
from .cpp.log import info, Table, set_log_level, get_log_level, LogLevel
from .cpp.math import ipow, near, between
from .cpp.mesh import (Mesh, MeshTopology, MeshGeometry, MeshEntity, MeshColoring,
                       Cell, Facet, Face, Edge, Vertex, cells,
                       facets, faces, edges, entities,
                       vertices, SubDomain, BoundaryMesh,
                       MeshEditor, MeshQuality,
                       SubMesh, DomainBoundary, PeriodicBoundaryComputation,
                       MeshTransformation, SubsetIterator)
from .cpp.nls import NonlinearProblem, NewtonSolver, OptimisationProblem
from .cpp.refinement import refine

from .cpp.parameter import Parameters, parameters

from .cpp.io import X3DOM, X3DOMParameters

# Python modules
from . import io
from . import la
from . import mesh
from . import parameter

from .fem.adaptivesolving import AdaptiveLinearVariationalSolver, AdaptiveNonlinearVariationalSolver

from .common import timer
from .common.timer import Timer, timed
from .common.plotting import plot

from .fem.assembling import assemble, assemble_system, SystemAssembler, assemble_local
from .fem.form import Form
from .fem.norms import norm
from .fem.dirichletbc import DirichletBC, AutoSubDomain
from .fem.interpolation import interpolate
from .fem.projection import project
from .fem.solving import (solve, LocalSolver, LinearVariationalProblem,
                          NonlinearVariationalProblem)
from .fem.formmanipulations import derivative, adjoint, increase_order, tear

from .function.functionspace import FunctionSpace, VectorFunctionSpace, TensorFunctionSpace
from .function.function import Function
from .function.argument import TestFunction, TrialFunction, TestFunctions, TrialFunctions
from .function.constant import Constant
from .function.specialfunctions import (FacetNormal, CellSize, SpatialCoordinate,
                                        CellVolume, Circumradius, FacetArea, MeshCoordinates)
from .function.expression import CompiledExpression, Expression, UserExpression

# experimental
from .jit.pybind11jit import compile_cpp_code

from .la import as_backend_type

from .mesh.ale import (compute_vertex_map, compute_edge_map, init_parent_edge_indices)
from .mesh.meshfunction import (MeshFunction, CellFunction,
                                FacetFunction, FaceFunction, EdgeFunction, VertexFunction)
from .mesh.meshvaluecollection import MeshValueCollection
from .mesh.subdomain import CompiledSubDomain

from .multistage.multistagescheme import RK4, CN2, ExplicitMidPoint, ESDIRK3, ESDIRK4, ForwardEuler, BackwardEuler
from .multistage.rushlarsenschemes import RL1, RL2, GRL1, GRL2

# ufl
from ufl import (FiniteElement, TensorElement, VectorElement, MixedElement, rhs, lhs,
                 conditional, le, lt, ge, gt, split,
                 cross, inner, dot, grad, curl, dx, div, Measure, det, pi, sin, cos, tan,
                 acos, asin, atan, ln, exp, sqrt, bessel_I, bessel_J, bessel_K, bessel_Y,
                 Dx, ds, dS, dP, triangle, tetrahedron, avg, jump, sym, tr, Identity,
                 variable, diff, as_vector, as_tensor, as_matrix)
from ufl.formoperators import action

# FIXME
def has_petsc4py():
    return False


def mpi_comm_self():
    return MPI.comm_self

def mpi_comm_world():
    return MPI.comm_world
