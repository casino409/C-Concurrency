#include <mutex>
#include <condition_variable>
#include <queue>

// Mutex - mutural execution.
// функциональность по защите данных/кода от одновременного доступа/выполнения
// защищаем куски программы, которые должны строго выполняться одним исполнителем

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

// если возникнет исключение мютекс останется заблокированным!

void mutex_simple_using_RAII()
{
  std::lock_guard<std::mutex> guard(m);
  // save code
}

// lock_guard более простой и без лишней функциональности
// unique_lock умеет передавать захваченный мьютекс

void mutex_simple_using_RAII_2()
{
  std::unique_lock<std::mutex> guard(m);
  // save code
  guard.unlock();
  // not save code
}

// lock_guard, unique_lock автоматически освобождают мьютекс
//
// unique_lock содержит в себе флаг, чтобы понимать владеет ли он мьютексом и
// тогда при освобождении вызовет unlock(), а если не владеет то освобождать
// будет некорректно.

// Одновременное использование нескольких блокировок
// Основная проблема - взаимные блокировки. Ситуация, когда 2 исполнителя
// ждут освобождения чужого ресурса и при этом каждый из них владеет своим
// ресурсом.

// Ex: t1 захватил l1, t2 захватил l2. t1 для завершения операции требуется
//     получить l2, а t2 - l1. И так оба исполнителя чего-то ждут.

// Захват нескольних мьютексов без взаимной блокировки
// можно получить мьтекс (я бы сказал скопировать к себе), и сразу же его
// std::adopt_lock - захватить мьютекс
// std::defer_lock - не захватить

void lock_several_mutex()
{
  std::lock(m1, m2); // !!! m1, m2 должны быть разные мьютексы
  std::lock_guard<std::mutex> lock_1(m1); // == lock_1(m1, std::adopt_lock)
  std::lock_guard<std::mutex> lock_2(m2);
  // save code
}

// У unique_lock есть возможность получить мьютекс и потом его захватить.

void lock_several_mutex2()
{
  std::unique_lock<std::mutex> lock_1(m1, std::defer_lock);
  std::unique_lock<std::mutex> lock_2(m2, std::defer_lock);
  std::lock(lock_1, lock_2);
  // save code
}

// Синхронизация (ожидание чего-то)
// Механизм ожидания события реализуется через conditional_varible

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
// а) используется unique_lock, чтобы можно было освободить мьютекс и
//    ожидать сигнал от условной переменной
// б) есть возможность использовать функцию проверки ложного срабатывания
//

// Ограничение времени ожидания

// Идея использовать абсолютное время, а не промежуток, если имеем дело с
// условной переменной, т.к. из-за ложных пробуждении можно не учесть время
// выполнения лишних циклов (стр 142)
//
// auto const timer = std::chrono::steady_clock::now() + std::chrono::milliseconds(10);
// ...
//  wait_until(timer);

void worker_limited() // гарантировано ожидание не превысит 10ms (проверки будут учтены)
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