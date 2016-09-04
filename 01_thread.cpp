#include <thread>    //
#include <chrono>    // std::chrono::milliseconds
#include <iostream>  // std::cout,
#include <cassert>   // assert
#include <vector>    // vector
#include <algorithm> // for_each
#include <memory>    // unique_ptr

// ��������� ����������
// assert ���������� �������, ������� ������ �����������, ��������
//   assert(false); - ��� ������������� ����� ����

void f()
{
  std::cout << "Run function f()";
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::cout << "Task done!";
}
void f_ex()
{
  std::cout << "There is some exception while running f_ex()";
  throw;
}

// Do some work (execute function without parameters and return value)
void simple_run_and_wait()
{
  std::thread worker(f);
  // some no except code
  worker.join();         // wait until work is done
}

// ��� �� ��������, ����������, ������� ������� �� �������, ����������� ��
// ������������ � ��������� ���������� (��� ����� �� main ����� ����������),
// ��� �������� � ������ std::terminate, �.�. ������ ���������� ������
// ��������������
// @see https://akrzemi1.wordpress.com/2012/11/14/not-using-stdthread/
//
void simple_run_and_wait_ex() // ! BAD EXAMPLE !
{
  std::thread worker(f_ex);
  try {
    worker.join();
  } catch (...) {
    assert(false);
    std::cerr << "Undefined error happened";
  }
}

// Th.
// ����� ������������� std::thread, �� ������ ����������� ����������� ������
// ������ ����� ������ �� ������� .join() ��� .detach(), ����� ����� ������
// ���� ����� 1 ���! (������ ��� ������������ ���-�� ������ ��������)
//
// .detach() ��������� ��������� ������� ������, ����� ����� ������ ����� �����
//   ����� ������.
//
// .joinable() ���� ������ �������� ������ ��������:
//   std::thread worker(work);
//   worker.detach();
//   assert(!worker.joinable());
//
// std::thread - ��� ������������ ������, �� ������������.
//   ����� ��������� ������� std::thread ������������� �� ��������� (��� ������)
//   ��� �� ������� �����, � ������ ��������� ������, ����� ����������� ���
//   ����� ��������������. ������� ��������� ��������� ���������� ������
//   ��������� �������� � ���������� ����������.
//

void thread_move_example()
{
  std::thread worker1(f);
  std::thread worker2 = std::move(worker1);
  worker1 = std::thread(f);
  std::thread worker3;
  worker3 = std::move(worker2);
}

// Th.
// ������� ������� std::thread ����� ���������� �� �������� � ������ ���:
// std::thread new_worker();
// void some_work(std::thread worker);
//
// ... some_work(std::move(...));

// � ����� �� ������ � ������ �������, ����� ������� ����� �����
class scoped_thread
{
  std::thread _t;
public:
  explicit scoped_thread(std::thread t) : _t(std::move(t))
  {
    if (!t.joinable())
      throw std::logic_error("No thread");
  }
  ~scoped_thread()
  {
    _t.join();
  }
  scoped_thread(const scoped_thread&) = delete;
  scoped_thread operator=(const scoped_thread&) = delete;
};

void simple_run_and_wait_RAII()
{
  scoped_thread(std::thread(f));
  f();
}

class operation
{
  int _d;
public:
  explicit operation() : _d(0) {}
  void operator()() {}
};

void run_and_wait_RAII()
{
  operation f2;
  scoped_thread(std::thread(f2));
}

void simple_run_many_and_wait()
{
  using std::begin;
  using std::end;
  using std::mem_fn;
  using std::thread;
  using std::for_each;
  std::vector<thread> worker;
  for (unsigned i = 0; i < 8; ++i)
    worker.push_back(thread(f));
  for_each(begin(worker), end(worker), mem_fn(&thread::join));
}

// Th. ����� � �����������
// 0) �������� ����������
// ���������� ����������� ���������� (��� ����������� ��������), �����������
// (��� ���������) ��� �������� ������, � �� ��������������� (��� �����������)
// ���������� � ��������� ������ ������.
// ������������� ��������������� ���������� ��� ���������� �� ���� ����������
// �������� � ������� ������.
//
// ��������� ������ ������ ������ ������ � ��� ���� ��������� ���������
// 1) operator()() - �������, ��� �� ����
// 2) bind
// 3) lambda function

void g(const std::string&) {}

void incorrect_using_parameters()
{
  char str[10] = "Hel";
  std::thread t(g, str);
  t.detach();
  memset(str, 0, sizeof(str));
  // �������� ������ ����� �����
  // � ��������� ������ ������ ���������� ��������������� string �� char*
}

void using_parameters()
{
  char str[10] = "Hel";
  std::thread t(g, std::string(str)); // ������������ string
  t.detach();
  memset(str, 0, sizeof(str));
  // �������� ������ ����� �����
  // � ��������� ������ ������ ����������������� string
}

void g1(std::unique_ptr<int> a) {}

void using_parameters_move()
{
  std::unique_ptr<int> x = std::make_unique<int>(5);
  std::thread t(g1, std::move(x));
  // or std::thread t(g1, std::make_unique<int>(5));
  t.join();
}

void g2(int, int& res)
{
  res = 5;
}

// VS not compile this
//
// void incorrect_using_alias_parameter()
// {
//   int res = -1;
//   std::thread t(g2, 0, res);
//   // ���������� ����������� res � ��������� ������ ������, ����� ���� � �������
//   // ���������� ������ �� �����, ��� � ������������ ���������, � ��������.
//   t.join();
//   assert(res == -1);
// }

void using_alias_parameter()
{
  int res = -1;
  std::thread t(g2, 0, std::ref(res)); // ���� �������, ��� �������� ������
  t.join();
  assert(res == 5);
}

// ����� ������
class Obj
{
public:
  void do_smth() {}
};

void method_calling()
{
  Obj x;
  std::thread t(&Obj::do_smth, &x);
  t.join();
}

// TODO
// 2) Bind
// 3) lambda

// Summary
// ����������� ������ ������ ������� ������� ��� ����������, � �����������, ���
// ��������� ������. �������� ���������� ������.