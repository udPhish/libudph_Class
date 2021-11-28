#include <iostream>

#include <libudph/Class/Event.h>
int main()
{
  UD::Event::Event<int>  e1;
  UD::Event::Event<char> e2;

  UD::Event::Handler<UD::Event::State&, int> h1{[](UD::Event::State& s, int i)
                                                {
                                                  std::cout << "h1: " << i
                                                            << std::endl;
                                                }};
  UD::Event::Handler<int>                    h2{[](int i)
                             {
                               std::cout << "h2: " << i << std::endl;
                             }};

  e2.AddCondition(
      []()
      {
        return false;
      });
  h1(e1);
  h2(e2);
  e2.Link(e1);
  e1(2);
}