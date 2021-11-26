#pragma once
#include <array>
#include <deque>
#include <functional>
#include <memory>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "Interface.h"
namespace UD::Traits
{
template<class T>
struct FunctionType;
template<class _Ret, class... _Params>
struct FunctionType<_Ret(_Params...)>
{
  using return_type = _Ret;
  using parameters  = std::tuple<_Params...>;
};
}  // namespace UD::Traits
namespace UD::Concept
{
template<class T>
concept Any = true;
template<class T, class... Params>
concept Invocable = requires(T t, Params&&... params)
{
  {
    t(std::forward<Params>(params)...)
    } -> Any;
};
template<class T, class Ret, class... Params>
concept CallableExplicit = requires(T t, Params&&... params)
{
  {
    t(std::forward<Params>(params)...)
    } -> std::convertible_to<Ret>;
};
namespace detail
{
  template<class... Ts>
  struct CallableHelper : public std::false_type
  {
  };
  template<class T, class Ret, class... Parameters>
    requires CallableExplicit<T, Ret, Parameters...>
  struct CallableHelper<T, Ret(Parameters...)> : public std::true_type
  {
  };
}  // namespace detail
template<class T, class F>
concept Callable = detail::CallableHelper<T, F>::value;
}  // namespace UD::Concept
namespace UD::Concept::Event
{
}  // namespace UD::Concept::Event
namespace UD::Event
{
namespace Concept = UD::Concept::Event;
class State
{
  bool                  _skip = false;
  std::shared_ptr<void> _data = nullptr;

 public:
  template<class T>
  void Set(std::shared_ptr<T> ptr)
  {
    _data = ptr;
  }
  template<class T, class... Ts>
    requires requires(Ts... ts)
    {
      {
        T
        {
          ts...
        }
        } -> std::same_as<T>;
    }
  void Emplace(Ts... ts)
  {
    _data = std::make_shared<T>(ts...);
  }
  template<class T>
  auto Get() -> std::shared_ptr<T>
  {
    return std::static_pointer_cast<T>(_data);
  }
  void Skip()
  {
    _skip = true;
  }
  bool IsSkipping()
  {
    return _skip;
  }
  void Clear()
  {
    _skip = false;
    _data = nullptr;
  }
};
template<class... _Parameters>
class Event;

template<class... _Parameters>
class Handler;

namespace detail
{
template<class... _Parameters>
struct EventConnection
{
  std::function<void(_Parameters&&...)> function = [](_Parameters&&...) {};
  std::shared_ptr<bool>                 valid    = std::make_shared<bool>(true);
};
struct Invalidator
{
  std::shared_ptr<bool> _valid = nullptr;
  Invalidator(std::shared_ptr<bool> valid) : _valid{valid} {}
  ~Invalidator()
  {
    *_valid = false;
  }
};

}  // namespace detail
template<class... _Parameters>
class Event
    : public UD::Interface::Interface<Event<_Parameters...>,
                                      UD::Interface::SimpleModifiers>
{
  std::vector<detail::EventConnection<_Parameters...>> _callers = {};
  State                                                _state   = {};

  void Fire(_Parameters... parameters)
  {
    static std::vector<std::size_t> remove_indices;
    for (std::size_t i = 0; i < _callers.size() && !_state.IsSkipping(); ++i)
    {
      auto& caller = _callers[i];
      if (caller.valid)
      {
        caller.function(std::move(parameters)...);
      }
      else
      {
        remove_indices.push_back(i);
      }
    }
    if (!remove_indices.empty())
    {
      for (auto it = remove_indices.rbegin(); it != remove_indices.rend(); ++it)
      {
        _callers.erase(_callers.begin() + *it);
      }
      remove_indices.clear();
    }
  }

 protected:
  virtual void PreFire(State& state, _Parameters... parameters) {}
  virtual void PostFire(State& state, _Parameters... parameters) {}

 public:
  template<class F>
    requires(UD::Concept::Invocable<F, _Parameters...>)
  detail::Invalidator Add(F function)
  {
    detail::EventConnection<_Parameters...> connection(std::bind_front(
        [](auto function, auto&&... ts)
        {
          function(std::forward<decltype(ts)>(ts)...);
        },
        function));
    detail::Invalidator ret{connection.valid};
    _callers.push_back(std::move(connection));
    return ret;
  }
  template<class F>
    requires(UD::Concept::Invocable<F, State&, _Parameters...>)
  detail::Invalidator Add(F function)
  {
    return Add(std::bind_front(std::move(function), std::ref(_state)));
  }

  ~Event() override       = default;
  Event(const Event&)     = default;
  Event(Event&&) noexcept = default;
  auto operator=(const Event&) -> Event& = default;
  auto operator=(Event&&) noexcept -> Event& = default;

  Event() noexcept = default;

  void operator()(_Parameters... parameters)
  {
    _state.Clear();
    PreFire(_state, parameters...);
    Fire(parameters...);
    PostFire(_state, std::move(parameters)...);
  }
};
template<class... _Parameters>
class HandlerBase
    : public UD::Interface::Interface<HandlerBase<_Parameters...>,
                                      UD::Interface::SimpleModifiers>
{
  using FunctionType = std::function<void(_Parameters...)>;

  FunctionType                     _function     = {};
  std::vector<detail::Invalidator> _invalidators = {};
  bool                             _enabled      = true;
};
template<class... _Parameters>
class Handler
    : public UD::Interface::Interface<Handler<_Parameters...>,
                                      UD::Interface::SimpleModifiers>
{
  using FunctionType = std::function<void(_Parameters...)>;

  FunctionType                     _function     = {};
  std::vector<detail::Invalidator> _invalidators = {};
  bool                             _enabled      = true;

  template<class... Ts>
  void Run(Ts&&... parameters)
  {
    if (_enabled)
    {
      _function(std::forward<Ts>(parameters)...);
    }
  }
  template<class... Ts>
  struct HasState : public std::false_type
  {
  };
  template<std::same_as<State&> S, class... Ts>
  struct HasState<S, Ts...> : public std::true_type
  {
  };
  template<class F, class... Ts>
    requires(!HasState<_Parameters...>::value)
  void RunAdapted(F func, Ts&&... parameters)
  {
    std::apply(&Handler::Run<_Parameters...>,
               std::tuple_cat(std::tuple(this),
                              func(std::forward<Ts>(parameters)...)));
  }
  template<class F, class... Ts>
    requires(HasState<_Parameters...>::value)
  void RunAdapted(F func, State& state, Ts&&... parameters)
  {
    std::apply(&Handler::Run<_Parameters...>,
               std::tuple_cat(std::tuple<Handler*, State&>(this, state),
                              func(std::forward<Ts>(parameters)...)));
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
  Handler(void (T::*function)(_Parameters...), T* type)
      : _function{std::bind_front(function, type)}
  {
  }
  template<std::size_t _Size, class _EventType>
  void operator()(std::array<_EventType&, _Size> events)
  {
    for (auto& e : events)
    {
      operator()(e);
    }
  }
  template<class... Ts>
  void operator()(Event<Ts...>& e)
  {
    _invalidators.push_back(
        e.Add(std::bind_front(&Handler::Run<_Parameters...>, this)));
  }
  template<class F, class... Ts>
  void operator()(Event<Ts...>& e, F adaptor_function)
  {
    _invalidators.push_back(
        e.Add(std::bind_front(&Handler::RunAdapted<F, Ts...>,
                              this,
                              adaptor_function)));
  }
  void operator()()
  {
    _invalidators.clear();
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
  void Reset(void (T::*function)(_Parameters...), T* type)
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
};  // namespace UD::Event

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
  void ProcessTuple(ValueTuple& values)
  {
    static auto bound_func
        = std::bind_front(&Event<_Parameters...>::operator(), this);
    std::apply(bound_func, values);
  }
  auto TestTuple(ValueTuple& values) -> bool
  {
    static auto bound_func = std::bind_front(&Queue::TestConditions, this);
    return std::apply(bound_func, values);
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
  Queue() noexcept
      : Queue::Super{
          Queue::Constructor<Handler<_Parameters...>>::Call(&Queue::Push, this)}
  {
  }

  using Handler<_Parameters...>::operator();

  void ProcessNextSkipConditions()
  {
    ProcessSkipConditions(1);
  }
  void ProcessNext()
  {
    Process(1);
  }
  void ProcessSkipConditions(std::size_t max = 0)
  {
    while (!_queued_event_values.empty())
    {
      ProcessTuple(_queued_event_values.front());
      _queued_event_values.pop_front();
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
      if (TestTuple(_queued_event_values.front()))
      {
        ProcessTuple(_queued_event_values.front());
      }
      _queued_event_values.pop_front();
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
template<class... _Parameters>
struct Chain
    : public Interface::Interface<
          Chain<_Parameters...>,
          UD::Pack::Pack<Event<_Parameters...>, Handler<_Parameters...>>,
          UD::Interface::SimpleModifiers>
{
  ~Chain() override       = default;
  Chain(const Chain&)     = default;
  Chain(Chain&&) noexcept = default;
  auto operator=(const Chain&) -> Chain& = default;
  auto operator=(Chain&&) noexcept -> Chain& = default;

  Chain() noexcept = default;

  // TODO: Should not be hiding base members... Requires rethinking inheritance

 private:
  using Event<_Parameters...>::operator();
};
}  // namespace UD::Event