#include <mutex>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>

// ����� ���������� ��������� ����������, � �� ������ ������ ������. ��� �����
// ���� ����������� ���������������� - futures.
//
// ����� ������ �������, ������ future ������� ������������ ������ thread - ���
// ��������� ����������. �������������� ����������, ����������� � ����������
// ���������� ������� �������� � ����� ������ get(), ��� ��� ��� ����� ����
// ��������� ����������

// future ����� �������� ����������� ���������:
// 1) async
// 2) packaged_task
// 3) promise
// �� ���� future - ������, ������� ����� �������� ���������� ����� �� ���
// ����������. � packaged_task ������ ���������� - ��� ���������� ������, �
// promise ������������ ��� �������� ������ ������ set_value(..)

int f(int a)
{
  return a+1;
}

int f_ex(int)
{
  throw std::exception("Critical error");
  return 1;
}

int long_task(unsigned ms)
{
  static int res = 0;
  std::this_thread::sleep_for(std::chrono::microseconds(ms));
  return ++res;
}

void wait_function_call()
{
  std::future<int> w = std::async(f, 5);
  // some work
  int res = w.get();
}

void wait_function_call_ex()
{
  std::future<int> w = std::async(f_ex, 5);
  try {
    int res = w.get();
  } catch (std::exception& e) {
    std::cerr << e.what();
  }
}

// �������� ���������� ��� async ����� ����� �� ����������� ��� thread.
//
// !!! .get() ����� �������� ������ 1 ���, ��� �������� ����� ������� ����
//     shared_future<>
//
//

void wait_function_call_2()
{
  std::future<int> w = std::async(std::launch::deferred, f, 5);
  // some work
  w.wait(); // ����� � �������� ���������� ������� (�������������) -> get() �������������
  // some work
  int res = w.get();
}

// ���� �� ��������� ������ �������, �� ������� ������ �� ���� ����������
// ��������, ����� �������� ��������� � ���� ������, �������� ����� ������
// ����� �����, ������� ��������, �� ��� ����� �������� ��� � ������.
//
// std::launch::async    - ������������� ��������� � ��������� ������
// std::launch::deferred - �������� ����� �� ������ wait() ��� get()

// ���� ������� .wait(), �� .get() �������������, ���� ��
// �������� .wait() - �����������.

// ����� ��������� ��������� �����, � �� ����������� ������������� � ��������
// ������ �� ���������.

void checking_status()
{
  std::future<int> f = std::async(long_task, 1000);
  // some work
#ifndef _MSC_VER
  if (f.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
#else
  if (f._Is_ready())
#endif // !_MS
  {
    std::cout << f.get(); // g(f) some operation with result
  }
}

void checking_status_2()
{
  std::future<int> f = std::async(long_task, 1000);
  while (true) {
    switch (f.wait_for(std::chrono::milliseconds(100))) {
    case std::future_status::deferred: // ������ ��������
      std::cout << "future_status::deferred" << std::endl;
      continue;
    case std::future_status::timeout: // ����� ����� �������� (������ �����������)
      std::cout << "future_status::timeout" << std::endl;
      continue;
    case std::future_status::ready: // ��������� �����
      std::cout << "future_status::ready" << std::endl;
      f.get();
      break;
    }
    break;
  }
}

// move future result ?

void simple_shared_furure()
{
  std::thread w1, w2;
  std::mutex m; // just for pretty print
  {
    std::shared_future<int> res = std::async(long_task, 1000);
    // ! copy res
    auto t = [&, res] { std::this_thread::sleep_for(std::chrono::microseconds(1000));
                        res.wait();
                        std::lock_guard<std::mutex> g(m);
                        std::cout << "get res = " << res.get() << std::endl;
                      };
    w1 = std::thread(t);
    w2 = std::thread(t);
    // leave block and destroy shared_future res, but there are 2 copies of it
    // in workings threads
  }
  w1.join();
  w2.join();
}

// packed task, promise - ����� ������� � ����� ������ ���� ����� ���.

// std::packaged_task - ���� ��� � async (���� ������, � ���. ���� ���������)
//                      �� ������ ����� ������ ���������� �� ���������� �
// �� ���������� �������� future, ��������� � ���� ������� ����� �� ����, ���
// ������, ����� ������ ���������, � ��������� ����� �� ����.

// task - �� ���� ������, �� ��� ���������� ��� �������� thread, �� ����
//        ����� � ��� ������ ��� ���������.

void task_lambda()
{
  std::packaged_task<int(int)> task(f);
  std::future<int> result = task.get_future();
  task(2);
  std::cout << "task_lambda:\t" << result.get() << '\n';
}

void task_bind()
{
  std::packaged_task<int()> task(std::bind(f, 2));
  std::future<int> result = task.get_future();
  task();
  std::cout << "task_bind:\t" << result.get() << '\n';
}

void task_thread()
{
  std::packaged_task<int(int)> task(f);
  std::future<int> result = task.get_future();
  std::thread task_td(std::move(task), 2);
  task_td.join();
  std::cout << "task_thread:\t" << result.get() << '\n';
}

// promise

// for example you can pass a value to new thread without need of any
// additional synchronizing mechanism.
void simple_promise()
{
  auto task = [](std::future<int> i) {
    std::cout << i.get() << std::flush;
  };
  std::promise<int> p;
  std::thread t{ task, p.get_future() };
  std::this_thread::sleep_for(std::chrono::seconds(5));
  p.set_value(5); // ����� ����� ����� ����� � ����� i.get()
  t.join();
}

// ���� ����� ������, � �� promise � ���������� �� ������������� + ����������
// �� ����� (���������). � promise ���� �������� ������� ��� ������, � ����
// ������� ��������, � ������ ���� �� �� ���� �������� �� ����� �����.