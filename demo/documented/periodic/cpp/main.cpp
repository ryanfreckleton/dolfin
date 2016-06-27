// Copyright (C) 2007-2008 Anders Logg
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
// First added:  2007-07-11
// Last changed: 2012-11-12
//
// This demo program solves Poisson's equation,
//
//     - div grad u(x, y) = f(x, y)
//
// on the unit square with homogeneous Dirichlet boundary conditions
// at y = 0, 1 and periodic boundary conditions at x = 0, 1.

#include <dolfin.h>
#include "Poisson.h"

using namespace dolfin;

int main()
{
  // Source term
  class Source : public Expression
  {
  public:

    void eval(Array<double>& values, const Array<double>& x) const
    {
      const double dx = x[0] - 0.5;
      const double dy = x[1] - 0.5;
      values[0] = x[0]*sin(5.0*DOLFIN_PI*x[1]) + 1.0*exp(-(dx*dx + dy*dy)/0.02);
    }

  };

  // Sub domain for Dirichlet boundary condition
  class DirichletBoundary : public SubDomain
  {
    bool inside(const Array<double>& x, bool on_boundary) const
    { return (x[1] < DOLFIN_EPS || x[1] > (1.0 - DOLFIN_EPS)) && on_boundary; }
  };

  // Sub domain for Periodic boundary condition
  class PeriodicBoundary : public SubDomain
  {
    // Left boundary is "target domain" G
    bool inside(const Array<double>& x, bool on_boundary) const
    { return (std::abs(x[0]) < DOLFIN_EPS); }

    // Map right boundary (H) to left boundary (G)
    void map(const Array<double>& x, Array<double>& y) const
    {
      y[0] = x[0] - 1.0;
      y[1] = x[1];
    }
  };

  // Create mesh
  auto mesh = std::make_shared<Mesh>();

  MeshEditor ed;
  ed.open(*mesh, 2, 2);
  const unsigned int nx = 33, ny = 33;
  ed.init_vertices(nx*ny);
  unsigned int c = 0;
  for (unsigned int i = 0; i != nx; ++i)
    for (unsigned int j = 0; j != ny; ++j)
    {
      double x = (double)i/(double)(nx - 1);
      double y = (double)j/(double)(ny - 1);
      ed.add_vertex(c, x, y);
      ++c;
    }

  ed.init_cells((nx - 1)*(ny - 1)*2);
  c = 0;
  for (unsigned int j = 0; j != ny - 2; ++j)
    for (unsigned int i = 0; i != nx - 1; ++i)
    {
      unsigned int ix = i + nx*j;
      ed.add_cell(c, ix, ix + 1,  ix + nx);
      ++c;
      ed.add_cell(c, ix + 1, ix + nx, ix + 1 + nx);
      ++c;
    }
  for (unsigned int i = 0; i != nx - 1; ++i)
  {
    unsigned int ix = i + nx*(ny - 2);
    ed.add_cell(c, ix, ix + 1, i);
    ++c;
    ed.add_cell(c, ix + 1, i, i + 1);
    ++c;
  }
  ed.close();

  File file_mesh("mesh.pvd");
  file_mesh << *mesh;

  // Create functions
  auto f = std::make_shared<Source>();

  // Define PDE
  auto V = std::make_shared<Poisson::FunctionSpace>(mesh);
  Poisson::BilinearForm a(V, V);
  Poisson::LinearForm L(V);
  L.f = f;

  // Create Dirichlet boundary condition
  auto u0 = std::make_shared<Constant>(0.0);
  auto dirichlet_boundary = std::make_shared<DirichletBoundary>();
  DirichletBC bc0(V, u0, dirichlet_boundary);

  // Collect boundary conditions
  std::vector<const DirichletBC*> bcs = {&bc0};

  // Compute solution
  Function u(V);


  solve(a == L, u, bcs);

  // Save solution in VTK format
  File file_u("periodic.pvd");
  file_u << u;

  // Plot solution
  //  plot(u);
  //  interactive();

  return 0;
}
