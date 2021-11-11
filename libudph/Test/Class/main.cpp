#include <iostream>
#include <string>

#include <libudph/Class/Event.h>
#include <libudph/Class/Interface.h>

struct base
{
  base()
  {
    std::cout << "base()" << std::endl;
  }
  base(int s)
  {
    std::cout << "base(int)" << std::endl;
  }
  base(int s, int s2)
  {
    std::cout << "base(int, int)" << std::endl;
  }
  virtual ~base() {}
};
struct base2
{
  base2()
  {
    std::cout << "base2()" << std::endl;
  }
  base2(int s)
  {
    std::cout << "base2(int)" << std::endl;
  }
  base2(int s, int s2)
  {
    std::cout << "base2(int, int)" << std::endl;
  }
  base2(std::string s)
  {
    std::cout << "base2(str)" << std::endl;
  }
  virtual ~base2() {}
};
struct derived
    : public UD::Interface::Interface<derived, UD::Pack::Pack<base, base2>>
{
  using b = UD::Interface::Interface<derived, UD::Pack::Pack<base, base2>>;
  derived() : b{Constructor<base>::Call{2,2}} {}
};

template<class... Ts>
struct s
{
  std::tuple<Ts...> i;
};
int main()
{
  derived d;
  // UD::Event::Queue<int> q;
  // UD::Event::Event<int> e1;
  // UD::Event::Event<int> e2;
  // q(e1);
  // q(e2);

  // UD::Event::Handler<int> h1(
  //     [](int i)
  //     {
  //       std::cout << i;
  //     });
  // UD::Event::Handler<int> h2(
  //     [](int i)
  //     {
  //       std::cout << i;
  //     });

  // h1(e1);
  // h1(e2);
  // h2(e1);

  // e1(23);
  // e2(2);
}