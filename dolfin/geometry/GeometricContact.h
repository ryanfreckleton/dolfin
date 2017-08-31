// Copyright (C) 2017 Nate Sime and Chris Richardson
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

#ifndef __GEOMETRIC_CONTACT_H
#define __GEOMETRIC_CONTACT_H

#include <map>
#include <utility>
#include <vector>

namespace dolfin
{

  // Forward declarations
  class Mesh;
  class Point;
  class Facet;
  class Function;

  class CellMetaData
  {
  public:
    CellMetaData(const std::size_t slave_facet_idx,
                 const std::size_t slave_facet_local_idx,
                 std::vector<double> dof_coords,
                 std::vector<std::size_t> cell_dofs,
                 std::vector<double> dof_coeffs) :
        slave_facet_idx(slave_facet_idx),
        slave_facet_local_idx(slave_facet_local_idx),
        _dof_coords(std::move(dof_coords)),
        _cell_dofs(std::move(cell_dofs)),
        _dof_coeffs(std::move(dof_coeffs)) {}

    const std::size_t slave_facet_idx;
    const std::size_t slave_facet_local_idx;

    const std::vector<double> get_dof_coords() { return _dof_coords; };
    const std::vector<double> get_dof_coeffs() { return _dof_coeffs; };
    const std::vector<std::size_t> get_cell_dofs() { return _cell_dofs; };

    const std::vector<Point> get_cell_vertices(const Mesh& mesh)
    {
      const std::size_t gdim = mesh.geometry().dim();

      // FIXME: is there a better way of getting this?
      const std::size_t num_cell_verts = Cell(mesh, 0).num_entities(0);
      std::vector<Point> cell_verts(num_cell_verts);

      for (std::size_t j=0; j<num_cell_verts; ++j)
        cell_verts[j] = Point(gdim, _dof_coeffs.data() + j*gdim);

      return cell_verts;
    }

    const std::vector<Point> get_displacement_at_vertices(const Mesh& mesh)
    {
      const std::size_t gdim = mesh.geometry().dim();

      // FIXME: is there a better way of getting this?
      const std::size_t num_cell_verts = Cell(mesh, 0).num_entities(0);
      std::vector<Point> displacement(num_cell_verts);

      for (std::size_t j=0; j<num_cell_verts; ++j)
        displacement[j] = Point(_dof_coeffs[0*num_cell_verts+j],
                                _dof_coeffs[1*num_cell_verts+j],
                                (gdim == 3) ? _dof_coeffs[2*num_cell_verts+j] : 0.0);

      return displacement;
    }

    const std::vector<Point> create_deformed_facet_position(const Mesh& mesh)
    {
      const auto v = get_cell_vertices(mesh);
      const auto u = get_displacement_at_vertices(mesh);
      std::vector<Point> deformed(v.size());
      for (std::size_t j=0; j<v.size(); ++j)
        deformed[j] = v[j] + u[j];

      return deformed;
    }

  private:
    const std::vector<double> _dof_coords;
    const std::vector<std::size_t> _cell_dofs;
    const std::vector<double> _dof_coeffs;
  };

  /// This class implements ...

  class GeometricContact
  {
  public:
    GeometricContact()
    {
      // Constructor
    }

    ~GeometricContact()
    {
      // Destructor
    }

    /// Calculate map from master facets to possible colliding slave facets
    void
      contact_surface_map_volume_sweep(Mesh& mesh, Function& u,
                                          const std::vector<std::size_t>& master_facets,
                                          const std::vector<std::size_t>& slave_facets);

    /// For each of the master facets on this process, compute the DoFs of the cells belonging
    /// to the facets in contact on the contact process(es).
    void
    tabulate_contact_cell_to_shared_dofs(Mesh& mesh, Function& u,
                                         const std::vector<std::size_t>& master_facets,
                                         const std::vector<std::size_t>& slave_facets);


    /// Tabulate the mapping from local master facet, which are in possible contact with their
    /// shared cells' metadata.
    void
    tabulate_contact_shared_cells(Mesh& mesh, Function& u,
                                  const std::vector<std::size_t>& master_facets,
                                  const std::vector<std::size_t>& slave_facets);

    /// Get master to slave mapping
    const std::map<std::size_t, std::vector<std::size_t>>& master_to_slave() const
    {
      return _master_to_slave;
    }

    /// Get slave to master mapping
    const std::map<std::size_t, std::vector<std::size_t>>& slave_to_master() const
    {
      return _slave_to_master;
    }

    /// Get dof matchup
    const std::map<std::size_t, std::vector<std::size_t>>& local_cells_to_contact_dofs() const
    {
      return _local_cell_to_contact_dofs;
    };

    /// Get dof matchup
    const std::map<std::size_t, std::vector<std::size_t>>& local_cell_to_off_proc_contact_dofs() const
    {
      return _local_cell_to_off_proc_contact_dofs;
    };


    const std::vector<std::shared_ptr<CellMetaData>> get_cell_meta_data(const std::size_t m_idx)
    {
      return _master_facet_to_contacted_cells[m_idx];
    };


  private:

    std::map<std::size_t, std::vector<std::shared_ptr<CellMetaData>>> _master_facet_to_contacted_cells;

    // Project surface forward from a facet using 'u', creating a prismoidal volume in 2D or 3D
    static std::vector<Point> create_deformed_segment_volume(const Mesh& mesh,
                                                             std::size_t facet_index,
                                                             const Function& u,
                                                             std::size_t gdim);

    // Make a mesh of the displacement volume
    static void create_displacement_volume_mesh(Mesh& displacement_mesh,
                                                const Mesh& mesh,
                                                const std::vector<std::size_t> contact_facets,
                                                const Function& u);

    // Make a mesh of a communicated facets mesh
    static void create_communicated_prism_mesh(Mesh& prism_mesh,
                                               const Mesh& mesh,
                                               const std::vector<std::size_t>& recv_facets,
                                               const std::vector<double>& coord);

    static void create_on_process_sub_mesh(Mesh& sub_mesh, const Mesh& mesh);

    // Tabulate pairings between collided displacement volume meshes on this process only.
    static void tabulate_on_process_bbox_collisions(const std::size_t mpi_rank,
                                                    const Mesh& master_mesh,
                                                    const std::vector<std::size_t>& master_facets,
                                                    const Mesh& slave_mesh,
                                                    const std::vector<std::size_t>& slave_facets,
                                                    std::map<std::size_t, std::vector<std::size_t>>& master_to_slave);

    // Tabulate pairings between collided displacement volume meshes.
    static void tabulate_off_process_displacement_volume_mesh_pairs(const Mesh& mesh,
                                                                    const Mesh& slave_mesh,
                                                                    const Mesh& master_mesh,
                                                                    const std::vector<std::size_t>& slave_facets,
                                                                    const std::vector<std::size_t>& master_facets,
                                                                    std::map<std::size_t, std::vector<std::size_t>>& contact_facet_map);

    // Tabulate pairings between facet index and collided cell DoFs
    void tabulate_collided_cell_dofs(const Mesh& mesh, const GenericDofMap& dofmap,
                                            const std::map<std::size_t, std::vector<std::size_t>>& master_to_slave,
                                            std::map<std::size_t, std::vector<std::size_t>>& facet_to_contacted_dofs,
                                            std::map<std::size_t, std::vector<std::size_t>>& facet_to_off_proc_contacted_dofs);

    // Find number of cells in projected prism in 2D or 3D
    static std::size_t cells_per_facet(std::size_t tdim) { return (tdim - 1)*4; };

    // Find number of cells in projected prism in 2D or 3D
    static std::size_t vertices_per_facet(std::size_t tdim) { return tdim*2; };

    std::map<std::size_t, std::vector<std::size_t>> _master_to_slave;
    std::map<std::size_t, std::vector<std::size_t>> _slave_to_master;
    std::map<std::size_t, std::vector<std::size_t>> _local_cell_to_contact_dofs;
    std::map<std::size_t, std::vector<std::size_t>> _local_cell_to_off_proc_contact_dofs;

  };

}

#endif
