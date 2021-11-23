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
int main()
{
  UD::Event::Event<int>   e;
  UD::Event::Handler<int> h(
      [](int c)
      {
        static int i = 0;
        i += c;
      });
  h(e);
  std::chrono::milliseconds start
      = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch());

  std::function<void(int)> f{[](int s)
                             {
                               static int i = 0;
                               i += s;
                             }};
  for (std::size_t i = 0; i < 1000000; ++i)
  {
    e(1);
  }
  std::chrono::milliseconds end
      = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch());
  std::cout << "Time(ms): " << (end - start).count() << std::endl;
}