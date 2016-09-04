#include <atomic>
#include <mutex>
#include <iostream>

// atomic - гарантия того, что запись, чтение проходят без гонки за данными.
// atomic - неделимая операция (вроде транзакции), ни из одного потока нельзя
// увидеть, что такая операция выполнена наполовину.

// Сразу надо сказать, что писать std::atomic<std::vector> не надо, для этого
// нужно пользоваться мьютексом.

void atomic_dec_like_mutex(int& x, std::mutex& m)
{
  std::lock_guard<std::mutex> guard(m);
  --x;
}

// плюсы, что по одтельности можно не защищать переменные, но нужно быть
// внимательным: нет гарантии, что на этой платформе операция с данным типом
// действительно атомарная, а не что-то вроде atomic_dec_like_mutex.

// Гарантировано без блокировок std::atomic_flag у него нет члена is_lock_free

void check_lock_free()
{
  std::cout << "is lock free" << std::endl;
  std::cout << "char\t" << std::atomic<char>().is_lock_free() << std::endl;
  std::cout << "int\t" << std::atomic<int>().is_lock_free() << std::endl;
  std::cout << "float\t" << std::atomic<float>().is_lock_free() << std::endl;
  std::cout << "double\t" << std::atomic<double>().is_lock_free() << std::endl;
}

// Мьютекс на основе std::atomic_flag
// флаг всегда надо инициализировать ATOMIC_FLAG_INIT, это состояние, когда
// он сброшен (можно понимать как false)
class spinlock_mutex
{
  std::atomic_flag _flag = ATOMIC_FLAG_INIT;
public:
  spinlock_mutex() /* :_flag(ATOMIC_FLAG_INIT) */ {}
  void lock()
  {
    while (_flag.test_and_set()); // если сброшен, выставляет
    // like:
    // while(_flag);
    // _flag = true;
  }
  void unlock()
  {
    _flag.clear(); // сбрасывает
  }
};

// Можно сказать, что это исключительно системный тип, ведь он настолько прост,
// что его нельзя прочитать без изменения значения, даже как bool не получится
// использовать.

// ! мы знаем значение переменной только на момент прочтения, по сути только на
// момент времени в прошлом. Это несколько неудобно, все же хочется по старому
// значению прийти к новому для этого есть compare_exchange, exchange.

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

// !!! double, float могут быть численно равны, при этом внутреннее
// представление будет отличаться и будем получать отказ

bool compare_exchange(std::atomic<int>& x, int& expected, int new_val) // представим себе, что это не параллельный код
{
  if (x.load() == expected) {
    x.store(new_val);
    return true;
  } else {
    expected = x.load();
    return false;
  }
}

// В некоторых системах сравнение и обмен - это 2 операции, так что между ними
// может произойти смена контекста из-за этого появляются ложные отказы.
// compare_exchange_weak   - может давать ложные отказы
// compare_exchange_strong - может чуть дольше работать, т.к. проверяет на ложный отказ
// если цикл очень трудоемкий, лучше storng, если нет weak.

void operations()
{
  v.fetch_add(1); // v++;     (возврат текущего и добавление)
  v.fetch_sub(2); // v; v-=2; (возврат текущего и вычитание)
}

// std::atomic<T>
// T - должен иметь ТРИВИАЛЬНЫЙ оператор присваивания (нет вирт.функций, классов)
// оператор присваивания, должен генерироваться компилятором. По сути компилятор
// будет пользоваться memcpy и memcpm.

// Далее надо говорить про модель памяти и особенности обновления данных,
// разделенных между несколькими процессами. Но я пока не уверен, что это
// может принести значительный выигрыш. По умолчанию каждое изменение
// атомарного типа видно всем исполнителям и этот подход не влечет критических
// дополнительных расходов. Если же Вам так не кажется, лучше смотреть
// первоисточники по этой теме.

int main(int, const char **)
{
  check_lock_free();
  return 0;
}