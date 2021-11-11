#include <chrono>
#include <iostream>
#include <string>

#include <libudph/Class/Event.h>
#include <libudph/Class/Interface.h>

int main()
{
  UD::Event::Queue<int> q;
  UD::Event::Event<int> e1;
  UD::Event::Event<int> e2;
  q(e1);
  UD::Event::Handler<int> h1(
      [](int i)
      {
        std::cout << i << std::endl;
      });
  UD::Event::Handler<int> h2(
      [](int i)
      {
        if (i % 100000 == 1)
        {
          std::cout << i << std::endl;
        }
      });
  h1(q);
  h2(e2);
  q.AddCondition(
      [](int s) -> bool
      {
        return (s % 100000) == 1;
      });

  auto start = std::chrono::high_resolution_clock::now().time_since_epoch();
  for (std::size_t i = 0; i < 1000000; ++i)
  {
    e1(i);
  }
  q.Process();
  auto end = std::chrono::high_resolution_clock::now().time_since_epoch();
  auto duration
      = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "Time: " << duration.count() << std::endl;

  start = std::chrono::high_resolution_clock::now().time_since_epoch();
  for (std::size_t i = 0; i < 1000000; ++i)
  {
    e1(i);
    q.Process();
  }
  end      = std::chrono::high_resolution_clock::now().time_since_epoch();
  duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "Time: " << duration.count() << std::endl;

  start = std::chrono::high_resolution_clock::now().time_since_epoch();
  for (std::size_t i = 0; i < 1000000; ++i)
  {
    e2(i);
  }
  end      = std::chrono::high_resolution_clock::now().time_since_epoch();
  duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "Time: " << duration.count() << std::endl;

  start = std::chrono::high_resolution_clock::now().time_since_epoch();
  for (std::size_t i = 0; i < 1000000; ++i)
  {
    std::function(
        [](int i)
        {
          if (i % 100000 == 1)
            std::cout << i << std::endl;
        })(i);
  }
  end      = std::chrono::high_resolution_clock::now().time_since_epoch();
  duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "Time: " << duration.count() << std::endl;

}