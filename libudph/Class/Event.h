#pragma once
#include <array>
#include <chrono>
#include <deque>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <libudph/Class/Interface.h>
#include <libudph/Class/Pack.h>
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
}
namespace UD::Event
{
namespace Concept = UD::Concept::Event;
enum class Request
{
  Skip,
  Continue,
  Remove
};
template<class... _Parameters>
class Handler;
template<class... Ts>
class Event;
class Validator
{
  friend class Invalidator;

  std::shared_ptr<bool> valid = std::make_shared<bool>(true);

 public:
  operator bool()
  {
    return valid && *valid;
  }
};
class Invalidator
{
  std::shared_ptr<bool> count = std::make_shared<bool>(true);
  std::shared_ptr<bool> valid = nullptr;

 public:
  ~Invalidator()
  {
    if (valid)
    {
      *valid = count.use_count() > 1;
    }
  }
  Invalidator(const Invalidator& other) = default;
  Invalidator(Invalidator&&) noexcept   = default;
  auto operator=(const Invalidator&) -> Invalidator& = default;
  auto operator=(Invalidator&&) noexcept -> Invalidator& = default;

  Invalidator() = default;
  Invalidator(const Validator& validator) : valid{validator.valid} {}
};
namespace detail
{
template<class... _Parameters>
struct EventConnection
{
  // TODO: Handle std::function<std::ref<Callable<void(_Parameters...)>>>
  // instead to avoid std::function dynamic alocation nonsense.
  //       Likely requires wrapper class (Caller?)
  std::function<Request(_Parameters...)> function
      = [](_Parameters...) -> Request
  {
    return Request::Continue;
  };
  Validator valid = {};
};
}  // namespace detail
namespace detail
{
using EmptyProcessorLambda = decltype(
    [](auto&& a) -> Request
    {
      return Request::Continue;
    });
}
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
template<class... Ts>
struct EmptyProcessor : Ts...
{
  using Ts::operator()...;
};
template<class... Ts>
struct Processor : public EmptyProcessor<Ts..., detail::EmptyProcessorLambda>
{
  using EmptyProcessor<Ts..., detail::EmptyProcessorLambda>::operator();

  template<class... Us>
  Processor(Us&&... us) : EmptyProcessor<Ts..., detail::EmptyProcessorLambda>
  {
    std::forward<Us>(us)..., detail::EmptyProcessorLambda {}
  }
  {
  }
};
template<class... Ts>
Processor(Ts...) -> Processor<Ts...>;
template<class... Ts>
struct Data
{
  std::variant<std::monostate, Ts...> content;

  template<class... Fs>
  auto Process(Fs&&... fs)
  {
    return Process(Processor{std::forward<Fs>(fs)...});
  }
  template<class... Fs>
  auto Process(Processor<Fs...> processor)
  {
    return std::visit(processor, content);
  }
};

namespace detail
{
template<class... Ts>
auto ManagerFunctionWrapper(UD::Concept::Invocable<Ts...> auto func, Ts&&... ts)
    -> std::function<void()>
{
  return [func = std::move(func), &ts...]()
  {
    func(std::forward<Ts>(ts)...);
  };
}
template<class Wrapper = decltype(ManagerFunctionWrapper<>([]() {}))>
class ManagerDef
{
  // TODO: replace std::function with own implementation because it's slow
  using Clock     = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;

  std::multimap<TimePoint, Wrapper> _event_queue           = {};
  std::multimap<TimePoint, Wrapper> _new_event_queue       = {};
  std::mutex                        _mutex_event_queue     = {};
  std::mutex                        _mutex_new_event_queue = {};

 public:
  template<class... Ts>
  void Queue(UD::Concept::Invocable<Ts...> auto func, Ts&&... ts)
  {
    auto wrapper
        = ManagerFunctionWrapper(std::move(func), std::forward<Ts>(ts)...);
    if (!_mutex_event_queue.try_lock())
    {
      QueueNew(std::move(wrapper));
    }
    else
    {
      auto lock = std::scoped_lock{std::adopt_lock, _mutex_event_queue};
      QueueSynchronous(std::move(wrapper));
    }
  }
  void QueueSynchronous(Wrapper wrapper)
  {
    _event_queue.insert(_event_queue.end(),
                        std::make_pair(Clock::now(), std::move(wrapper)));
  }
  void QueueNew(Wrapper wrapper)
  {
    auto lock = std::scoped_lock{_mutex_new_event_queue};
    _new_event_queue.insert(_event_queue.end(),
                            std::make_pair(Clock::now(), std::move(wrapper)));
  }
  void UpdateQueue()
  {
    auto lock = std::scoped_lock{_mutex_event_queue, _mutex_new_event_queue};
    for (auto& [timepoint, function] : _new_event_queue)
    {
      _event_queue.insert(
          _event_queue.end(),
          std::make_pair(std::move(timepoint), std::move(function)));
    }
    _new_event_queue.clear();
  }

  void RunAll()
  {
    UpdateQueue();
    auto lock = std::scoped_lock{_mutex_event_queue};
    RunAllSynchronous();
  }
  void RunAllSynchronous()
  {
    auto m       = std::move(_event_queue);
    _event_queue = std::multimap<TimePoint, Wrapper>{};

    for (auto& [timepoint, function] : m)
    {
      function();
    }
  }
  void RunNext()
  {
    UpdateQueue();
    auto lock = std::scoped_lock{_mutex_event_queue};
    RunNextSynchronous();
  }
  void RunNextSynchronous()
  {
    auto it = _event_queue.begin();
    it->second();
    _event_queue.erase(it);
  }
  void Run()
  {
    auto should = bool(false);
    {
      auto lock = std::scoped_lock{_mutex_event_queue, _mutex_new_event_queue};
      should    = !_event_queue.empty() || !_new_event_queue.empty();
    }
    while (should)
    {
      UpdateQueue();
      {
        auto lock = std::scoped_lock{_mutex_event_queue};
        RunSynchronous();
      }
      {
        auto lock
            = std::scoped_lock{_mutex_event_queue, _mutex_new_event_queue};
        should = !_event_queue.empty() || !_new_event_queue.empty();
      }
    }
  }
  void RunSynchronous()
  {
    while (!_event_queue.empty())
    {
      RunAllSynchronous();
    }
  }
};
}  // namespace detail
using Manager = detail::ManagerDef<>;

template<class... _Parameters>
class Handler;
template<class... Ts>
class Event;
template<class... _DataTypes, class... _Parameters>
class Event<Data<_DataTypes...>, _Parameters...>;
template<class... _DataTypes, class... _Parameters>
  requires(std::same_as<_DataTypes, std::remove_reference_t<_DataTypes>>&&...)
class Event<Data<_DataTypes...>, _Parameters...>
    : public UD::Interface::Interface<
          Event<Data<_DataTypes...>, _Parameters...>,
          UD::Pack::Pack<detail::PriorityBase>,
          UD::Interface::SimpleModifiers>
{
  using Interface
      = UD::Interface::Interface<Event<Data<_DataTypes...>, _Parameters...>,
                                 UD::Pack::Pack<detail::PriorityBase>,
                                 UD::Interface::SimpleModifiers>;
  using EventConnection
      = detail::EventConnection<Data<_DataTypes...>&, _Parameters...>;
  using CallerContainer    = std::vector<EventConnection>;
  using CallerContainerMap = std::map<int, CallerContainer>;

 public:
  template<class... T>
    requires(
        sizeof...(T) == 0
        || (sizeof...(T) == 1
            && (std::convertible_to<_DataTypes,
                                    typename Pack::Pack<T...>::Type> || ...)))
  using Handler = UD::Event::Handler<T..., _Parameters...>;

  struct Container
  {
    CallerContainerMap _positive_callers     = {};
    CallerContainerMap _new_positive_callers = {};
    CallerContainerMap _negative_callers     = {};
    CallerContainerMap _new_negative_callers = {};
    CallerContainer    _zero_callers         = {};
    CallerContainer    _new_zero_callers     = {};
    CallerContainer    _first_callers        = {};
    CallerContainer    _new_first_callers    = {};
    CallerContainer    _last_callers         = {};
    CallerContainer    _new_last_callers     = {};
    CallerContainer    _conditions           = {};
    CallerContainer    _new_conditions       = {};

    std::vector<std::function<void()>> _delayed_fires       = {};
    std::shared_ptr<Manager>           _manager             = nullptr;
    std::mutex                         _mutex               = {};
    std::mutex                         _mutex_firing        = {};
    std::mutex                         _mutex_delayed_fires = {};
    std::mutex                         _mutex_new           = {};
    std::mutex                         _mutex_manager       = {};
  };

 private:
  std::shared_ptr<Container> _container = std::make_shared<Container>();
  // TODO: Improve performance.
  //       Currently accepets callers by value because must handle case
  //       where callers is modified further in stack.
  static auto FireCallers(CallerContainer&     callers,
                          Data<_DataTypes...>& data,
                          auto&&... parameters)
  {
    for (auto it = callers.begin(); it != callers.end();)
    {
      if (!it->valid)
      {
        it = callers.erase(it);
        continue;
      }
      auto call_ret
          = it->function(data,
                         std::forward<decltype(parameters)>(parameters)...);
      if (call_ret == Request::Remove)
      {
        it = callers.erase(it);
        continue;
      }
      else if (call_ret == Request::Skip)
      {
        return Request::Skip;
      }
      ++it;
    }
    return Request::Continue;
  }
  static auto FireCallersReverse(CallerContainer&     callers,
                                 Data<_DataTypes...>& data,
                                 auto&&... parameters)
  {
    for (auto it = callers.rbegin(); it != callers.rend();)
    {
      if (!it->valid)
      {
        it = static_cast<decltype(it)>(callers.erase(std::next(it).base()));
        continue;
      }
      auto call_ret
          = it->function(data,
                         std::forward<decltype(parameters)>(parameters)...);
      if (call_ret == Request::Remove)
      {
        it = static_cast<decltype(it)>(callers.erase(std::next(it).base()));
        continue;
      }
      else if (call_ret == Request::Skip)
      {
        return Request::Skip;
      }
      ++it;
    }
    return Request::Continue;
  }

  static void UpdateCallersAssumedLockMutex(
      std::shared_ptr<Container> container)
  {
    auto lock = std::scoped_lock{container->_mutex_new};
    for (auto& [priority, new_callers] : container->_new_positive_callers)
    {
      if (new_callers.empty())
        continue;
      auto& caller_container
          = container->_positive_callers.insert({priority, CallerContainer{}})
                .first->second;
      caller_container.reserve(caller_container.size() + new_callers.size());
      caller_container.insert(caller_container.end(),
                              new_callers.begin(),
                              new_callers.end());
    }
    for (auto& [priority, new_callers] : container->_new_negative_callers)
    {
      if (new_callers.empty())
        continue;
      auto& caller_container
          = container->_negative_callers.insert({priority, CallerContainer{}})
                .first->second;
      caller_container.reserve(caller_container.size() + new_callers.size());
      caller_container.insert(caller_container.end(),
                              new_callers.begin(),
                              new_callers.end());
    }
    if (!container->_new_zero_callers.empty())
    {
      auto& caller_container = container->_zero_callers;
      auto& new_callers      = container->_new_zero_callers;
      caller_container.reserve(caller_container.size() + new_callers.size());
      caller_container.insert(caller_container.end(),
                              new_callers.begin(),
                              new_callers.end());
    }
    if (!container->_new_first_callers.empty())
    {
      auto& caller_container = container->_first_callers;
      auto& new_callers      = container->_new_first_callers;
      caller_container.reserve(caller_container.size() + new_callers.size());
      caller_container.insert(caller_container.end(),
                              new_callers.begin(),
                              new_callers.end());
    }
    if (!container->_new_last_callers.empty())
    {
      auto& caller_container = container->_last_callers;
      auto& new_callers      = container->_new_last_callers;
      caller_container.reserve(caller_container.size() + new_callers.size());
      caller_container.insert(caller_container.end(),
                              new_callers.begin(),
                              new_callers.end());
    }
    if (!container->_new_conditions.empty())
    {
      auto& caller_container = container->_conditions;
      auto& new_callers      = container->_new_conditions;
      caller_container.reserve(caller_container.size() + new_callers.size());
      caller_container.insert(caller_container.end(),
                              new_callers.begin(),
                              new_callers.end());
    }
  }
  static void FireBypassManager(std::shared_ptr<Container> container,
                                auto&&                     value,
                                auto&&... parameters)  //
      requires(sizeof...(parameters) == sizeof...(_Parameters))
  {
    auto data = Data<_DataTypes...>{std::forward<decltype(value)>(value)};

    if (!container->_mutex_firing.try_lock())
    {
      auto lock = std::scoped_lock{container->_mutex_delayed_fires};
      container->_delayed_fires.push_back(
          [container = container, &parameters...]()
          {
            FireContainer(std::move(container),
                          std::forward<decltype(parameters)>(parameters)...);
          });
      return;
    }
    else
    {
      container->_mutex.lock();
      auto lock = std::scoped_lock{std::adopt_lock,
                                   container->_mutex_firing,
                                   container->_mutex};
      UpdateCallersAssumedLockMutex(container);

      if (!container->_conditions.empty())
        FireCallers(container->_conditions, data, parameters...);
      if (!container->_first_callers.empty())
        FireCallersReverse(container->_first_callers, data, parameters...);
      if (!container->_positive_callers.empty())
      {
        for (auto it = container->_positive_callers.rbegin();
             it != container->_positive_callers.rend();
             ++it)
        {
          FireCallers(it->second, data, parameters...);
        }
      }
      if (!container->_zero_callers.empty())
      {
        FireCallers(container->_zero_callers, data, parameters...);
      }
      if (!container->_negative_callers.empty())
      {
        for (auto it = container->_negative_callers.rbegin();
             it != container->_negative_callers.rend();
             ++it)
        {
          FireCallersReverse(it->second, data, parameters...);
        }
      }
      if (!container->_last_callers.empty())
        FireCallersReverse(container->_last_callers, data, parameters...);
    }
    std::vector<std::function<void()>> delayed_fires;
    {
      auto lock     = std::scoped_lock{container->_mutex_delayed_fires};
      delayed_fires = std::move(container->_delayed_fires);
      container->_delayed_fires = std::vector<std::function<void()>>{};
    }
    for (auto& f : delayed_fires)
    {
      f();
    }
  }

  static void FireBypassManager(std::shared_ptr<Container> container,
                                auto&&... parameters)  //
      requires(sizeof...(parameters) == sizeof...(_Parameters))
  {
    FireBypassManager(std::move(container),
                      std::monostate{},
                      std::forward<decltype(parameters)>(parameters)...);
  }

 public
     :

     static void
     FireContainer(std::shared_ptr<Container> container, auto&&... parameters)
  {
    {
      auto lock = std::scoped_lock{container->_mutex_manager};
      if (container->_manager)
      {
        container->_manager->Queue(
            [container = std::move(container)](auto&&... parameters)
            {
              FireBypassManager(
                  std::move(container),
                  std::forward<decltype(parameters)>(parameters)...);
            },
            std::forward<decltype(parameters)>(parameters)...);
        return;
      }
    }
    FireBypassManager(std::move(container),
                      std::forward<decltype(parameters)>(parameters)...);
  }
  void Fire(auto&&... parameters)
  {
    FireContainer(_container,
                  std::forward<decltype(parameters)>(parameters)...);
  }
  void operator()(auto&&... parameters)
  {
    FireContainer(_container,
                  std::forward<decltype(parameters)>(parameters)...);
  }

 public:
  template<class... Ts>
    requires(UD::Concept::PackConvertibleTo<UD::Pack::Pack<Ts...>,
                                            UD::Pack::Pack<_Parameters...>>)
  void LinkFrom(Event<Ts...>& e)
  {
    e.Add(
        [container = std::weak_ptr<Container>{_container}](auto&&... ts)
        {
          if (auto lock = container.lock())
          {
            FireBypassManager(lock, std::forward<decltype(ts)>(ts)...);
            return Request::Continue;
          }
          else
          {
            return Request::Remove;
          }
        },
        Event::InternalPriority::LAST);
  }
  template<class... Ts>
    requires(UD::Concept::PackConvertibleTo<UD::Pack::Pack<_Parameters...>,
                                            UD::Pack::Pack<Ts...>>)
  void LinkTo(Event<Ts...>& e)
  {
    this->Add(
        [container = std::weak_ptr<Container>{e._container}](auto&&... ts)
        {
          if (auto lock = container.lock())
          {
            FireBypassManager(lock, std::forward<decltype(ts)>(ts)...);
            return Request::Continue;
          }
          else
          {
            return Request::Remove;
          }
        },
        Event::InternalPriority::LAST);
  }

 private:
  // TODO: Fix clang-format issue
  // HelperConnection and MakeHelperConnection are only necessary to avoid
  // clang-format bugs with templated lambdas
  template<class T, class F>
  struct HelperConnection : public F
  {
    auto operator()(auto&& t) requires std::convertible_to<decltype(t), T>
    {
      return F::operator()(std::forward<decltype(t)>(t));
    }
  };
  template<class T, class F>
  static auto MakeHelperConnection(F&& f)
  {
    return HelperConnection<T, F>{std::forward<F>(f)};
  }
  template<class T, class F>
    requires UD::Concept::Invocable<F, T, _Parameters...>        //
        && UD::Concept::Callable<F, Request(T, _Parameters...)>  //
        &&(std::convertible_to<_DataTypes&, T> || ...)           //
        auto CreateConnection(F function)
    {
      return EventConnection{
          [function = std::move(function)](Data<_DataTypes...>& data_variant,
                                           auto&&... parameters)
          {
            return data_variant.Process(MakeHelperConnection<T>(
                [function = std::move(function),
                 &parameters...](auto&& t) -> Request
                {
                  return function(
                      std::forward<decltype(t)>(t),
                      std::forward<decltype(parameters)>(parameters)...);
                }));
          }};
    }
    template<class T, class F>
      requires UD::Concept::Invocable<F, T, _Parameters...>          //
          &&(!UD::Concept::Callable<F, Request(T, _Parameters...)>)  //
          &&(std::convertible_to<_DataTypes&, T> || ...)             //
          auto CreateConnection(F function)
      {
        return EventConnection{
            [function = std::move(function)](Data<_DataTypes...>& data_variant,
                                             auto&&... parameters) -> Request
            {
              return data_variant.Process(MakeHelperConnection<T>(
                  [function = std::move(function),
                   &parameters...](auto&& t) -> Request
                  {
                    function(std::forward<decltype(t)>(t),
                             std::forward<decltype(parameters)>(parameters)...);
                    return Request::Continue;
                  }));
            }};
      }
      template<class F>
        requires UD::Concept::Invocable<F, _Parameters...>        //
            && UD::Concept::Callable<F, Request(_Parameters...)>  //
      auto CreateConnection(F function)
      {
        return EventConnection{
            [function = std::move(function)](Data<_DataTypes...>& data_variant,
                                             auto&&... parameters)
            {
              return function(
                  std::forward<decltype(parameters)>(parameters)...);
            }};
      }
      template<class F>
        requires UD::Concept::Invocable<F, _Parameters...>          //
            &&(!UD::Concept::Callable<F, Request(_Parameters...)>)  //
            auto CreateConnection(F function)
        {
          return EventConnection{
              [function = std::move(function)](
                  Data<_DataTypes...>& data_variant,
                  auto&&... parameters)
              {
                function(std::forward<decltype(parameters)>(parameters)...);
                return Request::Continue;
              }};
        }
        auto Add(EventConnection connection, int priority) -> Validator
        {
          auto valid = Validator{connection.valid};
          if (_container->_mutex.try_lock())
          {
            auto lock = std::scoped_lock{std::adopt_lock, _container->_mutex};
            if (priority < 0)
            {
              _container->_negative_callers
                  .insert({priority, CallerContainer{}})
                  .first->second.push_back(std::move(connection));
            }
            else if (priority > 0)
            {
              _container->_positive_callers
                  .insert({priority, CallerContainer{}})
                  .first->second.push_back(std::move(connection));
            }
            else
            {
              _container->_zero_callers.push_back(std::move(connection));
            }
          }
          else
          {
            auto lock = std::scoped_lock{_container->_mutex_new};
            if (priority < 0)
            {
              _container->_new_negative_callers
                  .insert({priority, CallerContainer{}})
                  .first->second.push_back(std::move(connection));
            }
            else if (priority > 0)
            {
              _container->_new_positive_callers
                  .insert({priority, CallerContainer{}})
                  .first->second.push_back(std::move(connection));
            }
            else
            {
              _container->_new_zero_callers.push_back(std::move(connection));
            }
          }
          return valid;
        }
        auto Add(EventConnection                  connection,
                 typename Event::InternalPriority priority) -> Validator
        {
          auto valid = Validator{connection.valid};
          if (_container->_mutex.try_lock())
          {
            auto lock = std::scoped_lock{std::adopt_lock, _container->_mutex};
            switch (priority)
            {
              case Event::InternalPriority::FIRST:
                _container->_first_callers.push_back(std::move(connection));
                break;
              case Event::InternalPriority::LAST:
                _container->_last_callers.push_back(std::move(connection));
                break;
              case Event::InternalPriority::CONDITION:
                _container->_conditions.push_back(std::move(connection));
                break;
            }
          }
          else
          {
            auto lock = std::scoped_lock{_container->_mutex_new};
            switch (priority)
            {
              case Event::InternalPriority::FIRST:
                _container->_new_first_callers.push_back(std::move(connection));
                break;
              case Event::InternalPriority::LAST:
                _container->_new_last_callers.push_back(std::move(connection));
                break;
              case Event::InternalPriority::CONDITION:
                _container->_new_conditions.push_back(std::move(connection));
                break;
            }
          }
          return valid;
        }
        auto Add(auto&& function, typename Event::InternalPriority priority)
            -> Validator
        {
          auto connection
              = CreateConnection(std::forward<decltype(function)>(function));
          auto ret = connection.valid;
          Add(std::move(connection), priority);
          return ret;
        }

       public:
        auto Add(auto&& function, int priority = 0)  //
            -> Validator                             //
        {
          auto connection
              = CreateConnection(std::forward<decltype(function)>(function));
          auto ret = connection.valid;
          Add(std::move(connection), priority);
          return ret;
        }
        template<class T>
        auto Add(auto&& function, int priority = 0)  //
            -> Validator                             //
        {
          auto connection
              = CreateConnection<T>(std::forward<decltype(function)>(function));
          auto ret = connection.valid;
          Add(std::move(connection), priority);
          return ret;
        }

        Event() noexcept : Event{nullptr} {}
        Event(std::shared_ptr<Manager> manager)
        {
          _container->_manager = std::move(manager);
        }

        ~Event() override       = default;
        Event(const Event&)     = default;
        Event(Event&&) noexcept = default;
        auto operator=(const Event&) -> Event& = default;
        auto operator=(Event&&) noexcept -> Event& = default;

        void SetManager(std::shared_ptr<Manager> manager)
        {
          auto lock            = std::scoped_lock{_container->_mutex_manager};
          _container->_manager = std::move(manager);
        }
};
template<class... Ts>
class Event : public Event<Data<>, Ts...>
{
 public:
  using Event<Data<>, Ts...>::Event;
  ~Event() override = default;
};

template<class... _Parameters>
class Handler
    : public UD::Interface::Interface<Handler<_Parameters...>,
                                      UD::Interface::SimpleModifiers>
{
  using FunctionType = std::function<Request(_Parameters...)>;
  struct Container
  {
    FunctionType function = [](_Parameters...)
    {
      return Request::Continue;
    };
    bool enabled = true;
  };

  std::shared_ptr<Container> _container = std::make_shared<Container>();

 public:
  ~Handler() override         = default;
  Handler(const Handler&)     = default;
  Handler(Handler&&) noexcept = default;
  auto operator=(const Handler&) -> Handler& = default;
  auto operator=(Handler&&) noexcept -> Handler& = default;

  Handler() noexcept = default;
  template<class F>
    requires std::convertible_to<F, FunctionType> Handler(F function)
        : _container{std::make_shared<Container>(std::move(function), true)}
    {
    }
    template<class F>
      requires(!std::convertible_to<F, FunctionType>)
    Handler(F function) : Handler
    {
      [function = std::move(function)](auto&&... ps)
      {
        function(std::forward<decltype(ps)>(ps)...);
        return Request::Continue;
      }
    }
    {
    }
    template<class T>
    Handler(Request (T::*function)(_Parameters...), T* type) : Handler
    {
      [type, function = std::move(function)](auto&&... ps)
      {
        return type->*function(std::forward<decltype(ps)>(ps)...);
      }
    }
    {
    }
    template<class T>
    Handler(void (T::*function)(_Parameters...), T* type) : Handler
    {
      [type, function = std::move(function)](auto&&... ps)
      {
        type->*function(std::forward<decltype(ps)>(ps)...);
        return Request::Continue;
      }
    }
    {
    }

   public:
    template<std::size_t _Size, class _EventType>
    void operator()(std::array<_EventType&, _Size> events, int priority = 0)
    {
      for (auto& e : events)
        operator()(e, priority);
    }
    template<class... Ts, class... Us>
      requires(sizeof...(Us) < sizeof...(_Parameters)
               && sizeof...(Us) == sizeof...(_Parameters) - 1)
    void operator()(Event<Data<Ts...>, Us...>& e, int priority = 0)
    {
      e.template Add<typename UD::Pack::Pack<_Parameters...>::Type>(
          [container = std::weak_ptr<Container>{_container}](auto&&... ps)
          {
            if (auto lock = container.lock())
            {
              if (lock->enabled)
              {
                return lock->function(std::forward<decltype(ps)>(ps)...);
              }
              return Request::Continue;
            }
            return Request::Remove;
          },
          priority);
    }
    template<class... Ts, class... Us>
      requires(sizeof...(Us) == sizeof...(_Parameters))
    void operator()(Event<Data<Ts...>, Us...>& e, int priority = 0)
    {
      e.Add(
          [container = std::weak_ptr<Container>{_container}](auto&&... ps)
          {
            if (auto lock = container.lock())
            {
              if (lock->enabled)
              {
                return lock->function(std::forward<decltype(ps)>(ps)...);
              }
              return Request::Continue;
            }
            return Request::Remove;
          },
          priority);
    }

    void Enable(bool enable = true)
    {
      _container->enabled = enable;
    }
    void Disable()
    {
      Enable(false);
    }
};
}  // namespace UD::Event