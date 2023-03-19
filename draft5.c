// 5. Разработать программу, заменяющую все строчные гласные буквы в заданной ASCII-строке заглавными

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUF_SIZE 5000
#define FIRST_FIFO_FILE "/tmp/first-fifo"
#define SECOND_FIFO_FILE "/tmp/second-fifo"

int main(int argc, char *argv[]) {
    // Проверка, достаточно ли аргументов командной строки (имя входного и выходного файла)
    if (argc < 3) {
        printf("Error: not enough arguments. Please, enter input and output file names\n");
        exit(1);
    }

    // Получаем имена входного и выходного файлов
    char *input_file = argv[1];
    char *output_file = argv[2];

    // Открываем файл с входными данными
    int in_fd = open(input_file, O_RDONLY);
    if (in_fd == -1) {
        perror("open input");
        exit(1);
    }

    // Открываем файл с выходными данными
    int out_fd = open(output_file,O_WRONLY | O_CREAT, 0666);
    if (out_fd == -1) {
        perror("open output");
        exit(1);
    }

    // Создаем именованные каналы
    if (mkfifo(FIRST_FIFO_FILE, 0666) < 0) {
        perror("mkfifo first fifo");
        exit(1);
    }
    if (mkfifo(SECOND_FIFO_FILE, 0666) < 0) {
        perror("mkfifo second fifo");
        exit(1);
    }

    // Объявляем FIFOs для чтения и записи
    int fd_first_read;
    int fd_first_write;
    int fd_second_read;
    int fd_second_write;

    pid_t pid_first, pid_second, pid_third;
    int status;

    // Запускаем три дочерних процесса
    pid_first = fork();
    if (pid_first == -1) {
        perror("fork");
        exit(1);
    }
    else if (pid_first == 0) {
        // Первый дочерний процесс считывает данные из входного файла
        printf("Child process #1 with id: %d with parent id: %d\n", (int)getpid(), (int)getppid());

        // Читаем данные из входного файла
        char buffer[BUF_SIZE];
        ssize_t read_bytes = read(in_fd, buffer, BUF_SIZE);
        if (read_bytes == -1) {
            perror("read");
            exit(1);
        }
        buffer[read_bytes] = '\0';

        // Открываем первый канал на запись
        if ((fd_first_write = open(FIRST_FIFO_FILE, O_WRONLY)) == -1) {
            perror("open fifo_first_write");
            exit(1);
        }

        // Записываем полученные данные в fifo first
        if (write(fd_first_write, buffer, read_bytes) == -1) {
            perror("write");
            exit(1);
        }

        // Закрываем канал
        if (close(fd_first_write) < 0){
            perror("close");
            exit(1);
        }

        printf("!!! Got buffer from input file !!!\n");
        printf("%s\n\n", buffer);

        exit(0);
    }

    pid_second = fork();
    if (pid_second == -1) {
        perror("fork");
        exit(1);
    }
    else if (pid_second == 0) {
        // Второй дочерний процесс заменяет все строчные гласные буквы в заданной ASCII-строке заглавными
        printf("Child process #2 with id: %d with parent id: %d\n", (int)getpid(), (int)getppid());

        // Открываем первый канал на чтение
        if ((fd_first_read = open(FIRST_FIFO_FILE, O_RDONLY)) == -1) {
            perror("open fifo_first_read");
            exit(1);
        }

        // Читаем данные из первого канала
        char buffer[BUF_SIZE];
        ssize_t read_bytes = read(fd_first_read, buffer, BUF_SIZE);
        if (read_bytes == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        buffer[read_bytes] = '\0';

        if (close(fd_first_read) < 0) {
            perror("close fifo_first_read");
            exit(1);
        }

        // Меняем регистр гласных на заглавный
        for (int i = 0; i < read_bytes; i++) {
            if (buffer[i] == 'a' || buffer[i] == 'e' || buffer[i] == 'i' || buffer[i] == 'o' || buffer[i] == 'u') {
                buffer[i] = buffer[i] - 32;
            }
        }

        // Открываем второй канал на запись
        if ((fd_second_write = open(SECOND_FIFO_FILE, O_WRONLY)) == -1) {
            perror("open fifo_second_write");
            exit(1);
        }

        // Передаем результат обработанных данных во второй канал
        if (write(fd_second_write, buffer, read_bytes) == -1) {
            perror("write");
            exit(1);
        }

        if (close(fd_second_write) < 0) {
            perror("close fifo_second_write");
            exit(1);
        }

        printf("!!! Changed buffer !!!\n");
        printf("%s\n", buffer);

        exit(0);
    }

    pid_third = fork();
    if (pid_third == -1) {
        perror("fork");
        exit(1);
    }
    else if (pid_third == 0) {
        // Третий дочерний процесс записывает данные в выходной файл
        printf("Child process #3 with id: %d with parent id: %d\n", (int)getpid(), (int)getppid());

        // Открываем второй канал на чтение
        if ((fd_second_read = open(SECOND_FIFO_FILE, O_RDONLY)) == -1) {
            perror("open fifo_second_read");
            exit(1);
        }

        // Читаем данные из второго канала
        char buffer[BUF_SIZE];
        ssize_t read_bytes = read(fd_second_read, buffer, BUF_SIZE);
        if (read_bytes == -1) {
            perror("read");
            exit(1);
        }
        buffer[read_bytes] = '\0';

        if (close(fd_second_read) < 0) {
            perror("close fifo_second_read");
            exit(1);
        }

        // Записываем полученные данные в выходной файл
        if (write(out_fd, buffer, read_bytes) == -1) {
            perror("write");
            exit(1);
        }

        exit(0);
    }

    // Процесс-родитель закрывает именованные каналы и завершает работу с файлами

    if (close(fd_first_read) < 0){
        perror("close");
        exit(1);
    }
    if (close(fd_first_write) < 0){
        perror("close");
        exit(1);
    }
    if (close(fd_second_read) < 0){
        perror("close");
        exit(1);
    }
    if (close(fd_second_write) < 0){
        perror("close");
        exit(1);
    }

    if (close(in_fd) < 0){
        perror("close");
        exit(1);
    }
    if (close(out_fd) < 0){
        perror("close");
        exit(1);
    }

    // Ожидаем завершения дочерних процессов, чтобы родитель у всех был одинаковый
    waitpid(pid_first, &status, 0);
    waitpid(pid_second, &status, 0);
    waitpid(pid_third, &status, 0);

    // Удаляем именованные каналы
    if (unlink(FIRST_FIFO_FILE) == -1) {
        perror("Error deleting named pipe");
        exit(1);
    }
    if (unlink(SECOND_FIFO_FILE) == -1) {
        perror("Error deleting named pipe");
        exit(1);
    }

    exit(0);
}

