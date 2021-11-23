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
concept Invocable = requires(T t, Params... params)
{
  {
    t(params...)
    } -> Any;
};
template<class T, class Ret, class... Params>
concept CallableExplicit = requires(T t, Params... params)
{
  {
    t(params...)
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
  std::vector<std::weak_ptr<FunctionType>> _callers = {};

 public:
  void Add(std::weak_ptr<FunctionType> function)
  {
    _callers.push_back(function);
  }

  ~Event() override       = default;
  Event(const Event&)     = default;
  Event(Event&&) noexcept = default;
  auto operator=(const Event&) -> Event& = default;
  auto operator=(Event&&) noexcept -> Event& = default;

  Event() noexcept = default;

  void operator()(_Parameters... parameters)
  {
    // TODO: Define Event struct that contains parameters for
    // Handler<Event<Parameters...>>
    //       Add another vector with Handler<Event<Parameters...>>
    //       Create struct in this function and pass to all handlers from both
    //       vectors Will require order-keeping between two vectors (maybe a 3rd
    //       with alternating counters?)

    static std::deque<std::vector<std::weak_ptr<FunctionType>>::iterator> its;
    for (auto it = _callers.begin(); it != _callers.end(); ++it)
    {
      if (auto locked = it->lock())
      {
        (*locked)(parameters...);
      }
      else
      {
        its.push_back(it);
      }
    }
    if (!its.empty())
    {
      for (auto it = its.rbegin(); it != its.rend(); ++it)
      {
        _callers.erase(*it);
      }
      its.clear();
    }
  }
};

struct FunctionHolder
{
  virtual ~FunctionHolder() = default;
};
template<class... Ts>
struct FunctionHolderSpecific : public FunctionHolder
{
  std::shared_ptr<std::function<void(Ts...)>> function;

  FunctionHolderSpecific(std::shared_ptr<std::function<void(Ts...)>> func)
      : function{func}
  {
  }
  ~FunctionHolderSpecific() override = default;
};
template<class... _Parameters>
class Handler
    : public UD::Interface::Interface<Handler<_Parameters...>,
                                      UD::Interface::SimpleModifiers>
{
  using FunctionType = std::function<void(_Parameters...)>;
  template<class T>
  using MemberFunctionType = std::function<void (T::*)(_Parameters...)>;

  FunctionType                                 _function = {};
  std::vector<std::unique_ptr<FunctionHolder>> _callers  = {};
  bool                                         _enabled  = true;

  void Run(_Parameters... parameters)
  {
    if (_enabled)
    {
      _function(parameters...);
    }
  }
  template<class F, class... Ts>
    requires UD::Concept::Callable < F,
        std::tuple<_Parameters...>(std::tuple<Ts...>)
  > void RunAdaptedTuple(F func, std::tuple<Ts...> parameters)
  {
    std::apply(&Handler::Run,
               std::tuple_cat(std::tuple(this), func(parameters)));
  }
  template<class F, class... Ts>
    requires UD::Concept::Callable < F, std::tuple<_Parameters...>(Ts...)
  > void RunAdapted(F func, Ts... parameters)
  {
    std::apply(&Handler::Run,
               std::tuple_cat(std::tuple(this), func(parameters...)));
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
  template<std::size_t _Size>
  void operator()(std::array<Event<_Parameters...>&, _Size> events)
  {
    for (auto& e : events)
    {
      operator()(e);
    }
  }
  template<class... Ts>
  void operator()(Event<_Parameters...>& e)
  {
    auto caller = std::make_unique<FunctionHolderSpecific<_Parameters...>>(
        std::make_shared<FunctionType>(std::bind_front(&Handler::Run, this)));
    e.Add(caller->function);
    _callers.push_back(std::move(caller));
  }
  template<class F, class... Ts>
    requires UD::Concept::Callable < F, std::tuple<_Parameters...>(Ts...)
  > &&(
      !UD::Concept::Callable<F,
                             std::tuple<_Parameters...>(std::tuple<Ts...>)>)void
      operator()(Event<Ts...>& e, F adaptor_function)
  {
    auto caller = std::make_unique<FunctionHolderSpecific<Ts...>>(
        std::make_shared<std::function<void(Ts...)>>(
            std::bind_front(&Handler::RunAdapted<F, Ts...>,
                            this,
                            adaptor_function)));
    e.Add(caller->function);
    _callers.push_back(std::move(caller));
  }
  template<class F, class... Ts>
    requires UD::Concept::Callable < F,
        std::tuple<_Parameters...>(std::tuple<Ts...>)
  > void operator()(Event<Ts...>& e, F adaptor_function)
  {
    auto caller = std::make_unique<FunctionHolderSpecific<Ts...>>(
        std::make_shared<std::function<void(Ts...)>>(std::bind_front(
            [](Handler* h, F f, Ts... ts)
            {
              static std::function<void(std::tuple<Ts...>)> func
                  = std::bind_front(&Handler::RunAdaptedTuple<F, Ts...>, h, f);
              func(std::tuple<Ts...>(ts...));
            },
            this,
            adaptor_function)));
    e.Add(caller->function);
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
};
// TODO: Same as Handler<_Parameters...> but handles params and an event object
// with
//       event functions and data (like Quit (or skip), Count, custom data,
//       etc...) instead of only handling params.
// template<class... _Parameters>
// struct Handler<Event<_Parameters...>>
//    : public UD::Interface::Interface<Handler<_Parameters...>,
//                                      UD::Pack::Pack<Handler<_Parameters...>>,
//                                      UD::Interface::SimpleModifiers>
//{
//  using Handler::Super::Super;
//};

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