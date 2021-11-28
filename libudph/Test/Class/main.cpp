#include <array>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include <libudph/Class/Event.h>
#include <libudph/Class/Interface.h>
#include <libudph/Class/SFINAE.h>

struct conv
{
  std::tuple<int> operator()(int c)
  {
    return c;
  }
};

static const std::size_t iterations = 1000000;
template<class... Ts>
void time_event(UD::Event::Event<Ts...>& e)
{
  std::chrono::milliseconds start
      = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch());
  for (std::size_t i = 0; i < iterations; ++i)
  {
    e(Ts{1}...);
  }
  std::chrono::milliseconds end
      = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch());
  std::cout << "E Time: " << (end - start).count() << std::endl;
}
template<class... Ts>
void time_queue(UD::Event::Queue<Ts...>& e)
{
  std::chrono::milliseconds start
      = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch());
  e.Process();
  // e.ProcessWhile(
  //     [start]()
  //     {
  //       static std::chrono::milliseconds end;
  //       end = std::chrono::duration_cast<std::chrono::milliseconds>(
  //           std::chrono::high_resolution_clock::now().time_since_epoch());
  //       return (end - start).count() < 100;
  //     });
  // e.ProcessIf(
  //     []()
  //     {
  //     static int s = 0;
  //     return (bool)(s++%4);
  //     });
  std::chrono::milliseconds end
      = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch());
  std::cout << "Q Time: " << (end - start).count() << std::endl;
}
template<class... Ts>
void time_op(UD::Concept::Invocable<Ts...> auto f)
{
  std::chrono::milliseconds start
      = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch());
  for (std::size_t i = 0; i < iterations; ++i)
  {
    f(Ts{}...);
  }
  std::chrono::milliseconds end
      = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch());
  std::cout << "O Time: " << (end - start).count() << std::endl;
}
template<class... Ts>
void callme()
{
  std::function<void(int&, Ts&&...)> func;
  static_assert(UD::Concept::Callable<decltype(func), void(int&, Ts...)>);
}

static int val = 0;
template<class... Ts>
void TestOp(Ts... ts)
{
  if (val >= 0)
    val += 1;
}
template<class... Ts>
void caller(Ts&&... ts)
{
  TestOp<Ts...>(std::forward<Ts>(ts)...);
}
int main()
{
  UD::Event::Event<int>                      e;
  UD::Event::Queue<int>                      q;
  UD::Event::Handler<UD::Event::State&, int> h1(
      &TestOp<UD::Event::State&, int>);

  h1(e);
  time_event(e);
  h1();
  h1(e,
     [](int&& l)
     {
       return std::tuple<int>(std::forward<int>(l));
     });
  time_event(e);
  h1();
  q(e);
  h1(q);
  time_event(e);
  std::cout << q.Size() << std::endl;
  time_queue(q);
  time_op<int>(&caller<int>);
  std::cout << q.Size() << std::endl;
  // q.Purge(
  //     [](int i)
  //     {
  //       return i == 1;
  //     });
  q.Purge();
  std::cout << q.Size() << std::endl;
  std::cout << val << std::endl;

  UD::Event::Event<>   ee;
  UD::Event::Handler<> hh1(
      []()
      {
        std::cout << "P1: " << -1 << std::endl;
      });
  UD::Event::Handler<> hh12(
      []()
      {
        std::cout << "P12: " << -1 << std::endl;
      });
  UD::Event::Handler<> hh2(
      []()
      {
        std::cout << "P: " << -100 << std::endl;
      });
  UD::Event::Handler<> hh3(
      []()
      {
        std::cout << "P3: " << 1 << std::endl;
      });
  UD::Event::Handler<> hh32(
      []()
      {
        std::cout << "P32: " << 1 << std::endl;
      });
  UD::Event::Handler<> hh4(
      []()
      {
        std::cout << "P: " << 2708 << std::endl;
      });
  UD::Event::Handler<> hh5(
      []()
      {
        std::cout << "P: " << 0 << std::endl;
      });
  hh2(ee, -100);
  hh12(ee, -1);
  hh1(ee, -1);
  hh32(ee, 1);
  hh3(ee, 1);
  hh4(ee, 2708);
  hh5(ee, 0);
  ee();

  UD::Event::Event<>                    es;
  UD::Event::Handler<UD::Event::State&> esh{[](UD::Event::State& state)
                                            {
                                              state.Emplace<int>(3);
                                            }};
  UD::Event::Handler<UD::Event::State&> esh2{[](UD::Event::State& state)
                                             {
                                               std::cout << *state.Get<int>()
                                                         << std::endl;
                                             }};
  esh(es);
  esh2(es);
  es();

  UD::Event::Chain<> c1;
  UD::Event::Chain<> c2;
  UD::Event::Chain<> c3;
  UD::Event::Chain<> c4;
  c2.Link(c1);
  c3.Link(c2);
  c4.Link(c3);
  UD::Event::Handler<UD::Event::State&> ch1{[](UD::Event::State& state)
                                            {
                                              state.Emplace<int>(1);
                                              std::cout << "ch1" << std::endl;
                                            }};
  UD::Event::Handler<UD::Event::State&> ch12{[](UD::Event::State& state)
                                             {
                                               (*state.Get<int>())++;
                                               std::cout << "ch12" << std::endl;
                                             }};
  UD::Event::Handler<UD::Event::State&> ch2{[](UD::Event::State& state)
                                            {
                                              (*state.Get<int>())++;
                                              std::cout << "ch2" << std::endl;
                                            }};
  UD::Event::Handler<UD::Event::State&> ch3{[](UD::Event::State& state)
                                            {
                                              (*state.Get<int>())++;
                                              std::cout << "ch3" << std::endl;
                                            }};
  UD::Event::Handler<UD::Event::State&> ch4{[](UD::Event::State& state)
                                            {
                                              (*state.Get<int>())++;
                                              std::cout << "ch4" << std::endl;
                                            }};
  ch1(c1, 1);
  ch2(c2);
  ch3(c3);
  ch4(c4);
  ch12(c1, 89);
  c1();
}