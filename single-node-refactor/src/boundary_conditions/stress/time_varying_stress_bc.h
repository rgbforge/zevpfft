/**********************************************************************************************
� 2020. Triad National Security, LLC. All rights reserved.
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

#ifndef BOUNDARY_STRESS_TIME_VARYING_H
#define BOUNDARY_STRESS_TIME_VARYING_H

#include "boundary_conditions.h"

struct BoundaryConditionEnums_t;

namespace TimeVaryingStressBC
{
// add an enum for boundary statevars and global vars
enum BCVars
{
    x_comp = 0,
    y_comp = 1,
    z_comp = 2,
    hydro_bc_stress_0 = 3,
    hydro_bc_stress_1 = 4,
    hydro_bc_stress_t_start = 5,
    hydro_bc_stress_t_end = 6
};

/////////////////////////////////////////////////////////////////////////////
///
/// \fn stress
///
/// \brief This is a function to force the boundary to move with a specified
///        pressure that varies in time.  The function form is an exponential
///        decay, given by:
///         if (t_end > time > t_start) then
///              p(t) = p0 exp(-p1*(time - time_start) )
///
/// \param Mesh object
/// \param Boundary condition enums to select options
/// \param Boundary condition global variables array
/// \param Boundary condition state variables array
/// \param Node velocity
/// \param Time of the simulation
/// \param Boundary global index for the surface node
/// \param Boundary set local id
///
/////////////////////////////////////////////////////////////////////////////
KOKKOS_FUNCTION
static void stress(const Mesh_t& mesh,
    const DCArrayKokkos<BoundaryConditionEnums_t>& BoundaryConditionEnums,
    const RaggedRightArrayKokkos<double>& stress_bc_global_vars,
    const DCArrayKokkos<double>& bc_state_vars,
    const ViewCArrayKokkos <double>& corner_surf_force,
    const ViewCArrayKokkos <double>& corner_surf_normal,
    const double time_value,
    const size_t rk_stage,
    const size_t bdy_node_gid,
    const size_t bdy_set)
{
    const double x_comp = stress_bc_global_vars(bdy_set, BCVars::x_comp);
    const double y_comp = stress_bc_global_vars(bdy_set, BCVars::y_comp);
    const double z_comp = stress_bc_global_vars(bdy_set, BCVars::z_comp);
    const double hydro_bc_stress_0 = stress_bc_global_vars(bdy_set, BCVars::hydro_bc_stress_0);
    const double hydro_bc_stress_1 = stress_bc_global_vars(bdy_set, BCVars::hydro_bc_stress_1);
    const double hydro_bc_stress_t_start = stress_bc_global_vars(bdy_set, BCVars::hydro_bc_stress_t_start);
    const double hydro_bc_stress_t_end   = stress_bc_global_vars(bdy_set, BCVars::hydro_bc_stress_t_end);

    // directions are as follows:
    // x_plane  = 0,
    // y_plane  = 1,
    // z_plane  = 2,

    // Set pressure in that direction to specified value
    // if t_end > time > t_start
    // p(t) = p0 exp(-p1*(time - time_start) )


    return;
} // end func
} // end namespace

#endif // end Header Guard