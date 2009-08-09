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


/*! \file copy_device_to_device.h
 *  \brief Device implementations for copying on the device.
 */

#pragma once

#include <thrust/iterator/iterator_traits.h>

#include <stdlib.h>           // for malloc & free
#include <thrust/distance.h>
#include <thrust/device_ptr.h>

#include <thrust/detail/device/trivial_copy.h>

namespace thrust
{

namespace detail
{

namespace device
{

template<typename InputIterator,
         typename OutputIterator>
  OutputIterator copy_host_to_device(InputIterator begin,
                                     InputIterator end,
                                     OutputIterator result,
                                     thrust::experimental::single_pass_traversal_tag, 
                                     thrust::experimental::random_access_traversal_tag)
{
    // host container to device container
    typedef typename thrust::iterator_traits<InputIterator>::value_type InputType;

    typename thrust::iterator_traits<InputIterator>::difference_type n = thrust::distance(begin, end);

    // allocate temporary storage
    InputType *temp = reinterpret_cast<InputType*>(malloc(sizeof(InputType) * n));
    InputType *temp_end = thrust::copy(begin, end, temp);

    result = thrust::copy(temp, temp_end, result);

    free(temp);
    return result;
}

// host pointer to device pointer with trivial copy
template<typename InputIterator,
         typename OutputIterator>
  OutputIterator copy_host_to_device(InputIterator begin,
                                     InputIterator end,
                                     OutputIterator result,
                                     thrust::experimental::random_access_traversal_tag,
                                     thrust::experimental::random_access_traversal_tag,
                                     true_type)
{
  // how many elements to copy?
  typename thrust::iterator_traits<OutputIterator>::difference_type n = end - begin;

  // what is the output type?
  typedef typename thrust::iterator_traits<OutputIterator>::value_type OutputType;

  trivial_copy_host_to_device(raw_pointer_cast(&*result), raw_pointer_cast(&*begin),  n * sizeof(OutputType));

  return result + n;
}

// random access host iterator to random access device iterator with non-trivial copy
template<typename InputIterator,
         typename OutputIterator>
  OutputIterator copy_host_to_device(InputIterator begin,
                                     InputIterator end,
                                     OutputIterator result,
                                     thrust::experimental::random_access_traversal_tag,
                                     thrust::experimental::random_access_traversal_tag,
                                     false_type)
{
  typedef typename thrust::iterator_traits<OutputIterator>::value_type OutputType;

  typename thrust::iterator_traits<InputIterator>::difference_type n = thrust::distance(begin,end);

  // allocate temporary storage
  OutputType *temp = reinterpret_cast<OutputType*>(malloc(sizeof(OutputType) * n));
  OutputType *temp_end = thrust::copy(begin, end, temp);

  result = thrust::copy(temp, temp_end, result);

  free(temp);
  return result;
}

// random access host iterator to random access device iterator
template<typename InputIterator,
         typename OutputIterator>
  OutputIterator copy_host_to_device(InputIterator begin,
                                     InputIterator end,
                                     OutputIterator result,
                                     thrust::experimental::random_access_traversal_tag input_traversal,
                                     thrust::experimental::random_access_traversal_tag output_traversal)
{
    // dispatch on whether this is a trivial copy
    return copy_host_to_device(begin, end, result, input_traversal, output_traversal,
            typename is_trivial_copy<InputIterator,OutputIterator>::type());
}


/////////////////
// Entry Point //
/////////////////

template<typename InputIterator,
         typename OutputIterator>
  OutputIterator copy_host_to_device(InputIterator begin, 
                                     InputIterator end, 
                                     OutputIterator result)
{
    return copy_host_to_device(begin, end, result, 
            typename thrust::experimental::iterator_traversal<InputIterator>::type(),
            typename thrust::experimental::iterator_traversal<OutputIterator>::type());
}

} // end namespace device

} // end namespace detail

} // end namespace thrust

