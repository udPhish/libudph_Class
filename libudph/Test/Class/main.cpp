#include <array>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include <libudph/Class/Event.h>
#include <libudph/Class/Interface.h>

struct conv
{
  std::tuple<int> operator()(int c)
  {
    return c;
  }
};

static const std::size_t iterations = 100000000;
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
  //e.ProcessWhile(
  //    [start]()
  //    {
  //      static std::chrono::milliseconds end;
  //      end = std::chrono::duration_cast<std::chrono::milliseconds>(
  //          std::chrono::high_resolution_clock::now().time_since_epoch());
  //      return (end - start).count() < 100;
  //    });
  //e.ProcessIf(
  //    []()
  //    {
  //    static int s = 0;
  //    return (bool)(s++%4);
  //    });
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
  //q.Purge(
  //    [](int i)
  //    {
  //      return i == 1;
  //    });
  q.Purge();
  std::cout << q.Size() << std::endl;
  std::cout << val << std::endl;
}