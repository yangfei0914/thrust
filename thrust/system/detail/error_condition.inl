#pragma once

#include <thrust/system/detail/error_condition.inl>
#include <thrust/functional.h>

namespace thrust
{

namespace experimental
{

namespace system
{

error_condition
  ::error_condition(void)
    :m_val(0),m_cat(&generic_category())
{
  ;
} // end error_condition::error_condition()


error_condition
  ::error_condition(int val, const error_category &cat)
    :m_val(val),m_cat(&cat)
{
  ;
} // end error_condition::error_condition()


template<typename ErrorConditionEnum>
  error_condition
    ::error_condition(ErrorConditionEnum e,
                      typename thrust::detail::enable_if<is_error_condition_enum<ErrorConditionEnum> >::type *)
{
  *this = make_error_condition(e);
} // end error_condition::error_condition()


void error_condition
  ::assign(int val, const error_category &cat)
{
  m_val = val;
  m_cat = cat;
} // end error_category::assign()


template<typename ErrorConditionEnum>
  typename thrust::detail::enable_if<is_error_condition_enum<ErrorConditionEnum>, error_code>::type &
    error_condition
      ::operator=(ErrorConditionEnum e)
{
  *this = make_error_condition(e);
} // end error_condition::operator=()


void error_condition
  ::clear(void)
{
  m_val = 0;
  m_cat = generic_category();
} // end error_condition::clear()


int error_condition
  ::value(void)
{
  return m_val;
} // end error_condition::value()


const error_category &error_condition
  ::category(void) const
{
  return *m_cat;
} // end error_condition::category()


std::string error_condition
  ::message(void) const
{
  return category().message(value());
} // end error_condition::message()


bool error_condition
  ::operator bool (void) const
{
  return value() != 0;
} // end error_condition::operator bool ()


error_condition make_error_condition(errc::errc_t e)
{
  return error_condition(static_cast<int>(e), generic_category());
} // end make_error_condition()


bool operator<(const error_condition &lhs,
               const error_condition &rhs)
{
  return (lhs.category() < rhs.category()) || ((lhs.category() == rhs.category()) && (lhs.value() < rhs.value()));
} // end operator<()


} // end system

} // end experimental

} // end thrust
