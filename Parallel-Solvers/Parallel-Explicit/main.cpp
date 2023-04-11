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
 
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <mpi.h>
#include <Kokkos_Core.hpp>
#include "Explicit_Solver.h"
#include "Explicit_Solver_SGH.h"
#include "Simulation_Parameters.h"

void solver_setup(int argc, char *argv[]);

//==============================================================================
//    Main
//==============================================================================

int main(int argc, char *argv[]){
  
  //initialize MPI
  MPI_Init(&argc,&argv);
  
  Kokkos::initialize();
  solver_setup(argc, argv);
  
  MPI_Barrier(MPI_COMM_WORLD);
  
  Kokkos::finalize();
  MPI_Finalize();

  return 0;
}

void solver_setup(int argc, char *argv[]){
  /*General strategy: process initial input here to determine which solver
    object to construct
  */
  int myrank;
  MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
  //base solver class pointer
  //Solver *solver;
  //read user solver settings
  Simulation_Parameters *simparam;
  Solver *solver;
  std::string filename = std::string(argv[1]);
  if(filename.find(".yaml") != std::string::npos){
      simparam = new Simulation_Parameters();
      std::string yaml_error;
      bool yaml_exit_flag = false;
    
      //check for user error in providing yaml options (flags unsupported options)
      yaml_error = simparam->yaml_input(filename);
      if(yaml_error!="success"){
        std::cout << yaml_error << std::endl;
        yaml_exit_flag = true;
      } 
    
      if(yaml_exit_flag){
        //exit_solver(0);
      }

      //use map of set options to set member variables of the class
      simparam->apply_settings();
      if(simparam->solver_type=="SGH"){
        solver = new Explicit_Solver_SGH();
        //assign parameters read in by the base simulation parameters class to derived class in solver;
        //this includes the map of all yaml options read in.
        solver->simparam->Simulation_Parameters::operator=(*simparam);
  
        //solver = new Static_Solver();
        //solver = new Pseudo_Laplacian();

        //checks for optional solver routines
        if(solver->setup_flag) solver->solver_setup();

        // invoke solver's run function (should perform most of the computation)//
        solver->run(argc,argv);
  
        //invoke optional finalize function
        if(solver->finalize_flag) solver->solver_finalize();
        delete solver;
      }
      else{
        if(myrank==0){
          std::cout << "Solver type undefined, please check yaml file for correctness" << std::endl;
        }
      }
    delete simparam;
  }
  else{
    solver = new Explicit_Solver_SGH();
  
    //solver = new Static_Solver();
    //solver = new Pseudo_Laplacian();

    //checks for optional solver routines
    if(solver->setup_flag) solver->solver_setup();

    // invoke solver's run function (should perform most of the computation)//
    solver->run(argc,argv);
  
    //invoke optional finalize function
    if(solver->finalize_flag) solver->solver_finalize();
  }
  
  //allocate chosen solver
  //solver = new Static_Solver_Parallel();
  //Static_Solver_Parallel solver;
  //delete solver;
}
