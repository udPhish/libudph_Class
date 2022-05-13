#include <iostream>

#include <libudph/Class/Event.h>

struct Object
{
  struct Event
  {
    enum class ID
    {
      A,
      B,
      C,
      D
    };
    template<ID id>
    struct Data;
    using ET = UD::Event::Event<ID>;
    ET et;
  };
};

template<>
struct Object::Event::Data<Object::Event::ID::A>
{
  using type = int;
};
template<>
struct Object::Event::Data<Object::Event::ID::B>
{
  using type = char;
};

int main()
{
  auto m     = std::make_shared<UD::Event::Manager>();
  auto s     = UD::Event::State{};
  auto event = Object::Event{m};

  auto handler = Object::Event::ET::StateHandler{
      [&](UD::Event::State& state, Object::Event::ID id)
      {
        s.Emplace<int>(3333);
        std::cout << "1" << std::endl;
        event.et(s, Object::Event::ID::A);
        event.et(s, Object::Event::ID::A);
      }};
  auto handler2 = Object::Event::ET::StateHandler{
      [](UD::Event::State& state, Object::Event::ID id)
      {
        std::cout << "2" << std::endl;
      }};
  handler(event.et);
  handler2(event.et);

  s.Emplace<int>(299);
  event.et(s, Object::Event::ID::A);

  std::cout << "--" << std::endl;
  m->RunAll();
  std::cout << "--" << std::endl;
  m->RunAll();
  std::cout << "--" << std::endl;
  m->RunAll();
  std::cout << "--" << std::endl;
}