/**********************************************************************************************
© 2020. Triad National Security, LLC. All rights reserved.
This program was produced under U.S. Government contract 89233218CNA000001 for Los Alamos
National Laboratory (LANL), which is operated by Triad National Security, LLC for the U.S.
Department of Energy/National Nuclear Security Administration. All rights in the program are
reserved by Triad National Security, LLC, and the U.S. Department of Energy/National Nuclear
Security Administration. The Government is granted for itself and others acting on its behalf a
nonexclusive, paid-up, irrevocable worldwide license in this material to reproduce, prepare
derivative works, distribute copies to the public, perform publicly and display publicly, and
to permit others to do so.
This program is open source under the BSD-3 License.
Redistribution and use in source and binary forms, with or without modification, are permitted
provided that the following conditions are met:
1.  Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer.
2.  Redistributions in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or other materials
provided with the distribution.
3.  Neither the name of the copyright holder nor the names of its contributors may be used
to endorse or promote products derived from this software without specific prior
written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********************************************************************************************/

#include "state.h"
#include "mesh.h"
#include "geometry_new.h"
#include "sgh_solver.h"
#include "io_utils.h"
#include <chrono>

/////////////////////////////////////////////////////////////////////////////
///
/// \fn solve
///
/// Evolve the state according to the SGH method
///
/////////////////////////////////////////////////////////////////////////////
void SGH::execute(SimulationParameters_t& SimulationParamaters, 
                  Material_t& Materials, 
                  BoundaryCondition_t& BoundaryConditions, 
                  mesh_t& mesh, 
                  State_t& State)
{
    std::cout << "In execute function in sgh solver" << std::endl;

    double fuzz  = SimulationParamaters.dynamic_options.fuzz;
    double tiny  = SimulationParamaters.dynamic_options.tiny;
    double small = SimulationParamaters.dynamic_options.small;

    double graphics_dt_ival  = SimulationParamaters.output_options.graphics_time_step;
    int    graphics_cyc_ival = SimulationParamaters.output_options.graphics_iteration_step;

    double time_initial = SimulationParamaters.dynamic_options.time_initial;
    double time_final   = SimulationParamaters.dynamic_options.time_final;
    double dt_min   = SimulationParamaters.dynamic_options.dt_min;
    double dt_max   = SimulationParamaters.dynamic_options.dt_max;
    double dt_start = SimulationParamaters.dynamic_options.dt_start;
    double dt_cfl   = SimulationParamaters.dynamic_options.dt_cfl;

    int rk_num_stages = SimulationParamaters.dynamic_options.rk_num_stages;
    int cycle_stop    = SimulationParamaters.dynamic_options.cycle_stop;

    // initialize time, time_step, and cycles
    double time_value = 0.0;
    double dt = dt_start;

    // Create mesh writer
    MeshWriter mesh_writer; // Note: Pull to driver after refactoring evolution

    // --- num vars ----
    size_t num_dims = 3;

    CArray<double> graphics_times = CArray<double>(20000);
    graphics_times(0) = 0.0;
    double graphics_time = 0.0; // the times for writing graphics dump
    size_t graphics_id   = 0;



    // printf("Writing outputs to file at %f \n", time_value);
    // mesh_writer.write_mesh(mesh, MaterialPoints, node, corner, SimulationParamaters, time_value, graphics_times);

    CArrayKokkos<double> node_extensive_mass(mesh.num_nodes);

    // extensive energy tallies over the entire mesh
    double IE_t0 = 0.0;
    double KE_t0 = 0.0;
    double TE_t0 = 0.0;

    double cached_pregraphics_dt = fuzz;

    // calculate the extensive node mass, its key to 2D
    calc_extensive_node_mass(node_extensive_mass
                             State.node.coords,
                             State.node.mass,
                             mesh.num_dims,
                             mesh.num_nodes)


    // extensive IE
    for(int mat_id=0; mat_id<num_mats; mat_id++){

        size_t num_mat_points = State.MaterialPoints(mat_id).num_material_points;

        IE_t0 += sum_domain_internal_energy(State.MaterialPoints(mat_id).mass,
                                            State.MaterialPoints(mat_id).sie,
                                            num_mat_points);
    } // end loop over mat_id

    // extensive KE
    KE_t0 = sum_domain_kinetic_energy(State.node.vel,
                                      State.node.coords,
                                      State.node.mass
                                      mesh);
    // extensive TE
    TE_t0 = IE_t0 + KE_t0;

    // a flag to exit the calculation
    size_t stop_calc = 0;

    auto time_1 = std::chrono::high_resolution_clock::now();

    std::cout << "Applying initial boundary conditions" << std::endl;
    boundary_velocity(mesh, BoundaryConditions, node.vel, time_value); // Time value = 0.0;

    // the number of materials specified by the user input
    const size_t num_mats = Materials.num_mats;

    // loop over the max number of time integration cycles
    for (size_t cycle = 0; cycle < cycle_stop; cycle++) {
        // stop calculation if flag
        if (stop_calc == 1) {
            break;
        }

        cached_pregraphics_dt = dt;

        // the smallest time step across all materials
        double min_dt_calc = dt_max;

        // calculating time step per material
        for(int mat_id=0; mat_id<num_mats; mat_id++){

            // initialize the material dt
            double dt_mat = dt;

            // get the step
            if (mesh.num_dims == 2) {
                get_timestep2D(mesh,
                               State.node.coords,
                               State.node.vel,
                               State.MaterialPoints(mat_id).sspd,
                               State.GaussPoints.vol,
                               State.MaterialToMeshMaps(mat_id).elem,
                               State.MaterialToMeshMaps(mat_id).num_material_elems,
                               time_value,
                               graphics_time,
                               time_final,
                               dt_max,
                               dt_min,
                               dt_cfl,
                               dt_mat,
                               fuzz);
            }
            else{
                get_timestep(mesh,
                             State.node.coords,
                             State.node.vel,
                             State.MaterialPoints(mat_id).sspd,
                             State.GaussPoints.vol,
                             State.MaterialToMeshMaps(mat_id).elem,
                             State.MaterialToMeshMaps(mat_id).num_material_elems,
                             time_value,
                             graphics_time,
                             time_final,
                             dt_max,
                             dt_min,
                             dt_cfl,
                             dt_mat,
                             fuzz);
            } // end if 2D

            // save the smallest dt of all materials
            min_dt_calc = fmin(dt_mat, min_dt_calc);
            
        } // end for loop over all mats

        dt = min_dt_calc;  // save this dt time step


        if (cycle == 0) {
            printf("cycle = %lu, time = %f, time step = %f \n", cycle, time_value, dt);
        }
        // print time step every 10 cycles
        else if (cycle % 20 == 0) {
            printf("cycle = %lu, time = %f, time step = %f \n", cycle, time_value, dt);
        } // end if

        // ---------------------------------------------------------------------
        //  integrate the solution forward to t(n+1) via Runge Kutta (RK) method
        // ---------------------------------------------------------------------
        for(int mat_id=0; mat_id<num_mats; mat_id++){
            // save the values at t_n
            rk_init(State.node.coords,
                    State.node.vel,
                    State.MaterialPoints(mat_id).sie,
                    State.MaterialPoints(mat_id).stress,
                    mesh.num_dims,
                    mesh.num_elems,
                    mesh.num_nodes,
                    State.MaterialPoints(mat_id).num_material_points);
        } // end for mat_id

        // integrate solution forward in time
        for (size_t rk_stage = 0; rk_stage < rk_num_stages; rk_stage++) {

            // ---- RK coefficient ----
            double rk_alpha = 1.0 / ((double)rk_num_stages - (double)rk_stage);

            // ---- Calculate velocity divergence for the element ----
            if (mesh.num_dims == 2) {
                get_divergence2D(GaussPoints.div,
                                 mesh,
                                 State.node.coords,
                                 State.node.vel,
                                 State.GaussPoints.vol);
            }
            else{
                get_divergence(State.GaussPoints.div,
                               mesh,
                               State.node.coords,
                               State.node.vel,
                               State.GaussPoints.vol);
            } // end if 2D

            // set corner force to zero
            FOR_ALL(corner_gid, 0, num_corners, {
                for (int dim = 0; dim < num_dims; dim++) {
                    corner_force(corner_gid, dim) = 0.0;
                }
            }); // end parallel for corners
            
            // ---- calculate the forces on the vertices and evolve stress (hypo model) ----
            for(int mat_id=0; mat_id<num_mats; mat_id++){

                size_t num_mat_elems = State.MaterialToMeshMaps(mat_id).num_material_elems;

                if (mesh.num_dims == 2) {
                    get_force_2D(Materials,
                                 mesh,
                                 State.GaussPoints.vol,
                                 State.GaussPoints.div,
                                 State.corner.force,
                                 State.node.coords,
                                 State.node.vel,
                                 State.MaterialPoints(mat_id).den,
                                 State.MaterialPoints(mat_id).sie,
                                 State.MaterialPoints(mat_id).pres,
                                 State.MaterialPoints(mat_id).stress,
                                 State.MaterialPoints(mat_id).sspd,
                                 State.MaterialPoints(mat_id).statev,
                                 State.MaterialCorners(mat_id).force,
                                 State.corners_in_mat_elem
                                 State.MaterialToMeshMaps(mat_id).elem,
                                 num_mat_elems,
                                 mat_id,
                                 fuzz,
                                 small,
                                 dt,
                                 rk_alpha);
                }
                else{
                    get_force(Materials,
                              mesh,
                              State.GaussPoints.vol,
                              State.GaussPoints.div,
                              State.GaussPoints.eroded,
                              State.corner.force,
                              State.node.coords,
                              State.node.vel,
                              State.MaterialPoints(mat_id).den,
                              State.MaterialPoints(mat_id).sie,
                              State.MaterialPoints(mat_id).pres,
                              State.MaterialPoints(mat_id).stress,
                              State.MaterialPoints(mat_id).sspd,
                              State.MaterialPoints(mat_id).statev,
                              State.MaterialCorners(mat_id).force,
                              State.corners_in_mat_elem,
                              State.MaterialToMeshMaps(mat_id).elem,
                              num_mat_elems,
                              mat_id,
                              fuzz,
                              small,
                              dt,
                              rk_alpha);
                }
            } // end for mat_id

            // ---- Update nodal velocities ---- //
            update_velocity(rk_alpha,
                            dt,
                            mesh,
                            State.node.vel,
                            State.node.mass,
                            State.corner.force);

            // ---- apply velocity boundary conditions to the boundary patches----
            boundary_velocity(mesh, BoundaryConditions, State.node.vel, time_value);

            // ---- apply contact boundary conditions to the boundary patches----
            boundary_contact(mesh, BoundaryConditions, State.node.vel, time_value);

            // mpi_coms();

            for(int mat_id=0; mat_id<num_mats; mat_id++){

                // ---- Update specific internal energy in the elements ----
                update_energy(rk_alpha,
                              dt,
                              mesh,
                              State.node.vel,
                              State.node.coords,
                              State.MaterialPoints(mat_id).sie,
                              State.MaterialPoints(mat_id).mass,
                              State.MaterialCorners(mat_id).force,
                              State.corners_in_mat_elem,
                              State.MaterialToMeshMaps(mat_id).elem,
                              State.MaterialToMeshMaps(mat_id).num_material_elems);
            } // end for mat_id

            // ---- Update nodal positions ----
            update_position(rk_alpha,
                            dt,
                            mesh.num_dims,
                            mesh.num_nodes,
                            node.coords,
                            node.vel);

            // ---- Calculate cell volume for next time step ----
            geometry::get_vol(GaussPoints.vol, node.coords, mesh);

            // ---- Calculate MaterialPoints state (den, pres, sound speed, stress) for next time step ----
            if (mesh.num_dims == 2) {
                update_state2D(Materials,
                               mesh,
                               State.node.coords,
                               State.node.vel,
                               State.MaterialPoints.den,
                               State.MaterialPoints.pres,
                               State.MaterialPoints.stress,
                               State.MaterialPoints.sspd,
                               State.MaterialPoints.sie,
                               State.GaussPoints.vol,
                               State.MaterialPoints.mass,
                               State.GaussPoints.mat_id,
                               State.MaterialPoints.statev,
                               dt,
                               rk_alpha);
            }
            else{
                update_state(Materials,
                             mesh,
                             State.node.coords,
                             State.node.vel,
                             State.MaterialPoints.den,
                             State.MaterialPoints.pres,
                             State.MaterialPoints.stress,
                             State.MaterialPoints.sspd,
                             State.MaterialPoints.sie,
                             State.GaussPoints.vol,
                             State.MaterialPoints.mass,
                             State.GaussPoints.mat_id,
                             State.MaterialPoints.statev,
                             State.GaussPoints.eroded,
                             dt,
                             rk_alpha);
            } // end if num_dims=2

            // ----
            // Notes on strength:
            //    1) hyper-elastic strength models are called in update_state
            //    2) hypo-elastic strength models are called in get_force
            //    3) strength models must be added by the user in user_mat.cpp


            // calculate the new corner masses if 2D
            if (mesh.num_dims == 2) {

                // calculate the nodal areal mass
                calc_nodal_area_mass(mesh, State.node_coords, node_extensive_mass, tiny);

            } // end of if 2D-RZ


        } // end of RK loop

        // increment the time
        time_value += dt;

        size_t write = 0;
        if ((cycle + 1) % graphics_cyc_ival == 0 && cycle > 0) {
            write = 1;
        }
        else if (cycle == cycle_stop) {
            write = 1;
        }
        else if (time_value >= time_final) {
            write = 1;
        }
        else if (time_value >= graphics_time) {
            write = 1;
        }

        // write outputs
        if (write == 1) {
            printf("Writing outputs to file at %f \n", graphics_time);
            mesh_writer.write_mesh(mesh, MaterialPoints, GaussPoints, node, corner, SimulationParamaters, time_value, graphics_times);

            graphics_time = time_value + graphics_dt_ival;

            dt = cached_pregraphics_dt;
        } // end if

        // end of calculation
        if (time_value >= time_final) {
            break;
        }

    } // end for cycle loop

    auto time_2    = std::chrono::high_resolution_clock::now();
    auto calc_time = std::chrono::duration_cast<std::chrono::nanoseconds>(time_2 - time_1).count();

    printf("\nCalculation time in seconds: %f \n", calc_time * 1e-9);

    // ---- Calculate energy tallies ----
    double IE_tend = 0.0;
    double KE_tend = 0.0;
    double TE_tend = 0.0;

    // extensive IE
    for(int mat_id=0; mat_id<num_mats; mat_id++){

        const size_t num_mat_points = State.MaterialPoints(mat_id).num_material_points;

        IE_tend += sum_domain_internal_energy(State.MaterialPoints(mat_id).mass,
                                              State.MaterialPoints(mat_id).sie,
                                              num_mat_points);
    } // end loop over mat_id

    // extensive KE
    KE_tend = sum_domain_kinetic_energy(State.node.vel,
                                        State.node.coords,
                                        State.node.mass
                                        mesh);
    // extensive TE
    TE_tend = IE_tend + KE_tend;

    printf("Time=0:   KE = %f, IE = %f, TE = %f \n", KE_t0, IE_t0, TE_t0);
    printf("Time=End: KE = %f, IE = %f, TE = %f \n", KE_tend, IE_tend, TE_tend);
    printf("total energy conservation error = %e \n\n", TE_tend - TE_t0);

} // end of SGH solve

/////////////////////////////////////////////////////////////////////////////
///
/// \fn max_Eigen3D
///
/// \brief Get the maximum eigenvalues of a given tensor
///
/// \param Input tensor
///
/////////////////////////////////////////////////////////////////////////////
KOKKOS_FUNCTION
double max_Eigen3D(const ViewCArrayKokkos<double> tensor)
{
    // Compute largest eigenvalue of a 3x3 tensor
    // Algorithm only works if tensor is symmetric
    double pi  = 3.141592653589793;
    size_t dim = tensor.dims(0);
    double trace, det;
    trace  = tensor(0, 0) + tensor(1, 1) + tensor(2, 2);
    det    = tensor(0, 0) * (tensor(1, 1) * tensor(2, 2) - tensor(1, 2) * tensor(2, 1));
    det   -= tensor(0, 1) * (tensor(1, 0) * tensor(2, 2) - tensor(1, 2) * tensor(2, 0));
    det   += tensor(0, 2) * (tensor(1, 0) * tensor(2, 1) - tensor(1, 1) * tensor(2, 0));
    trace /= 3.; // easier for computation
    double p2 = pow((tensor(0, 0) - trace), 2) + pow((tensor(1, 1) - trace), 2) +
                pow((tensor(2, 2) - trace), 2);
    p2 += 2. * (pow(tensor(0, 1), 2) + pow(tensor(0, 2), 2) + pow(tensor(1, 2), 2));

    double p = sqrt(p2 / 6.);

    // check for nan
    if (det != det) {
        return 0;
    }

    if (det == 0) {
        return 0;
    }

    double B_array[9];
    ViewCArrayKokkos<double> B(B_array, 3, 3);

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (i == j) {
                B(i, i) = (1 / p) * (tensor(i, i) - trace);
            }
            else{
                B(i, j) = (1 / p) * tensor(i, j);
            } // end if
        } // end for j
    } // end for i

    double r, phi;
    r  =  B(0, 0) * (B(1, 1) * B(2, 2) - B(1, 2) * B(2, 1));
    r -= B(0, 1) * (B(1, 0) * B(2, 2) - B(1, 2) * B(2, 0));
    r += B(0, 2) * (B(1, 0) * B(2, 1) - B(1, 1) * B(2, 0));
    r /= 2;
    // first two cases are to handle numerical difficulties.
    // In exact math -1 <= r <= 1
    if (r <= -1) {
        phi = pi / 3;
    }
    else if (r >= 1) {
        phi = 0;
    }
    else{
        phi = acos(r) / 3.;
    } // end if

    double eig1, eig2, eig3;
    eig1 = trace + 2 * p * cos(phi);
    eig2 = trace + 2 * p * cos(phi + (2 * pi / 3));
    eig3 = 3 * trace - (eig1 + eig2);

    double abs_max_val = fmax(fmax(fabs(eig1), fabs(eig2)), fabs(eig3));

    return abs_max_val;
}

/////////////////////////////////////////////////////////////////////////////
///
/// \fn max_Eigen2D
///
/// \brief Get the maximum eigenvalues of a given tensor
///
/// \param Input tensor
///
/////////////////////////////////////////////////////////////////////////////
KOKKOS_FUNCTION
double max_Eigen2D(const ViewCArrayKokkos<double> tensor)
{
    // Compute largest eigenvalue of a 2x2 tensor
    // Algorithm only works if tensor is symmetric
    size_t dim = tensor.dims(0);
    double trace, det;

    trace = tensor(0, 0) + tensor(1, 1);
    det   = tensor(0, 0) * tensor(1, 1) - tensor(0, 1) * tensor(1, 0);

    double eig1, eig2;

    eig1 = (trace / 2.) + sqrt(0.25 * trace * trace - det);
    eig2 = (trace / 2.) - sqrt(0.25 * trace * trace - det);

    double abs_max_val = fmax(fabs(eig1), fabs(eig2));
    return abs_max_val;
} // end 2D max eignen value


void calc_extensive_node_mass(CArrayKokkos<double>& node_extensive_mass
                              CArrayKokkos<double>& node_coords,
                              CArrayKokkos<double>& node_mass,
                              double num_dims,
                              double num_nodes){
    // save the nodal mass
    FOR_ALL(node_gid, 0, num_nodes, {

        double radius = 1.0;

        if (num_dims == 2) {
            radius = node_coords(1, node_gid, 1);
        }

        node_extensive_mass(node_gid) = node_mass(node_gid) * radius;
    }); // end parallel for

} // end function

// a function to tally the internal energy
double sum_domain_internal_energy(DCArrayKokkos<double>& MaterialPoints_mass,
                                  DCArrayKokkos<double>& MaterialPoints_sie,
                                  size_t num_mat_points,
                                  double &IE_sum){

    double IE_sum = 0;
    double IE_loc_sum;

    // loop over the material points and tally IE
    REDUCE_SUM(matpt_lid, 0, num_matpts, IE_loc_sum, {
        IE_loc_sum += MaterialPoints_mass(matpt_lid) * MaterialPoints_sie(matpt_lid);
    }, IE_sum);

    return IE_Sum;

} // end function 

// a function to tally the kinetic energy
double sum_domain_kinetic_energy(DCArrayKokkos<double>& node_vel,
                                 DCArrayKokkos<double>& node_coords,
                                 DCArrayKokkos<double>& node_mass,
                                 mest_t& mesh){
    // extensive KE
    double KE_sum = 0;
    double KE_loc_sum;

    REDUCE_SUM(node_gid, 0, mesh.num_nodes, KE_loc_sum, {
        double ke = 0;

        for (size_t dim = 0; dim < mesh.num_dims; dim++) {
            ke += node_vel(1, node_gid, dim) * node_vel(1, node_gid, dim); // 1/2 at end
        } // end for

        if (mesh.num_dims == 2) {
            KE_loc_sum += node_mass(node_gid) * node_coords(1, node_gid, 1) * ke;
        }
        else{
            KE_loc_sum += node_mass(node_gid) * ke;
        }
    }, KE_sum);
    Kokkos::fence();

    return 0.5*KE_sum;
} // end function


// a function to calculate the 2D-RZ areal mass (rho A = m/R)
// for R=0, it is interpolated from off-axis
void calc_node_areal_mass(const mesh_t& mesh,
                          const DCArrayKokkos<double>& node_coords,
                          CArrayKokkos<double> node_extensive_mass,
                          double tiny){

    // calculate the nodal areal mass
    FOR_ALL(node_gid, 0, mesh.num_nodes, {
        node.mass(node_gid) = 0.0;

        if (node.coords(1, node_gid, 1) > tiny) {
            node.mass(node_gid) = node_extensive_mass(node_gid) / node.coords(1, node_gid, 1);
        }
    }); // end parallel for over node_gid
    Kokkos::fence();

    // calculate the boundary areal mass
    FOR_ALL(node_bdy_gid, 0, mesh.num_bdy_nodes, {
        size_t node_gid = mesh.bdy_nodes(node_bdy_gid);

        if (node.coords(1, node_gid, 1) < tiny) {
            // node is on the axis

            for (size_t node_lid = 0; node_lid < mesh.num_nodes_in_node(node_gid); node_lid++) {
                size_t node_neighbor_gid = mesh.nodes_in_node(node_gid, node_lid);

                // if the node is off the axis, use it's areal mass on the boundary
                if (node.coords(1, node_neighbor_gid, 1) > tiny) {
                    node.mass(node_gid) = fmax(node.mass(node_gid), node.mass(node_neighbor_gid) / 2.0);
                }
            } // end for over neighboring nodes
        } // end if
    }); // end parallel for over elem_gid

    return;

}// end function