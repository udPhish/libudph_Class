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

static const std::size_t iterations = 10000000;
template<class... Ts>
void time_event(UD::Event::Event<Ts...>& e)
{
  std::chrono::milliseconds start
      = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch());
  for (std::size_t i = 0; i < iterations; ++i)
  {
    e(Ts{}...);
  }
  std::chrono::milliseconds end
      = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch());
  std::cout << "Ev Time: " << (end - start).count() << std::endl;
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
  std::cout << "Op Time: " << (end - start).count() << std::endl;
}
template<class... Ts>
void callme()
{
  std::function<void(int&, Ts&&...)> func;
  static_assert(UD::Concept::Callable<decltype(func), void(int&, Ts...)>);
}

template<class... Ts>
void TestOp(Ts... ts)
{
  static int val = 0;
  val += 1;
}
int main()
{
  // callme<int, char>();
  UD::Event::Event<int, char>                e;
  UD::Event::Handler<UD::Event::State&, int> h1(
      &TestOp<UD::Event::State&, int>);
  UD::Event::Handler<int, char>                    h2(&TestOp<int, char>);
  UD::Event::Handler<UD::Event::State&, int, char> h3(
      &TestOp<UD::Event::State&, int, char>);

  // h2(e);
  h1(e,
     [](int&& i, char&& c)
     {
       return std::tuple<int>(std::forward<char>(c));
     });
  // h3(e);

  time_event(e);
  time_op<int, char>(&TestOp<int, char>);
}