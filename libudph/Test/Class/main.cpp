#include <iostream>
#include <string>

#include <libudph/Class/Event.h>
#include <libudph/Class/Interface.h>


int main()
{
  UD::Event::Queue<int>   q;
  UD::Event::Event<int>   e1;
  UD::Event::Event<int>   e2;
  q(e1);
  q(e2);
  UD::Event::Handler<int> h1(
      [](int i)
      {
        std::cout << i << std::endl;
      });
  UD::Event::Handler<int> h2(
      [](int i)
      {
        std::cout << i << std::endl;
      });
  h1(e1);
  h1(e2);
  h2(e1);
  e1(23);
}