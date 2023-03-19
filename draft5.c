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
        perror("open");
        exit(1);
    }

    // Открываем файл с выходными данными
    int out_fd = open(output_file,O_WRONLY | O_CREAT, 0666);
    if (out_fd == -1) {
        perror("open");
        exit(1);
    }

    // Создаем именованные каналы (FIFOs)
    char *fifo_first = FIRST_FIFO_FILE;
    char *fifo_second = SECOND_FIFO_FILE;

    // Create the named pipe
    if (mkfifo(FIRST_FIFO_FILE, 0666) < 0) {
        perror("Error creating named pipe");
        exit(1);
    }

    // Create the named pipe
    if (mkfifo(SECOND_FIFO_FILE, 0666) < 0) {
        perror("Error creating named pipe");
        exit(1);
    }

    //mknod(fifo_first, S_IFIFO|0666, 0);
    //mknod(fifo_second, S_IFIFO|0666, 0);

    // Открываем FIFOs для чтения и записи
    int fd_first_read = open(FIRST_FIFO_FILE, O_RDONLY);
    int fd_first_write = open(FIRST_FIFO_FILE, O_WRONLY | O_NONBLOCK);

    int fd_second_read = open(SECOND_FIFO_FILE, O_RDONLY);
    int fd_second_write = open(SECOND_FIFO_FILE, O_WRONLY | O_NONBLOCK);

    if (fd_first_read == -1 || fd_first_write == -1 || fd_second_read == -1 || fd_second_write == -1) {
        perror("open");
        exit(1);
    }

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

        // Записываем полученные данные в fifo_first
        if (write(fd_first_write, buffer, read_bytes) == -1) {
            perror("write");
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

        // Закрываем неиспользуемые концы именованных каналов
        if (close(fd_first_write) < 0 || close(fd_second_read) < 0) {
            perror("close");
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

        // Меняем регистр гласных на заглавный
        for (int i = 0; i < read_bytes; i++) {
            if (buffer[i] == 'a' || buffer[i] == 'e' || buffer[i] == 'i' || buffer[i] == 'o' || buffer[i] == 'u') {
                buffer[i] = buffer[i] - 32;
            }
        }

        // Передаем результат обработанных данных во второй FIFO
        if (write(fd_second_write, buffer, read_bytes) == -1) {
            perror("write");
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

        if (close(fd_second_write) < 0){
            perror("close");
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

        // Записываем полученные данные в выходной файл
        if (write(out_fd, buffer, read_bytes) == -1) {
            perror("write");
            exit(1);
        }

        exit(0);
    }

    // Процесс-родитель закрывает неименованные каналы и завершает работу с файлами
    if (close(fd_first_read) < 0){
        perror("close");
        exit(-1);
    }
    if (close(fd_first_write) < 0){
        perror("close");
        exit(-1);
    }
    if (close(fd_second_read) < 0){
        perror("close");
        exit(-1);
    }
    if (close(fd_second_write) < 0){
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
    unlink(FIRST_FIFO_FILE);
    unlink(SECOND_FIFO_FILE);

    // Ожидаем завершения дочерних процессов, чтобы родитель у всех был одинаковый
    waitpid(pid_first, &status, 0);
    waitpid(pid_second, &status, 0);
    waitpid(pid_third, &status, 0);

    exit(0);
}

_________________________________________
    
    
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
#define FIFO_FILE "/tmp/fifo"
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
        perror("open");
        exit(1);
    }

    // Открываем файл с выходными данными
    int out_fd = open(output_file,O_WRONLY | O_CREAT, 0666);
    if (out_fd == -1) {
        perror("open");
        exit(1);
    }

    // Create the named pipe
    if (mkfifo(FIFO_FILE, 0666) < 0) {
        perror("Error creating named pipe");
        exit(1);
    }
    int fifo_fd;

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

        // Open the named pipe for writing
        if ((fifo_fd = open(FIFO_FILE, O_WRONLY)) < 0) {
            perror("Error opening named pipe");
            exit(1);
        }

        // Читаем данные из входного файла
        char buffer[BUF_SIZE];
        ssize_t read_bytes = read(in_fd, buffer, BUF_SIZE);
        if (read_bytes == -1) {
            perror("read input");
            exit(1);
        }
        buffer[read_bytes] = '\0';

        write(fifo_fd, buffer, read_bytes);

        close(fifo_fd);

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

        // Open the named pipe for reading
        if ((fifo_fd = open(FIFO_FILE, O_RDONLY)) < 0) {
            perror("Error opening named pipe");
            exit(1);
        }

        // Читаем данные из первого канала
        char buffer[BUF_SIZE];
        ssize_t read_bytes = read(fifo_fd, buffer, BUF_SIZE);
        if (read_bytes == -1) {
            perror("read fifo_first");
            exit(EXIT_FAILURE);
        }
        buffer[read_bytes] = '\0';
        close(fifo_fd);

        // Меняем регистр гласных на заглавный
        for (int i = 0; i < read_bytes; i++) {
            if (buffer[i] == 'a' || buffer[i] == 'e' || buffer[i] == 'i' || buffer[i] == 'o' || buffer[i] == 'u') {
                buffer[i] = buffer[i] - 32;
            }
        }

        // Open the named pipe for writing
        if ((fifo_fd = open(FIFO_FILE, O_WRONLY)) < 0) {
            perror("Error opening named pipe");
            exit(1);
        }

        // Передаем результат обработанных данных во второй FIFO
        if (write(fifo_fd, buffer, read_bytes) == -1) {
            perror("write fifo_second");
            exit(1);
        }

        close(fifo_fd);

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

        // Open the named pipe for reading
        if ((fifo_fd = open(FIFO_FILE, O_RDONLY)) < 0) {
            perror("Error opening named pipe");
            exit(1);
        }

        // Читаем данные из второго канала
        char buffer[BUF_SIZE];
        ssize_t read_bytes = read(fifo_fd, buffer, BUF_SIZE);
        if (read_bytes == -1) {
            perror("read fifo_second");
            exit(1);
        }
        buffer[read_bytes] = '\0';

        close(fifo_fd);

        // Записываем полученные данные в выходной файл
        if (write(out_fd, buffer, read_bytes) == -1) {
            perror("write output");
            exit(1);
        }

        exit(0);
    }

    // Процесс-родитель закрывает неименованные каналы и завершает работу с файлами
    if (close(fifo_fd) < 0){
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

    unlink(FIFO_FILE);

    //unlink(FIRST_FIFO_FILE);
    //unlink(SECOND_FIFO_FILE);

    // Ожидаем завершения дочерних процессов, чтобы родитель у всех был одинаковый
    waitpid(pid_first, &status, 0);
    waitpid(pid_second, &status, 0);
    waitpid(pid_third, &status, 0);

    exit(0);
}

    
