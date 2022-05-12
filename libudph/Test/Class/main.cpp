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

Object::Event event;

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
  UD::Event::State s;

  auto handler = Object::Event::ET::StateHandler{
      [&](UD::Event::State& state, Object::Event::ID id)
      {
        s.Emplace<int>(3);
        std::cout << *state.Get<int>() << std::endl;
        event.et(s, Object::Event::ID::A);
      }};
  auto handler2 = Object::Event::ET::StateHandler{
      [](UD::Event::State& state, Object::Event::ID id)
      {
        std::cout << "here" << std::endl;
        std::cout << *state.Get<int>() << std::endl;
      }};
  handler(event.et);
  handler2(event.et);

  s.Emplace<int>(2);
  event.et(s, Object::Event::ID::A);
}