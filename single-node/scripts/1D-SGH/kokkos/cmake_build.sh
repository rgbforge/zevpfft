#!/bin/bash -e

rm -rf ${SGH_BUILD_DIR}
mkdir -p ${SGH_BUILD_DIR}
cd ${SGH_BUILD_DIR}

NUM_TASKS=1
if [ "$1" = "hpc" ]
then
    NUM_TASKS=32
fi

# Kokkos flags for Cuda
CUDA_ADDITIONS=(
-D CUDA=ON
-D CMAKE_CXX_COMPILER=${KOKKOS_INSTALL_DIR}/bin/nvcc_wrapper
)

# Kokkos flags for Hip
HIP_ADDITIONS=(
-D HIP=ON
-D CMAKE_CXX_COMPILER=hipcc
)

# Kokkos flags for OpenMP
OPENMP_ADDITIONS=(
-D OPENMP=ON
)

# Kokkos flags for PThreads
PTHREADS_ADDITIONS=(
-D THREADS=ON
)

# Empty those lists if not building
if [ "$2" = "cuda" ]
then
    HIP_ADDITIONS=() 
    PTHREADS_ADDITIONS=() 
    OPENMP_ADDITIONS=()
elif [ "$2" = "hip" ]
then
    CUDA_ADDITIONS=()
    PTHREADS_ADDITIONS=() 
    OPENMP_ADDITIONS=()
elif [ "$2" = "openmp" ]
then
    HIP_ADDITIONS=() 
    CUDA_ADDITIONS=()
    PTHREADS_ADDITIONS=() 
elif [ "$2" = "pthreads" ]
then
    HIP_ADDITIONS=() 
    CUDA_ADDITIONS=()
    OPENMP_ADDITIONS=()
else
    HIP_ADDITIONS=() 
    CUDA_ADDITIONS=()
    PTHREADS_ADDITIONS=() 
    OPENMP_ADDITIONS=()
fi

ADDITIONS=(
${CUDA_ADDITIONS[@]}
${HIP_ADDITIONS[@]}
${OPENMP_ADDITIONS[@]}
${PTHREADS_ADDITIONS[@]}
)

OPTIONS=(
-D BUILD_1D_KOKKOS_SGH=ON
-D BUILD_EXPLICIT_SOLVER=OFF
-D KOKKOS=ON
${ADDITIONS[@]}
-D Kokkos_DIR=${KOKKOS_INSTALL_DIR}/lib64/cmake/Kokkos
)
set -x
cmake "${OPTIONS[@]}" "${SGH_BASE_DIR:-../}"
set +x
make -j${NUM_TASKS}

cd $basedir
