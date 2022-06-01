#include <iostream>

#include <libudph/Class/Event.h>

int main()
{
  using ev_type  = UD::Event::Event<UD::Event::Data<>>;
  auto ev        = ev_type{};
  using ev0_type = UD::Event::Event<int>;
  auto ev0       = ev0_type{};
  using ev1_type = UD::Event::Event<UD::Event::Data<int>>;
  auto ev1       = ev1_type{};
  using ev2_type = UD::Event::Event<UD::Event::Data<int>, int>;
  auto ev2       = ev2_type{};
  using ev3_type = UD::Event::Event<UD::Event::Data<int>, int, std::string>;
  auto ev3       = ev3_type{};
  using ev4_type = UD::Event::Event<UD::Event::Data<int, std::string>>;
  auto ev4       = ev4_type{};
  using ev5_type = UD::Event::Event<UD::Event::Data<int, std::string>, int>;
  auto ev5       = ev5_type{};
  using ev6_type
      = UD::Event::Event<UD::Event::Data<int, std::string>, int, std::string>;
  auto ev6 = ev6_type{};

  auto ha  = ev_type::Handler{[]()
                             {
                               printf("");
                               return UD::Event::Request::Continue;
                             }};
  auto ha0 = ev0_type::Handler{[](int)
                               {
                                 printf("");
                                 return UD::Event::Request::Continue;
                               }};
  auto ha1 = ev1_type::Handler<int>{[](int)
                                    {
                                      printf("");
                                      return UD::Event::Request::Continue;
                                    }};
  auto ha2 = ev2_type::Handler<int>{[](int, int)
                                    {
                                      printf("");
                                      return UD::Event::Request::Continue;
                                    }};
  auto ha3 = ev3_type::Handler{[](int, std::string)
                               {
                                 printf("");
                                 return UD::Event::Request::Continue;
                               }};
  auto ha4 = ev4_type::Handler{[]()
                               {
                                 printf("");
                                 return UD::Event::Request::Continue;
                               }};
  auto ha5 = ev5_type::Handler<int>{[](auto s, auto s2)
                                    {
                                      printf("");
                                      return UD::Event::Request::Continue;
                                    }};
  auto ha6 = ev6_type::Handler{[](int, std::string)
                               {
                                 printf("");
                                 return UD::Event::Request::Continue;
                               }};

  auto start = std::chrono::steady_clock::time_point{};
  auto end   = std::chrono::steady_clock::time_point{};

  auto i = 0;
  ev.Add(
      [&i]()
      {
        printf("");
      });
  ha(ev);
  ev0.Add(
      [&i](auto&&...)
      {
        printf("");
      });
  ha0(ev0);
  ev1.Add(
      [&i](auto&&...)
      {
        printf("");
      });
  ha1(ev1);
  ev2.Add(
      [&i](auto&&...)
      {
        printf("");
      });
  ha2(ev2);
  ev3.Add(
      [&i](auto&&...)
      {
        printf("");
      });
  ha3(ev3);
  ev4.Add(
      [&i](auto&&...)
      {
        printf("");
      });
  ha4(ev4);
  ev5.Add<int>(
      [&i](int s, auto&&...)
      {
        printf("");
      });
  ha5(ev5);
  ev6.Add(
      [&i](auto&&...)
      {
        printf("");
      });
  ha6(ev6);

  static const std::size_t loop_count = 100000;
  std::cout << "ev: ";
  start = std::chrono::steady_clock::now();
  for (auto i = 0; i < loop_count; ++i)
    ev();
  end = std::chrono::steady_clock::now();
  std::cout << std::chrono::duration<float>(end - start).count() << std::endl;

  std::cout << "ev0: ";
  start = std::chrono::steady_clock::now();
  for (auto i = 0; i < loop_count; ++i)
    ev0(1);
  end = std::chrono::steady_clock::now();
  std::cout << std::chrono::duration<float>(end - start).count() << std::endl;

  std::cout << "ev1: ";
  start = std::chrono::steady_clock::now();
  for (auto i = 0; i < loop_count; ++i)
    ev1(1);
  end = std::chrono::steady_clock::now();
  std::cout << std::chrono::duration<float>(end - start).count() << std::endl;

  std::cout << "ev2: ";
  start = std::chrono::steady_clock::now();
  for (auto i = 0; i < loop_count; ++i)
    ev2(1, 2);
  end = std::chrono::steady_clock::now();
  std::cout << std::chrono::duration<float>(end - start).count() << std::endl;

  std::cout << "ev3: ";
  start = std::chrono::steady_clock::now();
  for (auto i = 0; i < loop_count; ++i)
    ev3(1, 2, "s");
  end = std::chrono::steady_clock::now();
  std::cout << std::chrono::duration<float>(end - start).count() << std::endl;

  std::cout << "ev4: ";
  start = std::chrono::steady_clock::now();
  for (auto i = 0; i < loop_count; ++i)
    ev4(1);
  end = std::chrono::steady_clock::now();
  std::cout << std::chrono::duration<float>(end - start).count() << std::endl;

  std::cout << "ev5: ";
  start = std::chrono::steady_clock::now();
  for (auto i = 0; i < loop_count; ++i)
    ev5(4, 2);
  end = std::chrono::steady_clock::now();
  std::cout << std::chrono::duration<float>(end - start).count() << std::endl;

  std::cout << "ev6: ";
  start = std::chrono::steady_clock::now();
  for (auto i = 0; i < loop_count; ++i)
    ev6(1, 2, "s");
  end = std::chrono::steady_clock::now();
  std::cout << std::chrono::duration<float>(end - start).count() << std::endl;

  std::cout << "noev0: ";
  start = std::chrono::steady_clock::now();
  for (auto i = 0; i < loop_count; ++i)
  {
    [&i](auto&&...)
    {
      printf("");
    }();
    [&i](auto&&...)
    {
      printf("");
    }();
  }
  end = std::chrono::steady_clock::now();
  std::cout << std::chrono::duration<float>(end - start).count() << std::endl;

  std::cout << "noev1: ";
  start = std::chrono::steady_clock::now();
  for (auto i = 0; i < loop_count; ++i)
  {
    [&i](auto&&...)
    {
      printf("");
    }(1);
    [&i](auto&&...)
    {
      printf("");
    }(1);
  }
  end = std::chrono::steady_clock::now();
  std::cout << std::chrono::duration<float>(end - start).count() << std::endl;

  std::cout << "noev2: ";
  start = std::chrono::steady_clock::now();
  for (auto i = 0; i < loop_count; ++i)
  {
    [&i](auto&&...)
    {
      printf("");
    }(1, 2);
    [&i](auto&&...)
    {
      printf("");
    }(1, 2);
  }
  end = std::chrono::steady_clock::now();
  std::cout << std::chrono::duration<float>(end - start).count() << std::endl;

  std::cout << "noev3: ";
  start = std::chrono::steady_clock::now();
  for (auto i = 0; i < loop_count; ++i)
  {
    [&i](auto&&... q)
    {
      printf("");
    }(1, 2, "s");
    [&i](auto&&... q)
    {
      printf("");
    }(1, 2, "s");
  }
  end = std::chrono::steady_clock::now();
  std::cout << std::chrono::duration<float>(end - start).count() << std::endl;

  std::cout << "noev4: ";
  start = std::chrono::steady_clock::now();
  for (auto i = 0; i < loop_count; ++i)
  {
    printf("");
    printf("");
  }
  end = std::chrono::steady_clock::now();
  std::cout << std::chrono::duration<float>(end - start).count() << std::endl;

  auto m1 = std::make_shared<UD::Event::Manager>();
  ev2.SetManager(m1);

  std::cout << "ev2 queue: ";
  start = std::chrono::steady_clock::now();
  for (auto i = 0; i < loop_count; ++i)
    ev2(1);
  end = std::chrono::steady_clock::now();
  std::cout << std::chrono::duration<float>(end - start).count() << std::endl;

  std::cout << "ev2 call: ";
  start = std::chrono::steady_clock::now();
  m1->Run();
  end = std::chrono::steady_clock::now();
  std::cout << std::chrono::duration<float>(end - start).count() << std::endl;
}
