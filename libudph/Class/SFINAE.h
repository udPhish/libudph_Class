#pragma once
#include <memory>
#include <type_traits>

#include <libudph/Class/Pack.h>

namespace UD
{
namespace SFINAE
{
template<class T, class = void>
struct is_defined : std::false_type
{
};
template<class T>
struct is_defined<
    T,
    std::enable_if_t<std::is_object<T>::value && !std::is_pointer<T>::value
                     && (sizeof(T) > 0)>> : std::true_type
{
};
template<typename First, typename... Rest>
struct all_same
{
  static_assert(std::conjunction_v<std::is_same<First, Rest>...>);
  using type = First;
};
template<class... Ts>
struct pack_is_same : public std::false_type{};

template<class T, class... Ts, class... Us>
struct pack_is_same<UD::Pack::Pack<T, Ts...>, UD::Pack::Pack<T, Us...>>
    : public pack_is_same<UD::Pack::Pack<Ts...>, UD::Pack::Pack<Us...>>
{
};
template<>
struct pack_is_same<UD::Pack::Pack<>, UD::Pack::Pack<>>
    : public std::true_type
{
};
template<class... Ts>
struct pack_is_convertible : public std::false_type
{
};

template<class T, class U, class... Ts, class... Us> requires(std::is_convertible<T, U>::value)
struct pack_is_convertible<UD::Pack::Pack<T, Ts...>, UD::Pack::Pack<U, Us...>>
    : public pack_is_convertible<UD::Pack::Pack<Ts...>, UD::Pack::Pack<Us...>>
{
};
template<>
struct pack_is_convertible<UD::Pack::Pack<>, UD::Pack::Pack<>>
    : public std::true_type
{
};

template<class T>
struct is_shared_ptr : std::false_type
{
};
template<class T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type
{
};
template<class T>
using is_shared_ptr_t = typename is_shared_ptr<T>::type;
template<class T>
inline constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

template<class T>
struct is_weak_ptr : std::false_type
{
};
template<class T>
struct is_weak_ptr<std::weak_ptr<T>> : std::true_type
{
};
template<class T>
using is_weak_ptr_t = typename is_weak_ptr<T>::type;
template<class T>
inline constexpr bool is_weak_ptr_v = is_weak_ptr<T>::value;

template<class T>
struct is_unique_ptr : std::false_type
{
};
template<class T>
struct is_unique_ptr<std::unique_ptr<T>> : std::true_type
{
};
template<class T>
using is_unique_ptr_t = typename is_unique_ptr<T>::type;
template<class T>
inline constexpr bool is_unique_ptr_v = is_unique_ptr<T>::value;

template<class T>
struct is_smart_ptr : std::false_type
{
};
template<class T>
struct is_smart_ptr<std::shared_ptr<T>> : std::true_type
{
};
template<class T>
struct is_smart_ptr<std::weak_ptr<T>> : std::true_type
{
};
template<class T>
struct is_smart_ptr<std::unique_ptr<T>> : std::true_type
{
};
template<class T>
using is_smart_ptr_t = typename is_smart_ptr<T>::type;
template<class T>
inline constexpr bool is_smart_ptr_v = is_smart_ptr<T>::value;

template<class T>
struct is_any_ptr : std::false_type
{
};
template<class T>
struct is_any_ptr<T*> : std::true_type
{
};
template<class T>
struct is_any_ptr<std::shared_ptr<T>> : std::true_type
{
};
template<class T>
struct is_any_ptr<std::weak_ptr<T>> : std::true_type
{
};
template<class T>
struct is_any_ptr<std::unique_ptr<T>> : std::true_type
{
};
template<class T>
using is_any_ptr_t = typename is_any_ptr<T>::type;
template<class T>
inline constexpr bool is_any_ptr_v = is_any_ptr<T>::value;

template<class T>
struct remove_smart_ptr
{
  typedef T type;
};
template<class T>
struct remove_smart_ptr<std::shared_ptr<T>>
{
  typedef T type;
};
template<class T>
struct remove_smart_ptr<std::unique_ptr<T>>
{
  typedef T type;
};
template<class T>
struct remove_smart_ptr<std::weak_ptr<T>>
{
  typedef T type;
};
template<class T>
using remove_smart_ptr_t = typename remove_smart_ptr<T>::type;

template<class T>
struct remove_any_ptr
{
  typedef T type;
};
template<class T>
struct remove_any_ptr<T*>
{
  typedef T type;
};
template<class T>
struct remove_any_ptr<std::shared_ptr<T>>
{
  typedef T type;
};
template<class T>
struct remove_any_ptr<std::unique_ptr<T>>
{
  typedef T type;
};
template<class T>
struct remove_any_ptr<std::weak_ptr<T>>
{
  typedef T type;
};
template<class T>
using remove_any_ptr_t = typename remove_any_ptr<T>::type;

template<class T, std::size_t = 0>
constexpr std::shared_ptr<T> make_shared_ptr()
{
  return std::move(std::make_shared<T>());
}

template<class T, size_t... Indices>
constexpr std::array<std::shared_ptr<T>, sizeof...(Indices)>
    make_shared_array_sequence(std::index_sequence<Indices...>)
{
  return std::move<std::array<std::shared_ptr<T>, sizeof...(Indices)>>(
      {make_shared_ptr<T, Indices>()...});
}

template<class T, size_t Size>
constexpr std::array<std::shared_ptr<T>, Size> make_shared_array()
{
  return std::move(
      make_shared_array_sequence<T>(std::make_index_sequence<Size>{}));
}
template<class T,
         size_t Size,
         typename std::enable_if_t<!is_any_ptr_v<T>>* = nullptr>
constexpr std::array<T, Size> make_array()
{
  return std::move(std::array<T, Size>{});
}
template<class T,
         size_t Size,
         typename std::enable_if_t<is_shared_ptr_v<T>>* = nullptr>
constexpr std::array<T, Size> make_array()
{
  return std::move(make_shared_array<remove_any_ptr_t<T>, Size>());
}
// template<size_t... Ss>
// constexpr std::array<std::size_t, sizeof...(Ss)> make_index_array()
// {
//     return
// }
template<typename F, size_t... Is>
auto indices_impl(F f, std::index_sequence<Is...>)
{
  return f(std::integral_constant<size_t, Is>()...);
}
template<size_t N, class F>
auto indices(F f)
{
  return indices_impl(f, std::make_index_sequence<N>());
}
template<class T>
constexpr T product(const T& t1, const T& t2)
{
  return t1 * t2;
}
template<class T, class... Ts>
constexpr T product(const T& t1, const T& t2, const Ts&... ts)
{
  return product(t1 * t2, ts...);
}

template<template<typename...> class base, typename derived>
struct is_base_of_template
{
  template<typename... Ts>
  static constexpr std::true_type  test(const base<Ts...>*);
  static constexpr std::false_type test(...);
  using type = decltype(test(std::declval<derived*>()));
};
template<template<typename...> class base, typename derived>
using is_base_of_template_t = typename is_base_of_template<base, derived>::type;
template<template<typename...> class base, typename derived>
inline constexpr bool is_base_of_template_v
    = is_base_of_template<base, derived>::value;

}  // namespace SFINAE
}  // namespace UD