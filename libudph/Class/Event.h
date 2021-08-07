#pragma once
#include <array>
#include <functional>
#include <memory>
#include <ranges>
#include <utility>
#include <vector>

#include "Interface.h"

namespace UD::Event
{
template<class... _Parameters>
class Event;

template<class... _Parameters>
class Handler;

template<class... _Parameters>
class Event
    : public UD::Interface::Interface<Event<_Parameters...>,
                                      UD::Interface::SimpleModifiers>
{
  using FunctionType = std::function<void(_Parameters...)>;

 public:
  using Handler = Handler<_Parameters...>;

 private:
  friend Handler;

  std::vector<std::weak_ptr<FunctionType>> _callers = {};

  void Add(std::weak_ptr<FunctionType> function)
  {
    _callers.push_back(function);
  }

 public:
  ~Event() override       = default;
  Event(const Event&)     = default;
  Event(Event&&) noexcept = default;
  auto operator=(const Event&) -> Event& = default;
  auto operator=(Event&&) noexcept -> Event& = default;

  Event() noexcept = default;

  void operator()(_Parameters... parameters)
  {
    for (auto& caller : _callers)
    {
      if (auto locked = caller.lock())
      {
        (*locked)(parameters...);
      }
    }
  }
};
template<class... _Parameters>
class Handler
    : public UD::Interface::Interface<Handler<_Parameters...>,
                                      UD::Interface::SimpleModifiers>
{
  using FunctionType = std::function<void(_Parameters...)>;
  template<class T>
  using MemberFunctionType = std::function<void (T::*)(_Parameters...)>;

  FunctionType                               _function = {};
  std::vector<std::shared_ptr<FunctionType>> _callers  = {};
  bool                                       _enabled  = true;

  void Run(_Parameters... parameters)
  {
    if (_enabled)
    {
      _function(parameters...);
    }
  }

 public:
  ~Handler() override         = default;
  Handler(const Handler&)     = default;
  Handler(Handler&&) noexcept = default;
  auto operator=(const Handler&) -> Handler& = default;
  auto operator=(Handler&&) noexcept -> Handler& = default;

  Handler() noexcept = default;
  Handler(FunctionType function) : _function{std::move(function)} {}
  template<class T>
  Handler(FunctionType function, T* type)
      : _function{std::bind_front(function, type)}
  {
  }

  void operator()(Event<_Parameters...>& e)
  {
    auto caller
        = std::make_shared<FunctionType>(std::bind_front(&Handler::Run, this));
    e.Add(caller);
    _callers.push_back(std::move(caller));
  }
  void operator()()
  {
    _callers.clear();
  }
  void Reset()
  {
    _function = {};
  }
  void Reset(FunctionType function)
  {
    _function = std::move(function);
  }
  template<class T>
  void Reset(MemberFunctionType<T> function, T* type)
  {
    _function = std::bind_front(function, type);
  }
  void Enable(bool enable = true)
  {
    _enabled = enable;
  }
  void Disable()
  {
    Enable(false);
  }
};
}  // namespace UD::Event