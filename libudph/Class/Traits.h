#pragma once

#include <type_traits>

namespace UD
{
namespace Traits
{
// template <class T>
// struct BasicTraits {
//  using Self = T;
//  template <class T2>
//  static constexpr bool Is = std::is_same_v<T, T2>;
//  template <class T2>
//  static constexpr bool IsDerived = std::is_base_of_v<T2, T>;
//  template <class T2>
//  static constexpr bool IsBase = std::is_base_of_v<T, T2>;
//};
// template <class _Type, class _Derived = _Type>
// struct Traits;
namespace detail
{
template<class T>
struct BasicWrapper
{
  struct BasicImpl
  {
    using Self = T;
    template<class T2>
    static constexpr bool Is = std::is_same_v<T, T2>;
    template<class T2>
    static constexpr bool IsDerived = std::is_base_of_v<T2, T>;
    template<class T2>
    static constexpr bool IsBase = std::is_base_of_v<T, T2>;
  };
};
}  // namespace detail
template<class T>
using Basic = typename detail::BasicWrapper<T>::BasicImpl;

template<class T, class U>
struct Register;
template<class T, class... Ts>
struct Inherit : public Register<Ts, T>...
{
};

template<class T>
struct Traits
    : public detail::BasicWrapper<T>
    , public Register<T, T>
{
};
// template <class T, class... Ts>
// struct Define;
// template <class T>
// struct Register;
//
// template <class T, class... Ts>
// struct Defined {};
// template <class T>
// requires requires() {
//  {Define<T>()};
//}
// struct Defined<T> : public Define<T> {};
// template <class T, class U>
// requires requires() {
//  {Define<T, U>()};
//}
// struct Defined<T, U> : public Define<T, U> {};
// template <class T>
// struct Registered {};
// template <class T>
// requires requires() {
//  {Register<T>()};
//}
// struct Registered<T> : public Register<T> {};
//
// template <class T, class... Ts>
// struct Base : public Defined<Ts>...,
//              public Defined<Ts, T>...,
//              public Registered<Ts>... {};
//
// template <class T, Type = Type::All>
// struct Traits;
// template <class T>
// struct Traits<T, Type::All> : public Traits<T, Type::Basic>,
//                              public Traits<T, Type::Simple>,
//                              public Traits<T, Type::Complex>,
//                              public Traits<T, Type::Super> {};
// template <class T>
// struct Traits<T, Type::Complex> : public Defined<T, T> {};
//
// template <class T>
// struct Traits<T, Type::Super> : public Registered<T> {};
// template <class T>
// struct Traits<T, Type::Simple> : public Defined<T> {};
// template <class T>
// struct Traits<T, Type::Basic> {
//  using Self = T;
//  template <class T2>
//  static constexpr bool Is = std::is_same_v<T, T2>;
//  template <class T2>
//  static constexpr bool IsDerived = std::is_base_of_v<T2, T>;
//  template <class T2>
//  static constexpr bool IsBase = std::is_base_of_v<T, T2>;
//};
// template <class T>
// using All = Traits<T, Type::All>;
// template <class T>
// using Complex = Traits<T, Type::Complex>;
// template <class T>
// using Simple = Traits<T, Type::Simple>;
// template <class T>
// using Basic = Traits<T, Type::Basic>;
// template <class T>
// using Super = Traits<T, Type::Super>;
//
// template <class T>
// struct Helper {
//  using Complex = UD::Traits::Complex<T>;
//  using Simple = UD::Traits::Simple<T>;
//  using Basic = UD::Traits::Basic<T>;
//  using Super = UD::Traits::Super<T>;
//};
//
// struct A {};
// struct B : public A {};
//
// template <>
// struct Define<A> : Helper<A> {
//  using S = Helper::All::Self;
//};
// template <class U>
// struct Define<A, U> : Helper<A> {
//  using NotS = U;
//};
//
// template <>
// struct Register<B> : Base<B, A> {};
//
// static_assert(std::is_same_v<Simple<A>::S, A>);
// static_assert(std::is_same_v<Traits<B>::NotS, B>);
}  // namespace Traits
}  // namespace UD
