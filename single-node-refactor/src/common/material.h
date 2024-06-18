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

#ifndef FIERRO_MATERIAL_H
#define FIERRO_MATERIAL_H

#include <stdio.h>

#include "matar.h"

// eos files
#include "gamma_law_eos.h"
#include "no_eos.h"
#include "user_defined_eos.h"
#include "void_eos.h"

// strength
#include "no_strength.h"
#include "user_defined_strength.h"

// erosion files
#include "basic_erosion.h"
#include "no_erosion.h"

// fracture files
#include "user_defined_fracture.h"


namespace model
{
    // strength model types
    enum StrengthType
    {
        noStrengthType = 0,
        incrementBased = 1,        ///<  Model evaluation is inline with the time integration
        stateBased = 2,            ///<  Model is based on the state after each stage of the time step
    };

    // Specific strength models
    enum StrengthModels
    {
        noStrengthModel = 0,
        userDefinedStrength = 1,
    };

    // EOS model types
    enum EOSType
    {
        noEOSType = 0,          ///< No EOS used
        decoupledEOSType = 1,   ///< only an EOS, or an EOS plus deviatoric stress model
        coupledEOSType = 2,     ///< EOS is part of a full stress tensor evolution model
    };

    // The names of the eos models
    enum EOSModels
    {
        noEOS = 0,              ///<  no model evaluation
        gammaLawGasEOS = 1,     ///<  gamma law gas
        voidEOS = 2,            ///<  a void material, no sound speed and no pressure
        userDefinedEOS = 3,     ///<  an eos function defined by the user
    };

    // failure models
    enum FailureModels
    {
        noFailure = 0,
        brittleFailure = 1,        ///< Material fails after exceeding yield stress
        ductileFailure = 2,        ///< Material grows voids that lead to complete failure
    };

    // erosion model t
    enum ErosionModels
    {
        noErosion = 1,
        basicErosion = 2,      ///<  basic element erosion
    };

} // end model namespace

static std::map<std::string, model::StrengthType> strength_type_map
{
    { "no_strength",     model::noStrengthType },
    { "increment_based", model::incrementBased },
    { "state_based",     model::stateBased },
};

static std::map<std::string, model::StrengthModels> strength_models_map
{
    { "no_strength",           model::noStrengthModel },
    { "user_defined_strength", model::userDefinedStrength },
};

static std::map<std::string, model::EOSType> eos_type_map
{
    { "no_eos",    model::noEOSType },
    { "coupled",   model::coupledEOSType },
    { "decoupled", model::decoupledEOSType },
};

static std::map<std::string, model::EOSModels> eos_models_map
{
    { "no_eos",        model::noEOS },
    { "gamma_law_gas", model::gammaLawGasEOS },
    { "void",          model::voidEOS },
    { "user_defined",  model::userDefinedEOS },
};

static std::map<std::string, model::ErosionModels> erosion_model_map
{
    { "no_erosion", model::noErosion },
    { "basic", model::basicErosion},
};

namespace model_init
{
// strength model setup
enum strength_setup_tag
{
    input = 0,
    user_init = 1,
};
} // end of namespace

/////////////////////////////////////////////////////////////////////////////
///
/// \struct material_t
///
/// \brief  Material models
///
/// In the code: CArrayKokkos <Material_t> Material;
/////////////////////////////////////////////////////////////////////////////
struct material_t
{
    size_t id;

    // -- EOS --
    // none, decoupled, or coupled eos
    model::EOSType EOSType = model::noEOSType;

    // Equation of state (EOS) function pointers
    void (*calc_pressure)(const DCArrayKokkos<double>& elem_pres,
                          const DCArrayKokkos<double>& elem_stress,
                          const size_t elem_gid,
                          const size_t mat_id,
                          const DCArrayKokkos<double>& elem_state_vars,
                          const DCArrayKokkos<double>& elem_sspd,
                          const double den,
                          const double sie) = NULL;

    void (*calc_sound_speed)(const DCArrayKokkos<double>& elem_pres,
                             const DCArrayKokkos<double>& elem_stress,
                             const size_t elem_gid,
                             const size_t mat_id,
                             const DCArrayKokkos<double>& elem_state_vars,
                             const DCArrayKokkos<double>& elem_sspd,
                             const double den,
                             const double sie) = NULL;

    // Strength model type: none, or increment- or state-based
    model::StrengthType StrengthType = model::noStrengthType;

    // Material strength model function pointers
    void (*calc_stress)(const DCArrayKokkos<double>& elem_pres,
                        const DCArrayKokkos<double>& elem_stress,
                        const size_t elem_gid,
                        const size_t mat_id,
                        const DCArrayKokkos<double>& elem_state_vars,
                        const DCArrayKokkos<double>& elem_sspd,
                        const double den,
                        const double sie,
                        const ViewCArrayKokkos<double>& vel_grad,
                        const ViewCArrayKokkos<size_t>& elem_node_gids,
                        const DCArrayKokkos<double>&    node_coords,
                        const DCArrayKokkos<double>&    node_vel,
                        const double vol,
                        const double dt,
                        const double rk_alpha) = NULL;


    // -- Erosion --

    size_t void_mat_id;        ///< eroded elements get this mat_id
    double erode_tension_val;   ///< tension threshold to initiate erosion
    double erode_density_val;   ///< density threshold to initiate erosion
    // above should be removed, they go in CArrayKokkos<double> erosion_global_vars;
    void (*erode)(const DCArrayKokkos<double>& elem_pres,
                  const DCArrayKokkos<double>& elem_stress,
                  const DCArrayKokkos<bool>& elem_eroded,
                  const DCArrayKokkos<size_t>& elem_mat_id,
                  const size_t elem_gid,
                  const size_t void_mat_id,
                  const double erode_tension_val,
                  const double erode_density_val,
                  const DCArrayKokkos<double>& elem_sspd,
                  const DCArrayKokkos<double>& elem_den,
                  const double sie) = NULL;


    // setup the strength model via the input file for via a user_setup
    model_init::strength_setup_tag strength_setup = model_init::input;

    double q1   = 1.0;      ///< acoustic coefficient in Riemann solver for compression
    double q1ex = 1.3333;   ///< acoustic coefficient in Riemann solver for expansion
    double q2   = 1.0;      ///< linear coefficient in Riemann solver for compression
    double q2ex = 1.3333;   ///< linear coefficient in Riemann solver for expansion

}; // end material_t

/////////////////////////////////////////////////////////////////////////////
///
/// \struct material_model_values_t
///
/// \brief  Material model state, parameters, and values 
///
/// In the code: CArray <MaterialModelValues_t> MaterialModelValues;
/////////////////////////////////////////////////////////////////////////////
struct MaterialModelVars_t{

    ///<enums can be implemented in the model namespaces to unpack e.g., physics_global_vars

    DCArrayKokkos<double> eos_global_vars;      ///< Array of global variables for the EOS
    DCArrayKokkos<double> eos_state_vars;       ///< Array of state (in each element) variables for the EOS

    DCArrayKokkos<double> strength_global_vars; ///< Array of global variables for the strength model
    DCArrayKokkos<double> strength_state_vars;  ///< Array of state (in each element) variables for the strength
    
    DCArrayKokkos<double> failure_global_vars;  ///< Array of global variables for the failure model

    DCArrayKokkos<double> erosion_global_vars;  ///< Array of global variables for the erosion model

    DCArrayKokkos<double> art_viscosity_global_vars; ///< Array holding q1, q1ex, q2, ...

    // ...

}; // end MaterialModelVars_t
// The above struct eliminates all the variables in material_t, making material_t a collection of function ptrs


// ----------------------------------
// valid inputs for material options
// ----------------------------------
static std::vector<std::string> str_material_inps
{
    "id",
    "eos_model",
    "eos_model_type",
    "strength_model",
    "strength_model_type",
    "q1",
    "q2",
    "q1ex",
    "q2ex",
    "eos_global_vars",
    "strength_global_vars",
    "erosion_model",
    "erode_tension_val",
    "erode_density_val",
    "void_mat_id",
};

// ---------------------------------------------------------------
// required inputs for material options are specified here.
// The requirements vary depending on the problem type and solver
// ---------------------------------------------------------------
static std::vector<std::string> material_hydrodynamics_required_inps
{
    "id",
    "eos_model",
    "eos_model_type",
};
// required inputs are only required for eos problems

static std::vector<std::string> material_solid_dynamics_required_inps
{
    "id",
    "strength_model",
    "strength_model_type"
};


static std::vector<std::string> material_solid_statics_required_inps
{
    "id",
    "strength_global_vars"
};


static std::vector<std::string> material_thermal_statics_required_inps
{
    "id",
    "thermal_global_vars"
};


#endif // end Header Guard