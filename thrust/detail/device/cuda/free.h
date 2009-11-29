/*
 *  Copyright 2008-2009 NVIDIA Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


/*! \file free.h
 *  \brief Defines the interface to free() on CUDA.
 */

#pragma once

#ifdef __CUDACC__

#include <thrust/device_ptr.h>
#include <cuda_runtime_api.h>
#include <stdexcept>
#include <string>

namespace thrust
{
namespace detail
{
namespace device
{
namespace cuda
{

inline void free(thrust::device_ptr<void> ptr)
{
  cudaError_t error = cudaFree(ptr.get());

  if(error)
  {
    throw std::runtime_error(std::string("CUDA error: ") + std::string(cudaGetErrorString(error)));
  } // end error
} // end free()

} // end namespace cuda
} // end namespace device
} // end namespace detail
} // end namespace thrust

#endif // __CUDACC__

