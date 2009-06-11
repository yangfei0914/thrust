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


/*! \file logical.h
 *  \brief Logical operations on ranges.
 */

#pragma once

#include <thrust/detail/config.h>

namespace thrust
{

/*! \addtogroup reductions
 *  \{
 *  \addtogroup logical
 *  \ingroup reductions
 *  \{
 */


/*! \p all_of determines whether all elements in a range satify a predicate.
 * Specifically, \all_of returns \c true if <tt>pred(*i)</tt> is \c true
 * for every iterator \c i in the range <tt>[begin, end)</tt> and 
 * \c false otherwise.
 *
 *  \param begin The beginning of the sequence.
 *  \param end  The end of the sequence.
 *  \param pred A predicate used to test range elements.
 *  \return \c true, if all elements satisfy the predicate; \c false, otherwise.
 *
 *  \tparam InputIterator is a model of <a href="http://www.sgi.com/tech/stl/InputIterator.html">Input Iterator</a>,
 *  \tparam Predicate must be a model of <a href="http://www.sgi.com/tech/stl/Predicate.html">Predicate</a>.
 *
 *  \code
 *  #include <thrust/logical.h>
 *  #include <thrust/functional.h>
 *  ...
 *  bool A[3] = {true, true, false};
 *
 *  thrust::all_of(A, A + 2, thrust::identity<bool>()); // returns true
 *  thrust::all_of(A, A + 3, thrust::identity<bool>()); // returns false
 *
 *  // empty range
 *  thrust::all_of(A, A, thrust::identity<bool>()); // returns false
 *  
 *  \endcode
 *
 *  \see any_of
 *  \see none_of
 *  \see transform_reduce
 */
template <class InputIterator, class Predicate>
bool all_of(InputIterator begin, InputIterator end, Predicate pred);
   

/*! \p any_of determines whether any element in a range satifies a predicate.
 * Specifically, \any_of returns \c true if <tt>pred(*i)</tt> is \c true
 * for any iterator \c i in the range <tt>[begin, end)</tt> and 
 * \c false otherwise.
 *
 *  \param begin The beginning of the sequence.
 *  \param end  The end of the sequence.
 *  \param pred A predicate used to test range elements.
 *  \return \c true, if any element satisfies the predicate; \c false, otherwise.
 *
 *  \tparam InputIterator is a model of <a href="http://www.sgi.com/tech/stl/InputIterator.html">Input Iterator</a>,
 *  \tparam Predicate must be a model of <a href="http://www.sgi.com/tech/stl/Predicate.html">Predicate</a>.
 *
 *  \code
 *  #include <thrust/logical.h>
 *  #include <thrust/functional.h>
 *  ...
 *  bool A[3] = {true, true, false};
 *
 *  thrust::any_of(A, A + 2, thrust::identity<bool>()); // returns true
 *  thrust::any_of(A, A + 3, thrust::identity<bool>()); // returns true
 *
 *  thrust::any_of(A + 2, A + 3, thrust::identity<bool>()); // returns false
 *
 *  // empty range
 *  thrust::any_of(A, A, thrust::identity<bool>()); // returns false
 *  \endcode
 *
 *  \see all_of
 *  \see none_of
 *  \see transform_reduce
 */
template <class InputIterator, class Predicate>
bool any_of(InputIterator begin, InputIterator end, Predicate pred);


/*! \p none_of determines whether no element in a range satifies a predicate.
 * Specifically, \p none_of returns \c true if there is no iterator \c i in 
 * the range <tt>[begin, end)</tt> such that <tt>pred(*i)</tt> is \c true,
 * and \c false otherwise.
 *
 *  \param begin The beginning of the sequence.
 *  \param end  The end of the sequence.
 *  \param pred A predicate used to test range elements.
 *  \return \c true, if no element satisfies the predicate; \c false, otherwise.
 *
 *  \tparam InputIterator is a model of <a href="http://www.sgi.com/tech/stl/InputIterator.html">Input Iterator</a>,
 *  \tparam Predicate must be a model of <a href="http://www.sgi.com/tech/stl/Predicate.html">Predicate</a>.
 *
 *  \code
 *  #include <thrust/logical.h>
 *  #include <thrust/functional.h>
 *  ...
 *  bool A[3] = {true, true, false};
 *
 *  thrust::any_of(A, A + 2, thrust::identity<bool>()); // returns false
 *  thrust::any_of(A, A + 3, thrust::identity<bool>()); // returns false
 *
 *  thrust::any_of(A + 2, A + 3, thrust::identity<bool>()); // returns true
 *
 *  // empty range
 *  thrust::any_of(A, A, thrust::identity<bool>()); // returns true
 *  \endcode
 *
 *  \see all_of
 *  \see any_of
 *  \see transform_reduce
 */
template <class InputIterator, class Predicate>
bool none_of(InputIterator begin, InputIterator end, Predicate pred);

/*! \} // end logical
 *  \} // end reductions
 */

} // end namespace thrust

#include <thrust/detail/logical.inl>

