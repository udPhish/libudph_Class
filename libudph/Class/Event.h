#pragma once
#include <array>
#include <deque>
#include <functional>
#include <list>
#include <map>
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
// TODO: Add conditions to UD::Event and/or UD::Handler... Maybe?
//       Performance hit should be mitigated by checking if conditions are
//       empty. But is it necessary?
template<class... _Parameters>
class Event;

template<class... _Parameters>
class Handler;

namespace detail
{
template<class... _Parameters>
struct EventConnection
{
  std::function<void(_Parameters...)> function = [](_Parameters...) {};
  std::shared_ptr<bool>               valid    = std::make_shared<bool>(true);
};
struct Invalidator
{
  // TODO: Find better way to reference count... Using unnecessary bool* here.
  std::shared_ptr<bool> _count = std::make_shared<bool>(true);
  std::shared_ptr<bool> _valid = nullptr;

  Invalidator(std::shared_ptr<bool> valid) : _valid{valid} {}
  ~Invalidator()
  {
    if (_valid)
    {
      *_valid = _count && _count.use_count() > 1;
    }
  }
  Invalidator(const Invalidator& other) = default;
  Invalidator(Invalidator&&) noexcept   = default;
  auto operator=(const Invalidator&) -> Invalidator& = default;
  auto operator=(Invalidator&&) noexcept -> Invalidator& = default;
};

}  // namespace detail

struct PriorityBase
{
  virtual ~PriorityBase() = default;

 protected:
  enum class Priority
  {
    FIRST,
    LAST
  };
};
template<class... _Parameters>
class Event
    : public UD::Interface::Interface<Event<_Parameters...>,
                                      UD::Pack::Pack<PriorityBase>,
                                      UD::Interface::SimpleModifiers>
{
 public:
  using Handler = Handler<State&, _Parameters...>;

 private:
  std::map<int, std::deque<detail::EventConnection<_Parameters...>>>
      _positive_callers = {};
  std::map<int, std::deque<detail::EventConnection<_Parameters...>>>
                                                      _negative_callers = {};
  std::deque<detail::EventConnection<_Parameters...>> _zero_callers     = {};
  std::deque<detail::EventConnection<_Parameters...>> _first_callers    = {};
  std::deque<detail::EventConnection<_Parameters...>> _last_callers     = {};
  State                                               _state            = {};

  void FireCallers(_Parameters... parameters,
                   std::deque<detail::EventConnection<_Parameters...>>& callers)
  {
    static std::deque<std::size_t> remove_indices;
    for (std::size_t i = 0; i < callers.size() && !_state.IsSkipping(); ++i)
    {
      auto& caller = callers[i];
      if (*caller.valid)
      {
        caller.function(parameters...);
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
        callers.erase(callers.begin() + *it);
      }
      remove_indices.clear();
    }
  }
  void Fire(_Parameters... parameters)
  {
    for (auto& callers : _positive_callers)
    {
      FireCallers(parameters..., callers.second);
    }
    FireCallers(parameters..., _zero_callers);
    for (auto& callers : _negative_callers)
    {
      FireCallers(parameters..., callers.second);
    }
  }
  template<class... Ts>
  void FireSequence(Ts&&... parameters)
  {
    PreFire(_state, parameters...);
    Fire(parameters...);
    PostFire(_state, std::forward<Ts>(parameters)...);
  }

 protected:
  virtual void PreFire(State& state, _Parameters... parameters) {}
  virtual void PostFire(State& state, _Parameters... parameters) {}

 public:
  template<class F>
    requires(UD::Concept::Invocable<F, _Parameters...>)
  detail::Invalidator Add(F function, int priority = 0)
  {
    detail::EventConnection<_Parameters...> connection(
        [function](auto&&... ts) mutable
        {
          function(std::forward<decltype(ts)>(ts)...);
        });
    detail::Invalidator ret{connection.valid};
    if (priority < 0)
    {
      _negative_callers
          .insert(
              {priority, std::deque<detail::EventConnection<_Parameters...>>{}})
          .first->second.push_front(std::move(connection));
    }
    else if (priority > 0)
    {
      _positive_callers
          .insert(
              {priority, std::deque<detail::EventConnection<_Parameters...>>{}})
          .first->second.push_back(std::move(connection));
    }
    else
    {
      _zero_callers.push_back(std::move(connection));
    }
    return std::move(ret);
  }
  template<class F>
    requires(UD::Concept::Invocable<F, State&, _Parameters...>)
  detail::Invalidator Add(F function, int priority = 0)
  {
    return Add(
        [this, function](_Parameters&&... ps) mutable
        {
          function(this->_state, std::forward<decltype(ps)>(ps)...);
        },
        priority);
  }
  template<class F>
    requires(UD::Concept::Invocable<F, _Parameters...>)
  detail::Invalidator Add(F function, typename Event::Priority priority)
  {
    detail::EventConnection<_Parameters...> connection(
        [function](auto&&... ts) mutable
        {
          function(std::forward<decltype(ts)>(ts)...);
        });
    detail::Invalidator ret{connection.valid};
    switch (priority)
    {
      case Event::Priority::FIRST:
        _first_callers.push_back(std::move(connection));
        break;
      case Event::Priority::LAST:
        _last_callers.push_front(std::move(connection));
        break;
    }
    return std::move(ret);
  }
  template<class F>
    requires(UD::Concept::Invocable<F, State&, _Parameters...>)
  detail::Invalidator Add(F function, typename Event::Priority priority)
  {
    return Add(
        [this, function](_Parameters&&... ps) mutable
        {
          function(this->_state, std::forward<decltype(ps)>(ps)...);
        },
        priority);
  }

  ~Event() override       = default;
  Event(const Event&)     = default;
  Event(Event&&) noexcept = default;
  auto operator=(const Event&) -> Event& = default;
  auto operator=(Event&&) noexcept -> Event& = default;

  Event() noexcept = default;

  template<class... Ts>
  void operator()(Ts&&... parameters)
  {
    _state.Clear();
    FireSequence(std::forward<Ts>(parameters)...);
  }
  template<class T, class... Ts>
    requires(std::same_as<std::remove_reference<T>::type, State>)
  void operator()(T&& state, Ts&&... parameters)
  {
    _state = std::forward<T>(state);
    FireSequence(std::forward<Ts>(parameters)...);
  }
};
template<class... _Parameters>
class Handler
    : public UD::Interface::Interface<Handler<_Parameters...>,
                                      UD::Pack::Pack<PriorityBase>,
                                      UD::Interface::SimpleModifiers>
{
  using FunctionType = std::function<void(_Parameters...)>;

  FunctionType                     _function     = [](_Parameters...) {};
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
  auto GenerateAdaptedRun(F func)
  {
    return [this, func](Ts&&... ts) mutable
    {
      std::apply(&Handler::Run<_Parameters...>,
                 std::tuple_cat(std::tuple<Handler*>(this),
                                func(std::forward<Ts>(ts)...)));
    };
  }
  template<class F, class... Ts>
    requires(HasState<_Parameters...>::value)
  auto GenerateAdaptedRun(F func)
  {
    return [this, func](State& state, Ts&&... ts) mutable
    {
      std::apply(&Handler::Run<_Parameters...>,
                 std::tuple_cat(std::tuple<Handler*, State&>(this, state),
                                func(std::forward<Ts>(ts)...)));
    };
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
      : _function{[type, function](_Parameters&&... ps) mutable
                  {
                    (type->*function)(std::forward<_Parameters>(ps)...);
                  }}
  {
  }

 protected:
  template<std::size_t _Size, class _EventType>
  void operator()(std::array<_EventType&, _Size> events,
                  typename Handler::Priority     priority)
  {
    for (auto& e : events)
    {
      operator()(e, priority);
    }
  }
  template<class... Ts>
  void operator()(Event<Ts...>& e, typename Handler::Priority priority)
  {
    _invalidators.push_back(e.Add(
        [this](_Parameters&&... ps) mutable
        {
          this->Run<_Parameters...>(std::forward<_Parameters>(ps)...);
        },
        priority));
  }
  template<class F, class... Ts>
  void operator()(Event<Ts...>&              e,
                  F                          adaptor_function,
                  typename Handler::Priority priority)
  {
    _invalidators.push_back(
        e.Add(GenerateAdaptedRun<F, Ts...>(std::move(adaptor_function)),
              priority));
  }

 public:
  template<std::size_t _Size, class _EventType>
  void operator()(std::array<_EventType&, _Size> events, int priority = 0)
  {
    for (auto& e : events)
    {
      operator()(e, priority);
    }
  }
  template<class... Ts>
  void operator()(Event<Ts...>& e, int priority = 0)
  {
    _invalidators.push_back(e.Add(
        [this](_Parameters&&... ps) mutable
        {
          this->Run<_Parameters...>(std::forward<_Parameters>(ps)...);
        },
        priority));
  }
  template<class F, class... Ts>
  void operator()(Event<Ts...>& e, F adaptor_function, int priority = 0)
  {
    _invalidators.push_back(
        e.Add(GenerateAdaptedRun<F, Ts...>(std::move(adaptor_function)),
              priority));
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
    _function = [type, function](_Parameters... ps)
    {
      (type->*function)(std::move(ps)...);
    };
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
  using ValueTuple = std::tuple<_Parameters...>;

  std::list<ValueTuple> _queued_event_values;

  void Push(_Parameters... parameters)
  {
    _queued_event_values.push_back(ValueTuple{std::move(parameters)...});
  }
  void ProcessTuple(ValueTuple& values)
  {
    std::apply(
        [this](auto&&... ps)
        {
          this->Event<_Parameters...>::operator()(
              std::forward<decltype(ps)>(ps)...);
        },
        values);
  }
  template<UD::Concept::Callable<bool(_Parameters...)> T>
  bool ProcessCondition(T condition, ValueTuple& tup)
  {
    return std::apply(condition, tup);
  }
  template<UD::Concept::Callable<bool()> T>
  bool ProcessCondition(T condition, ValueTuple& tup)
  {
    return condition();
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

  void ProcessNext()
  {
    Process(1);
  }
  std::size_t Size() const
  {
    return _queued_event_values.size();
  }
  void Process(std::size_t max = 0)
  {
    if (!max)
    {
      ProcessWhile(
          []()
          {
            return true;
          });
    }
    else
    {
      ProcessWhile(
          [max]()
          {
            static std::size_t val = max;
            return val-- != 0;
          });
    }
  }
  template<class T>
  void ProcessWhile(T&& condition)
  {
    ProcessWhileIf(std::forward<T>(condition),
                   []()
                   {
                     return true;
                   });
  }
  template<class T>
  void ProcessIf(T&& condition)
  {
    ProcessWhileIf(
        []()
        {
          return true;
        },
        std::forward<T>(condition));
  }
  // TODO: Rethink this function as it is inflexible and too specific.  Maybe
  // separate out conditions to be specified elsewhere.
  template<class T, class U>
  void ProcessWhileIf(T while_condition, U if_condition)
  {
    for (auto it = _queued_event_values.begin();
         it != _queued_event_values.end()
         && ProcessCondition(while_condition, *it);)
    {
      if (!ProcessCondition(if_condition, *it))
      {
        ++it;
        continue;
      }
      ProcessTuple(*it);
      it = _queued_event_values.erase(it);
    }
  }

  template<UD::Concept::Callable<bool(_Parameters...)> T>
  void Purge(T condition)
  {
    std::erase_if(_queued_event_values,
                  [condition](std::tuple<_Parameters...> tup)
                  {
                    return std::apply(condition, tup);
                  });
  }
  void Purge()
  {
    _queued_event_values.clear();
  }
};
template<class... _Parameters>
struct Chain
    : public Interface::Interface<
          Chain<_Parameters...>,
          UD::Pack::Pack<Event<_Parameters...>,
                         Handler<State&, _Parameters...>>,
          UD::Interface::SimpleModifiers>
{
 private:

 public:
  ~Chain() override       = default;
  Chain(const Chain&)     = default;
  Chain(Chain&&) noexcept = default;
  auto operator=(const Chain&) -> Chain& = default;
  auto operator=(Chain&&) noexcept -> Chain& = default;

  Chain()
      : Chain::Super{Chain::Constructor<Handler<State&, _Parameters...>>::Call(
          &Chain::Callback,
          this)}
  {
  }

 private:
  void Callback(State& state, _Parameters... ps)
  {
    Event<_Parameters...>::operator()(state, ps...);
  }

 public:
  template<std::size_t _Size, class _EventType>
  void operator()(std::array<_EventType&, _Size> events)
  {
    Handler<State&, _Parameters...>::operator()(
        events,
        Event<_Parameters...>::Priority::LAST);
  }
  template<class... Ts>
  void operator()(Event<Ts...>& e)
  {
    Handler<State&, _Parameters...>::operator()(
        e,
        Event<_Parameters...>::Priority::LAST);
  }
  template<class F, class... Ts>
  void operator()(Event<Ts...>& e, F adaptor_function)
  {
    Handler<State&, _Parameters...>::operator()(
        e,
        std::move(adaptor_function),
        Event<_Parameters...>::Priority::LAST);
  }
};
}  // namespace UD::Event