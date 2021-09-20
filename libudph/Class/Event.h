#pragma once
#include <array>
#include <deque>
#include <functional>
#include <memory>
#include <ranges>
#include <tuple>
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

template<class... _Parameters>
class Queue
    : public UD::Interface::Interface<
          Queue<_Parameters...>,
          UD::Pack::Pack<Event<_Parameters...>, Handler<_Parameters...>>,
          UD::Interface::SimpleModifiers>
{
  using Handler<_Parameters...>::Reset;

  using ValueTuple             = std::tuple<_Parameters...>;
  using EmptyConditionFunction = std::function<bool()>;
  using ValueConditionFunction = std::function<bool(_Parameters...)>;

  std::deque<ValueTuple>              _queued_event_values;
  std::vector<EmptyConditionFunction> _empty_conditions;
  std::vector<ValueConditionFunction> _value_conditions;

  void Push(_Parameters... parameters)
  {
    _queued_event_values.push_back(ValueTuple{parameters...});
  }
  void ProcessTuple(ValueTuple values)
  {
    std::apply(Event<_Parameters...>::operator(), values);
  }
  auto TestTuple(ValueTuple values) -> bool
  {
    return std::apply(TestConditions, values);
  }
  auto TestEmptyConditions() -> bool
  {
    for (auto& condition : _empty_conditions)
    {
      if (!condition())
      {
        return false;
      }
    }
    return true;
  }
  auto TestValueConditions(_Parameters... parameters) -> bool
  {
    for (auto& condition : _value_conditions)
    {
      if (!condition(parameters...))
      {
        return false;
      }
    }
    return true;
  }
  auto TestConditions(_Parameters... parameters) -> bool
  {
    return TestEmptyConditions() && TestValueConditions(parameters...);
  }

 public:
  ~Queue() override       = default;
  Queue(const Queue&)     = default;
  Queue(Queue&&) noexcept = default;
  auto operator=(const Queue&) -> Queue& = default;
  auto operator=(Queue&&) noexcept -> Queue& = default;
  Queue() : Handler<_Parameters...>(&Queue::Push, this) noexcept {}

  using Handler<_Parameters...>::operator();

  void ProcessNextSkipConditions()
  {
    ProcessSkipCondition(1);
  }
  void ProcessNext()
  {
    Process(1);
  }
  void ProcessSkipConditions(std::size_t max = 0)
  {
    while (!_queued_event_values.empty())
    {
      auto values = _queued_event_values.pop_front();
      ProcessTuple(values);
      if (max == 1)
      {
        break;
      }
      else if (max != 0)
      {
        max--;
      }
    }
  }
  void Process(std::size_t max = 0)
  {
    while (!_queued_event_values.empty())
    {
      auto values = _queued_event_values.pop_front();
      if (!TestTuple(values))
      {
        break;
      }
      ProcessTuple(values);
      if (max == 1)
      {
        break;
      }
      else if (max != 0)
      {
        max--;
      }
    }
  }
  void AddCondition(EmptyConditionFunction function)
  {
    _empty_conditions.push_back(function);
  }
  void AddCondition(ValueConditionFunction function)
  {
    _value_conditions.push_back(function);
  }
};
}  // namespace UD::Event