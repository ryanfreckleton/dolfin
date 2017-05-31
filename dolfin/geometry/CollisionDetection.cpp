// Copyright (C) 2014 Anders Logg and August Johansson
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
// Modified by Chris Richardson, 2014.
//
// First added:  2014-02-03
// Last changed: 2014-04-03
//
//-----------------------------------------------------------------------------
// Special note regarding the function collides_tetrahedron_tetrahedron
//-----------------------------------------------------------------------------
//
// The source code for the tetrahedron-tetrahedron collision test is
// from Fabio Ganovelli, Federico Ponchio and Claudio Rocchini: Fast
// Tetrahedron-Tetrahedron Overlap Algorithm, Journal of Graphics
// Tools, 7(2), 2002, and is under the following copyright:
//
// Visual Computing Group
// IEI Institute, CNUCE Institute, CNR Pisa
//
// Copyright(C) 2002 by Fabio Ganovelli, Federico Ponchio and Claudio
// Rocchini
//
// All rights reserved.
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without
// fee, provided that the above copyright notice appear in all copies
// and that both that copyright notice and this permission notice
// appear in supporting documentation. the author makes no
// representations about the suitability of this software for any
// purpose. It is provided "as is" without express or implied
// warranty.
//
//-----------------------------------------------------------------------------
// Special note regarding the function collides_triangle_triangle
//-----------------------------------------------------------------------------
//
// The source code for the triangle-triangle collision test is from
// Tomas Moller: A Fast Triangle-Triangle Intersection Test, Journal
// of Graphics Tools, 2(2), 1997, and is in the public domain.
//
//-----------------------------------------------------------------------------

#include <dolfin/mesh/MeshEntity.h>
#include "Point.h"
#include "CollisionDetection.h"

using namespace dolfin;

//-----------------------------------------------------------------------------
bool CollisionDetection::collides(const MeshEntity& entity,
                                  const Point& point)
{
  switch (entity.dim())
  {
  case 0:
    dolfin_not_implemented();
    break;
  case 1:
    return collides_interval_point(entity, point);
  case 2:
    return collides_triangle_point(entity, point);
  case 3:
    return collides_tetrahedron_point(entity, point);
  default:
    dolfin_error("CollisionDetection.cpp",
		 "collides entity with point",
		 "Unknown dimension of entity");
  }

  return false;
}
//-----------------------------------------------------------------------------
bool CollisionDetection::collides(const MeshEntity& entity,
                                  const Point& x1,
                                  const Point& x2)
{
  switch (entity.dim())
  {
    case 0:
      dolfin_not_implemented();
      break;
    case 1:
      return collides_interval_interval(entity, x1, x2);
    case 2:
      return collides_triangle_interval(entity, x1, x2);
    case 3:
      dolfin_not_implemented();
      break;
    default:
      dolfin_error("CollisionDetection.cpp",
                   "collides entity with interval",
                   "Unknown dimension of entity");
  }

  return false;
}
//-----------------------------------------------------------------------------
bool
CollisionDetection::collides(const MeshEntity& entity_0,
			     const MeshEntity& entity_1)
{
  switch (entity_0.dim())
  {
  case 0:
    // Collision with PointCell
    dolfin_not_implemented();
    break;
  case 1:
    // Collision with interval
    switch (entity_1.dim())
    {
    case 0:
      dolfin_not_implemented();
      break;
    case 1:
      return collides_interval_interval(entity_1, entity_0);
      break;
    case 2:
      dolfin_not_implemented();
      break;
    case 3:
      dolfin_not_implemented();
      break;
    default:
      dolfin_error("CollisionDetection.cpp",
                   "collides entity_0 with entity_1",
                   "Unknown dimension of entity_1 in IntervalCell collision");
    }
    break;
  case 2:
    // Collision with triangle
    switch (entity_1.dim())
    {
    case 0:
      dolfin_not_implemented();
      break;
    case 1:
      dolfin_not_implemented();
      break;
    case 2:
      return collides_triangle_triangle(entity_0, entity_1);
    case 3:
      return collides_tetrahedron_triangle(entity_1, entity_0);
    default:
      dolfin_error("CollisionDetection.cpp",
		   "collides entity_0 with entity_1",
		   "Unknown dimension of entity_1 in TriangleCell collision");
    }
    break;
  case 3:
    // Collision with tetrahedron
    switch (entity_1.dim())
    {
    case 0:
     dolfin_not_implemented();
      break;
    case 1:
      dolfin_not_implemented();
      break;
    case 2:
      return collides_tetrahedron_triangle(entity_0, entity_1);
      break;
    case 3:
      return collides_tetrahedron_tetrahedron(entity_0, entity_1);
      break;
    default:
      dolfin_error("CollisionDetection.cpp",
		   "collides entity_0 with entity_1",
		   "Unknown dimension of entity_1 in TetrahedronCell collision");
    }
    break;
  default:
    dolfin_error("CollisionDetection.cpp",
		 "collides entity_0 with entity_1",
		 "Unknown dimension of entity_0");
  }

  return false;
}
//-----------------------------------------------------------------------------
bool CollisionDetection::collides_interval_point(const MeshEntity& entity,
                                                 const Point& point)
{
  // Get coordinates
  const MeshGeometry& geometry = entity.mesh().geometry();
  const unsigned int* vertices = entity.entities(0);
  return collides_interval_point(geometry.point(vertices[0]),
                                 geometry.point(vertices[1]),
                                 point);
}
//-----------------------------------------------------------------------------
bool
CollisionDetection::collides_interval_interval(const MeshEntity& interval_0,
                                               const MeshEntity& interval_1)
{
  // Get coordinates
  const MeshGeometry& geometry_0 = interval_0.mesh().geometry();
  const MeshGeometry& geometry_1 = interval_1.mesh().geometry();
  const unsigned int* vertices_0 = interval_0.entities(0);
  const unsigned int* vertices_1 = interval_1.entities(0);
  const Point x00 = geometry_0.point(vertices_0[0]);
  const Point x01 = geometry_0.point(vertices_0[1]);
  const Point x10 = geometry_1.point(vertices_1[0]);
  const Point x11 = geometry_1.point(vertices_1[1]);

  if (interval_0.mesh().topology().dim() == 1
      && interval_1.mesh().topology().dim() == 1)
    return collides_interval_interval(x00[0], x01[0], x10[0], x11[0]);
  else if (interval_0.mesh().topology().dim() == 2
           && interval_1.mesh().topology().dim() == 2)
  {
    Point p;
    return collides_interval_interval(x00, x01, x10, x11, p);
  }

  dolfin_error("CollisionDetection.cpp",
               "collides interval with interval",
               "Not implemented for 3D meshes");
  return false;
}
//-----------------------------------------------------------------------------
bool
CollisionDetection::collides_interval_interval(const MeshEntity& interval,
                                               const Point& x1,
                                               const Point& x2)
{
  const unsigned int* vertices = interval.entities(0);
  const Point interval_x1 = MeshEntity(interval.mesh(), 0, vertices[0]).midpoint();
  const Point interval_x2 = MeshEntity(interval.mesh(), 0, vertices[1]).midpoint();
  if (interval.mesh().topology().dim() == 1)
  {
    return collides_interval_interval(interval_x1[0], interval_x2[1], x1[0], x2[0]);
  }
  else if (interval.mesh().topology().dim() == 2)
  {
    Point p;
    return collides_interval_interval(interval_x1, interval_x2, x1, x2, p);
  }

  dolfin_error("CollisionDetection.cpp",
               "collides interval with interval",
               "Not implemented for 3D meshes");
  return false;
}
//-----------------------------------------------------------------------------
bool
CollisionDetection::collides_interval_interval(const double& x00, const double& x01,
                                               const double& x10, const double& x11)
{
  const double a0 = std::min(x00, x01);
  const double b0 = std::max(x00, x01);
  const double a1 = std::min(x10, x11);
  const double b1 = std::max(x10, x11);

  // Check for collisions
  const double dx = std::min(b0 - a0, b1 - a1);
  const double eps = std::max(DOLFIN_EPS_LARGE, DOLFIN_EPS_LARGE*dx);
  return b1 > a0 - eps && a1 < b0 + eps;
}
//-----------------------------------------------------------------------------
bool
CollisionDetection::collides_interval_interval(
    const Point &a0, const Point &a1,
    const Point &b0, const Point &b1,
    Point &intersect)
{
  // First test if lines are collinear or parallel
  const Point A = a1 - a0;
  const Point B = b0 - b1;

  const double d = A[1]*B[0] - A[0]*B[1];

  // Are the two lines collinear or parallel?
  if (std::abs(d) < DOLFIN_EPS)
  {
    const double slope = (a1[1] - a0[1])/(a1[0] - a0[0]);

    // Test if parallel:
    if (!(std::isinf(slope) || std::abs(slope) < DOLFIN_EPS))
    {
      const double c_a = a1[1] - slope*a1[0];
      const double c_b = b1[1] - slope*b1[0];
      const double dist = std::abs(c_a - c_b);
      if (dist > DOLFIN_EPS_LARGE)
        return false; // Lines are parallel
    }

    // Lines are collinear, but do they overlap?
    const bool horizontal = std::abs(slope) < DOLFIN_EPS;
    const bool descending = slope < 0 && !horizontal;
    const double invert_y = descending || horizontal ? -1 : 1;

    const Point min1(std::min(a0[0], a1[0]), std::min(a0[1] * invert_y, a1[1] * invert_y));
    const Point max1(std::max(a0[0], a1[0]), std::max(a0[1] * invert_y, a1[1] * invert_y));

    const Point min2(std::min(a1[0], a1[1]), std::min(b0[1] * invert_y, b1[1] * invert_y));
    const Point max2(std::max(a1[0], a1[1]), std::max(b0[1] * invert_y, b1[1] * invert_y));

    Point min_isect;
    if (descending)
      min_isect = Point(std::max(min1[0], min2[0]), std::min(min1[1] * invert_y, min2[1] * invert_y));
    else
      min_isect = Point(std::max(min1[0], min2[0]), std::max(min1[1] * invert_y, min2[1] * invert_y));

    Point max_isect;
    if (descending)
      max_isect = Point(std::min(max1[0], max2[0]), std::max(max1[1] * invert_y, max2[1] * invert_y));
    else
      max_isect = Point(std::min(max1[0], max2[0]), std::min(max1[1] * invert_y, max2[1] * invert_y));

    // FIXME: correctly implement numerical precision checks
    bool isected = min_isect[0] <= max_isect[0] + DOLFIN_EPS &&
                   (!descending && min_isect[1] <= max_isect[1] + DOLFIN_EPS ||
                    descending && min_isect[1] >= max_isect[1] - DOLFIN_EPS);

    intersect[0] = max_isect[0];
    intersect[1] = max_isect[1];
    return isected; // Collinear and overlapping?
  }

  // Test for intersection
  // Algorithm:  Kirk, D., Graphics Gems III. (2012). Elsevier Science.
  // pp 199 - 201.
  const Point C = a0 - b0;
  const double n_alpha = B[1]*C[0] - B[0]*C[1];

  if (d > 0.0)
  {
    if ((n_alpha < 0.0) || (n_alpha > d + DOLFIN_EPS_LARGE))
      return false;
  }
  else if (d < 0.0)
  {
    if ((n_alpha > 0.0) || (n_alpha < d - DOLFIN_EPS_LARGE))
      return false;
  }

  const double n_beta = A[0]*C[1] - A[1]*C[0];

  if (d > 0.0)
  {
    if ((n_beta < 0.0) || (n_beta > d + DOLFIN_EPS_LARGE))
      return false;
  }
  else if (d < 0.0)
  {
    if ((n_beta > 0.0) || (n_beta < d - DOLFIN_EPS_LARGE))
      return false;
  }

  const double alpha = n_alpha/d;
  intersect = a0 + alpha*A;
  return true;
}
//-----------------------------------------------------------------------------
bool CollisionDetection::collides_triangle_point(const MeshEntity& triangle,
                                                 const Point& point)
{
  dolfin_assert(triangle.mesh().topology().dim() == 2);

  const MeshGeometry& geometry = triangle.mesh().geometry();
  const unsigned int* vertices = triangle.entities(0);

  if (triangle.mesh().geometry().dim() == 2)
    return collides_triangle_point_2d(geometry.point(vertices[0]),
                                      geometry.point(vertices[1]),
                                      geometry.point(vertices[2]),
                                      point);
  else
    return collides_triangle_point(geometry.point(vertices[0]),
                                   geometry.point(vertices[1]),
                                   geometry.point(vertices[2]),
                                   point);

}
//-----------------------------------------------------------------------------
bool
CollisionDetection::collides_triangle_triangle(const MeshEntity& triangle_0,
                                               const MeshEntity& triangle_1)
{
  dolfin_assert(triangle_0.mesh().topology().dim() == 2);
  dolfin_assert(triangle_1.mesh().topology().dim() == 2);

  // Get vertices as points
  const MeshGeometry& geometry_0 = triangle_0.mesh().geometry();
  const unsigned int* vertices_0 = triangle_0.entities(0);
  const MeshGeometry& geometry_1 = triangle_1.mesh().geometry();
  const unsigned int* vertices_1 = triangle_1.entities(0);

  return collides_triangle_triangle(geometry_0.point(vertices_0[0]),
				    geometry_0.point(vertices_0[1]),
				    geometry_0.point(vertices_0[2]),
				    geometry_1.point(vertices_1[0]),
				    geometry_1.point(vertices_1[1]),
				    geometry_1.point(vertices_1[2]));
}
//-----------------------------------------------------------------------------
bool
CollisionDetection::collides_tetrahedron_point(const MeshEntity& tetrahedron,
                                               const Point& point)
{
  dolfin_assert(tetrahedron.mesh().topology().dim() == 3);

  // Get the vertices as points
  const MeshGeometry& geometry = tetrahedron.mesh().geometry();
  const unsigned int* vertices = tetrahedron.entities(0);

  return collides_tetrahedron_point(geometry.point(vertices[0]),
				    geometry.point(vertices[1]),
				    geometry.point(vertices[2]),
				    geometry.point(vertices[3]),
				    point);
}
//-----------------------------------------------------------------------------
bool
CollisionDetection::collides_tetrahedron_triangle(const MeshEntity& tetrahedron,
                                                  const MeshEntity& triangle)
{
  dolfin_assert(tetrahedron.mesh().topology().dim() == 3);
  dolfin_assert(triangle.mesh().topology().dim() == 2);

  // Get the vertices of the tetrahedron as points
  const MeshGeometry& geometry_tet = tetrahedron.mesh().geometry();
  const unsigned int* vertices_tet = tetrahedron.entities(0);

  // Get the vertices of the triangle as points
  const MeshGeometry& geometry_tri = triangle.mesh().geometry();
  const unsigned int* vertices_tri = triangle.entities(0);

  return collides_tetrahedron_triangle(geometry_tet.point(vertices_tet[0]),
				       geometry_tet.point(vertices_tet[1]),
				       geometry_tet.point(vertices_tet[2]),
				       geometry_tet.point(vertices_tet[3]),
				       geometry_tri.point(vertices_tri[0]),
				       geometry_tri.point(vertices_tri[1]),
				       geometry_tri.point(vertices_tri[2]));
}
//-----------------------------------------------------------------------------
bool
CollisionDetection::collides_tetrahedron_tetrahedron
(const MeshEntity& tetrahedron_0,
 const MeshEntity& tetrahedron_1)
{
  // This algorithm checks whether two tetrahedra intersect.

  // Algorithm and source code from Fabio Ganovelli, Federico Ponchio
  // and Claudio Rocchini: Fast Tetrahedron-Tetrahedron Overlap
  // Algorithm, Journal of Graphics Tools, 7(2), 2002. DOI:
  // 10.1080/10867651.2002.10487557. Source code available at
  // http://web.archive.org/web/20031130075955/http://www.acm.org/jgt/papers/GanovelliPonchioRocchini02/tet_a_tet.html

  dolfin_assert(tetrahedron_0.mesh().topology().dim() == 3);
  dolfin_assert(tetrahedron_1.mesh().topology().dim() == 3);

  // Get the vertices as points
  const MeshGeometry& geometry = tetrahedron_0.mesh().geometry();
  const unsigned int* vertices = tetrahedron_0.entities(0);
  const MeshGeometry& geometry_q = tetrahedron_1.mesh().geometry();
  const unsigned int* vertices_q = tetrahedron_1.entities(0);
  std::vector<Point> V1(4), V2(4);
  for (std::size_t i = 0; i < 4; ++i)
  {
    V1[i] = geometry.point(vertices[i]);
    V2[i] = geometry_q.point(vertices_q[i]);
  }

  // Get the vectors between V2 and V1[0]
  std::vector<Point> P_V1(4);
  for (std::size_t i = 0; i < 4; ++i)
    P_V1[i] = V2[i]-V1[0];

  // Data structure for edges of V1 and V2
  std::vector<Point> e_v1(5), e_v2(5);
  e_v1[0] = V1[1] - V1[0];
  e_v1[1] = V1[2] - V1[0];
  e_v1[2] = V1[3] - V1[0];
  Point n = e_v1[1].cross(e_v1[0]);

  // Maybe flip normal. Normal should be outward.
  if (n.dot(e_v1[2]) > 0)
    n *= -1;
  std::vector<int> masks(4);
  std::vector<std::vector<double>> Coord_1(4, std::vector<double>(4));
  if (separating_plane_face_A_1(P_V1, n, Coord_1[0], masks[0]))
    return false;
  n = e_v1[0].cross(e_v1[2]);

  // Maybe flip normal
  if (n.dot(e_v1[1]) > 0)
    n *= -1;
  if (separating_plane_face_A_1(P_V1, n, Coord_1[1], masks[1]))
    return false;
  if (separating_plane_edge_A(Coord_1, masks, 0, 1))
    return false;
  n = e_v1[2].cross(e_v1[1]);

  // Maybe flip normal
  if (n.dot(e_v1[0]) > 0)
    n *= -1;
  if (separating_plane_face_A_1(P_V1, n, Coord_1[2], masks[2]))
    return false;
  if (separating_plane_edge_A(Coord_1, masks, 0, 2))
    return false;
  if (separating_plane_edge_A(Coord_1, masks, 1,2))
    return false;
  e_v1[4] = V1[3] - V1[1];
  e_v1[3] = V1[2] - V1[1];
  n = e_v1[3].cross(e_v1[4]);

  // Maybe flip normal. Note the < since e_v1[0]=v1-v0.
  if (n.dot(e_v1[0]) < 0)
    n *= -1;
  if (separating_plane_face_A_2(V1, V2, n, Coord_1[3], masks[3]))
    return false;
  if (separating_plane_edge_A(Coord_1, masks, 0, 3))
    return false;
  if (separating_plane_edge_A(Coord_1, masks, 1, 3))
    return false;
  if (separating_plane_edge_A(Coord_1, masks, 2, 3))
    return false;
  if ((masks[0] | masks[1] | masks[2] | masks[3] )!= 15)
    return true;

  // From now on, if there is a separating plane, it is parallel to a
  // face of b.
  std::vector<Point> P_V2(4);
  for (std::size_t i = 0; i < 4; ++i)
    P_V2[i] = V1[i] - V2[0];
  e_v2[0] = V2[1] - V2[0];
  e_v2[1] = V2[2] - V2[0];
  e_v2[2] = V2[3] - V2[0];
  n = e_v2[1].cross(e_v2[0]);

  // Maybe flip normal
  if (n.dot(e_v2[2])>0)
    n *= -1;
  if (separating_plane_face_B_1(P_V2, n))
    return false;
  n=e_v2[0].cross(e_v2[2]);

  // Maybe flip normal
  if (n.dot(e_v2[1]) > 0)
    n *= -1;
  if (separating_plane_face_B_1(P_V2, n))
    return false;
  n = e_v2[2].cross(e_v2[1]);

  // Maybe flip normal
  if (n.dot(e_v2[0]) > 0)
    n *= -1;
  if (separating_plane_face_B_1(P_V2, n))
    return false;
  e_v2[4] = V2[3] - V2[1];
  e_v2[3] = V2[2] - V2[1];
  n = e_v2[3].cross(e_v2[4]);

  // Maybe flip normal. Note the < since e_v2[0] = V2[1] - V2[0].
  if (n.dot(e_v2[0]) < 0)
    n *= -1;
  if (separating_plane_face_B_2(V1, V2, n))
    return false;

  return true;
}
//-----------------------------------------------------------------------------
bool
CollisionDetection::collides_edge_edge(const Point& a,
				       const Point& b,
				       const Point& c,
				       const Point& d)
{
  const double tol = DOLFIN_EPS_LARGE;

  // Check if two edges are the same
  if ((a - c).norm() < tol and (b - d).norm() < tol)
    return false;
  if ((a - d).norm() < tol and (b - c).norm() < tol)
    return false;

  // Get edges as vectors and compute the normal
  const Point L1 = b - a;
  const Point L2 = d - c;
  const Point n = L1.cross(L2);

  // Check if L1 and L2 are coplanar
  const Point ca = c - a;
  if (std::abs(ca.dot(n)) > tol)
    return false;

  // Find orthogonal plane with normal n1
  const Point n1 = n.cross(L1);
  const double n1dotL2 = n1.dot(L2);
  if (std::abs(n1dotL2) < tol)
    return false;
  const double t = n1.dot(a - c) / n1dotL2;
  if (t <= 0 or t >= 1)
    return false;

  // Find orthogonal plane with normal n2
  const Point n2 = n.cross(L2);
  const double n2dotL1 = n2.dot(L1);
  if (std::abs(n2dotL1) < tol)
    return false;
  const double s = n2.dot(c - a) / n2dotL1;
  if (s <= 0 or s >= 1)
    return false;

  return true;
}
//-----------------------------------------------------------------------------
bool CollisionDetection::collides_interval_point(const Point& p0,
                                                 const Point& p1,
                                                 const Point& point)
{
  // Compute angle between v = p1 - p0 and w = point - p0
  Point v = p1 - p0;
  const double vnorm = v.norm();

  // p0 and p1 are the same points
  if (vnorm < DOLFIN_EPS_LARGE)
    return false;

  const Point w = point - p0;
  const double wnorm = w.norm();

  // point and p0 are the same points
  if (wnorm < DOLFIN_EPS)
    return true;

  // Compute cosine
  v /= vnorm;
  const double a = v.dot(w) / wnorm;

  // Cosine should be 1, and point should lie between p0 and p1
  if (std::abs(1-a) < DOLFIN_EPS_LARGE and wnorm <= vnorm)
    return true;

  return false;
}

//-----------------------------------------------------------------------------
bool CollisionDetection::collides_triangle_point_2d(const Point& p0,
                                                    const Point& p1,
                                                    const Point& p2,
                                                    const Point &point)
{
  // Simplified algorithm for coplanar triangles and points (z=0)
  // This algorithm is robust because it will perform the same numerical
  // test on each edge of neighbouring triangles. Points cannot slip
  // between the edges, and evade detection.

  // Vectors defining each edge in consistent orientation
  const Point r0 = p0 - p2;
  const Point r1 = p1 - p0;
  const Point r2 = p2 - p1;

  // Normal to triangle
  double normal = r1.x()*r0.y() - r1.y()*r0.x();

  // Compute normal to triangle based on point and first edge
  // Will have opposite sign if outside triangle

  Point r = point - p0;
  double pnormal = r.x()*r0.y() - r.y()*r0.x();
  if (pnormal != 0.0 and std::signbit(normal) != std::signbit(pnormal))
    return false;

  // Repeat for each edge
  r = point - p1;
  pnormal = r.x()*r1.y() - r.y()*r1.x();
  if (pnormal != 0.0 and std::signbit(normal) != std::signbit(pnormal))
    return false;

  r = point - p2;
  pnormal = r.x()*r2.y() - r.y()*r2.x();
  if (pnormal != 0.0 and std::signbit(normal) != std::signbit(pnormal))
    return false;

  return true;
}
//-----------------------------------------------------------------------------
bool CollisionDetection::collides_triangle_point(const Point& p0,
                                                 const Point& p1,
                                                 const Point& p2,
                                                 const Point &point)
{
  // Algorithm from http://www.blackpawn.com/texts/pointinpoly/

  // Vectors defining each edge in consistent orientation
  const Point r0 = p0 - p2;
  const Point r1 = p1 - p0;
  const Point r2 = p2 - p1;

  // Normal to triangle: should be the same as
  // r2.cross(r1) and r0.cross(r2).
  Point normal = r1.cross(r0);

  Point r = point - p0;
  // Check point is in plane of triangle (for manifold)
  double volume = r.dot(normal);
  if (std::abs(volume) > DOLFIN_EPS)
    return false;

  // Compute normal to triangle based on point and first edge
  // Dot product of two normals should be positive, if inside.
  Point pnormal = r.cross(r0);
  double t1 = normal.dot(pnormal);
  if (t1 < 0) return false;

  // Repeat for each edge
  r = point - p1;
  pnormal = r.cross(r1);
  double t2 = normal.dot(pnormal);
  if (t2 < 0) return false;

  r = point - p2;
  pnormal = r.cross(r2);
  double t3 = normal.dot(pnormal);
  if (t3 < 0) return false;

  return true;
}
//-----------------------------------------------------------------------------
std::pair<bool, double> CollisionDetection::collides_triangle_line(const Point& p0,
                                                                   const Point& p1,
                                                                   const Point& p2,
                                                                   const Point& o,
                                                                   const Point& d)
{
  // Algorithm and code from Fast, Minimum Storage Ray/Triangle Intersection.
  // Möller & Trumbore. Journal of Graphics Tools, 1997.
  // Description at http://www.scratchapixel.com/lessons/3d-basic-rendering/
  // ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection

  const Point e1 = p1 - p0;
  const Point e2 = p2 - p1;

  const Point P = d.cross(e2);
  const double det = e1.dot(P);

  // singular mapping
  if (det > -DOLFIN_EPS && det < DOLFIN_EPS)
    return std::make_pair(false, 0.0);;

  const double inv_det = 1.0/det;

  const Point T = o - p0;
  const double u = T.dot(P)*inv_det;

  // ray does not intersect triangle
  if (u < 0.0 || u > 1.0)
    return std::make_pair(false, 0.0);

  const Point Q = T.cross(e1);

  const double v = d.dot(Q)*inv_det;

  // ray does not intersect triangle
  if (v < 0.0 || u + v > 1.0)
    return std::make_pair(false, 0.0);;

  const double t = e2.dot(Q) * inv_det;

  // ray intersects triangle at o + t*d
  if (t > DOLFIN_EPS)
    return std::make_pair(true, t);

  return std::make_pair(false, 0.0);
}
//-----------------------------------------------------------------------------
bool CollisionDetection::collides_triangle_interval(const MeshEntity& triangle,
                                                    const Point& x0,
                                                    const Point& x1)
{

  const unsigned int* vertices = triangle.entities(0);
  const MeshGeometry& geometry = triangle.mesh().geometry();
  const Point p0 = geometry.point(vertices[0]);
  const Point p1 = geometry.point(vertices[1]);
  const Point p2 = geometry.point(vertices[2]);

  const std::size_t mesh_dim = triangle.mesh().geometry().dim();
  if (mesh_dim == 2)
    return collides_triangle_interval_2d(p0, p1, p2, x0, x1);
  else if (mesh_dim == 3)
    return collides_triangle_interval(p0, p1, p2, x0, x1);

  dolfin_error("CollisionDetection.cpp",
               "collides triangle with interval",
               "Unsupported geometric dimension of mesh");
  return false;
}
//-----------------------------------------------------------------------------
bool CollisionDetection::collides_triangle_interval_2d(const Point& p0,
                                                       const Point& p1,
                                                       const Point& p2,
                                                       const Point& x0,
                                                       const Point& x1)
{
  // Deduce whether the vectors (a0, a1) and (b0, b1) intersect at infinity
  // by deducing whether a1 and a0 share the same half-plane bisected by
  // (b0, b1). I.e. do B and C share the same half-plane defined by A?
  // +ve if true, -ve if false, 0 if parallel.
  const auto isect_at_inf = [](const Point& a0, const Point& a1,
                               const Point& b0, const Point& b1) -> double
  {
    const Point A = b1 - b0, B = a0 - b0, C = a1 - b0;
    return (A[0]*B[1] - A[1]*B[0]) * (A[0]*C[1] - A[1]*C[0]); // (A x B) * (A x C)
  };

  // For the three pairs of half-planes defined by the triangle, test whether
  // the points of the interval lie on the outside of the triangle in these
  // half-planes
  const double x0_012 = isect_at_inf(x0, p0, p1, p2);
  const double x1_012 = isect_at_inf(x1, p0, p1, p2);
  const double x0_120 = isect_at_inf(x0, p1, p2, p0);
  const double x1_120 = isect_at_inf(x1, p1, p2, p0);
  const double x0_201 = isect_at_inf(x0, p2, p0, p1);
  const double x1_201 = isect_at_inf(x1, p2, p0, p1);

  // If two sides of the triangle lie within the same half-plane defined by
  // the interval (x0, x1), then the interval lies outside of the triangle
  const double p0p1_x0x1 = isect_at_inf(p0, p1, x0, x1);
  const double p1p2_x0x1 = isect_at_inf(p1, p2, x0, x1);

  // return not (Is the interval outside the triangle?)
  return !((x0_201 < 0 && x1_201 < 0) || (x0_012 < 0 && x1_012 < 0)
           || (x0_120 < 0 && x1_120 < 0) || (p0p1_x0x1 > 0 && p1p2_x0x1 > 0));
}
//-----------------------------------------------------------------------------
bool CollisionDetection::collides_triangle_interval(const Point& p0,
                                                    const Point& p1,
                                                    const Point& p2,
                                                    const Point& x0,
                                                    const Point& x1)
{
  const Point o = x0;
  const Point d = x1 - x0;

  const std::pair<bool, double> result = collides_triangle_line(p0, p1, p2, o, d);
  const bool& collides = result.first;
  const double& magnitude = result.second;

  return collides && magnitude <= 1.0 && magnitude >= 0.0;
}
//-----------------------------------------------------------------------------
bool
CollisionDetection::collides_triangle_triangle(const Point& p0,
					       const Point& p1,
					       const Point& p2,
					       const Point& q0,
					       const Point& q1,
					       const Point& q2)
{
  // Algorithm and code from Tomas Moller: A Fast Triangle-Triangle
  // Intersection Test, Journal of Graphics Tools, 2(2), 1997. Source
  // code is available at
  // http://fileadmin.cs.lth.se/cs/Personal/Tomas_Akenine-Moller/code/opttritri.txt

  // First check if the triangles are the same. We need to do this
  // separately if we do _not_ allow for adjacent edges to be
  // classified as colliding (see the edge_edge_test).

  const Point Vmid = (p0 + p1 + p2) / 3.;
  const Point Umid = (q0 + q1 + q2) / 3.;
  if ((Vmid-Umid).norm() < DOLFIN_EPS_LARGE)
    return true;

  Point E1, E2;
  Point N1, N2;
  double d1, d2;
  double du0, du1, du2, dv0, dv1, dv2;
  Point D;
  double isect1[2], isect2[2];
  double du0du1, du0du2, dv0dv1, dv0dv2;
  int index;
  double vp0, vp1, vp2;
  double up0, up1, up2;
  double bb, cc, max;

  // Compute plane equation of triangle(p0,p1,p2)
  E1 = p1-p0;
  E2 = p2-p0;
  N1 = E1.cross(E2);
  d1 = -N1.dot(p0);

  // Plane equation 1: N1.X+d1=0. Put q0,q1,q2 into plane equation 1
  // to compute signed distances to the plane
  du0 = N1.dot(q0)+d1;
  du1 = N1.dot(q1)+d1;
  du2 = N1.dot(q2)+d1;

  // Coplanarity robustness check
  if (std::abs(du0) < DOLFIN_EPS_LARGE)
    du0 = 0.0;
  if (std::abs(du1) < DOLFIN_EPS_LARGE)
    du1 = 0.0;
  if (std::abs(du2) < DOLFIN_EPS_LARGE)
    du2 = 0.0;
  du0du1 = du0*du1;
  du0du2 = du0*du2;

  // Same sign on all of them + not equal 0?
  if (du0du1>0. && du0du2>0.)
    return false;

  // Compute plane of triangle (q0,q1,q2)
  E1 = q1-q0;
  E2 = q2-q0;
  N2 = E1.cross(E2);
  d2 = -N2.dot(q0);
  // Plane equation 2: N2.X+d2=0. Put p0,p1,p2 into plane equation 2
  dv0 = N2.dot(p0)+d2;
  dv1 = N2.dot(p1)+d2;
  dv2 = N2.dot(p2)+d2;

  // Coplanarity check
  if (std::abs(dv0) < DOLFIN_EPS_LARGE)
    dv0 = 0.0;
  if (std::abs(dv1) < DOLFIN_EPS_LARGE)
    dv1 = 0.0;
  if (std::abs(dv2) < DOLFIN_EPS_LARGE)
    dv2 = 0.0;
  dv0dv1 = dv0*dv1;
  dv0dv2 = dv0*dv2;

  // Same sign on all of them + not equal 0 ?
  if (dv0dv1>0. && dv0dv2>0.)
    return false;

  // Compute direction of intersection line
  D = N1.cross(N2);

  // Compute and index to the largest component of D
  max = (double)std::abs(D[0]);
  index = 0;
  bb = (double)std::abs(D[1]);
  cc = (double)std::abs(D[2]);
  if (bb > max)
    max = bb, index = 1;
  if (cc > max)
    max = cc, index = 2;

  // This is the simplified projection onto L
  vp0 = p0[index];
  vp1 = p1[index];
  vp2 = p2[index];

  up0 = q0[index];
  up1 = q1[index];
  up2 = q2[index];

  // Compute interval for triangle 1
  double a, b, c, x0, x1;
  if (compute_intervals(vp0, vp1, vp2, dv0, dv1, dv2, dv0dv1, dv0dv2,
                        a, b, c, x0, x1))
    return coplanar_tri_tri(N1, p0, p1, p2, q0, q1, q2);

  // Compute interval for triangle 2
  double d, e, f, y0, y1;
  if (compute_intervals(up0, up1, up2, du0, du1, du2, du0du1, du0du2,
                        d, e, f, y0, y1))
    return coplanar_tri_tri(N1, p0, p1, p2, q0, q1, q2);

  double xx, yy, xxyy, tmp;
  xx = x0*x1;
  yy = y0*y1;
  xxyy = xx*yy;

  tmp = a*xxyy;
  isect1[0] = tmp+b*x1*yy;
  isect1[1] = tmp+c*x0*yy;

  tmp = d*xxyy;
  isect2[0] = tmp+e*xx*y1;
  isect2[1] = tmp+f*xx*y0;

  if (isect1[0] > isect1[1])
    std::swap(isect1[0], isect1[1]);
  if (isect2[0] > isect2[1])
    std::swap(isect2[0], isect2[1]);

  if (isect1[1] < isect2[0] ||
      isect2[1] < isect1[0])
    return false;

  return true;
}
//-----------------------------------------------------------------------------
bool
CollisionDetection::collides_tetrahedron_point(const Point& p0,
					       const Point& p1,
					       const Point& p2,
					       const Point& p3,
					       const Point& point)
{
  // Algorithm from http://www.blackpawn.com/texts/pointinpoly/
  // See also "Real-Time Collision Detection" by Christer Ericson.

  const Point *p[4] = {&p0, &p1, &p2, &p3};

  // Consider each face in turn
  for (unsigned int i = 0; i != 4; ++i)
  {
    // Compute vectors relative to p[i]
    const Point v1 = *p[(i + 1)%4] - *p[i];
    const Point v2 = *p[(i + 2)%4] - *p[i];
    const Point v3 = *p[(i + 3)%4] - *p[i];
    const Point v = point - *p[i];
    // Normal to plane containing v1 and v2
    const Point n1 = v1.cross(v2);
    // Find which side of face plane points v and v3 lie
    const double t1 = n1.dot(v);
    const double t2 = n1.dot(v3);
    // Catch case where point is exactly on plane
    // otherwise require points to be on same side
    if (t1 != 0.0 and std::signbit(t1) != std::signbit(t2))
      return false;
  }
  return true;
}
//-----------------------------------------------------------------------------
bool
CollisionDetection::collides_tetrahedron_triangle(const Point& p0,
						  const Point& p1,
						  const Point& p2,
						  const Point& p3,
						  const Point& q0,
						  const Point& q1,
						  const Point& q2)
{
  // Collision is determined by first if any triangle vertex is inside
  // the tetrahedron. If not, we continue checking the intersection of
  // the triangle with the four faces of the tetrahedron.

  // Triangle vertex in tetrahedron collision
  if (collides_tetrahedron_point(p0, p1, p2, p3, q0))
    return true;
  if (collides_tetrahedron_point(p0, p1, p2, p3, q1))
    return true;
  if (collides_tetrahedron_point(p0, p1, p2, p3, q2))
    return true;

  // Triangle-triangle collision tests
  if (collides_triangle_triangle(q0, q1, q2, p1, p2, p3))
    return true;
  if (collides_triangle_triangle(q0, q1, q2, p0, p2, p3))
    return true;
  if (collides_triangle_triangle(q0, q1, q2, p0, p1, p3))
    return true;
  if (collides_triangle_triangle(q0, q1, q2, p0, p1, p2))
    return true;

  return false;
}
//-----------------------------------------------------------------------------
bool CollisionDetection::edge_edge_test(int i0,
                                        int i1,
                                        double Ax,
                                        double Ay,
					const Point& V0,
					const Point& U0,
					const Point& U1)
{
  // Helper function for triangle triangle collision. Test edge vs
  // edge.

  // Here we have the option of classifying adjacent edges of two
  // triangles as colliding by changing > to >= and < to <= below.

  const double Bx = U0[i0] - U1[i0];
  const double By = U0[i1] - U1[i1];
  const double Cx = V0[i0] - U0[i0];
  const double Cy = V0[i1] - U0[i1];
  const double f = Ay*Bx - Ax*By;
  const double d = By*Cx - Bx*Cy;

  if ((f > 0 && d >= 0 && d <= f) ||
      (f < 0 && d <= 0 && d >= f))
  {
    const double e = Ax*Cy - Ay*Cx;
    if (f > 0)
    {
      // Allow or not allow adjacent edges as colliding:
      //if (e >= 0 && e <= f) return true;
      if (e > 0 && e < f)
        return true;
    }
    else
    {
      // Allow or not allow adjacent edges as colliding:
      //if (e <= 0 && e >= f) return true;
      if (e < 0 && e > f)
        return true;
    }
  }
  return false;
}
//-----------------------------------------------------------------------------
bool CollisionDetection::edge_against_tri_edges(int i0,
                                                int i1,
						const Point& V0,
						const Point& V1,
						const Point& U0,
						const Point& U1,
						const Point& U2)
{
  // Helper function for triangle triangle collision
  const double Ax = V1[i0] - V0[i0];
  const double Ay = V1[i1] - V0[i1];

  // Test edge U0,U1 against V0,V1
  if (edge_edge_test(i0, i1, Ax, Ay, V0, U0, U1))
    return true;

  // Test edge U1,U2 against V0,V1
  if (edge_edge_test(i0, i1, Ax, Ay, V0, U1, U2))
    return true;

  // Test edge U2,U1 against V0,V1
  if (edge_edge_test(i0, i1, Ax, Ay, V0, U2, U0))
    return true;

  return false;
}
//-----------------------------------------------------------------------------
bool CollisionDetection::point_in_tri(int i0,
                                      int i1,
				      const Point& V0,
				      const Point& U0,
				      const Point& U1,
				      const Point& U2)
{
  // Helper function for triangle triangle collision
  // Is T1 completely inside T2?
  // Check if V0 is inside tri(U0,U1,U2)
  double a = U1[i1] - U0[i1];
  double b = -(U1[i0] - U0[i0]);
  double c = -a*U0[i0] - b*U0[i1];
  const double d0 = a*V0[i0] + b*V0[i1] + c;

  a = U2[i1] - U1[i1];
  b = -(U2[i0] - U1[i0]);
  c = -a*U1[i0] - b*U1[i1];
  const double d1 = a*V0[i0] + b*V0[i1] + c;

  a = U0[i1] - U2[i1];
  b = -(U0[i0] - U2[i0]);
  c = -a*U2[i0] - b*U2[i1];
  const double d2 = a*V0[i0] + b*V0[i1] + c;

  if (d0*d1 > 0.)
  {
    if (d0*d2 > 0.)
      return true;
  }

  return false;
}
//-----------------------------------------------------------------------------
bool CollisionDetection::coplanar_tri_tri(const Point& N,
					  const Point& V0,
					  const Point& V1,
					  const Point& V2,
					  const Point& U0,
					  const Point& U1,
					  const Point& U2)
{
  // Helper function for triangle triangle collision

  double A[3];
  int i0,i1;

  // First project onto an axis-aligned plane, that maximizes the area
  // of the triangles, compute indices: i0,i1.
  A[0] = std::abs(N[0]);
  A[1] = std::abs(N[1]);
  A[2] = std::abs(N[2]);

  if (A[0] > A[1])
  {
    if (A[0] > A[2])
    {
      i0 = 1;      // A[0] is greatest
      i1 = 2;
    }
    else
    {
      i0 = 0;      // A[2] is greatest
      i1 = 1;
    }
  }
  else   // A[0]<=A[1]
  {
    if (A[2] > A[1])
    {
      i0 = 0;      // A[2] is greatest
      i1 = 1;
    }
    else
    {
      i0 = 0;      // A[1] is greatest
      i1 = 2;
    }
  }

  // Test all edges of triangle 1 against the edges of triangle 2
  if (edge_against_tri_edges(i0, i1, V0, V1, U0, U1, U2))
    return true;
  if (edge_against_tri_edges(i0, i1, V1, V2, U0, U1, U2))
    return true;
  if (edge_against_tri_edges(i0, i1, V2, V0, U0, U1, U2))
    return true;

  // Finally, test if tri1 is totally contained in tri2 or vice versa
  if (point_in_tri(i0, i1, V0, U0, U1, U2))
    return true;
  if (point_in_tri(i0, i1, U0, V0, V1, V2))
    return true;

  return false;
}
//-----------------------------------------------------------------------------
bool CollisionDetection::compute_intervals(double VV0,
					   double VV1,
					   double VV2,
					   double D0,
					   double D1,
					   double D2,
					   double D0D1,
					   double D0D2,
					   double& A,
					   double& B,
					   double& C,
					   double& X0,
					   double& X1)
{
  // Helper function for triangle triangle collision

  if (D0D1 > 0.)
  {
    // Here we know that D0D2<=0.0, that is D0, D1 are on the same
    // side, D2 on the other or on the plane
    A = VV2;
    B = (VV0 - VV2)*D2;
    C = (VV1 - VV2)*D2;
    X0 = D2 - D0;
    X1 = D2 - D1;
  }
  else if (D0D2 > 0.)
  {
    // Here we know that d0d1<=0.0
    A = VV1;
    B = (VV0 - VV1)*D1;
    C = (VV2 - VV1)*D1;
    X0 = D1 - D0;
    X1 = D1 - D2;
  }
  else if (D1*D2 > 0. || D0 != 0.)
  {
    // Here we know that d0d1<=0.0 or that D0!=0.0
    A = VV0;
    B = (VV1 - VV0)*D0;
    C = (VV2 - VV0)*D0;
    X0 = D0 - D1;
    X1 = D0 - D2;
  }
  else if (D1 != 0.)
  {
    A = VV1;
    B = (VV0 - VV1)*D1;
    C = (VV2 - VV1)*D1;
    X0 = D1 - D0;
    X1 = D1 - D2;
  }
  else if (D2 != 0.)
  {
    A = VV2;
    B = (VV0 - VV2)*D2;
    C = (VV1 - VV2)*D2;
    X0 = D2 - D0;
    X1 = D2 - D1;
  }
  else {
    // Go to coplanar test
    return true;
  }

  return false;
}
//-----------------------------------------------------------------------------
bool
CollisionDetection::separating_plane_face_A_1(const std::vector<Point>& pv1,
					      const Point& n,
					      std::vector<double>& coord,
					      int&  mask_edges)
{
  // Helper function for tetrahedron-tetrahedron collision test:
  // checks if plane pv1 is a separating plane. Stores local
  // coordinates and the mask bit mask_edges.

  mask_edges = 0;
  const int shifts[4] = {1, 2, 4, 8};

  for (std::size_t i = 0; i < 4; ++i)
  {
    coord[i] = pv1[i].dot(n);
    if (coord[i] > 0)
      mask_edges |= shifts[i];
  }

  return (mask_edges == 15);
}
//-----------------------------------------------------------------------------
bool
CollisionDetection::separating_plane_face_A_2(const std::vector<Point>& V1,
					      const std::vector<Point>& V2,
					      const Point& n,
					      std::vector<double>& coord,
					      int&  mask_edges)
{
  // Helper function for tetrahedron-tetrahedron collision test:
  // checks if plane v1,v2 is a separating plane. Stores local
  // coordinates and the mask bit mask_edges.

  mask_edges = 0;
  const int shifts[4] = {1, 2, 4, 8};

  for (std::size_t i = 0; i < 4; ++i)
  {
    coord[i] = (V2[i] - V1[1]).dot(n);
    if (coord[i] > 0)
      mask_edges |= shifts[i];
  }

  return (mask_edges == 15);
}
//-----------------------------------------------------------------------------
bool CollisionDetection::separating_plane_edge_A(
  const std::vector<std::vector<double>>& coord_1,
  const std::vector<int>& masks, int f0, int f1)
{
  // Helper function for tetrahedron-tetrahedron collision: checks if
  // edge is in the plane separating faces f0 and f1.

  const std::vector<double>& coord_f0 = coord_1[f0];
  const std::vector<double>& coord_f1 = coord_1[f1];

  int maskf0 = masks[f0];
  int maskf1 = masks[f1];

  if ((maskf0 | maskf1) != 15) // if there is a vertex of b
    return false; // included in (-,-) return false

  maskf0 &= (maskf0 ^ maskf1); // exclude the vertices in (+,+)
  maskf1 &= (maskf0 ^ maskf1);

  // edge 0: 0--1
  if ((maskf0 & 1) && // the vertex 0 of b is in (-,+)
      (maskf1 & 2)) // the vertex 1 of b is in (+,-)
    if ((coord_f0[1]*coord_f1[0] - coord_f0[0]*coord_f1[1]) > 0)
      // the edge of b (0,1) intersect (-,-) (see the paper)
      return false;

  if ((maskf0 & 2) &&
      (maskf1 & 1))
    if ((coord_f0[1]*coord_f1[0] - coord_f0[0]*coord_f1[1]) < 0)
      return false;

  // edge 1: 0--2
  if ((maskf0 & 1) &&
      (maskf1 & 4))
    if ((coord_f0[2]*coord_f1[0] - coord_f0[0]*coord_f1[2]) > 0)
      return false;

  if ((maskf0 & 4) &&
      (maskf1 & 1))
    if ((coord_f0[2]*coord_f1[0] - coord_f0[0]*coord_f1[2]) < 0)
      return false;

  // edge 2: 0--3
  if ((maskf0 & 1) &&
      (maskf1 & 8))
    if ((coord_f0[3]*coord_f1[0] - coord_f0[0]*coord_f1[3]) > 0)
      return false;

  if ((maskf0 & 8) &&
      (maskf1 & 1))
    if ((coord_f0[3]*coord_f1[0] - coord_f0[0]*coord_f1[3]) < 0)
      return false;

  // edge 3: 1--2
  if ((maskf0 & 2) &&
      (maskf1 & 4))
    if ((coord_f0[2]*coord_f1[1] - coord_f0[1]*coord_f1[2]) > 0)
      return false;

  if ((maskf0 & 4) &&
      (maskf1 & 2))
    if ((coord_f0[2]*coord_f1[1] - coord_f0[1]*coord_f1[2]) < 0)
      return false;

  // edge 4: 1--3
  if ((maskf0 & 2) &&
      (maskf1 & 8))
    if ((coord_f0[3]*coord_f1[1] - coord_f0[1]*coord_f1[3]) > 0)
      return false;

  if ((maskf0 & 8) &&
      (maskf1 & 2))
    if ((coord_f0[3]*coord_f1[1] - coord_f0[1]*coord_f1[3]) < 0)
      return false;

  // edge 5: 2--3
  if ((maskf0 & 4) &&
      (maskf1 & 8))
    if ((coord_f0[3]*coord_f1[2] - coord_f0[2]*coord_f1[3]) > 0)
      return false;

  if ((maskf0 & 8) &&
      (maskf1 & 4))
    if ((coord_f0[3]*coord_f1[2] - coord_f0[2]*coord_f1[3]) < 0)
      return false;

  // Now there exists a separating plane supported by the edge shared
  // by f0 and f1.
  return true;
}
//-----------------------------------------------------------------------------
