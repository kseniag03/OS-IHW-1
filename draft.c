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

    int fd[2];
    int fd2[2];
    pid_t pid1, pid2, pid3;
    int status;

    int in_fd = open(input_file, O_RDONLY);
    if (in_fd == -1) {
        perror("open");
        exit(1);
    }

    int out_fd = open(output_file,O_WRONLY | O_CREAT, 0666);
    if (out_fd == -1) {
        perror("open");
        exit(1);
    }

    if (pipe(fd) == -1 || pipe(fd2) == -1) {
        perror("pipe");
        exit(1);
    }

    pid1 = fork();

    // fd[0] -- read, fd[1] -- write

    if (pid1 == -1) {
        perror("fork");
        exit(1);
    }
    else if (pid1 == 0) {
        // First child process reads data from input file
        printf("Child process #1 with id: %d with parent id: %d\n", (int)getpid(), (int)getppid());

        if (close(fd[0]) < 0){
            perror("close");
            exit(1);
        }

        // Read data from input file
        char buf[BUF_SIZE];
        ssize_t bytes_read = read(in_fd, buf, BUF_SIZE);
        if (bytes_read == -1) {
            perror("read");
            exit(1);
        }
        buf[bytes_read] = '\0';

        // Write data to first pipe
        if (write(fd[1], buf, bytes_read) == -1) {
            perror("write");
            exit(1);
        }
/*
        if (close(fd[1]) < 0){
            perror("close");
            exit(1);
        }
        if (close(in_fd) < 0){
            perror("close");
            exit(1);
        }*/

        printf("!!! Get buffer from input file 222 !!!\n");printf("%s\n\n", buf);

        exit(0);
    }

    pid2 = fork();

    if (pid2 == -1) {
        perror("fork");
        exit(1);
    }
    else if (pid2 == 0) {
        // Second child process converts lowercase vowels to uppercase
        printf("Child process #2 with id: %d with parent id: %d\n", (int)getpid(), (int)getppid());

        if (close(fd[1]) < 0){
            perror("close");
            exit(1);
        }
        if (close(fd2[0]) < 0){
            perror("close");
            exit(1);
        }

        char buf[BUF_SIZE];
        ssize_t bytes_read = read(fd[0], buf, BUF_SIZE);
        if (bytes_read == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        buf[bytes_read] = '\0';

        // Convert lowercase vowels to uppercase
        for (int i = 0; i < bytes_read; i++) {
            if (buf[i] == 'a' || buf[i] == 'e' || buf[i] == 'i' || buf[i] == 'o' || buf[i] == 'u') {
                buf[i] = buf[i] - 32; // convert to uppercase
            }
        }

        // Write processed data to second pipe
        if (write(fd2[1], buf, bytes_read) == -1) {
            perror("write");
            exit(1);
        }
/*
        if (close(fd[0]) < 0){
            perror("close");
            exit(1);
        }
        if (close(fd2[1]) < 0){
            perror("close");
            exit(1);
        }
        if (close(out_fd) < 0){
            perror("close");
            exit(1);
        }
*/
        printf("!!! Changed buffer !!!\n");printf("%s\n", buf);

        exit(0);
    }

    pid3 = fork();

    if (pid3 == -1) {
        perror("fork");
        exit(1);
    }
    else if (pid3 == 0) {
        // Third child process writes data to output file
        printf("Child process #3 with id: %d with parent id: %d\n", (int)getpid(), (int)getppid());

        // Close unused write end of second pipe
        if (close(fd2[1]) < 0){
            perror("close");
            exit(1);
        }

        char buf[BUF_SIZE];
        ssize_t bytes_read = read(fd2[0], buf, BUF_SIZE);
        if (bytes_read == -1) {
            perror("read");
            exit(1);
        }
        buf[bytes_read] = '\0';

        // Write processed data to output file
        if (write(out_fd, buf, bytes_read) == -1) {
            perror("write");
            exit(1);
        }
/*
        if (close(fd2[0]) < 0){
            perror("close");
            exit(1);
        }
        if (close(out_fd) < 0){
            perror("close");
            exit(1);
        }
*/
        exit(0);
    }

    // Процесс-родитель закрывает неименованные каналы и завершает работу с файлами
    if (close(fd[0]) < 0){
        perror("close");
        exit(-1);
    }
    if (close(fd[1]) < 0){
        perror("close");
        exit(-1);
    }
    if (close(fd2[0]) < 0){
        perror("close");
        exit(-1);
    }
    if (close(fd2[1]) < 0){
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
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);
    waitpid(pid3, &status, 0);

    exit(0);
}

