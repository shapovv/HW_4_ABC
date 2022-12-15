#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <vector>
#include <fstream>
#include <cstring>
#include <iostream>
#include <ctime>

using namespace std;

// Вектор представляющий собой БД
static vector<int> DB;
static bool isFileInput;
static string outFileName;

sem_t controller; // Семафор ограничивающий доступ к БД

pthread_mutex_t mutexWrite; //Мьютекс для операции записи
pthread_mutex_t mutexRead; //Мьютекс для операции чтения

// Стартовая функция потоков – производителей (писателей)
void *Producer(void *param) {
    int pNum = *((int *) param);
    cout << "Я писатель №" << pNum << "\n";
    if (isFileInput) {
        ofstream fout;
        fout.open(outFileName, std::ios_base::app);
        fout << "Я писатель №" << pNum << "\n";
        fout.close();
    }
    while (1) {
        // Создаем случайную транзакцию для БД
        int data = rand() % 10;
        int value = rand() % 2 == 0 ? -data : data;
        // Защита операции записи
        pthread_mutex_lock(&mutexWrite);
        // Критическая секция
        int *val = new int;
        sem_getvalue(&controller, val);
        // Понижаем значение семафора, чтобы заблокировать остальные потоки
        if (*val > 0)
            sem_wait(&controller);
        free(val);
        cout << "[Писатель №" << pNum << "] - Выполняю транзакцию: " << value << "\n";
        if (isFileInput) {
            ofstream fout;
            fout.open(outFileName, std::ios_base::app);
            fout << "[Писатель №" << pNum << "] - Выполняю транзакцию: " << value << "\n";
            fout.close();
        }
        // Отдельная транзакция переводит БД из одного непротиворечивого состояния в другое
        sleep(6 + rand() % 3);
        for (int i = 0; i < DB.size(); i++) {
            DB[i] += data;
        }
        // Повышаем значение семафора для разблокировки
        sem_post(&controller);
        cout << "[Писатель №" << pNum << "] - Готово\n";
        if (isFileInput) {
            ofstream fout;
            fout.open(outFileName, std::ios_base::app);
            fout << "[Писатель №" << pNum << "] - Готово\n";
            fout.close();
        }
        pthread_mutex_unlock(&mutexWrite);
        sleep(5);
    }
    return nullptr;
}

// Стартовая функция потоков – потребителей (читателей)
void *Consumer(void *param) {
    int cNum = *((int *) param);
    cout << "Я читатель №" << cNum << "\n";
    if (isFileInput) {
        ofstream fout;
        fout.open(outFileName, std::ios_base::app);
        fout << "Я читатель №" << cNum << "\n";
        fout.close();
    }
    int result;
    while (1) {
        // Получаем случайный индекс для чтения
        int index = rand() % DB.size();
        sleep(rand() % 5 + 5);
        // Блокируем чтение (нужно для файловой записи и вывода в консоль в порядке)
        pthread_mutex_lock(&mutexRead);
        int *val = new int;
        sem_getvalue(&controller, val);
        cout << "[Читатель №" << cNum << "] - Хочу прочитать данные! [" << index << "]\n";
        if (isFileInput) {
            ofstream fout;
            fout.open(outFileName, std::ios_base::app);
            fout << "[Читатель №" << cNum << "] - Хочу прочитать данные! [" << index << "]\n";
            fout.close();
        }
        // Если нет опрераций записи, то можем читать
        sem_wait(&controller);
        sem_post(&controller);
        result = DB[index];
        cout << "[Читатель №" << cNum << "] - Читаю данные из: [" << index << "] = " << result << "\n";
        if (isFileInput) {
            ofstream fout;
            fout.open(outFileName, std::ios_base::app);
            fout << "[Читатель №" << cNum << "] - Читаю данные из: [" << index << "] = " << result << "\n";
            fout.close();
        }
        free(val);
        pthread_mutex_unlock(&mutexRead);
    }
    return nullptr;
}

int main(int argc, char *argv[]) {
    srand(time(nullptr));
    // Проверка аргументов
    if (argc < 3) {
        cout << "Неправильное количество аргументов!\n";
        return 0;
    }
    int n, p, c;
    string output_filename = "";
    if (strcmp(argv[1], "-c") == 0) {
        if (argc < 5) {
            cout << "Неправильное количество аргументов!\n";
            return 0;
        }
        try {
            n = atoi(argv[2]);
            p = atoi(argv[3]);
            c = atoi(argv[4]);
        } catch (exception e) {
            cout << "Неверные входные данные!\n";
            return 0;
        }
        if (n <= 0 || p <= 0 || c <= 0) {
            cout << "Неверные входные данные!\n";
            return 0;
        }
    } else if (strcmp(argv[1], "-f") == 0) {
        if (argc < 4) {
            cout << "Неправильное количество аргументов!\n";
            return 0;
        }
        ifstream fin;
        fin.open(argv[2]);
        if (!fin) {
            cout << "Неверный входной файл\n";
            return 0;
        }
        fin >> n >> p >> c;
        fin.close();
        if (n <= 0 || p <= 0 || c <= 0) {
            cout << "Неверные входные данные!\n";
            return 0;
        }
        isFileInput = true;
        outFileName = argv[3];
    } else if (strcmp(argv[1], "-r") == 0) {
        if (argc < 4) {
            cout << "Неправильное количество аргументов!\n";
            return 0;
        }
        try {
            int low = atoi(argv[2]);
            int high = atoi(argv[3]);
            if (low < 1 || low >= high) {
                throw invalid_argument("");
            }
            n = rand() % (high - low) + low;
            p = rand() % (high - low) + low;
            c = rand() % (high - low) + low;
        } catch (exception e) {
            cout << "Неправильные границы для случайной генерации!\n";
            return 0;
        }
        if (argc == 5) {
            isFileInput = true;
            outFileName = argv[4];
        }
    } else {
        cout << "Неправильное значение флага!\n";
        return 0;
    }
    // Создаем непротиворечивое состояние: отсортированный вектор элементов
    DB = vector<int>(n);
    for (int i = 0; i < n; i++) {
        DB[i] = i + 1;
    }
    cout << "Размер БД: " << n << " Писатели: " << p << " Читатели: " << c << "\n";
    if (isFileInput) {
        ofstream fout;
        fout.open(outFileName);
        fout << "Размер БД: " << n << " Писатели: " << p << " Читатели: " << c << "\n";
        fout.close();
    }
    cout << "Нажмите 'q' чтобы остановить программу.\n";
    sleep(1);
    // Инициализация мьютексов и семафоров
    pthread_mutex_init(&mutexWrite, nullptr);
    pthread_mutex_init(&mutexRead, nullptr);
    sem_init(&controller, 0, 0);
    // Запуск производителей
    pthread_t threadP[p];
    int producers[p];
    for (int i = 0; i < p; i++) {
        producers[i] = i + 1;
        pthread_create(&threadP[i], nullptr, Producer, (void *) (producers + i));
    }
    // Запуск потребителей
    pthread_t threadC[c];
    int consumers[c];
    for (int i = 0; i < c; i++) {
        consumers[i] = i + 1;
        pthread_create(&threadC[i], nullptr, Consumer, (void *) (consumers + i));
    }
    // Главный поток ждет польховательского ввода
    while (cin.get() != 'q');
    if (isFileInput) {
        cout << "Все события записаны в файл: " << outFileName << "\n";
    }
    return 0;
}
