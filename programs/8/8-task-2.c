// 5. Разработать программу, заменяющую все строчные гласные буквы в заданной ASCII-строке заглавными
// 2-й процесс, обработка

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
    // Объявляем FIFOs для чтения и записи
    int fd_read;
    int fd_write;

    // Второй дочерний процесс заменяет все строчные гласные буквы в заданной ASCII-строке заглавными
    printf("PROCESS process #2 with id: %d with parent id: %d\n", (int)getpid(), (int)getppid());

    // Открываем первый канал на чтение
    if ((fd_read = open(FIFO_FILE, O_RDONLY)) == -1) {
        perror("open fifo_first_read");
        exit(1);
    }

    // Читаем данные из первого канала
    char buffer[BUF_SIZE];
    ssize_t read_bytes = read(fd_read, buffer, BUF_SIZE);
    if (read_bytes == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    buffer[read_bytes] = '\0';

    if (close(fd_read) < 0) {
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
    if ((fd_write = open(FIFO_FILE, O_WRONLY)) == -1) {
        perror("open fifo_second_write");
        exit(1);
    }

    // Передаем результат обработанных данных во второй канал
    if (write(fd_write, buffer, read_bytes) == -1) {
        perror("write");
        exit(1);
    }

    if (close(fd_write) < 0) {
        perror("close fifo_second_write");
        exit(1);
    }

    printf("!!! Changed buffer !!!\n");
    printf("%s\n", buffer);

    exit(0);
}