#include <atomic>
#include <mutex>
#include <iostream>

// atomic - �������� ����, ��� ������, ������ �������� ��� ����� �� �������.
// atomic - ��������� �������� (����� ����������), �� �� ������ ������ ������
// �������, ��� ����� �������� ��������� ����������.

// ����� ���� �������, ��� ������ std::atomic<std::vector> �� ����, ��� �����
// ����� ������������ ���������.

void atomic_dec_like_mutex(int& x, std::mutex& m)
{
  std::lock_guard<std::mutex> guard(m);
  --x;
}

// �����, ��� �� ����������� ����� �� �������� ����������, �� ����� ����
// ������������: ��� ��������, ��� �� ���� ��������� �������� � ������ �����
// ������������� ���������, � �� ���-�� ����� atomic_dec_like_mutex.

// ������������� ��� ���������� std::atomic_flag � ���� ��� ����� is_lock_free

void check_lock_free()
{
  std::cout << "is lock free" << std::endl;
  std::cout << "char\t" << std::atomic<char>().is_lock_free() << std::endl;
  std::cout << "int\t" << std::atomic<int>().is_lock_free() << std::endl;
  std::cout << "float\t" << std::atomic<float>().is_lock_free() << std::endl;
  std::cout << "double\t" << std::atomic<double>().is_lock_free() << std::endl;
}

// ������� �� ������ std::atomic_flag
// ���� ������ ���� ���������������� ATOMIC_FLAG_INIT, ��� ���������, �����
// �� ������� (����� �������� ��� false)
class spinlock_mutex
{
  std::atomic_flag _flag = ATOMIC_FLAG_INIT;
public:
  spinlock_mutex() /* :_flag(ATOMIC_FLAG_INIT) */ {}
  void lock()
  {
    while (_flag.test_and_set()); // ���� �������, ����������
    // like:
    // while(_flag);
    // _flag = true;
  }
  void unlock()
  {
    _flag.clear(); // ����������
  }
};

// ����� �������, ��� ��� ������������� ��������� ���, ���� �� ��������� �����,
// ��� ��� ������ ��������� ��� ��������� ��������, ���� ��� bool �� ���������
// ������������.

// ! �� ����� �������� ���������� ������ �� ������ ���������, �� ���� ������ ��
// ������ ������� � �������. ��� ��������� ��������, ��� �� ������� �� �������
// �������� ������ � ������ ��� ����� ���� compare_exchange, exchange.

std::atomic<bool> flag(false);
std::atomic<int>  v(0);

// load  - get
// store - set
// exchange - get and set(..) in one operation
void simple_read_write()
{
  bool x = flag.load();
  flag.store(true);
  x = flag.exchange(false);
}

void read_write()
{
  auto f = [](int x) {
    return 2 * x - 1;
  };
  int current = 0;
  int new_val = f(current);
  while (!v.compare_exchange_weak(current, new_val)) {
    new_val = f(current);
  }
}

// !!! double, float ����� ���� �������� �����, ��� ���� ����������
// ������������� ����� ���������� � ����� �������� �����

bool compare_exchange(std::atomic<int>& x, int& expected, int new_val) // ���������� ����, ��� ��� �� ������������ ���
{
  if (x.load() == expected) {
    x.store(new_val);
    return true;
  } else {
    expected = x.load();
    return false;
  }
}

// � ��������� �������� ��������� � ����� - ��� 2 ��������, ��� ��� ����� ����
// ����� ��������� ����� ��������� ��-�� ����� ���������� ������ ������.
// compare_exchange_weak   - ����� ������ ������ ������
// compare_exchange_strong - ����� ���� ������ ��������, �.�. ��������� �� ������ �����
// ���� ���� ����� ����������, ����� storng, ���� ��� weak.

void operations()
{
  v.fetch_add(1); // v++;     (������� �������� � ����������)
  v.fetch_sub(2); // v; v-=2; (������� �������� � ���������)
}

// std::atomic<T>
// T - ������ ����� ����������� �������� ������������ (��� ����.�������, �������)
// �������� ������������, ������ �������������� ������������. �� ���� ����������
// ����� ������������ memcpy � memcpm.

// ����� ���� �������� ��� ������ ������ � ����������� ���������� ������,
// ����������� ����� ����������� ����������. �� � ���� �� ������, ��� ���
// ����� �������� ������������ �������. �� ��������� ������ ���������
// ���������� ���� ����� ���� ������������ � ���� ������ �� ������ �����������
// �������������� ��������. ���� �� ��� ��� �� �������, ����� ��������
// �������������� �� ���� ����.

int main(int, const char **)
{
  check_lock_free();
  return 0;
}