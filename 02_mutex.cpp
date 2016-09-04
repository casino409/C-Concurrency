#include <mutex>
#include <condition_variable>
#include <queue>

// Mutex - mutural execution.
// ���������������� �� ������ ������/���� �� �������������� �������/����������
// �������� ����� ���������, ������� ������ ������ ����������� ����� ������������

std::mutex m;
std::mutex m1, m2;
std::condition_variable flag;
std::queue<int> Q;

void mutex_simple_using()
{
  m.lock();
  // save code
  m.unlock();
}

// ���� ��������� ���������� ������ ��������� ���������������!

void mutex_simple_using_RAII()
{
  std::lock_guard<std::mutex> guard(m);
  // save code
}

// lock_guard ����� ������� � ��� ������ ����������������
// unique_lock ����� ���������� ����������� �������

void mutex_simple_using_RAII_2()
{
  std::unique_lock<std::mutex> guard(m);
  // save code
  guard.unlock();
  // not save code
}

// lock_guard, unique_lock ������������� ����������� �������
//
// unique_lock �������� � ���� ����, ����� �������� ������� �� �� ��������� �
// ����� ��� ������������ ������� unlock(), � ���� �� ������� �� �����������
// ����� �����������.

// ������������� ������������� ���������� ����������
// �������� �������� - �������� ����������. ��������, ����� 2 �����������
// ���� ������������ ������ ������� � ��� ���� ������ �� ��� ������� �����
// ��������.

// Ex: t1 �������� l1, t2 �������� l2. t1 ��� ���������� �������� ���������
//     �������� l2, � t2 - l1. � ��� ��� ����������� ����-�� ����.

// ������ ���������� ��������� ��� �������� ����������
// ����� �������� ������ (� �� ������ ����������� � ����), � ����� �� ���
// std::adopt_lock - ��������� �������
// std::defer_lock - �� ���������

void lock_several_mutex()
{
  std::lock(m1, m2); // !!! m1, m2 ������ ���� ������ ��������
  std::lock_guard<std::mutex> lock_1(m1); // == lock_1(m1, std::adopt_lock)
  std::lock_guard<std::mutex> lock_2(m2);
  // save code
}

// � unique_lock ���� ����������� �������� ������� � ����� ��� ���������.

void lock_several_mutex2()
{
  std::unique_lock<std::mutex> lock_1(m1, std::defer_lock);
  std::unique_lock<std::mutex> lock_2(m2, std::defer_lock);
  std::lock(lock_1, lock_2);
  // save code
}

// ������������� (�������� ����-��)
// �������� �������� ������� ����������� ����� conditional_varible

bool have_more_data()
{
  return true;
}

void main_worker()
{
  while (have_more_data()) {
    int x = rand();
    std::lock_guard<std::mutex> guard(m);
    Q.push(x);
    flag.notify_one();
  }
}

void worker()
{
  while (true) {
    std::unique_lock<std::mutex> guard(m);
    flag.wait(guard, [] {return !Q.empty(); });
    int x = Q.front();
    Q.pop();
    guard.unlock();
    // some work with x
    // some exit condition like: if(is_last_data) break;
  }
}

// flag.wait(guard, [] {return !Q.empty; });
// �) ������������ unique_lock, ����� ����� ���� ���������� ������� �
//    ������� ������ �� �������� ����������
// �) ���� ����������� ������������ ������� �������� ������� ������������
//

// ����������� ������� ��������

// ���� ������������ ���������� �����, � �� ����������, ���� ����� ���� �
// �������� ����������, �.�. ��-�� ������ ����������� ����� �� ������ �����
// ���������� ������ ������ (��� 142)
//
// auto const timer = std::chrono::steady_clock::now() + std::chrono::milliseconds(10);
// ...
//  wait_until(timer);

void worker_limited() // ������������� �������� �� �������� 10ms (�������� ����� ������)
{
  while (true) {
    auto const timer = std::chrono::steady_clock::now() + std::chrono::milliseconds(10);
    std::unique_lock<std::mutex> guard(m);
    flag.wait_until(guard, timer, [] {return !Q.empty(); });
    int x = Q.front();
    Q.pop();
    guard.unlock();
    // some work with x
    // some exit condition like: if(is_last_data) break;
  }
}