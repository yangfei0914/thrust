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

#include <thrust/random/linear_feedback_shift_engine.h>

namespace thrust
{

namespace experimental
{

namespace random
{

template<typename UIntType, size_t w, size_t k, size_t q, size_t s>
  linear_feedback_shift_engine<UIntType,w,k,q,s>
    ::linear_feedback_shift_engine(result_type value)
{
  seed(value);
} // end linear_feedback_shift_engine::linear_feedback_shift_engine()

template<typename UIntType, size_t w, size_t k, size_t q, size_t s>
  void linear_feedback_shift_engine<UIntType,w,k,q,s>
    ::seed(result_type value)
{
  m_value = value;
} // end linear_feedback_shift_engine::seed()

template<typename UIntType, size_t w, size_t k, size_t q, size_t s>
  typename linear_feedback_shift_engine<UIntType,w,k,q,s>::result_type
    linear_feedback_shift_engine<UIntType,w,k,q,s>
      ::operator()(void)
{
  const UIntType b = (((m_value << q) ^ m_value) & wordmask) >> (k-s);
  const UIntType mask = ( (~static_cast<UIntType>(0)) << (w-k) ) & wordmask;
  m_value = ((m_value & mask) << s) ^ b;
  return m_value;
} // end linear_feedback_shift_engine::operator()()

template<typename UIntType, size_t w, size_t k, size_t q, size_t s>
  void linear_feedback_shift_engine<UIntType,w,k,q,s>
    ::discard(unsigned long long z)
{
  for(; z > 0; --z)
  {
    this->operator()();
  } // end for
} // end linear_feedback_shift_engine::discard()


template<typename UIntType_, size_t w_, size_t k_, size_t q_, size_t s_,
         typename CharT, typename Traits>
std::basic_ostream<CharT,Traits>&
operator<<(std::basic_ostream<CharT,Traits> &os,
           const linear_feedback_shift_engine<UIntType_,w_,k_,q_,s_> &e)
{
  typedef std::basic_ostream<CharT,Traits> ostream_type;
  typedef typename ostream_type::ios_base  ios_base;

  // save old flags & fill character
  const typename ios_base::fmtflags flags = os.flags();
  const CharT fill = os.fill();

  os.flags(ios_base::dec | ios_base::fixed | ios_base::left);
  os.fill(os.widen(' '));

  // output one word of state
  os << e.m_value;

  // restore flags & fill character
  os.flags(flags);
  os.fill(fill);

  return os;
}


template<typename UIntType_, size_t w_, size_t k_, size_t q_, size_t s_,
         typename CharT, typename Traits>
std::basic_istream<CharT,Traits>&
operator>>(std::basic_istream<CharT,Traits> &is,
           linear_feedback_shift_engine<UIntType_,w_,k_,q_,s_> &e)
{
  typedef std::basic_istream<CharT,Traits> istream_type;
  typedef typename istream_type::ios_base     ios_base;

  // save old flags
  const typename ios_base::fmtflags flags = is.flags();

  is.flags(ios_base::dec);

  // input one word of state
  is >> e.m_value;

  // restore flags
  is.flags(flags);

  return is;
}


} // end random

} // end experimental

} // end thrust

