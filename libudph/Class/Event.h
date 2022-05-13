#pragma once
#include <array>
#include <deque>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <libudph/Class/Interface.h>
#include <libudph/Class/SFINAE.h>

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

template<class T, class U>
concept PackSameAs = UD::SFINAE::pack_is_same<T, U>::value;
template<class T, class U>
concept PackConvertibleTo = UD::SFINAE::pack_is_convertible<T, U>::value;
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
  auto Emplace(Ts... ts) -> std::shared_ptr<T>
  {
    auto ret = std::make_shared<T>(ts...);
    _data    = std::static_pointer_cast<void>(ret);
    return ret;
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
  // TODO: Handle std::function<std::ref<Callable<void(_Parameters...)>>>
  // instead to avoid std::function dynamic alocation nonsense.
  //       Likely requires wrapper class (Caller?)
  std::function<void(_Parameters...)> function = [](_Parameters...) {};
  std::shared_ptr<bool>               valid    = std::make_shared<bool>(true);
};

}  // namespace detail
class Invalidator
{
  // TODO: Find better way to reference count... Using unnecessary bool* here.
  std::shared_ptr<bool> _count = std::make_shared<bool>(true);
  std::shared_ptr<bool> _valid = nullptr;

 public:
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
enum Priority : int
{
  LOWEST  = std::numeric_limits<int>::min(),
  HIGHEST = std::numeric_limits<int>::max()
};
namespace detail
{
struct PriorityBase
{
  virtual ~PriorityBase() = default;

 protected:
  enum class InternalPriority
  {
    CONDITION,
    FIRST,
    LAST
  };
};
}  // namespace detail
class Manager
{
  // TODO: replace std::function with own implementation because it's slow
  std::deque<std::function<void()>> _event_queue = {};

 public:
  void Queue(std::function<void()> func)
  {
    _event_queue.push_back(std::move(func));
  }
  auto Empty() -> bool
  {
    return _event_queue.empty();
  }

  void RunAll()
  {
    auto s = _event_queue.size();
    for (std::size_t i = 0; i < s; ++i)
    {
      RunNext();
    }
  }
  void RunNext()
  {
    _event_queue.front()();
    _event_queue.pop_front();
  }
  void Run()
  {
    while (!Empty())
    {
      RunNext();
    }
  }
};
template<class... _Parameters>
class Event
    : public UD::Interface::Interface<Event<_Parameters...>,
                                      UD::Pack::Pack<detail::PriorityBase>,
                                      UD::Interface::SimpleModifiers>
{
 public:
  using StateHandler = Handler<State&, _Parameters...>;
  using Handler      = Handler<_Parameters...>;

 private:
  std::map<int, std::deque<detail::EventConnection<_Parameters...>>>
      _positive_callers = {};
  std::map<int, std::deque<detail::EventConnection<_Parameters...>>>
                                                      _negative_callers = {};
  std::deque<detail::EventConnection<_Parameters...>> _zero_callers     = {};
  std::deque<detail::EventConnection<_Parameters...>> _first_callers    = {};
  std::deque<detail::EventConnection<_Parameters...>> _last_callers     = {};
  std::deque<detail::EventConnection<_Parameters...>> _conditions       = {};
  State                                               _state            = {};
  std::deque<std::function<void()>>                   _delayed_fires    = {};
  std::shared_ptr<Manager>                            _manager = nullptr;
  bool                                                _firing  = false;

  // TODO: Improve performance.
  //       Currently accepets callers by value because must handle case
  //       where callers is modified further in stack.
  void FireCallers(_Parameters... parameters,
                   std::deque<detail::EventConnection<_Parameters...>> callers)
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

  template<class... Ts>
    requires(UD::Concept::PackConvertibleTo<UD::Pack::Pack<Ts...>,
                                            UD::Pack::Pack<_Parameters...>>)
  void Fire(Ts&&... parameters)
  {
    Fire(State{}, std::forward<Ts>(parameters)...);
  }
  template<class... Ts>
    requires(UD::Concept::PackConvertibleTo<UD::Pack::Pack<Ts...>,
                                            UD::Pack::Pack<_Parameters...>>)
  void Fire(State state, Ts&&... parameters)
  {
    if (_manager)
    {
      _manager->Queue(
          [this, state, ... parameters = std::forward<Ts>(parameters)]() mutable
          {
            FireBypassManager(std::move(state),
                              std::forward<Ts>(parameters)...);
          });
      return;
    }
    FireBypassManager(std::move(state), std::forward<Ts>(parameters)...);
  }
  template<class... Ts>
    requires(UD::Concept::PackConvertibleTo<UD::Pack::Pack<Ts...>,
                                            UD::Pack::Pack<_Parameters...>>)
  void FireBypassManager(Ts&&... parameters)
  {
    FireBypassManager(State{}, std::forward<Ts>(parameters)...);
  }
  void FireBypassManager(State state, _Parameters... parameters)
  {
    if (_firing)
    {
      _delayed_fires.push_back(
          [this, state, ... parameters = parameters]() mutable
          {
            operator()(std::move(state), parameters...);
          });
      return;
    }

    _firing = true;
    _state  = std::move(state);

    if (!_conditions.empty())
      FireCallers(parameters..., _conditions);
    if (!_first_callers.empty())
      FireCallers(parameters..., _first_callers);
    if (!_positive_callers.empty())
    {
      for (auto& callers : _positive_callers)
      {
        FireCallers(parameters..., callers.second);
      }
    }
    if (!_zero_callers.empty())
      FireCallers(parameters..., _zero_callers);
    if (!_negative_callers.empty())
    {
      for (auto& callers : _negative_callers)
      {
        FireCallers(parameters..., callers.second);
      }
    }
    if (!_last_callers.empty())
      FireCallers(parameters..., _last_callers);

    _firing = false;

    for (auto& f : _delayed_fires)
    {
      f();
    }
  }

  template<class... Ts>
  void LinkCallback(State& state, Ts&&... ps)
  {
    FireBypassManager(state, std::forward<Ts>(ps)...);
  }

 protected:
  virtual void PreFire(State& state, _Parameters... parameters) {}
  virtual void PostFire(State& state, _Parameters... parameters) {}

 public:
  template<class F>
    requires(UD::Concept::Invocable<F, _Parameters...>)
  std::shared_ptr<bool> Add(F function, int priority = 0)
  {
    detail::EventConnection<_Parameters...> connection(
        [function](auto&&... ts) mutable
        {
          function(std::forward<decltype(ts)>(ts)...);
        });
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
    return connection.valid;
  }
  template<class F>
    requires(UD::Concept::Invocable<F, State&, _Parameters...>)
  std::shared_ptr<bool> Add(F function, int priority = 0)
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
  std::shared_ptr<bool> Add(F                                function,
                            typename Event::InternalPriority priority)
  {
    detail::EventConnection<_Parameters...> connection(
        [function](auto&&... ts) mutable
        {
          function(std::forward<decltype(ts)>(ts)...);
        });
    switch (priority)
    {
      case Event::InternalPriority::FIRST:
        _first_callers.push_back(std::move(connection));
        break;
      case Event::InternalPriority::LAST:
        _last_callers.push_front(std::move(connection));
        break;
      case Event::InternalPriority::CONDITION:
        _conditions.push_back(std::move(connection));
        break;
    }
    return connection.valid;
  }
  template<class F>
    requires(UD::Concept::Invocable<F, State&, _Parameters...>)
  std::shared_ptr<bool> Add(F                                function,
                            typename Event::InternalPriority priority)
  {
    return Add(
        [this, function](_Parameters&&... ps) mutable
        {
          function(this->_state, std::forward<decltype(ps)>(ps)...);
        },
        priority);
  }
  template<class F>
    requires(UD::Concept::Callable<F, bool(_Parameters...)>)
  std::shared_ptr<bool> AddCondition(F function)
  {
    return Add(
        [function](State& state, _Parameters... parameters)
        {
          if (!function(std::move(parameters)...))
          {
            state.Skip();
          }
        },
        Event::InternalPriority::CONDITION);
  }
  template<class F>
    requires(UD::Concept::Callable<F, bool()>)
  std::shared_ptr<bool> AddCondition(F function)
  {
    return Add(
        [function](State& state, _Parameters... parameters)
        {
          if (!function())
          {
            state.Skip();
          }
        },
        Event::InternalPriority::CONDITION);
  }

  ~Event() override       = default;
  Event(const Event&)     = default;
  Event(Event&&) noexcept = default;
  auto operator=(const Event&) -> Event& = default;
  auto operator=(Event&&) noexcept -> Event& = default;

  Event() noexcept : Event{nullptr} {}
  Event(std::shared_ptr<Manager> manager) : _manager{std::move(manager)}
  {
    Add(
        [this](State& state, _Parameters&&... parameters)
        {
          this->PreFire(state, std::forward<_Parameters>(parameters)...);
        },
        Event::InternalPriority::FIRST);
    Add(
        [this](State& state, _Parameters&&... parameters)
        {
          this->PostFire(state, std::forward<_Parameters>(parameters)...);
        },
        Event::InternalPriority::LAST);
  }

  void SetManager(std::shared_ptr<Manager> manager)
  {
    _manager = std::move(manager);
  }
  template<class... Ts>
    requires(UD::Concept::PackConvertibleTo<UD::Pack::Pack<Ts...>,
                                            UD::Pack::Pack<_Parameters...>>)
  void operator()(Ts&&... parameters)
  {
    operator()(State{}, std::forward<Ts>(parameters)...);
  }
  template<class... Ts>
    requires(UD::Concept::PackConvertibleTo<UD::Pack::Pack<Ts...>,
                                            UD::Pack::Pack<_Parameters...>>)
  void operator()(State state, Ts&&... parameters)
  {
    Fire(std::move(state), std::forward<Ts>(parameters)...);
  }

  template<class... Ts>
    requires(UD::Concept::PackConvertibleTo<UD::Pack::Pack<Ts...>,
                                            UD::Pack::Pack<_Parameters...>>)
  void Link(Event<Ts...>& e)
  {
    e.Add(
        [this](State& state, Ts&&... ts)
        {
          this->LinkCallback<_Parameters...>(state, std::forward<Ts>(ts)...);
        },
        Event::InternalPriority::LAST);
  }
};
template<class... _Parameters>
class Handler
    : public UD::Interface::Interface<Handler<_Parameters...>,
                                      UD::Interface::SimpleModifiers>
{
  using FunctionType = std::function<void(_Parameters...)>;

  FunctionType             _function     = [](_Parameters...) {};
  std::vector<Invalidator> _invalidators = {};
  bool                     _enabled      = true;

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
  Handler(void (T::*function)(_Parameters...), T* type) : _function
  {
    [type, function](_Parameters&&... ps) mutable
    {
      (type->*function)(std::forward<_Parameters>(ps)...);
    }
  }
  {
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
    requires(!std::convertible_to<F, int>)
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
template<class _Return, class... _Parameters>
struct Handler<_Return(_Parameters...)> : public Handler<_Parameters...>
{
  ~Handler() override = default;
  using Handler<_Parameters...>::Handler;
};
namespace detail
{
template<class T>
struct CallableToFunctionType
{
};
template<class T, class Return, class... Args>
struct CallableToFunctionType<Return (T::*)(Args...) const>
{
  using type = Return(Args...);
};
template<class T, class Return, class... Args>
struct CallableToFunctionType<Return (T::*)(Args...)>
{
  using type = Return(Args...);
};
template<class Return, class... Args>
struct CallableToFunctionType<Return (*)(Args...)>
{
  using type = Return(Args...);
};
}  // namespace detail
template<class T>
Handler(T) -> Handler<
    typename detail::CallableToFunctionType<decltype(&T::operator())>::type>;
template<class T>
Handler(T) -> Handler<typename detail::CallableToFunctionType<T>::type>;

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
      : Queue::Super{Queue::template Constructor<Handler<_Parameters...>>::Call(
          &Queue::Push,
          this)}
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
}  // namespace UD::Event