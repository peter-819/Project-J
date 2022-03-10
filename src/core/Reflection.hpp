#pragma once
template <unsigned I>
struct tag : tag<I - 1> {};

template <>
struct tag<0> {};

struct init
{
  template <typename T>
  operator T(); // never defined
};
template <typename T>
constexpr auto size_(tag<4>) 
  -> decltype(T{init{}, init{}, init{}, init{}}, 0u)
{ return 4u; }

template <typename T>
constexpr auto size_(tag<3>) 
  -> decltype(T{init{}, init{}, init{}}, 0u)
{ return 3u; }

template <typename T>
constexpr auto size_(tag<2>) 
  -> decltype(T{init{}, init{}}, 0u)
{ return 2u; }

template <typename T>
constexpr auto size_(tag<1>) 
  -> decltype(T{init{}}, 0u)
{ return 1u; }

template <typename T>
constexpr auto size_(tag<0>) 
  -> decltype(T{}, 0u)
{ return 0u; }

template <typename T>
constexpr size_t size() 
{ 
  static_assert(std::is_aggregate_v<T>);
  return size_<T>(tag<4>{}); // highest supported number 
}
template <typename T, typename F>
void for_each_member(T const& v, F f)
{
  static_assert(std::is_aggregate_v<T>);

  if constexpr (size<T>() == 4u)
  {
    const auto& [m0, m1, m2, m3] = v;
    f(0, m0); f(1, m1); f(2, m2); f(3, m3);
  }
  else if constexpr (size<T>() == 3u)
  {
    const auto& [m0, m1, m2] = v;
    f(0, m0); f(1, m1); f(2, m2);
  }
  else if constexpr (size<T>() == 2u)
  {
    const auto& [m0, m1] = v;
    f(0, m0); f(1, m1);
  }
  else if constexpr (size<T>() == 1u)
  {
    const auto& [m0] = v;
    f(0, m0);
  }
}