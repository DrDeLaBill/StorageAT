<p align="center">
  <h1 align="center">StorageAT</h1>
</p>

<p align="right">
  <img alt="Test passing" src="https://github.com/DrDeLaBill/StorageAT/actions/workflows/build-test.yml/badge.svg?branch=main">
</p>


## Language
- [English](README.md)
- [Русский](README.ru.md)

## Описание

StorageAT - библиотека таблицы распределения данных (файловая система) в памяти для embedded устройств. 

Навигация происходит посредством использования префикса и идентификатора, принадлежащих каждой записи. Префикс используется для определения назначения данных, а идентификатор - для их нумерации.

## Структура таблицы

Память делится на макроблоки равного размера (см. Рисунок 1).
Навигация по макроблокам осуществляется с помощью индексов, начиная с 0.

<img src="https://github.com/DrDeLaBill/StorageAT/assets/40359652/da24a984-cd57-42ee-b1b4-8728b4f9a375">
<p align="center">Рисунок 1</p>

Каждый макроблок имеет оглавление (Header) и область хранения данных (User data), их размеры фиксированы. Оглавление и область хранения данных делятся на фиксированное количество страниц по 256 байт каждая. Навигация по области хранения данных осуществляется с помощью индексов, начиная с 0. Разметка приведена на Рисунке 2.

<img src="https://github.com/DrDeLaBill/StorageAT/assets/40359652/7c6230a3-a5bb-4c65-8a0a-4714986a6588">
<p align="center">Рисунок 2</p>

Страница - минимальный оперируемый объект в таблице распределения данных, она имеет область мета-данных и область записи пользовательских данных.
Каждая страница (в том числе в оглавлении макроблока) сохраняется со специально сформированными мета-данными. Специальный код (Page code) - определяет начало страницы, версия (Version) - версия библиотеки, которая использовалась при записи страницы, предыдущий адрес (Prev page address) и следующий адрес (Next page address) - адреса связанных страниц, если сохранённые ранее данные превышали по размеру область записи страницы, префикс (prefix) и идентификатор (ID) - специальные данные задаваемые пользователем (в том числе хранящиеся также в оглавлении макроблока), предназначены для поиска конкретных данных в памяти, CRC16 - двухбайтная CRC данных страницы (см. Рисунок 3).

<img src="https://github.com/DrDeLaBill/StorageAT/assets/40359652/8636232b-68e4-49b3-bb6f-1a5a3f945f14">
<p align="center">Рисунок 3</p>

В оглавлении макроблока зарезервированы 4 страницы для хранения его оглавления. Структура оглавления идентична странице (см. Рисунок 3), однако её область пользовательских данных делится на две части, предназначенных для быстрой навигации по таблице распределения данных. Первый часть - массив пар префикс-идентификатор, индекс такого элемента в оглавлении соответствует индексу страницы в макроблоке; вторая часть - массив двухбитных значений текущего состояния страницы (0b01 - занята, 0b10 - пуста, 0b11 - заблокирована), индекс такого элемента в массиве также соответствует индексу страницы в макроблоке. Разметка приведена на Рисунке 4.

<img src="https://github.com/DrDeLaBill/StorageAT/assets/40359652/ce123792-f612-47a1-8380-78ef3ffe1973">
<p align="center">Рисунок 4</p>

## API

Весь API, после обработки запроса, возвращает статус выполнения:
* STORAGE_OK - запрос выполнен успешно
* STORAGE_ERROR - произошла внутренняя ошибка
* STORAGE_BUSY - память недоступна и/или занята
* STORAGE_OOM - выход за пределы памяти
* STORAGE_NOT_FOUND - данные не найдены
* STORAGE_DATA_EXISTS - область памяти уже занята другими данными

Для любых манипуляций с памятью используется 32-битный адрес, поэтому для сохранения и загрузки данных сначала необходимо выполнить поиск в соответствующем режиме.

Функция поиска - ищет данные в памяти
* mode - режим поиска данных в памяти
    * FIND_MODE_EQUAL - ищет данные с идентичными префиксом и идентификатором
    * FIND_MODE_NEXT - ищет данные, имеющие идентификатор следующий по счёту переданному, с префиксом идентичным переданному
    * FIND_MODE_MIN - ищет данные по префиксу с минимальным идентификатором
    * FIND_MODE_MAX - ищет данные по префиксу с максимальным идентификатором
    * FIND_MODE_EMPTY - ищет первую пустую страницу
* address - переменная, в которую будет возвращён результат
* prefix - префикс
* id - идентификатор
```c++
StorageStatus find(
    StorageFindMode mode,
    uint32_t*       address,
    const char*     prefix = "",
    uint32_t        id = 0
);
```

Функция загрузки - выгружает данные из памяти
* address - адрес данных в памяти
* data - переменная, в которую будет записан результат
* len - размер ожидаемого результата в байтах
```c++
StorageStatus load(uint32_t address, uint8_t* data, uint32_t len);
```

Функция сохранения - сохраняет данные в память по указанному адресу, если область памяти не занята или имеет идентичные префикс и идентификатор
* address - адрес сохранения
* prefix - префикс
* id - идентификатор
* data - данные
* len - размер данных в байтах
```c++
StorageStatus save(
    uint32_t    address,
    const char* prefix,
    uint32_t    id,
    uint8_t*    data,
    uint32_t    len
);
```

Функция перезаписи - сохраняет данные в память по указанному адресу в любом случае
* address - адрес сохранения
* prefix - префикс
* id - идентификатор
* data - данные
* len - размер данных в байтах
```c++
StorageStatus rewrite(
    uint32_t    address,
    const char* prefix,
    uint32_t    id,
    uint8_t*    data,
    uint32_t    len
);
```

Форматирование - форматирует всю доступную память
```c++
StorageStatus format();
```

Удаление - удаляет данные из оглавления
* address - адрес данных для удаления
```c++
StorageStatus deleteData(uint32_t address);
```

Возвращает общее количество страниц в памяти
```c++
static uint32_t getStoragePagesCount();
```

Возвращает размер памяти в байтах
```c++
static uint32_t getStorageSize();
```

<p align="center">
  <h2 align="center">Порядок работы с таблицей распределения данных</h2>
</p>

### 1. Создание класса драйвера 

Для взаимодействия с таблицей необходимо создать драйвер чтения-записи физической памяти, унаследовав его от интерфейса IStorageDriver

```c++
class StorageDriver: public IStorageDriver
{
public: 
    StorageDriver() {}
	StorageStatus read(uint32_t address, uint8_t* data, uint32_t len) override
    {
        DriverStatus status = storage.readPage(address, data, len);
        if (status == DRIVER_BUSY) {
            return STORAGE_BUSY;
        }
        if (status == DRIVER_OOM) {
            return STORAGE_OOM;
        }
        if (status == DRIVER_ERROR) {
            return STORAGE_ERROR;
        }
        return STORAGE_OK;
    }
	StorageStatus write(uint32_t address, uint8_t* data, uint32_t len) override
    {
        DriverStatus status = storage.writePage(address, data, len);
        if (status == DRIVER_BUSY) {
            return STORAGE_BUSY;
        }
        if (status == DRIVER_OOM) {
            return STORAGE_OOM;
        }
        if (status == DRIVER_ERROR) {
            return STORAGE_ERROR;
        }
        return STORAGE_OK;
    }
};
```

### 2. Создание объекта таблицы

```c++
StorageAT storage(
    memory.getPagesCount(), // Physical memory pages count
    (new StorageDriver())   // Storage driver
);
```

### 3. Использования таблицы

Форматирование памяти:
```c++
storage.format();
```

Запись данных в память:
```c++
uint32_t address = 0;
uint8_t data[1000] = { 1, 2, 3, 4, 5 };

StorageStatus status = storage.find(FIND_MODE_EMPTY, &address);
if (status == STORAGE_OK) {
    storage.save(address, "DAT", 1, data, sizeof(data));
}
```

Чтение данных из памяти:
```c++
uint32_t address = 0;
uint8_t data[1000] = { };

StorageStatus status = storage.find(FIND_MODE_EQUAL, &address, "DAT", 1);
if (status == STORAGE_OK) {
    storage.load(address, data, sizeof(data));
}
```

Перезапись данных в памяти:
```c++
uint32_t address = 0;
uint8_t data1[1000] = { 1, 2, 3, 4, 5 };

StorageStatus status = storage.find(FIND_MODE_EMPTY, &address);
if (status == STORAGE_OK) {
    storage.save(address, "DAT", 1, data1, sizeof(data1));
    uint8_t data2[1000] = { 6, 7, 8, 9, 10 }; 
    storage.rewrite(address, "DAT", 2, data2, sizeof(data2));
}
```

