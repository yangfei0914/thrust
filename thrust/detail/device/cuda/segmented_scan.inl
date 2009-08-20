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


/*! \file segmented_scan.inl
 *  \brief Inline file for segmented_scan.h.
 */

// do not attempt to compile this file with any other compiler
#ifdef __CUDACC__


#include <thrust/experimental/arch.h>
#include <thrust/functional.h>
#include <thrust/device_malloc.h>
#include <thrust/device_free.h>
#include <thrust/copy.h>

#include <thrust/segmented_scan.h>    // for second level scans
#include <stdlib.h>                   // for malloc & free

#include <thrust/detail/util/blocking.h>
#include <thrust/detail/device/cuda/warp/scan.h>

#include <thrust/detail/device/dereference.h>

namespace thrust
{

namespace detail
{

namespace device
{

namespace cuda
{

namespace segmented_scan
{


/////////////    
// Kernels //
/////////////    


template<typename InputType,
         typename FlagType,
         typename InputIterator,
         typename FlagIterator,
         typename AssociativeOperator>
         __device__
InputType segscan_warp1(const unsigned int thread_lane, InputType val, FlagType mindex, InputIterator sval, FlagIterator sflg, AssociativeOperator binary_op)
{
#if __CUDA_ARCH__ >= 120
    // optimization
    if (!__any(mindex))
        return thrust::detail::device::cuda::warp::scan(thread_lane, val, sval, binary_op);
#endif

    // (1) Convert head flags to min_indices form
    mindex = thrust::detail::device::cuda::warp::scan(thread_lane, mindex, sflg, thrust::maximum<FlagType>());

    // (2) Perform segmented scan across warp
    sval[threadIdx.x] = val;

    if (thread_lane >= mindex +  1)  sval[threadIdx.x] = val = binary_op(sval[threadIdx.x -  1], val);
    if (thread_lane >= mindex +  2)  sval[threadIdx.x] = val = binary_op(sval[threadIdx.x -  2], val);
    if (thread_lane >= mindex +  4)  sval[threadIdx.x] = val = binary_op(sval[threadIdx.x -  4], val);
    if (thread_lane >= mindex +  8)  sval[threadIdx.x] = val = binary_op(sval[threadIdx.x -  8], val);
    if (thread_lane >= mindex + 16)  sval[threadIdx.x] = val = binary_op(sval[threadIdx.x - 16], val);

    return val;
}

template<typename FlagType,
         typename InputIterator,
         typename FlagIterator,
         typename AssociativeOperator>
         __device__
void segscan_warp2(const unsigned int thread_lane, FlagType flg, InputIterator sval, FlagIterator sflg, AssociativeOperator binary_op)
{
  
    typedef typename thrust::iterator_traits<InputIterator>::value_type InputType;

//// currently broken
//#if __CUDA_ARCH__ >= 120
//    // optimization
//    if (!__any(flg))
//        thrust::detail::device::cuda::warp::scan(thread_lane, sval[threadIdx.x], sval, binary_op);
//#endif

    // (1) Convert head flags to min_indices form
    FlagType mindex = (flg) ? thread_lane : 0;
    mindex = thrust::detail::device::cuda::warp::scan(thread_lane, mindex, sflg, thrust::maximum<FlagType>());

    // (2) Perform segmented scan across warp
    if (thread_lane >= mindex +  1)  sval[threadIdx.x] = binary_op(sval[threadIdx.x -  1], sval[threadIdx.x]);
    if (thread_lane >= mindex +  2)  sval[threadIdx.x] = binary_op(sval[threadIdx.x -  2], sval[threadIdx.x]);
    if (thread_lane >= mindex +  4)  sval[threadIdx.x] = binary_op(sval[threadIdx.x -  4], sval[threadIdx.x]);
    if (thread_lane >= mindex +  8)  sval[threadIdx.x] = binary_op(sval[threadIdx.x -  8], sval[threadIdx.x]);
    if (thread_lane >= mindex + 16)  sval[threadIdx.x] = binary_op(sval[threadIdx.x - 16], sval[threadIdx.x]);
}


template<unsigned int BLOCK_SIZE,
         typename OutputIterator,
         typename OutputType,
         typename AssociativeOperator>
__global__ void
inclusive_update_kernel(OutputIterator result,
                        const AssociativeOperator binary_op,
                        const unsigned int n,
                        const unsigned int interval_size,
                        OutputType * carry_in,
                        unsigned int * segment_lengths)
{
    const unsigned int thread_id   = BLOCK_SIZE * blockIdx.x + threadIdx.x;  // global thread index
    const unsigned int thread_lane = threadIdx.x & 31;                       // thread index within the warp
    const unsigned int warp_id     = thread_id   / 32;                       // global warp index

    const unsigned int interval_begin = warp_id * interval_size;                   // beginning of this warp's segment
    const unsigned int interval_end   = interval_begin + segment_lengths[warp_id]; // end of this warp's segment
    
    if(warp_id == 0 || interval_begin >= n) return;                         // nothing to do

    OutputType carry = carry_in[warp_id - 1];                                // value to add to this segment

    for(unsigned int i = interval_begin + thread_lane; i < interval_end; i += 32)
    {
        thrust::detail::device::dereference(result, i) = binary_op(carry, thrust::detail::device::dereference(result, i));
    }
}


template<unsigned int BLOCK_SIZE,
         typename OutputIterator,
         typename OutputType,
         typename AssociativeOperator>
__global__ void
exclusive_update_kernel(OutputIterator result,
                        OutputType init,
                        const AssociativeOperator binary_op,
                        const unsigned int n,
                        const unsigned int interval_size,
                        OutputType * carry_in,
                        unsigned int * segment_lengths)
                        
{
    const unsigned int thread_id   = BLOCK_SIZE * blockIdx.x + threadIdx.x;  // global thread index
    const unsigned int thread_lane = threadIdx.x & 31;                       // thread index within the warp
    const unsigned int warp_id     = thread_id   / 32;                       // global warp index

    const unsigned int interval_begin = warp_id * interval_size;                   // beginning of this warp's segment
    const unsigned int interval_end   = interval_begin + segment_lengths[warp_id]; // end of this warp's segment
    
    if(warp_id == 0 || interval_begin >= n) return;                                // nothing to do

    OutputType carry = binary_op(init, carry_in[warp_id - 1]);                      // value to add to this segment

    unsigned int i = interval_begin + thread_lane;

    if(i < interval_end)
    {
        OutputType val = thrust::detail::device::dereference(result, i);
        
        if (thread_lane == 0)
            val = carry;
        else
            val = binary_op(carry, val);
        
        thrust::detail::device::dereference(result, i) = val;
    }

    for(i += 32; i < interval_end; i += 32)
    {
        OutputType val = thrust::detail::device::dereference(result, i);

        val = binary_op(carry, val);

        thrust::detail::device::dereference(result, i) = val;
    }
}



/* Perform an inclusive scan on separate intervals
 *
 * For intervals of length 2:
 *    [ a, b, c, d, ... ] -> [ a, a+b, c, c+d, ... ]
 *
 * Each warp is assigned an interval of [first, first + n)
 */
template<unsigned int BLOCK_SIZE,
         typename InputIterator1, 
         typename InputIterator2,
         typename OutputIterator,
         typename AssociativeOperator,
         typename BinaryPredicate,
         typename OutputType>
__global__ 
void inclusive_scan_kernel(InputIterator1 first1,
                           InputIterator2 first2,
                           OutputIterator result,
                           AssociativeOperator binary_op,
                           BinaryPredicate pred,
                           const unsigned int n,
                           const unsigned int interval_size,
                           OutputType * final_val,
                           unsigned int * segment_lengths)
{
  typedef typename thrust::iterator_traits<InputIterator2>::value_type KeyType;
  typedef unsigned int FlagType;

  // XXX warpSize exists, but is not known at compile time,
  //     so define our own constant
  const unsigned int WARP_SIZE = 32;

  //__shared__ volatile OutputType sval[BLOCK_SIZE];
  //__shared__ volatile KeyType    skey[BLOCK_SIZE];
  __shared__ unsigned char sval_workaround[BLOCK_SIZE * sizeof(OutputType)];
  __shared__ unsigned char skey_workaround[BLOCK_SIZE * sizeof(KeyType)];
  OutputType * sval = reinterpret_cast<OutputType*>(sval_workaround);
  KeyType    * skey = reinterpret_cast<KeyType*>(skey_workaround);
  __shared__ FlagType    sflg[BLOCK_SIZE];

  const unsigned int thread_id   = BLOCK_SIZE * blockIdx.x + threadIdx.x;      // global thread index
  const unsigned int thread_lane = threadIdx.x & (WARP_SIZE - 1);              // thread index within the warp
  const unsigned int warp_id     = thread_id   / WARP_SIZE;                    // global warp index

  const unsigned int interval_begin = warp_id * interval_size;                 // beginning of this warp's segment
  const unsigned int interval_end   = min(interval_begin + interval_size, n);  // end of this warp's segment

  unsigned int i = interval_begin + thread_lane;                               // initial thread starting position

  unsigned int first_segment_end = interval_end;                               // length of initial segment in this interval

  if(interval_begin >= interval_end)                                           // warp has nothing to do
      return;

  FlagType   mindex;

  if(i < interval_end)
  {
      OutputType val = thrust::detail::device::dereference(first1, i);
      KeyType    key = thrust::detail::device::dereference(first2, i);

      // compute head flags
      skey[threadIdx.x] = key;
      if (thread_lane == 0)
      {
          if(warp_id == 0 || !pred(thrust::detail::device::dereference(first2, i - 1), key))
              first_segment_end = i;
          mindex = thread_lane;
      }
      else if (pred(skey[threadIdx.x - 1], key))
      {
          mindex = 0;
      }
      else
      {
          first_segment_end = i;
          mindex = thread_lane;
      }

      val = segscan_warp1(thread_lane, val, mindex, sval, sflg, binary_op);

      thrust::detail::device::dereference(result, i) = val;
      
      i += 32;
  }
     
  
  while(i < interval_end)
  {
      OutputType val = thrust::detail::device::dereference(first1, i);
      KeyType    key = thrust::detail::device::dereference(first2, i);

      if (thread_lane == 0)
      {
          if(pred(skey[threadIdx.x + 31], key))
              val = binary_op(sval[threadIdx.x + 31], val);                    // segment spans warp boundary
          else
              first_segment_end = min(first_segment_end, i);                   // new segment begins here
      }

      // compute head flags
      skey[threadIdx.x] = key;
      if (thread_lane == 0 || pred(skey[threadIdx.x - 1], key))
      {
          mindex = 0;
      }
      else
      {
          first_segment_end = min(first_segment_end, i);
          mindex = thread_lane;
      }

      val = segscan_warp1(thread_lane, val, mindex, sval, sflg, binary_op);

      thrust::detail::device::dereference(result, i) = val;
      
      i += 32;
  }
   
  // write out final value
  if (i == interval_end + 31)
  {
      final_val[warp_id] = sval[threadIdx.x];
  }

  // compute first segment boundary
  first_segment_end = thrust::detail::device::cuda::warp::scan(thread_lane, first_segment_end, sflg, thrust::minimum<FlagType>());

  // write out initial segment length
  if (thread_lane == 31)
      segment_lengths[warp_id] = first_segment_end - interval_begin;

//  /// XXX BEGIN TEST
//  if(thread_lane == 0){
//    unsigned int initial_segment_length = interval_end - interval_begin;
//
//    OutputType sum = thrust::detail::device::dereference(first1, i);
//    thrust::detail::device::dereference(result, i) = sum;
//
//    i++;
//    while( i < interval_end ){
//        if (pred(thrust::detail::device::dereference(first2, i - 1), thrust::detail::device::dereference(first2, i)))
//        {
//            sum = binary_op(sum, thrust::detail::device::dereference(first1, i));
//        }
//        else 
//        {
//            sum = thrust::detail::device::dereference(first1, i);
//            initial_segment_length = min(initial_segment_length, i - interval_begin);
//        }
//
//        thrust::detail::device::dereference(result, i) = sum;
//        i++;
//    }
//
//    if (warp_id > 0 && !pred(thrust::detail::device::dereference(first2, interval_begin - 1), 
//                             thrust::detail::device::dereference(first2, interval_begin)))
//        initial_segment_length = 0; // segment does not overlap interval boundary
//    
//    final_val[warp_id] = sum;
//    segment_lengths[warp_id] = initial_segment_length;
//  }
//  // XXX END TEST

} // end kernel()



/* Perform an exclusive scan on separate intervals
 *
 * For intervals of length 3:
 *    [ a, b, c, d, ... ] -> [ init, a, a+b, init, c, ... ]
 *
 * Each warp is assigned an interval of [first, first + n)
 */
template<unsigned int BLOCK_SIZE,
         typename InputIterator1, 
         typename InputIterator2,
         typename OutputIterator,
         typename AssociativeOperator,
         typename BinaryPredicate,
         typename OutputType>
__global__ 
void exclusive_scan_kernel(InputIterator1 first1,
                           InputIterator2 first2,
                           OutputIterator result,
                           OutputType init,
                           AssociativeOperator binary_op,
                           BinaryPredicate pred,
                           const unsigned int n,
                           const unsigned int interval_size,
                           OutputType * final_val,
                           unsigned int * segment_lengths)
{
  typedef typename thrust::iterator_traits<InputIterator2>::value_type KeyType;
  typedef unsigned int FlagType;

  // XXX warpSize exists, but is not known at compile time,
  //     so define our own constant
  const unsigned int WARP_SIZE = 32;

  //__shared__ volatile OutputType sval[BLOCK_SIZE];
  //__shared__ volatile KeyType    skey[BLOCK_SIZE];
  __shared__ unsigned char sval_workaround[BLOCK_SIZE * sizeof(OutputType)];
  __shared__ unsigned char skey_workaround[BLOCK_SIZE * sizeof(KeyType)];
  OutputType * sval = reinterpret_cast<OutputType*>(sval_workaround);
  KeyType    * skey = reinterpret_cast<KeyType*>(skey_workaround);
  __shared__ FlagType    sflg[BLOCK_SIZE];

  const unsigned int thread_id   = BLOCK_SIZE * blockIdx.x + threadIdx.x;      // global thread index
  const unsigned int thread_lane = threadIdx.x & (WARP_SIZE - 1);              // thread index within the warp
  const unsigned int warp_id     = thread_id   / WARP_SIZE;                    // global warp index

  const unsigned int interval_begin = warp_id * interval_size;                 // beginning of this warp's segment
  const unsigned int interval_end   = min(interval_begin + interval_size, n);  // end of this warp's segment

  unsigned int i = interval_begin + thread_lane;                               // initial thread starting position

  unsigned int first_segment_end = interval_end;                               // length of initial segment in this interval

  if(interval_begin >= interval_end)                                           // warp has nothing to do
      return;
  
  OutputType val;
  KeyType    key;
  FlagType   flg;


  if(i < interval_end)
  {
      sval[threadIdx.x] = thrust::detail::device::dereference(first1, i);
      skey[threadIdx.x] = thrust::detail::device::dereference(first2, i);

      // compute head flags
      if (thread_lane == 0)
      {
          if(warp_id == 0 || !pred(thrust::detail::device::dereference(first2, i - 1), skey[threadIdx.x]))
              first_segment_end = i;
          flg = 1;
      }
      else if (pred(skey[threadIdx.x - 1], skey[threadIdx.x]))
      {
          flg = 0;
      }
      else
      {
          first_segment_end = i;
          flg = 1;
      }

      segscan_warp2(thread_lane, flg, sval, sflg, binary_op);
  
      first_segment_end = thrust::detail::device::cuda::warp::scan(thread_lane, first_segment_end, sflg, thrust::minimum<FlagType>());
      
      if (thread_lane != 0)
          val = sval[threadIdx.x - 1]; // value to the left

      if (flg)
          val = init;
      else if (first_segment_end < i)
          val = binary_op(init, val);

      // when thread_lane == 0 and warp_id != 0, result is bogus
      thrust::detail::device::dereference(result, i) = val;
      
      i += 32;
  }
     
  
  while(i < interval_end)
  {
      if (thread_lane == 0)
      {
          first_segment_end = sflg[threadIdx.x + 31];
          val = sval[threadIdx.x + 31];
          key = skey[threadIdx.x + 31];
      }
             
      sval[threadIdx.x] = thrust::detail::device::dereference(first1, i);
      skey[threadIdx.x] = thrust::detail::device::dereference(first2, i);

      if (thread_lane == 0 && pred(key, skey[threadIdx.x]))
          sval[threadIdx.x] = binary_op(val, sval[threadIdx.x]);           // segment spans warp boundary
      else
          key = skey[threadIdx.x - 1];

      // compute head flags
      if(pred(key, skey[threadIdx.x]))
      {
          flg = 0;
      }
      else
      {
          flg = 1;
          first_segment_end = min(first_segment_end, i);
      }

      segscan_warp2(thread_lane, flg, sval, sflg, binary_op);

      first_segment_end = thrust::detail::device::cuda::warp::scan(thread_lane, first_segment_end, sflg, thrust::minimum<FlagType>());

      if (thread_lane != 0)
          val = sval[threadIdx.x - 1]; // value to the left

      if (flg)
          val = init;
      else if (first_segment_end < i)
          val = binary_op(init, val);

      thrust::detail::device::dereference(result, i) = val;
     
      i += 32;
  }
   
  // write out final value
  if (i == interval_end + 31)
  {
      final_val[warp_id] = sval[threadIdx.x];
  }

  // compute first segment boundary
  first_segment_end = thrust::detail::device::cuda::warp::scan(thread_lane, first_segment_end, sflg, thrust::minimum<FlagType>());

  // write out initial segment length
  if (thread_lane == 31)
      segment_lengths[warp_id] = first_segment_end - interval_begin;


//  /// XXX BEGIN TEST
//  if(thread_lane == 0){
//    unsigned int initial_segment_length = interval_end - interval_begin;
//
//    OutputType temp = thrust::detail::device::dereference(first1, i);
//    OutputType next;
//    
//    if (warp_id == 0 || !pred(thrust::detail::device::dereference(first2, interval_begin - 1), 
//                              thrust::detail::device::dereference(first2, interval_begin)))
//
//    {
//        initial_segment_length = 0; // segment does not overlap interval boundary
//        next = binary_op(init, temp);
//        thrust::detail::device::dereference(result, i) = init;
//    }
//    else
//    {
//        next = temp;
//        //thrust::detail::device::dereference(result, i) = ???; // no value to put here
//    }
//      
//
//    i++;
//
//    while( i < interval_end ){
//        temp = thrust::detail::device::dereference(first1, i);
// 
//        if (!pred(thrust::detail::device::dereference(first2, i - 1), thrust::detail::device::dereference(first2, i)))
//        {
//            next = init;
//            initial_segment_length = min(initial_segment_length, i - interval_begin);
//        }
//
//        thrust::detail::device::dereference(result, i) = next;
//        
//        next = binary_op(next, temp);
//
//        i++;
//    }
//
//    
//    final_val[warp_id] = next;
//    segment_lengths[warp_id] = initial_segment_length;
//  }
//  // XXX END TEST

} // end kernel()




struct __segment_spans_interval
{
    const unsigned int interval_size;

    __segment_spans_interval(const int _interval_size) : interval_size(_interval_size) {}
    template <typename T>
    __host__ __device__
    bool operator()(const T& a, const T& b) const
    {
        return b == interval_size;
    }
};

} // end namespace segmented_scan




//////////////////
// Entry Points //
//////////////////

template<typename InputIterator1,
         typename InputIterator2,
         typename OutputIterator,
         typename AssociativeOperator,
         typename BinaryPredicate>
  OutputIterator inclusive_segmented_scan(InputIterator1 first1,
                                          InputIterator1 last1,
                                          InputIterator2 first2,
                                          OutputIterator result,
                                          AssociativeOperator binary_op,
                                          BinaryPredicate pred)
{
    typedef typename thrust::iterator_traits<OutputIterator>::value_type OutputType;

    if(first1 == last1) 
        return result;
    
    const size_t n = last1 - first1;

    // XXX todo query for warp size
    const unsigned int WARP_SIZE  = 32;
    const unsigned int BLOCK_SIZE = 256;
    const unsigned int MAX_BLOCKS = experimental::arch::max_active_threads()/BLOCK_SIZE;
    const unsigned int WARPS_PER_BLOCK = BLOCK_SIZE/WARP_SIZE;

    const unsigned int num_units  = thrust::detail::util::divide_ri(n, WARP_SIZE);
    const unsigned int num_warps  = std::min(num_units, WARPS_PER_BLOCK * MAX_BLOCKS);
    const unsigned int num_blocks = thrust::detail::util::divide_ri(num_warps,WARPS_PER_BLOCK);
    const unsigned int num_iters  = thrust::detail::util::divide_ri(num_units, num_warps);          // number of times each warp iterates, interval length is 32*num_iters

    const unsigned int interval_size = WARP_SIZE * num_iters;

    // create a temp vector for per-warp results
    thrust::device_ptr<OutputType>   d_final_val       = thrust::device_malloc<OutputType>(num_warps + 1);
    thrust::device_ptr<unsigned int> d_segment_lengths = thrust::device_malloc<unsigned int>(num_warps + 1);

    //////////////////////
    // first level scan
    segmented_scan::inclusive_scan_kernel<BLOCK_SIZE> <<<num_blocks, BLOCK_SIZE>>>
        (first1, first2, result, binary_op, pred, n, interval_size, d_final_val.get(), d_segment_lengths.get());

    bool second_scan_device = true;

    ///////////////////////
    // second level scan
    if (second_scan_device) {
        // scan final_val on the device (use one warp of GPU method for second level scan)
        segmented_scan::inclusive_scan_kernel<WARP_SIZE> <<<1, WARP_SIZE>>>
            (d_final_val.get(), d_segment_lengths.get(), d_final_val.get(), binary_op, segmented_scan::__segment_spans_interval(interval_size),
             num_warps, num_warps, (d_final_val + num_warps).get(), (d_segment_lengths + num_warps).get());
    } else {
        // scan final_val on the host
        OutputType*   h_final_val       = (OutputType*)  (::malloc(num_warps * sizeof(OutputType)));
        unsigned int* h_segment_lengths = (unsigned int*)(::malloc(num_warps * sizeof(unsigned int)));

        thrust::copy(d_final_val,       d_final_val        + num_warps, h_final_val);
        thrust::copy(d_segment_lengths, d_segment_lengths  + num_warps, h_segment_lengths);

        thrust::experimental::inclusive_segmented_scan(h_final_val, h_final_val + num_warps, h_segment_lengths, h_final_val, binary_op, segmented_scan::__segment_spans_interval(interval_size));

        // copy back to device
        thrust::copy(h_final_val, h_final_val + num_warps, d_final_val);
        ::free(h_final_val);
        ::free(h_segment_lengths);
    }
        
    //////////////////////
    // update intervals
    segmented_scan::inclusive_update_kernel<BLOCK_SIZE> <<<num_blocks, BLOCK_SIZE>>>
        (result, binary_op, n, interval_size, d_final_val.get(), d_segment_lengths.get());

    // free device work array
    thrust::device_free(d_final_val);
    thrust::device_free(d_segment_lengths);

    return result + n;
} // end inclusive_segmented_scan()


template<typename InputIterator1,
         typename InputIterator2,
         typename OutputIterator,
         typename T,
         typename AssociativeOperator,
         typename BinaryPredicate>
  OutputIterator exclusive_segmented_scan(InputIterator1 first1,
                                          InputIterator1 last1,
                                          InputIterator2 first2,
                                          OutputIterator result,
                                          const T init,
                                          AssociativeOperator binary_op,
                                          BinaryPredicate pred)
{
    typedef typename thrust::iterator_traits<OutputIterator>::value_type OutputType;

    if(first1 == last1) 
        return result;
    
    const size_t n = last1 - first1;

    // XXX todo query for warp size
    const unsigned int WARP_SIZE  = 32;
    const unsigned int BLOCK_SIZE = 256;
    const unsigned int MAX_BLOCKS = experimental::arch::max_active_threads()/BLOCK_SIZE;
    const unsigned int WARPS_PER_BLOCK = BLOCK_SIZE/WARP_SIZE;

    const unsigned int num_units  = thrust::detail::util::divide_ri(n, WARP_SIZE);
    const unsigned int num_warps  = std::min(num_units, WARPS_PER_BLOCK * MAX_BLOCKS);
    const unsigned int num_blocks = thrust::detail::util::divide_ri(num_warps,WARPS_PER_BLOCK);
    const unsigned int num_iters  = thrust::detail::util::divide_ri(num_units, num_warps);          // number of times each warp iterates, interval length is 32*num_iters

    const unsigned int interval_size = WARP_SIZE * num_iters;

    // create a temp vector for per-warp results
    thrust::device_ptr<OutputType>   d_final_val       = thrust::device_malloc<OutputType>(num_warps + 1);
    thrust::device_ptr<unsigned int> d_segment_lengths = thrust::device_malloc<unsigned int>(num_warps + 1);

    //////////////////////
    // first level scan
    segmented_scan::exclusive_scan_kernel<BLOCK_SIZE> <<<num_blocks, BLOCK_SIZE>>>
        (first1, first2, result, OutputType(init), binary_op, pred, n, interval_size, d_final_val.get(), d_segment_lengths.get());

    bool second_scan_device = true;

    ///////////////////////
    // second level scan
    if (second_scan_device) {
        // scan final_val on the device (use one warp of GPU method for second level scan)
        segmented_scan::inclusive_scan_kernel<WARP_SIZE> <<<1, WARP_SIZE>>>
            (d_final_val.get(), d_segment_lengths.get(), d_final_val.get(), binary_op, segmented_scan::__segment_spans_interval(interval_size),
             num_warps, num_warps, (d_final_val + num_warps).get(), (d_segment_lengths + num_warps).get());
    } else {
        // scan final_val on the host
        OutputType*   h_final_val       = (OutputType*)  (::malloc(num_warps * sizeof(OutputType)));
        unsigned int* h_segment_lengths = (unsigned int*)(::malloc(num_warps * sizeof(unsigned int)));

        thrust::copy(d_final_val,       d_final_val        + num_warps, h_final_val);
        thrust::copy(d_segment_lengths, d_segment_lengths  + num_warps, h_segment_lengths);

        thrust::experimental::inclusive_segmented_scan(h_final_val, h_final_val + num_warps, h_segment_lengths, h_final_val, binary_op, segmented_scan::__segment_spans_interval(interval_size));

        // copy back to device
        thrust::copy(h_final_val, h_final_val + num_warps, d_final_val);
        ::free(h_final_val);
        ::free(h_segment_lengths);
    }
        
    //////////////////////
    // update intervals
    segmented_scan::exclusive_update_kernel<BLOCK_SIZE> <<<num_blocks, BLOCK_SIZE>>>
        (result, OutputType(init), binary_op, n, interval_size, d_final_val.get(), d_segment_lengths.get());
    
    // free device work array
    thrust::device_free(d_final_val);
    thrust::device_free(d_segment_lengths);

    return result + n;
} // end exclusive_interval_scan()


} // end namespace cuda

} // end namespace device

} // end namespace detail

} // end namespace thrust

#endif // __CUDACC__
