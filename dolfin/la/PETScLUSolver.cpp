// Copyright (C) 2005-2009 Anders Logg.
// Licensed under the GNU LGPL Version 2.1.
//
// Modified by Garth N. Wells, 2009-2010.
// Modified by Niclas Jansson, 2009.
//
// First added:  2005
// Last changed: 2010-04-05

#ifdef HAS_PETSC

#include <boost/assign/list_of.hpp>
#include <dolfin/main/MPI.h>
#include <dolfin/log/dolfin_log.h>
#include <dolfin/common/constants.h>
#include "LUSolver.h"
#include "PETScMatrix.h"
#include "PETScVector.h"
#include "PETScLUSolver.h"

using namespace dolfin;

//-----------------------------------------------------------------------------
namespace dolfin
{
  class PETScKSPDeleter
  {
  public:
    void operator() (KSP* ksp)
    {
      if (ksp)
        KSPDestroy(*ksp);
      delete ksp;
    }
  };
}
//-----------------------------------------------------------------------------
// Available LU solver
const std::map<std::string, const MatSolverPackage> PETScLUSolver::lu_packages
  = boost::assign::map_list_of("default", "")
                              ("umfpack",      MAT_SOLVER_UMFPACK)
                              ("mumps",        MAT_SOLVER_MUMPS)
                              ("spooles",      MAT_SOLVER_SPOOLES)
                              ("superlu_dist", MAT_SOLVER_SUPERLU_DIST)
                              ("superlu",      MAT_SOLVER_SUPERLU);
//-----------------------------------------------------------------------------
Parameters PETScLUSolver::default_parameters()
{
  Parameters p(LUSolver::default_parameters());
  p.rename("petsc_lu_solver");
  return p;
}
//-----------------------------------------------------------------------------
PETScLUSolver::PETScLUSolver(std::string lu_package) : lu_package(lu_package)
{
  // Check package string
  if (lu_packages.count(lu_package) == 0)
    error("Requested PETSc LU solver '%s' is unknown,", lu_package.c_str());

  // Choose appropriate 'default' solver
  if (lu_package == "default")
  {
    if (MPI::num_processes() == 1)
      this->lu_package = "umfpack";
    else
    {
      #if PETSC_HAVE_MUMPS
      this->lu_package = "mumps";
      #elif PETSC_HAVE_SPOOLES
      this->lu_package = "spooles";
      #elif PETSC_HAVE_SUPERLU_DIST
      this->lu_package = "superlu_dist";
      #else
      error("No suitable solver for parallel LU. Consider configuring PETSc with MUMPS or SPOOLES.");
      #endif
    }
  }

  // Set parameter values
  parameters = default_parameters();

  // Initialize PETSc LU solver
  init();
}
//-----------------------------------------------------------------------------
PETScLUSolver::~PETScLUSolver()
{
  // Do nothing
}
//-----------------------------------------------------------------------------
void PETScLUSolver::set_operator(const PETScMatrix& A)
{
  error("not_implemented");
}
//-----------------------------------------------------------------------------
dolfin::uint PETScLUSolver::solve(GenericVector& x, const GenericVector& b)
{
  error("not_implemented");
  return 0;
}
//-----------------------------------------------------------------------------
dolfin::uint PETScLUSolver::solve(const GenericMatrix& A, GenericVector& x,
                                  const GenericVector& b)
{
  return solve(A.down_cast<PETScMatrix>(), x.down_cast<PETScVector>(),
               b.down_cast<PETScVector>());
}
//-----------------------------------------------------------------------------
dolfin::uint PETScLUSolver::solve(const PETScMatrix& A, PETScVector& x,
                                  const PETScVector& b)
{
  // Initialise solver
  init();

  const MatSolverPackage solver_type;
  PC pc;
  KSPGetPC(*ksp, &pc);
  PCFactorGetMatSolverPackage(pc, &solver_type);

  // Get parameters
  const bool report = parameters["report"];

  // Check dimensions
  const uint M = A.size(0);
  const uint N = A.size(1);
  if (N != b.size())
    error("Non-matching dimensions for linear system.");

  // Initialize solution vector (remains untouched if dimensions match)
  x.resize(M);

  // Write a message
  if (report)
    info(PROGRESS, "Solving linear system of size %d x %d (PETSc LU solver, %s).",
         A.size(0), A.size(1), solver_type);

  // Solve linear system
  KSPSetOperators(*ksp, *A.mat(), *A.mat(), DIFFERENT_NONZERO_PATTERN);
  KSPSolve(*ksp, *b.vec(), *x.vec());

  return 1;
}
//-----------------------------------------------------------------------------
std::string PETScLUSolver::str(bool verbose) const
{
  std::stringstream s;

  if (verbose)
  {
    warning("Verbose output for PETScLUSolver not implemented, calling PETSc KSPView directly.");
    KSPView(*ksp, PETSC_VIEWER_STDOUT_WORLD);
  }
  else
    s << "<PETScLUSolver>";

  return s.str();
}
//-----------------------------------------------------------------------------
void PETScLUSolver::init()
{
  // Create a PETSc Krylov solver and instruct it to use LU preconditioner

  // Destroy old solver environment if necessary
  //if (!ksp.unique())
  //  error("Cannot create new KSP Krylov solver. More than one object points to the underlying PETSc object.");

  ksp.reset(new KSP, PETScKSPDeleter());

  // Set up solver environment
  if (MPI::num_processes() > 1)
  {
    info(TRACE, "Creating parallel PETSc Krylov solver (for LU factorization).");
    KSPCreate(PETSC_COMM_WORLD, ksp.get());
  }
  else
    KSPCreate(PETSC_COMM_SELF, ksp.get());

  // Set preconditioner to LU factorization
  PC pc;
  KSPGetPC(*ksp, &pc);
  PCSetType(pc, PCLU);

  // Set solver package
  PCFactorSetMatSolverPackage(pc, lu_packages.find(lu_package)->second);

  // Allow matrices with zero diagonals to be solved
  #if PETSC_VERSION_MAJOR == 3 && PETSC_VERSION_MINOR == 1
  PCFactorSetShiftType(pc, MAT_SHIFT_NONZERO);
  PCFactorSetShiftAmount(pc, PETSC_DECIDE);
  #else
  PCFactorSetShiftNonzero(pc, PETSC_DECIDE);
  #endif
}
//-----------------------------------------------------------------------------

#endif
