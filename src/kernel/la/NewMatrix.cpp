// Copyright (C) 2004 Johan Hoffman and Anders Logg.
// Licensed under the GNU GPL Version 2.

#include <dolfin/dolfin_log.h>
#include <dolfin/PETScManager.h>
#include <dolfin/NewMatrix.h>
#include <dolfin/NewVector.h>

using namespace dolfin;

//-----------------------------------------------------------------------------
NewMatrix::NewMatrix()
{
  // Initialize PETSc
  PETScManager::init();

  // Don't initialize the matrix
  A = 0;
}
//-----------------------------------------------------------------------------
NewMatrix::NewMatrix(int m, int n)
{
  // Initialize PETSc
  PETScManager::init();

  // Create PETSc matrix
  A = 0;
  init(m, n);
}
//-----------------------------------------------------------------------------
NewMatrix::NewMatrix(const Matrix &B)
{
  // Initialize PETSc
  PETScManager::init();

  // Create PETSc matrix
  A = 0;
  init(B.size(0), B.size(1));

  
  unsigned int m = B.size(0);
  unsigned int n = B.size(1);

  for(unsigned int i = 0; i < m; i++)
  {
    for(unsigned int j = 0; j < n; j++)
    {
      setval(i, j, B(i, j));
    }
  }
}
//-----------------------------------------------------------------------------
NewMatrix::~NewMatrix()
{
  // Free memory of matrix
  if ( A ) MatDestroy(A);
}
//-----------------------------------------------------------------------------
void NewMatrix::init(int m, int n)
{
  // Free previously allocated memory if necessary
  if ( A )
    if ( m == size(0) && n == size(1) )
      return;
    else
      MatDestroy(A);
  
  //  MatCreate(PETSC_COMM_WORLD, PETSC_DECIDE, PETSC_DECIDE, m, n, &A);
  //  MatSetFromOptions(A);

  // Creates a sparse matrix in block AIJ (block compressed row) format.
  // Assuming blocksize bs=1, and max no connectivity = 50 
  MatCreateSeqBAIJ(PETSC_COMM_SELF, 1, m, m, 50, PETSC_NULL, &A);
}
//-----------------------------------------------------------------------------
void NewMatrix::init(int m, int n, int bs)
{
  // Free previously allocated memory if necessary
  if ( A )
    if ( m == size(0) && n == size(1) )
      return;
    else
      MatDestroy(A);
  
  // Creates a sparse matrix in block AIJ (block compressed row) format.
  // Given blocksize bs, and assuming max no connectivity = 50. 
  MatCreateSeqBAIJ(PETSC_COMM_SELF, bs, bs*m, bs*m, 50, PETSC_NULL, &A);
}
//-----------------------------------------------------------------------------
void NewMatrix::init(int m, int n, int bs, int mnc)
{
  // Free previously allocated memory if necessary
  if ( A )
    if ( m == size(0) && n == size(1) )
      return;
    else
      MatDestroy(A);
  
  // Creates a sparse matrix in block AIJ (block compressed row) format.
  // Given blocksize bs, and max no connectivity mnc.  
  MatCreateSeqBAIJ(PETSC_COMM_SELF, bs, bs*m, bs*m, mnc, PETSC_NULL, &A);
}
//-----------------------------------------------------------------------------
int NewMatrix::size(int dim) const
{
  int m = 0;
  int n = 0;
  MatGetSize(A, &m, &n);
  return (dim == 0 ? m : n);
}
//-----------------------------------------------------------------------------
NewMatrix& NewMatrix::operator= (real zero)
{
  if ( zero != 0.0 )
    dolfin_error("Argument must be zero.");
  MatZeroEntries(A);
  return *this;
}
//-----------------------------------------------------------------------------
void NewMatrix::add(const real block[],
		    const int rows[], int m,
		    const int cols[], int n)
{
  MatSetValues(A, m, rows, n, cols, block, ADD_VALUES);
}
//-----------------------------------------------------------------------------
void NewMatrix::mult(const NewVector& x, NewVector& Ax) const
{
  MatMult(A, x.vec(), Ax.vec());
}
//-----------------------------------------------------------------------------
void NewMatrix::apply()
{
  MatAssemblyBegin(A, MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(A, MAT_FINAL_ASSEMBLY);
}
//-----------------------------------------------------------------------------
Mat NewMatrix::mat()
{
  return A;
}
//-----------------------------------------------------------------------------
const Mat NewMatrix::mat() const
{
  return A;
}
//-----------------------------------------------------------------------------
void NewMatrix::disp() const
{
  MatView(A, PETSC_VIEWER_STDOUT_SELF);
}
//-----------------------------------------------------------------------------
LogStream& dolfin::operator<< (LogStream& stream, const NewMatrix& A)
{
  MatType type = 0;
  MatGetType(A.mat(), &type);
  int m = A.size(0);
  int n = A.size(1);
  stream << "[ PETSc matrix (type " << type << ") of size "
	 << m << " x " << n << " ]";

  return stream;
}
//-----------------------------------------------------------------------------
NewMatrix::Element NewMatrix::operator()(uint i, uint j)
{
  Element element(i, j, *this);

  return element;
}
//-----------------------------------------------------------------------------
real NewMatrix::getval(uint i, uint j) const
{
  const int ii = static_cast<int>(i);
  const int jj = static_cast<int>(j);

  PetscScalar val;
  MatGetValues(A, 1, &ii, 1, &jj, &val);

  return val;
}
//-----------------------------------------------------------------------------
void NewMatrix::setval(uint i, uint j, const real a)
{
  MatSetValue(A, i, j, a, INSERT_VALUES);

  MatAssemblyBegin(A, MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(A, MAT_FINAL_ASSEMBLY);
}
//-----------------------------------------------------------------------------
void NewMatrix::addval(uint i, uint j, const real a)
{
  MatSetValue(A, i, j, a, ADD_VALUES);

  MatAssemblyBegin(A, MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(A, MAT_FINAL_ASSEMBLY);
}
//-----------------------------------------------------------------------------
// NewMatrix::Element
//-----------------------------------------------------------------------------
NewMatrix::Element::Element(uint i, uint j, NewMatrix& A) : i(i), j(j), A(A)
{
  // Do nothing
}
//-----------------------------------------------------------------------------
NewMatrix::Element::operator real() const
{
  return A.getval(i, j);
}
//-----------------------------------------------------------------------------
const NewMatrix::Element& NewMatrix::Element::operator=(const real a)
{
  A.setval(i, j, a);

  return *this;
}
//-----------------------------------------------------------------------------
const NewMatrix::Element& NewMatrix::Element::operator+=(const real a)
{
  A.addval(i, j, a);

  return *this;
}
//-----------------------------------------------------------------------------
const NewMatrix::Element& NewMatrix::Element::operator-=(const real a)
{
  A.addval(i, j, -a);

  return *this;
}
//-----------------------------------------------------------------------------
const NewMatrix::Element& NewMatrix::Element::operator*=(const real a)
{
  const real val = A.getval(i, j) * a;
  A.setval(i, j, val);
  
  return *this;
}
//-----------------------------------------------------------------------------
