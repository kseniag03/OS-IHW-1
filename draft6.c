// 5. Разработать программу, заменяющую все строчные гласные буквы в заданной ASCII-строке заглавными

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define BUF_SIZE 5000

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
        perror("open");
        exit(1);
    }

    // Открываем файл с выходными данными
    int out_fd = open(output_file,O_WRONLY | O_CREAT, 0666);
    if (out_fd == -1) {
        perror("open");
        exit(1);
    }

    int pipe_first[2], pipe_second[2];

    // Запускаем системный вызов неименованных каналов
    if (pipe(pipe_first) == -1 || pipe(pipe_second) == -1) {
        perror("pipe");
        exit(1);
    }

    pid_t pid_first, pid_second;
    int status;

    // Запускаем три дочерних процесса
    pid_first = fork();
    if (pid_first == -1) {
        perror("fork");
        exit(1);
    }
    else if (pid_first == 0) {
        // Первый дочерний процесс считывает данные из входного файла
        printf("READ Child process #1 with id: %d with parent id: %d\n", (int)getpid(), (int)getppid());

        // Закрываем неиспользуемую операцию чтения первого канала ([0] -- для чтения)
        if (close(pipe_first[0]) < 0){
            perror("close");
            exit(1);
        }

        // Читаем данные из входного файла
        char buffer[BUF_SIZE];
        ssize_t read_bytes = read(in_fd, buffer, BUF_SIZE);
        if (read_bytes == -1) {
            perror("read");
            exit(1);
        }
        buffer[read_bytes] = '\0';

        // Записываем полученные данные в pipe_first ([1] -- для записи)
        if (write(pipe_first[1], buffer, read_bytes) == -1) {
            perror("write");
            exit(1);
        }

        printf("!!! Got buffer from input file !!!\n");
        printf("%s\n\n", buffer);




        // Третий дочерний процесс записывает данные в выходной файл
        printf("WRITE Child process #1 with id: %d with parent id: %d\n", (int)getpid(), (int)getppid());

        if (close(pipe_second[1]) < 0){
            perror("close");
            exit(1);
        }

        // Читаем данные из второго канала
        //char buffer[BUF_SIZE];
        read_bytes = read(pipe_second[0], buffer, BUF_SIZE);
        if (read_bytes == -1) {
            perror("read");
            exit(1);
        }
        buffer[read_bytes] = '\0';

        // Записываем полученные данные в выходной файл
        if (write(out_fd, buffer, read_bytes) == -1) {
            perror("write");
            exit(1);
        }

        exit(0);
    }

    pid_second = fork();
    if (pid_second == -1) {
        perror("fork");
        exit(1);
    }
    else if (pid_second == 0) {
        // Второй дочерний процесс заменяет все строчные гласные буквы в заданной ASCII-строке заглавными
        printf("PROCESS Child process #2 with id: %d with parent id: %d\n", (int)getpid(), (int)getppid());

        if (close(pipe_first[1]) < 0){
            perror("close");
            exit(1);
        }
        if (close(pipe_second[0]) < 0){
            perror("close");
            exit(1);
        }

        // Читаем данные из первого канала
        char buffer[BUF_SIZE];
        ssize_t read_bytes = read(pipe_first[0], buffer, BUF_SIZE);
        if (read_bytes == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        buffer[read_bytes] = '\0';

        // Меняем регистр гласных на заглавный
        for (int i = 0; i < read_bytes; i++) {
            if (buffer[i] == 'a' || buffer[i] == 'e' || buffer[i] == 'i' || buffer[i] == 'o' || buffer[i] == 'u') {
                buffer[i] = buffer[i] - 32;
            }
        }

        // Передаем результат обработанных данных в pipe_second
        if (write(pipe_second[1], buffer, read_bytes) == -1) {
            perror("write");
            exit(1);
        }

        printf("!!! Changed buffer !!!\n");
        printf("%s\n", buffer);

        exit(0);
    }

    // Процесс-родитель закрывает неименованные каналы и завершает работу с файлами
    if (close(pipe_first[0]) < 0){
        perror("close");
        exit(-1);
    }
    if (close(pipe_first[1]) < 0){
        perror("close");
        exit(-1);
    }
    if (close(pipe_second[0]) < 0){
        perror("close");
        exit(-1);
    }
    if (close(pipe_second[1]) < 0){
        perror("close");
        exit(-1);
    }
    if (close(in_fd) < 0){
        perror("close");
        exit(-1);
    }
    if (close(out_fd) < 0){
        perror("close");
        exit(-1);
    }

    // Ожидаем завершения дочерних процессов, чтобы родитель у всех был одинаковый
    waitpid(pid_first, &status, 0);
    waitpid(pid_second, &status, 0);

    exit(0);
}

