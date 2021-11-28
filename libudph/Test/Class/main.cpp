#include <iostream>

#include <libudph/Class/Event.h>
int main()
{
  UD::Event::Event<int>  e1;
  UD::Event::Event<char> e2;

  UD::Event::Handler<UD::Event::State&, int> h1{[](UD::Event::State& s, int i)
                                                {
                                                  s.Skip();
                                                  std::cout << "h1: " << i
                                                            << std::endl;
                                                }};
  UD::Event::Handler<int>                    h2{[](int i)
                             {
                               std::cout << "h2: " << i << std::endl;
                             }};

  h1(e2);
  h2(e1);
  e2.Link(e1);
  e1(2);
}