// 5. Разработать программу, заменяющую все строчные гласные буквы в заданной ASCII-строке заглавными
// 1-й процесс, ввод & вывод

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
#define FIFO_FILE "/tmp/fifo"

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
    if (mkfifo(FIFO_FILE, 0666) < 0) {
        perror("mkfifo fifo");
        exit(1);
    }

    // Объявляем FIFOs для чтения и записи
    int fd_read;
    int fd_write;
    
    // Первый процесс считывает данные из входного файла
    printf("READ process #1 with id: %d with parent id: %d\n", (int)getpid(), (int)getppid());

    // Читаем данные из входного файла
    char buffer[BUF_SIZE];
    ssize_t read_bytes = read(in_fd, buffer, BUF_SIZE);
    if (read_bytes == -1) {
        perror("read");
        exit(1);
    }
    buffer[read_bytes] = '\0';

    // Открываем первый канал на запись
    if ((fd_write = open(FIFO_FILE, O_WRONLY)) == -1) {
        perror("open fifo_first_write");
        exit(1);
    }

    // Записываем полученные данные в fifo first
    if (write(fd_write, buffer, read_bytes) == -1) {
        perror("write");
        exit(1);
    }

    // Закрываем канал
    if (close(fd_write) < 0){
        perror("close");
        exit(1);
    }

    printf("!!! Got buffer from input file !!!\n");
    printf("%s\n\n", buffer);

    // Первый процесс записывает данные в выходной файл
    printf("WRITE process #1 with id: %d with parent id: %d\n", (int)getpid(), (int)getppid());

    // Открываем второй канал на чтение
    if ((fd_read = open(FIFO_FILE, O_RDONLY)) == -1) {
        perror("open fifo_second_read");
        exit(1);
    }

    // Читаем данные из второго канала
    read_bytes = read(fd_read, buffer, BUF_SIZE);
    if (read_bytes == -1) {
        perror("read");
        exit(1);
    }
    buffer[read_bytes] = '\0';

    if (close(fd_read) < 0) {
        perror("close fifo_second_read");
        exit(1);
    }

    // Записываем полученные данные в выходной файл
    if (write(out_fd, buffer, read_bytes) == -1) {
        perror("write");
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

    // Удаляем именованный канал
    unlink(FIFO_FILE);
    exit(0);
}
