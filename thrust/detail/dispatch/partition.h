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


/*! \file partition.h
 *  \brief Dispatch layer for the partition functions.
 */

#pragma once

#include <thrust/iterator/iterator_traits.h>

#include <thrust/detail/host/partition.h>
#include <thrust/detail/device/partition.h>

namespace thrust
{

namespace detail
{

namespace dispatch
{

////////////////
// Host Paths //
////////////////
template<typename ForwardIterator,
         typename Predicate>
  ForwardIterator partition(ForwardIterator first,
                            ForwardIterator last,
                            Predicate pred,
                            thrust::experimental::space::host)
{
    return thrust::detail::host::partition(first, last, pred);
}

template<typename ForwardIterator1,
         typename ForwardIterator2,
         typename Predicate>
  ForwardIterator2 partition_copy(ForwardIterator1 first,
                                  ForwardIterator1 last,
                                  ForwardIterator2 result,
                                  Predicate pred,
                                  thrust::experimental::space::host,
                                  thrust::experimental::space::host)
{
    return thrust::detail::host::partition_copy(first, last, result, pred);
}

template<typename ForwardIterator,
         typename Predicate>
  ForwardIterator stable_partition(ForwardIterator first,
                                   ForwardIterator last,
                                   Predicate pred,
                                   thrust::experimental::space::host)
{
    return thrust::detail::host::stable_partition(first, last, pred);
}

template<typename ForwardIterator1,
         typename ForwardIterator2,
         typename Predicate>
  ForwardIterator2 stable_partition_copy(ForwardIterator1 first,
                                         ForwardIterator1 last,
                                         ForwardIterator2 result,
                                         Predicate pred,
                                         thrust::experimental::space::host,
                                         thrust::experimental::space::host)
{
    return thrust::detail::host::stable_partition_copy(first, last, result, pred);
}


//////////////////
// Device Paths //
//////////////////
template<typename ForwardIterator,
         typename Predicate>
  ForwardIterator partition(ForwardIterator first,
                            ForwardIterator last,
                            Predicate pred,
                            thrust::experimental::space::device)
{
    return thrust::detail::device::partition(first, last, pred);
}

template<typename ForwardIterator1,
         typename ForwardIterator2,
         typename Predicate>
  ForwardIterator2 partition_copy(ForwardIterator1 first,
                                  ForwardIterator1 last,
                                  ForwardIterator2 result,
                                  Predicate pred,
                                  thrust::experimental::space::device,
                                  thrust::experimental::space::device)
{
    return thrust::detail::device::partition_copy(first, last, result, pred);
}

template<typename ForwardIterator,
         typename Predicate>
  ForwardIterator stable_partition(ForwardIterator first,
                                   ForwardIterator last,
                                   Predicate pred,
                                   thrust::experimental::space::device)
{
    return thrust::detail::device::stable_partition(first, last, pred);
}

template<typename ForwardIterator1,
         typename ForwardIterator2,
         typename Predicate>
  ForwardIterator2 stable_partition_copy(ForwardIterator1 first,
                                         ForwardIterator1 last,
                                         ForwardIterator2 result,
                                         Predicate pred,
                                         thrust::experimental::space::device,
                                         thrust::experimental::space::device)
{
    return thrust::detail::device::stable_partition_copy(first, last, result, pred);
}

} // end namespace dispatch

} // end namespace detail

} // end namespace thrust

