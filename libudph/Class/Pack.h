#pragma once
#include <cstddef>

namespace UD::Pack
{
template<class T>
concept HasType = requires(T)
{
  typename T::type;
};
template<class... _Ts>
struct Pack;
template<>
struct Pack<>
{
  static constexpr bool Empty = true;
};
template<class _T, class... _Ts>
struct Pack<_T, _Ts...>
{
  static constexpr bool Empty = false;
  using Type                  = _T;
  using Next                  = Pack<_Ts...>;
};
template<class... _Ts>
struct Pack<Pack<_Ts...>> : public Pack<_Ts...>
{
};
template<class... _Ts, class... _Us>
struct Pack<Pack<_Ts...>, _Us...> : public Pack<_Ts..., _Us...>
{
};
template<class... _Ts, class... _Us>
struct Pack<Pack<_Ts...>, Pack<_Us...>> : public Pack<_Ts..., _Us...>
{
};

template<class _Pack, std::size_t _Count>
struct _Skip;
template<class _T, class... _Ts, std::size_t _Count>
  requires(_Count > 0)
struct _Skip<Pack<_T, _Ts...>, _Count>
{
  using Type = typename _Skip<Pack<_Ts...>, _Count - 1>::Type;
};
template<class... _Ts, std::size_t _Count>
  requires(_Count == 0)
struct _Skip<Pack<_Ts...>, _Count>
{
  using Type = Pack<_Ts...>;
};
template<class _Pack, std::size_t _Count>
using Skip = typename _Skip<_Pack, _Count>::Type;

template<class... _Ts>
struct Construct
{
  using Pack = UD::Pack::Pack<_Ts...>;
};
template<class... _Ts>
struct Construct<Pack<_Ts...>>
{
  using Pack = UD::Pack::Pack<_Ts...>;
};
template<class... _Ts, class... _Us>
struct Construct<Pack<_Ts...>, Pack<_Us...>>
{
  using Pack = UD::Pack::Pack<_Ts..., _Us...>;
};
namespace detail
{
template<class... _Ts>
struct TemplatePackJoin;
template<template<template<class...> class...> class _T,
         template<class...>
         class... _Ts,
         template<class...>
         class... _Us>
struct TemplatePackJoin<_T<_Ts...>, _T<_Us...>>
{
  using type = _T<_Ts..., _Us...>;
};
}  // namespace detail
template<template<class...> class... _Ts>
struct TemplatePack;
template<>
struct TemplatePack<>
{
  static constexpr bool Empty = true;
  template<class T>
  using Join = typename detail::TemplatePackJoin<TemplatePack<>, T>::type;
  template<template<class...> class... Us>
  using Add =typename  detail::TemplatePackJoin<TemplatePack<>, TemplatePack<Us...>>::type;
};
template<template<class...> class _T, template<class...> class... _Ts>
struct TemplatePack<_T, _Ts...>
{
  static constexpr bool Empty = false;
  using Next                  = TemplatePack<_Ts...>;
  template<class T>
  using Join = typename detail::TemplatePackJoin<TemplatePack<_Ts...>, T>::type;
  template<template<class...> class... Us>
  using Add =typename  detail::TemplatePackJoin<TemplatePack<_Ts...>, TemplatePack<Us...>>::type;
};

template<class _Pack, template<class...> class... _Templates>
struct DistributePack;
template<class... _Ts, template<class...> class... _Templates>
struct DistributePack<Pack<_Ts...>, _Templates...>
{
  using Pack = UD::Pack::Pack<_Templates<_Ts...>...>;
};

template<class _T>
struct InheritPackBase : public _T
{
  template<class T>
  friend struct InheritPack;

 private:
  struct Constructor
  {
    template<class... _Args>
    struct Call
    {
      using type = InheritPackBase<_T>;
      std::tuple<_Args...> args;
      Call(_Args... args) : args{args...} {}
    };
  };

 public:
  ~InheritPackBase() override                 = default;
  InheritPackBase(const InheritPackBase&)     = default;
  InheritPackBase(InheritPackBase&&) noexcept = default;
  auto operator=(const InheritPackBase&) -> InheritPackBase& = default;
  auto operator=(InheritPackBase&&) noexcept -> InheritPackBase& = default;
  InheritPackBase() noexcept                                     = default;
  using _T::_T;
  template<class... Args>
  InheritPackBase(typename Constructor::template Call<Args...>&& constructor)
      : InheritPackBase(std::forward<Constructor::Call<Args...>>(constructor),
                        std::make_index_sequence<
                            std::tuple_size<std::tuple<Args...>>::value>{})
  {
  }

 private:
  template<class... Args, std::size_t... Indices>
  InheritPackBase(typename Constructor::template Call<Args...>&& constructor,
                  std::index_sequence<Indices...>&&              indices)
      : _T{std::get<Indices>(constructor.args)...}
  {
  }
};
template<class _Pack>
struct InheritPack;
template<class _T, class... _Ts>
struct InheritPack<Pack<_T, _Ts...>>
    : public InheritPackBase<_T>
    , public InheritPackBase<_Ts>...
{
  ~InheritPack() override             = default;
  InheritPack(const InheritPack&)     = default;
  InheritPack(InheritPack&&) noexcept = default;
  auto operator=(const InheritPack&) -> InheritPack& = default;
  auto operator=(InheritPack&&) noexcept -> InheritPack& = default;

  template<class T>
  using Constructor = typename InheritPackBase<T>::Constructor;

  using InheritPackBase<_T>::InheritPackBase;
  using InheritPackBase<_Ts>::InheritPackBase...;
  InheritPack() noexcept = default;

  template<HasType... Ts>
  InheritPack(Ts&&... ts) : Ts::type{std::forward<Ts>(ts)}...
  {
  }
};
template<>
struct InheritPack<Pack<>>
{
  using Pack                          = Pack<>;
  virtual ~InheritPack()              = default;
  InheritPack(const InheritPack&)     = default;
  InheritPack(InheritPack&&) noexcept = default;
  auto operator=(const InheritPack&) -> InheritPack& = default;
  auto operator=(InheritPack&&) noexcept -> InheritPack& = default;

  InheritPack() noexcept = default;
};

template<class _Pack, class _Templates>
struct ChainPack;
template<class... _Ts>
struct ChainPack<Pack<_Ts...>, TemplatePack<>>
{
  static constexpr bool Empty = true;
  using Parameters            = Pack<_Ts...>;
};
template<class... _Ts,
         template<class, class...>
         class _Template,
         template<class, class...>
         class... _Templates>
struct ChainPack<Pack<_Ts...>, TemplatePack<_Template, _Templates...>>
{
  static constexpr bool Empty = false;
  using Next       = ChainPack<Pack<_Ts...>, TemplatePack<_Templates...>>;
  using Type       = _Template<Next, _Ts...>;
  using Parameters = Pack<_Ts...>;
};
}  // namespace UD::Pack