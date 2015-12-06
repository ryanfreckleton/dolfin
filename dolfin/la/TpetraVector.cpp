// Copyright (C) 2014 Chris Richardson
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
//
// First added: Nov 2014

#ifdef HAS_TRILINOS

#include <cmath>
#include <numeric>
#include <dolfin/common/Timer.h>
#include <dolfin/common/Array.h>
#include <dolfin/common/MPI.h>
#include <dolfin/common/NoDeleter.h>
#include <dolfin/common/Set.h>
#include "TpetraFactory.h"
#include "TpetraVector.h"

using namespace dolfin;

//-----------------------------------------------------------------------------
TpetraVector::TpetraVector()
{
  // Do nothing
}
//-----------------------------------------------------------------------------
TpetraVector::TpetraVector(MPI_Comm comm, std::size_t N)
{
  init(comm, N);
}
//-----------------------------------------------------------------------------
TpetraVector::TpetraVector(const TpetraVector& v)
{
  if (v._x.is_null())
    return;

  // Create with same map
  Teuchos::RCP<const map_type> v_ghostmap(v._x_ghosted->getMap());
  Teuchos::RCP<const map_type> v_xmap(v._x->getMap());
  _x_ghosted = Teuchos::rcp(new vector_type(v_ghostmap, 1));

  _x_ghosted->assign(*v._x_ghosted);
  _x = _x_ghosted->offsetViewNonConst(v_xmap, 0);
}
//-----------------------------------------------------------------------------
TpetraVector::~TpetraVector()
{
  // Do nothing
}
//-----------------------------------------------------------------------------
void TpetraVector::zero()
{
  dolfin_assert(!_x.is_null());
  _x_ghosted->putScalar(0.0);
}
//-----------------------------------------------------------------------------
void TpetraVector::apply(std::string mode)
{
  dolfin_assert(!_x.is_null());
  std::cout << "Apply called with: " << mode << "\n";

  Teuchos::RCP<const map_type> xmap(_x->getMap());
  Teuchos::RCP<vector_type> y(new vector_type(xmap, 1));
  Teuchos::RCP<const map_type> ghostmap(_x_ghosted->getMap());

  // Export from overlapping map ghostmap, to non-overlapping xmap
  Tpetra::Export<dolfin::la_index> exporter(ghostmap, xmap);

  // Forward export to reduction vector
  if (mode == "add")
    y->doExport(*_x_ghosted, exporter, Tpetra::ADD);
  else if (mode == "insert")
    y->doExport(*_x_ghosted, exporter, Tpetra::INSERT);

  // Copy back into _x
  std::copy(y->getData(0).begin(), y->getData(0).end(),
            _x->getDataNonConst(0).begin());
}
//-----------------------------------------------------------------------------
MPI_Comm TpetraVector::mpi_comm() const
{
  // Unwrap MPI_Comm
  const Teuchos::RCP<const Teuchos::MpiComm<int>> _mpi_comm
    = Teuchos::rcp_dynamic_cast<const Teuchos::MpiComm<int>>(_x->getMap()->getComm());

  return *(_mpi_comm->getRawMpiComm());
}
//-----------------------------------------------------------------------------
std::string TpetraVector::str(bool verbose) const
{
  if (_x.is_null())
    return "<Uninitialized TpetraVector>";

  std::stringstream s;
  if (verbose)
    s << "< " << _x->description() << " >";
  else
    s << "<TpetraVector of size " << size() << ">";

  return s.str();
}
//-----------------------------------------------------------------------------
std::shared_ptr<GenericVector> TpetraVector::copy() const
{
  return std::shared_ptr<GenericVector>(new TpetraVector(*this));
}
//-----------------------------------------------------------------------------
void TpetraVector::init(MPI_Comm comm, std::size_t N)
{
  std::pair<std::size_t, std::size_t> range = MPI::local_range(comm, N);
    std::vector<dolfin::la_index> local_to_global_map;
    _init(comm, range, local_to_global_map);
}
//-----------------------------------------------------------------------------
void TpetraVector::init(MPI_Comm comm,
                        std::pair<std::size_t, std::size_t> range)
{
  std::vector<dolfin::la_index> local_to_global_map;
  _init(comm, range, local_to_global_map);
}
//-----------------------------------------------------------------------------
void TpetraVector::init(MPI_Comm comm,
                        std::pair<std::size_t, std::size_t> range,
                        const std::vector<std::size_t>& local_to_global_map,
                        const std::vector<la_index>& ghost_indices)
{
  std::vector<dolfin::la_index> _global_map(local_to_global_map.begin(),
                                            local_to_global_map.end());
  _init(comm, range, _global_map);
}
//-----------------------------------------------------------------------------
bool TpetraVector::empty() const
{
  return (size() == 0);
}
//-----------------------------------------------------------------------------
std::size_t TpetraVector::size() const
{
  if (_x.is_null())
    return 0;
  return (_x->getMap()->getMaxAllGlobalIndex() + 1);
}
//-----------------------------------------------------------------------------
std::size_t TpetraVector::local_size() const
{
  if (_x.is_null())
    return 0;
  return _x->getLocalLength();
}
//-----------------------------------------------------------------------------
std::pair<std::size_t, std::size_t> TpetraVector::local_range() const
{
  dolfin_assert(!_x.is_null());

  return std::make_pair(_x->getMap()->getMinGlobalIndex(),
                        _x->getMap()->getMaxGlobalIndex() + 1);
}
//-----------------------------------------------------------------------------
bool TpetraVector::owns_index(std::size_t i) const
{
  dolfin_assert(!_x.is_null());

  std::pair<std::size_t, std::size_t> range = local_range();
  return (i >= range.first and i < range.second);
}
//-----------------------------------------------------------------------------
void TpetraVector::get(double* block, std::size_t m,
                       const dolfin::la_index* rows) const
{
  dolfin_assert(!_x_ghosted.is_null());

  Teuchos::RCP<const map_type> xmap(_x_ghosted->getMap());
  Teuchos::ArrayRCP<const double> xarr = _x_ghosted->getData(0);
  for (std::size_t i = 0; i != m; ++i)
  {
    const int idx = xmap->getLocalElement(rows[i]);
    if (idx != Teuchos::OrdinalTraits<int>::invalid())
      block[i] = xarr[idx];
    else
      dolfin_error("TpetraVector.cpp", "get data",
                   "Row %d not valid", rows[i]);
  }
}
//-----------------------------------------------------------------------------
void TpetraVector::update_ghost_values()
{
  dolfin_assert(!_x.is_null());

  Teuchos::RCP<const map_type> xmap(_x->getMap());
  Teuchos::RCP<const map_type> ghostmap(_x_ghosted->getMap());

  // Export from non-overlapping map x, to overlapping ghostmap
  Tpetra::Import<dolfin::la_index> importer(xmap, ghostmap);

  // FIXME: is this safe, since _x is a view into _x_ghosted?
  _x_ghosted->doImport(*_x, importer, Tpetra::INSERT);

  // Copy back into _x_ghosted from temp vector
  //  std::copy(y->getData(0).begin(), y->getData(0).end(),
  //            _x_ghosted->getDataNonConst(0).begin());
}
//-----------------------------------------------------------------------------
void TpetraVector::get_local(double* block, std::size_t m,
                             const dolfin::la_index* rows) const
{
  dolfin_assert(!_x_ghosted.is_null());
  Teuchos::ArrayRCP<const double> arr = _x_ghosted->getData(0);
  for (std::size_t i = 0; i!=m; ++i)
  {
    if (_x_ghosted->getMap()->isNodeLocalElement(rows[i]))
      block[i] = arr[rows[i]];
    else
      dolfin_error("TpetraVector.cpp",
                   "get local row",
                   "Row %d is not local on rank %d", rows[i],
                   _x_ghosted->getMap()->getComm()->getRank());
  }
}
//-----------------------------------------------------------------------------
void TpetraVector::set(const double* block, std::size_t m,
                       const dolfin::la_index* rows)
{
  dolfin_assert(!_x_ghosted.is_null());
  for (std::size_t i = 0; i != m; ++i)
  {
    if(_x_ghosted->getMap()->isNodeGlobalElement(rows[i]))
      _x_ghosted->replaceGlobalValue(rows[i], 0, block[i]);
    else
    {
      dolfin_error("TpetraVector.cpp", "set data",
                   "Row %d not valid", rows[i]);
    }
  }
}
//-----------------------------------------------------------------------------
void TpetraVector::set_local(const double* block, std::size_t m,
                             const dolfin::la_index* rows)
{
  dolfin_assert(!_x_ghosted.is_null());
  for (std::size_t i = 0; i != m; ++i)
  {
    if(_x_ghosted->getMap()->isNodeLocalElement(rows[i]))
      _x_ghosted->replaceLocalValue(rows[i], 0, block[i]);
    else
    {
      dolfin_error("TpetraVector.cpp", "set data",
                   "Row %d not valid", rows[i]);
    }
  }
}
//-----------------------------------------------------------------------------
void TpetraVector::add(const double* block, std::size_t m,
                       const dolfin::la_index* rows)
{
  dolfin_assert(!_x_ghosted.is_null());
  for (std::size_t i = 0; i != m; ++i)
  {
    if(_x_ghosted->getMap()->isNodeGlobalElement(rows[i]))
      _x_ghosted->sumIntoGlobalValue(rows[i], 0, block[i]);
    else
      dolfin_error("TpetraVector.cpp", "add into row",
                   "Row %d is not local", rows[i]);
  }
}
//-----------------------------------------------------------------------------
void TpetraVector::add_local(const double* block, std::size_t m,
                             const dolfin::la_index* rows)
{
  dolfin_assert(!_x_ghosted.is_null());

  for (std::size_t i = 0; i != m; ++i)
  {
    if(_x_ghosted->getMap()->isNodeLocalElement(rows[i]))
      _x_ghosted->sumIntoLocalValue(rows[i], 0, block[i]);
    else
      dolfin_error("TpetraVector.cpp",
                   "add into local row",
                   "Row %d is not local", rows[i]);
  }
}
//-----------------------------------------------------------------------------
void TpetraVector::get_local(std::vector<double>& values) const
{
  dolfin_assert(!_x.is_null());
  values.resize(local_size());
  Teuchos::ArrayRCP<const double> arr = _x->getData(0);
  std::copy(arr.get(), arr.get() + values.size(), values.begin());
}
//-----------------------------------------------------------------------------
void TpetraVector::set_local(const std::vector<double>& values)
{
  dolfin_assert(!_x.is_null());
  const std::size_t num_values = local_size();
  if (values.size() != num_values)
  {
    dolfin_error("TpetraVector.cpp",
                 "set local values of Tpetra vector",
                 "Size of values array is not equal to local vector size");
  }

  if (num_values == 0)
    return;

  Teuchos::ArrayRCP<double> arr = _x->getDataNonConst(0);
  std::copy(values.begin(), values.end(), arr.get());
}
//-----------------------------------------------------------------------------
void TpetraVector::add_local(const Array<double>& values)
{
  dolfin_assert(!_x.is_null());

  const std::size_t num_values = local_size();
  if (values.size() != num_values)
  {
    dolfin_error("TpetraVector.cpp",
                 "add local values to Tpetra vector",
                 "Size of values array is not equal to local vector size");
  }

  for (std::size_t i = 0; i != num_values; ++i)
    _x->sumIntoLocalValue(i, 0, values[i]);
}
//-----------------------------------------------------------------------------
void TpetraVector::gather(GenericVector& y,
                          const std::vector<dolfin::la_index>& indices) const
{
  dolfin_assert(!_x.is_null());

  // FIXME: not working?

  TpetraVector& _y = as_type<TpetraVector>(y);

  const std::pair<std::size_t, std::size_t> range(0, indices.size());

  if (_y._x.is_null())
    _y._init(MPI_COMM_SELF, range, indices);
  else if (y.size() != indices.size() || MPI::size(y.mpi_comm()) != 0)
  {
    dolfin_error("TpetraVector.cpp",
                 "gather vector entries",
                 "Cannot re-initialize gather vector. Must be empty, or have correct size and be a local vector");
  }

  const Tpetra::Export<dolfin::la_index>
    exporter(_x->getMap(), _y._x->getMap());
  _y._x->doExport(*_x, exporter, Tpetra::INSERT);
}
//-----------------------------------------------------------------------------
void TpetraVector::gather(std::vector<double>& x,
                          const std::vector<dolfin::la_index>& indices) const
{
  x.resize(indices.size());
  TpetraVector y;
  gather(y, indices);
  dolfin_assert(y.local_size() == x.size());
  y.get_local(x);
}
//-----------------------------------------------------------------------------
void TpetraVector::gather_on_zero(std::vector<double>& v) const
{
  dolfin_assert(!_x.is_null());

  if (_x->getMap()->getComm()->getRank() == 0)
    v.resize(size());
  else
    v.resize(0);

  // Create map with elements only on process zero
  Teuchos::RCP<map_type> ymap(new map_type(size(), v.size(), 0,
                                           _x->getMap()->getComm()));
  Teuchos::RCP<vector_type> y(new vector_type(ymap, 1));

  // Export from vector x to vector y
  const Tpetra::Export<dolfin::la_index>
    exporter(_x->getMap(), y->getMap());
  y->doExport(*_x, exporter, Tpetra::INSERT);
  Teuchos::ArrayRCP<const double> yarr(y->getData(0));
  std::copy(yarr.get(), yarr.get() + v.size(), v.begin());
}
//-----------------------------------------------------------------------------
void TpetraVector::axpy(double a, const GenericVector& y)
{
  dolfin_assert(!_x_ghosted.is_null());
  const TpetraVector& _y = as_type<const TpetraVector>(y);
  dolfin_assert(!_y._x_ghosted.is_null());
  _x_ghosted->update(1.0, *_y._x_ghosted, a);
}
//-----------------------------------------------------------------------------
void TpetraVector::abs()
{
  dolfin_assert(!_x_ghosted.is_null());
  // FIXME: check this is OK
  _x_ghosted->abs(*_x_ghosted);
}
//-----------------------------------------------------------------------------
double TpetraVector::inner(const GenericVector& y) const
{
  dolfin_assert(!_x.is_null());
  const TpetraVector& _y = as_type<const TpetraVector>(y);
  dolfin_assert(!_y._x.is_null());
  std::vector<double> val(1);
  const Teuchos::ArrayView<double> result(val);
  _x->dot(*_y._x, result);
  return val[0];
}
//-----------------------------------------------------------------------------
double TpetraVector::norm(std::string norm_type) const
{
  dolfin_assert(!_x.is_null());
  typedef Tpetra::MultiVector<>::mag_type mag_type;
  std::vector<mag_type> norms(1);
  const Teuchos::ArrayView<mag_type> norm_view(norms);
  _x->norm2(norm_view);
  return norms[0];
}
//-----------------------------------------------------------------------------
double TpetraVector::min() const
{
  dolfin_assert(!_x.is_null());
  Teuchos::ArrayRCP<const double> arr = _x->getData(0);
  double min_local
    = *std::min_element(arr.get(), arr.get() + arr.size());

  return MPI::min(mpi_comm(), min_local);
}
//-----------------------------------------------------------------------------
double TpetraVector::max() const
{
  dolfin_assert(!_x.is_null());
  Teuchos::ArrayRCP<const double> arr = _x->getData(0);
  double max_local
    = *std::max_element(arr.get(), arr.get() + arr.size());

  return MPI::max(mpi_comm(), max_local);
}
//-----------------------------------------------------------------------------
double TpetraVector::sum() const
{
  dolfin_assert(!_x.is_null());

  Teuchos::ArrayRCP<const double> arr(_x->getData(0));

  double _sum = 0.0;
  const std::size_t m = local_size();
  for (std::size_t i = 0; i != m; ++i)
    _sum += arr[i];

  return MPI::sum(mpi_comm(), _sum);
}
//-----------------------------------------------------------------------------
double TpetraVector::sum(const Array<std::size_t>& rows) const
{
  dolfin_assert(!_x.is_null());

  // FIXME

  dolfin_not_implemented();

  return 0.0;
}
//-----------------------------------------------------------------------------
const TpetraVector& TpetraVector::operator*= (double a)
{
  dolfin_assert(!_x_ghosted.is_null());
  _x_ghosted->scale(a);
  return *this;
}
//-----------------------------------------------------------------------------
const TpetraVector& TpetraVector::operator*= (const GenericVector& y)
{
  dolfin_assert(!_x.is_null());
  const TpetraVector& _y = as_type<const TpetraVector>(y);
  _x->elementWiseMultiply(1.0, *(_x->getVector(0)), *(_y._x), 0.0);
  return *this;
}
//-----------------------------------------------------------------------------
const TpetraVector& TpetraVector::operator/= (double a)
{
  dolfin_assert(!_x.is_null());
  dolfin_assert(a != 0.0);
  const double b = 1.0/a;
  (*this) *= b;
  return *this;
}
//-----------------------------------------------------------------------------
const TpetraVector& TpetraVector::operator+= (const GenericVector& y)
{
  axpy(1.0, y);
  return *this;
}
//-----------------------------------------------------------------------------
const TpetraVector& TpetraVector::operator+= (double a)
{
  dolfin_assert(!_x_ghosted.is_null());

  const std::size_t num_values = local_size();
  for (std::size_t i = 0; i != num_values; ++i)
    _x_ghosted->sumIntoLocalValue(i, 0, a);

  return *this;
}
//-----------------------------------------------------------------------------
const TpetraVector& TpetraVector::operator-= (const GenericVector& x)
{
  dolfin_assert(!_x.is_null());
  axpy(-1.0, x);
  return *this;
}
//-----------------------------------------------------------------------------
const TpetraVector& TpetraVector::operator-= (double a)
{
  dolfin_assert(!_x.is_null());
  (*this) += -a;
  return *this;
}
//-----------------------------------------------------------------------------
const GenericVector& TpetraVector::operator= (const GenericVector& v)
{
  *this = as_type<const TpetraVector>(v);
  return *this;
}
//-----------------------------------------------------------------------------
const TpetraVector& TpetraVector::operator= (double a)
{
  dolfin_assert(!_x.is_null());
  _x->putScalar(a);
  return *this;
}
//-----------------------------------------------------------------------------
const TpetraVector& TpetraVector::operator= (const TpetraVector& v)
{
  // Check that vector lengths are equal
  if (size() != v.size())
  {
    dolfin_error("TpetraVector.cpp",
                 "assign one vector to another",
                 "Vectors must be of the same length when assigning. "
                 "Consider using the copy constructor instead");
  }

  // Check that vector local ranges are equal (relevant in parallel)
  if (local_range() != v.local_range())
  {
    dolfin_error("TpetraVector.cpp",
                 "assign one vector to another",
                 "Vectors must have the same parallel layout when assigning. "
                 "Consider using the copy constructor instead");
  }

  // Check for self-assignment
  if (this != &v)
  {
    // Copy data (local operation)
    dolfin_assert(!v._x.is_null());
    dolfin_assert(!_x.is_null());

    _x->assign(*v._x);
  }

  return *this;
}
//-----------------------------------------------------------------------------
GenericLinearAlgebraFactory& TpetraVector::factory() const
{
  return TpetraFactory::instance();
}
//-----------------------------------------------------------------------------
void TpetraVector::init(const TensorLayout& layout)
{
  if (!_x.is_null())
    dolfin_error("TpetraVector.h",
                 "initialize vector",
                 "Vector cannot be initialised more than once");
  if (layout.rank() != 1 || layout.sparsity_pattern())
  {
    dolfin_error("TpetraVector.h",
                 "calling Tpetra::init(const TensorLayout&)",
                 "Expected dense, rank 1, layout"
  }

  // Make a Trilinos version of the MPI Comm
  Teuchos::RCP<const Teuchos::Comm<int>> _comm(new Teuchos::MpiComm<int>(comm));

  // Mapping across processes
  const std::size_t n = layout.index_map(0).size(IndexMap::MapSize::OWNED);
  const std::size_t N = layout.index_map(0).size(IndexMap::MapSize::GLOBAL);
  const std::size_t u = layout.index_map(0).size(IndexMap::MapSize::UNOWNED);
  const int bs = layout.index_map(0).block_size();
  const std::size_t r0 = layout.index_map(0).local_range().first;

  Teuchos::RCP<map_type> _map(new map_type(N, n, 0, _comm));
  Teuchos::RCP<map_type> _ghost_map;

  // Save a map for the ghosting of values on other processes
  std::vector<dolfin::la_index> local_to_global_map(n+u);
  const std::vector<std::size_t>& local_to_global_unowned
    = layout.local_to_global_unowned();
  dolfin_assert(bs*local_to_global_unowned.size() == u);

  // FIXME: Is trivial part of local_to_global required by Tpetra?
  for (std::size_t i = 0; i < n; ++i)
    local_to_global_map[i] = i + r0;
  for (std::size_t i = 0; i < local_to_global_unowned.size(); ++i)
    local_to_global_map[i+n] = bs*local_to_global_unowned[i];

  // FIXME: Is it needed to distinguish GHOSTED and UNGHOSTED in Tpetra?
  //        Should the following condition read
  //        if (layout.is_ghosted() == TensorLayout::Ghosts::GHOSTED)
  if (local_to_global_map.size() != 0)
  {
    const Teuchos::ArrayView<const dolfin::la_index>
      local_indices(local_to_global_map);
    _ghost_map = Teuchos::rcp(new map_type(N, local_indices, 0, _comm));
  }
  else
    _ghost_map = _map;

  // Vector - create with overlap
  _x_ghosted = Teuchos::rcp(new vector_type(_ghost_map, 1));

  dolfin_assert(!_x_ghosted.is_null());

  // Get a modifiable view into the ghosted vector
  _x = _x_ghosted->offsetViewNonConst(_map, 0);
}
//-----------------------------------------------------------------------------
void
TpetraVector::_init(MPI_Comm comm,
                    std::pair<std::size_t, std::size_t> local_range,
                    const std::vector<dolfin::la_index>& local_to_global_map)
{
  if (!_x.is_null())
    dolfin_error("TpetraVector.h",
                 "initialize vector",
                 "Vector cannot be initialised more than once");

  // Make a Trilinos version of the MPI Comm
  Teuchos::RCP<const Teuchos::Comm<int>> _comm(new Teuchos::MpiComm<int>(comm));

  // Mapping across processes
  std::size_t Nlocal = local_range.second - local_range.first;
  std::size_t N = MPI::sum(comm, Nlocal);

  Teuchos::RCP<map_type> _map(new map_type(N, Nlocal, 0, _comm));
  Teuchos::RCP<map_type> _ghost_map;

  // Save a map for the ghosting of values on other processes
  if (local_to_global_map.size() != 0)
  {
    const Teuchos::ArrayView<const dolfin::la_index>
      local_indices(local_to_global_map);
    _ghost_map = Teuchos::rcp(new map_type(N, local_indices, 0, _comm));
  }
  else
    _ghost_map = _map;

  // Vector - create with overlap
  _x_ghosted = Teuchos::rcp(new vector_type(_ghost_map, 1));

  dolfin_assert(!_x_ghosted.is_null());

  // Get a modifiable view into the ghosted vector
  _x = _x_ghosted->offsetViewNonConst(_map, 0);
}
//-----------------------------------------------------------------------------
Teuchos::RCP<TpetraVector::vector_type> TpetraVector::vec() const
{
  return _x;
}
//-----------------------------------------------------------------------------
void TpetraVector::mapdump(const std::string desc)
{
  mapdump(_x->getMap(), desc);
}
//-----------------------------------------------------------------------------
void TpetraVector::mapdump(Teuchos::RCP<const map_type> xmap,
                           const std::string desc)
{
  std::stringstream ss;

  int rank = xmap->getComm()->getRank();
  int m = xmap->getMaxAllGlobalIndex() + 1;
  if (rank == 0)
  {
    ss << xmap->description() << "\n" << desc << "\n---";
    for (int j = 0; j != m ; ++j)
      ss << "-";
    ss << "\n";
  }

  ss << rank << "] ";
  for (int j = 0; j != m ; ++j)
  {
    if (xmap->isNodeGlobalElement(j))
      ss << "X";
    else
      ss << " ";
  }
  ss << "\n";

  const Teuchos::RCP<const Teuchos::MpiComm<int>> _mpi_comm
    = Teuchos::rcp_dynamic_cast<const Teuchos::MpiComm<int>>(xmap()->getComm());

  MPI_Comm mpi_comm = *(_mpi_comm->getRawMpiComm());

  std::vector<std::string> out_str;
  MPI::gather(mpi_comm, ss.str(), out_str);

  if (rank == 0)
  {
    for (auto &s: out_str)
      std::cout << s;
  }
}
//-----------------------------------------------------------------------------

#endif
