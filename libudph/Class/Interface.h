#pragma once
#include <concepts>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace UD::Pack
{
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
template<template<class...> class... _Ts>
struct TemplatePack;
template<>
struct TemplatePack<>
{
  static constexpr bool Empty = true;
};
template<template<class...> class _T, template<class...> class... _Ts>
struct TemplatePack<_T, _Ts...>
{
  static constexpr bool Empty = false;
  using Next                  = TemplatePack<_Ts...>;
};

template<class _Pack, template<class...> class... _Templates>
struct DistributePack;
template<class... _Ts, template<class...> class... _Templates>
struct DistributePack<Pack<_Ts...>, _Templates...>
{
  using Pack = UD::Pack::Pack<_Templates<_Ts...>...>;
};

template<class _Pack>
struct InheritPack;
template<class... _Ts>
  requires(!Pack<_Ts...>::Empty)
struct InheritPack<Pack<_Ts...>> : public _Ts...
{
  ~InheritPack() override             = default;
  InheritPack(const InheritPack&)     = default;
  InheritPack(InheritPack&&) noexcept = default;
  auto operator=(const InheritPack&) -> InheritPack& = default;
  auto operator=(InheritPack&&) noexcept -> InheritPack& = default;

  InheritPack() = default;

  using _Ts::_Ts...;
};
template<>
struct InheritPack<Pack<>>
{
  virtual ~InheritPack()              = default;
  InheritPack(const InheritPack&)     = default;
  InheritPack(InheritPack&&) noexcept = default;
  auto operator=(const InheritPack&) -> InheritPack& = default;
  auto operator=(InheritPack&&) noexcept -> InheritPack& = default;

  InheritPack() = default;
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
namespace UD::Tag
{
template<class _T, class _Enum, _Enum _ID>
struct Tag
{
  virtual ~Tag()      = default;
  Tag(const Tag&)     = default;
  Tag(Tag&&) noexcept = default;
  auto operator=(const Tag&) -> Tag& = default;
  auto operator=(Tag&&) noexcept -> Tag& = default;

  Tag() = default;
};
}  // namespace UD::Tag
namespace UD::Tag::Interface::Modifier
{
enum class TagID
{
  Cloneable,
  Abstract
};
template<class _T, TagID _ID>
using Tag = UD::Tag::Tag<_T, TagID, _ID>;

template<class _T>
using Cloneable = Tag<_T, TagID::Cloneable>;
template<class _T>
using Abstract = Tag<_T, TagID::Abstract>;
}  // namespace UD::Tag::Interface::Modifier
namespace UD::Interface::Traits
{
template<class T>
struct Basic
{
  virtual ~Basic()        = default;
  Basic(const Basic&)     = default;
  Basic(Basic&&) noexcept = default;
  auto operator=(const Basic&) -> Basic& = default;
  auto operator=(Basic&&) noexcept -> Basic& = default;
  using Self                                 = T;
  template<class T2>
  static constexpr bool Is = std::is_same_v<T, T2>;
  template<class T2>
  static constexpr bool IsDerived = std::is_base_of_v<T2, T>;
  template<class T2>
  static constexpr bool IsBase = std::is_base_of_v<T, T2>;
  template<template<class> class _Tag>
  static constexpr bool IsTagged
      = Basic<InheritDescriptor<T, T>>::template IsDerived<_Tag<T>>;
};

struct conststr
{
  const char* p;
  std::size_t sz;

  template<std::size_t N>
  constexpr conststr(const char (&a)[N]) : p(a)
                                         , sz(N - 1)
  {
  }

  constexpr char operator[](std::size_t n) const
  {
    return n < sz ? p[n] : throw std::out_of_range("");
  }
  constexpr std::size_t size() const
  {
    return sz;
  }
};
template<auto _Pointer, conststr _Name>
struct Member;
template<class _ClassType,
         class _MemberType,
         _MemberType _ClassType::*_Pointer,
         conststr                 _Name>
struct Member<_Pointer, _Name>
{
  using Class = _ClassType;
  using Type  = _MemberType _ClassType::*;

  static constexpr _MemberType _ClassType::*Pointer = _Pointer;
  static constexpr char const*              Name    = _Name.p;
};

template<class _Self, class _Leaf>
struct InheritDescriptor
{
  virtual ~InheritDescriptor()                    = default;
  InheritDescriptor(const InheritDescriptor&)     = default;
  InheritDescriptor(InheritDescriptor&&) noexcept = default;
  auto operator=(const InheritDescriptor&) -> InheritDescriptor& = default;
  auto operator=(InheritDescriptor&&) noexcept -> InheritDescriptor& = default;
};
template<class _Self>
struct Descriptor
{
  virtual ~Descriptor()             = default;
  Descriptor(const Descriptor&)     = default;
  Descriptor(Descriptor&&) noexcept = default;
  auto operator=(const Descriptor&) -> Descriptor& = default;
  auto operator=(Descriptor&&) noexcept -> Descriptor& = default;
};
template<class _Leaf, class... _Bases>
struct Inherit
    : public Pack::InheritPack<Pack::Pack<InheritDescriptor<_Bases, _Leaf>...>>
{
  ~Inherit() override         = default;
  Inherit(const Inherit&)     = default;
  Inherit(Inherit&&) noexcept = default;
  auto operator=(const Inherit&) -> Inherit& = default;
  auto operator=(Inherit&&) noexcept -> Inherit& = default;

  using DirectBases = Pack::Pack<_Bases...>;
};
template<class _Self, template<class> class... _Tags>
struct Tag : public Pack::InheritPack<Pack::Pack<_Tags<_Self>...>>
{
  ~Tag() override     = default;
  Tag(const Tag&)     = default;
  Tag(Tag&&) noexcept = default;
  auto operator=(const Tag&) -> Tag& = default;
  auto operator=(Tag&&) noexcept -> Tag& = default;

  using Tags = Pack::TemplatePack<_Tags...>;
};
// TODO: Here... Traits should be functional.  To use, create Descriptor that
// inherits from Inherit
//       DirectBases might work... AllBases (Bases?) to follow
template<class _Self, class _Leaf>
struct SelectDirectBases
{
  using Type = Pack::Pack<>;
};
template<class _Self, class _Leaf>
  requires requires
  {
    typename InheritDescriptor<_Self, _Leaf>::DirectBases;
  }
struct SelectDirectBases<_Self, _Leaf>
{
  using Type = typename InheritDescriptor<_Self, _Leaf>::DirectBases;
};
template<class _Self, class _Leaf>
struct SelectTags
{
  using Type = Pack::TemplatePack<>;
};
template<class _Self, class _Leaf>
  requires requires
  {
    typename InheritDescriptor<_Self, _Leaf>::Tags;
  }
struct SelectTags<_Self, _Leaf>
{
  using Type = typename InheritDescriptor<_Self, _Leaf>::Tags;
};
template<class... _Ts>
struct Traits;
template<class _Self>
struct Traits<_Self> : public Traits<_Self, _Self>
{
  ~Traits() override        = default;
  Traits(const Traits&)     = default;
  Traits(Traits&&) noexcept = default;
  auto operator=(const Traits&) -> Traits& = default;
  auto operator=(Traits&&) noexcept -> Traits& = default;
};
template<class _Self, class _Leaf>
struct Traits<_Self, _Leaf>
    : public Basic<_Leaf>
    , public InheritDescriptor<_Self, _Leaf>
    , public Descriptor<_Self>
{
  ~Traits() override        = default;
  Traits(const Traits&)     = default;
  Traits(Traits&&) noexcept = default;
  auto operator=(const Traits&) -> Traits& = default;
  auto operator=(Traits&&) noexcept -> Traits& = default;

  using DirectBases = typename SelectDirectBases<_Self, _Leaf>::Type;
  using Tags        = typename SelectTags<_Self, _Leaf>::Type;
};
}  // namespace UD::Interface::Traits
namespace UD::Concept
{
template<class _T, template<class> class _Tag, template<class> class... _Tags>
concept Tagged
    = (UD::Interface::Traits::Basic<_T>::template IsTagged<_Tag> && ...
       && UD::Interface::Traits::Basic<_T>::template IsTagged<_Tags>);
}
namespace UD::Concept::Interface::Modifier
{
template<class _T>
concept Cloneable
    = std::derived_from<_T, Tag::Interface::Modifier::Cloneable<_T>>;
template<class... _Ts>
concept SomeCloneable = (Cloneable<_Ts> || ...);
template<class _T>
concept Abstract
    = std::derived_from<_T, Tag::Interface::Modifier::Abstract<_T>>;
}  // namespace UD::Concept::Interface::Modifier
namespace UD::Interface::Modifier
{
namespace Tag     = UD::Tag::Interface::Modifier;
namespace Concept = UD::Concept::Interface::Modifier;

template<class _Next>
struct _NextModifier;
template<class _Next>
  requires(_Next::Empty)
struct _NextModifier<_Next>
{
  using Type = Pack::InheritPack<Pack::Skip<typename _Next::Parameters, 1>>;
};
template<class _Next>
  requires(!_Next::Empty)
struct _NextModifier<_Next>
{
  using Type = typename _Next::Type;
};
template<class _Next>
using NextModifier = typename _NextModifier<_Next>::Type;
template<class _Next, class _Derived, class... _Bases>
struct Cloneable
    : public Tag::Cloneable<_Derived>
    , public NextModifier<_Next>
{
  ~Cloneable() override           = default;
  Cloneable(const Cloneable&)     = default;
  Cloneable(Cloneable&&) noexcept = default;
  auto operator=(const Cloneable&) -> Cloneable& = default;
  auto operator=(Cloneable&&) noexcept -> Cloneable& = default;

  Cloneable() = default;

  [[nodiscard]] auto Clone() const  //
      -> std::unique_ptr<_Derived>
  {
    return std::unique_ptr<_Derived>(static_cast<_Derived*>(this->CloneRaw()));
  }

 private:
  [[nodiscard]] virtual auto CloneRaw() const  //
      -> Cloneable*
  {
    return new _Derived{static_cast<const _Derived&>(*this)};
  }
};

template<class _Next, class _Derived, class... _Bases>
  requires Concept::SomeCloneable<_Bases...>
struct Cloneable<_Next, _Derived, _Bases...>
    : public Tag::Cloneable<_Derived>
    , public NextModifier<_Next>
{
  ~Cloneable() override           = default;
  Cloneable(const Cloneable&)     = default;
  Cloneable(Cloneable&&) noexcept = default;
  auto operator=(const Cloneable&) -> Cloneable& = default;
  auto operator=(Cloneable&&) noexcept -> Cloneable& = default;

  Cloneable() = default;

  [[nodiscard]] auto Clone() const  //
      -> std::unique_ptr<_Derived>
  {
    return std::unique_ptr<_Derived>(static_cast<_Derived*>(this->CloneRaw()));
  }

 private:
  [[nodiscard]] auto CloneRaw() const  //
      -> Cloneable* override
  {
    return new _Derived{static_cast<const _Derived&>(*this)};
  }
};

template<class _Next, class _Derived, class... _Bases>
  requires UD::Concept::Tagged<_Derived, Tag::Abstract>
struct Cloneable<_Next, _Derived, _Bases...>
    : public Tag::Cloneable<_Derived>
    , public NextModifier<_Next>
{
  ~Cloneable() override           = default;
  Cloneable(const Cloneable&)     = default;
  Cloneable(Cloneable&&) noexcept = default;
  auto operator=(const Cloneable&) -> Cloneable& = default;
  auto operator=(Cloneable&&) noexcept -> Cloneable& = default;

  Cloneable() = default;

  [[nodiscard]] auto Clone() const  //
      -> std::unique_ptr<_Derived>
  {
    return std::unique_ptr<_Derived>(static_cast<_Derived*>(this->CloneRaw()));
  }

 private:
  [[nodiscard]] virtual auto CloneRaw() const  //
      -> Cloneable* = 0;
};

template<class _Next, class _Derived, class... _Bases>
  requires UD::Concept::Tagged<_Derived, Tag::Abstract> && Concept::
      SomeCloneable<_Bases...>
struct Cloneable<_Next, _Derived, _Bases...>
    : public Tag::Cloneable<_Derived>
    , public NextModifier<_Next>
{
  ~Cloneable() override           = default;
  Cloneable(const Cloneable&)     = default;
  Cloneable(Cloneable&&) noexcept = default;
  auto operator=(const Cloneable&) -> Cloneable& = default;
  auto operator=(Cloneable&&) noexcept -> Cloneable& = default;

  Cloneable() = default;

  [[nodiscard]] auto Clone() const  //
      -> std::unique_ptr<_Derived>
  {
    return std::unique_ptr<_Derived>(static_cast<_Derived*>(this->CloneRaw()));
  }

 private:
  [[nodiscard]] auto CloneRaw() const  //
      -> Cloneable* override = 0;
};

template<class _Next, class _Derived, class... _Bases>
struct Traited : public NextModifier<_Next>
{
  ~Traited() override         = default;
  Traited(const Traited&)     = default;
  Traited(Traited&&) noexcept = default;
  auto operator=(const Traited&) -> Traited& = default;
  auto operator=(Traited&&) noexcept -> Traited& = default;

  Traited() noexcept = default;

  using Traits = Traits::Traits<_Derived>;
};

}  // namespace UD::Interface::Modifier
namespace UD::Interface
{
using DefaultModifiers
    = UD::Pack::TemplatePack<Modifier::Cloneable, Modifier::Traited>;
using SimpleModifiers = UD::Pack::TemplatePack<Modifier::Traited>;
template<class... _Ts>
struct Interface;
template<class _Derived,
         class... _Bases,
         template<class, class, class...>
         class... _Modifiers>
struct Interface<_Derived,
                 Pack::Pack<_Bases...>,
                 Pack::TemplatePack<_Modifiers...>>
    : public Pack::ChainPack<Pack::Pack<_Derived, _Bases...>,
                             Pack::TemplatePack<_Modifiers...>>::Type
{
  ~Interface() override           = default;
  Interface(const Interface&)     = default;
  Interface(Interface&&) noexcept = default;
  auto operator=(const Interface&) -> Interface& = default;
  auto operator=(Interface&&) noexcept -> Interface& = default;

  Interface() = default;
};
template<class _Derived, class... _Bases>
struct Interface<_Derived, Pack::Pack<_Bases...>>
    : public Interface<_Derived, Pack::Pack<_Bases...>, DefaultModifiers>
{
  ~Interface() override           = default;
  Interface(const Interface&)     = default;
  Interface(Interface&&) noexcept = default;
  auto operator=(const Interface&) -> Interface& = default;
  auto operator=(Interface&&) noexcept -> Interface& = default;

  Interface() = default;
};
template<class _Derived>
struct Interface<_Derived>
    : public Interface<
          _Derived,
          typename Traits::SelectDirectBases<_Derived, _Derived>::Type,
          DefaultModifiers>
{
  ~Interface() override           = default;
  Interface(const Interface&)     = default;
  Interface(Interface&&) noexcept = default;
  auto operator=(const Interface&) -> Interface& = default;
  auto operator=(Interface&&) noexcept -> Interface& = default;

  Interface() = default;
};
template<class _Derived, template<class, class, class...> class... _Modifiers>
struct Interface<_Derived, Pack::TemplatePack<_Modifiers...>>
    : public Interface<
          _Derived,
          typename Traits::SelectDirectBases<_Derived, _Derived>::Type,
          Pack::TemplatePack<_Modifiers...>>
{
  ~Interface() override           = default;
  Interface(const Interface&)     = default;
  Interface(Interface&&) noexcept = default;
  auto operator=(const Interface&) -> Interface& = default;
  auto operator=(Interface&&) noexcept -> Interface& = default;

  Interface() = default;
};
}  // namespace UD::Interface
