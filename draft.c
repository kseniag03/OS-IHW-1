// 5. Разработать программу, заменяющую все строчные гласные буквы в заданной ASCII-строке заглавными

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <string.h>

#define BUF_SIZE 5000

void convert_vowels_to_uppercase(char *str) {
    int i;
    for (i = 0; i < strlen(str); ++i) {
        if (str[i] == 'a' || str[i] == 'e' || str[i] == 'i' || str[i] == 'o' || str[i] == 'u') {
            str[i] = str[i] - 32; // convert to uppercase
        }
    }
}

int main(int argc, char *argv[]) {

    // Проверка, достаточно ли аргументов командной строки (имя входного и выходного файла)
    if (argc < 3) {
        printf("Error: not enough arguments. Please, enter input and output file names\n");
        exit(1);
    }

    // Получаем имена входного и выходного файлов
    char *input_file = argv[1];
    char *output_file = argv[2];


    // Буфер для чтения
    char buffer[BUF_SIZE];
    // Количество прочитанных байт
    ssize_t read_bytes;

    int status;

    int pipes1[2];
    int pipes2[2];

    if (pipe(pipes1) == -1) {
        perror("pipe1");
        exit(1);
    }
    if (pipe(pipes2) == -1) {
        perror("pipe2");
        exit(1);
    }

    pid_t pid_first, pid_second, pid_third;

    pid_first = fork();

    if (pid_first == -1) {
        perror("Error: fork");
        exit(1);
    } else if (pid_first == 0) {

        printf("Child process #1 with id: %d with parent id: %d\n", (int)getpid(), (int)getppid());

        // close(fd_output);

        int fd_input;
        if ((fd_input = open(input_file, O_RDONLY)) == -1) {
            perror("Error: cannot open input file\n");
            exit(1);
        }

        // close the write end of pipe 1
        close(pipes1[1]);

        // pid1

        // Читаем файл и записываем в неименованный канал, используя цикл
        while ((read_bytes = read(fd_input, buffer, BUF_SIZE)) > 0) {
            if (write(pipes1[0], buffer, read_bytes) != read_bytes) {
                printf("Writing to output file error\n");
                return 1;
            }
        }
        // Проверяем, что файл был прочитан полностью
        if (read_bytes == -1) {
            printf("Reading input file error\n");
            return 1;
        }
        // Читаем данные из канала в буфер
        read(pipes1[0], buffer, read_bytes);

        printf("!!! Get buffer from input file !!!\n");
        printf("%s\n", buffer);

        // close the read end of pipe 1
        close(pipes1[0]);

        if (close(fd_input) < 0){
            printf("Error: cannot close input file\n");
            exit(-1);
        }

        exit(0);
    } else {
        // parent process

        pid_second = fork();

        if (pid_second == -1) {
            perror("Error: fork");
            exit(1);
        } else if (pid_second == 0) {

            printf("Child process #2 with id: %d with parent id: %d\n", (int)getpid(), (int)getppid());

            // close the read end of pipe 1
            close(pipes1[0]);

            // close the write end of pipe 2
            close(pipes2[0]);

            // read the data from pipe 1, process it, and write the result to pipe 2
            while ((read_bytes = read(pipes1[1], buffer, BUF_SIZE)) > 0) {
                convert_vowels_to_uppercase(buffer);
                if (write(pipes2[1], buffer, read_bytes) == -1) {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
            }

            // close the write end of pipe 2
            close(pipes2[1]);

            // exit the child process
         //   exit(EXIT_SUCCESS);



/*
            close(pipes1[0]);
            close(fd_input);
            close(fd_output);

            while ((read_bytes = read(pipes1[1], buffer, BUF_SIZE)) > 0) {
                printf("while change go\n");
                int i;
                for (i = 0; i < read_bytes; ++i) {
                    char c = buffer[i];
                    if (buffer[i] == 'a' || buffer[i] == 'e' || buffer[i] == 'i' || buffer[i] == 'o' || buffer[i] == 'u') {
                        buffer[i] = c - 32; // convert to uppercase
                    }
                }
                if (write(pipes1[1], buffer, read_bytes) == -1) {
                    perror("write");
                    exit(1);
                }
            }
            /*
            int i;
            for (i = 0; i < read_bytes; ++i) {
                char c = buffer[i];
                if (buffer[i] == 'a' || buffer[i] == 'e' || buffer[i] == 'i' || buffer[i] == 'o' || buffer[i] == 'u') {
                    buffer[i] = buffer[i] - 32; // convert to uppercase
                }
            }
    */
            printf("!!! Changed buffer !!!\n");
            printf("%s\n", buffer);

           // write(pipes1[1], buffer, read_bytes);

            exit(0);
        } else {
            // parent process

            pid_third = fork();

            if (pid_third == -1) {
                perror("Error: fork");
                exit(1);
            } else if (pid_third == 0) {

                printf("Child process #3 with id: %d with parent id: %d\n", (int)getpid(), (int)getppid());

                // close the write end of pipe 1
                close(pipes1[1]);

                // close the read end of pipe 2
                close(pipes2[1]);


                int fd_output;
                if ((fd_output = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) {
                    perror("Error: cannot open output file\n");
                    exit(1);
                }

                //close(fd_input);

                printf("String begins to be written\n");

                // read the data from pipe 2 and write it to the output file
                while ((read_bytes = read(pipes2[0], buffer, BUF_SIZE)) > 0) {
                    if (write(fd_output, buffer, read_bytes) == -1) {
                        perror("write");
                        exit(EXIT_FAILURE);
                    }
                }
/*
                while ((read_bytes = read(pipes1[0], buffer, BUF_SIZE)) > 0) {
                    printf("while cycle go\n");
                    if (write(fd_output, buffer, read_bytes) == -1) {
                        perror("write");
                        exit(1);
                    }
                }*/
                if (read_bytes == -1) {
                    perror("read");
                    exit(1);
                }

                // close the read end of pipe 2 and the output file
                close(pipes2[0]);
                close(fd_output);

                

                printf("String has been written\n");

                if (close(fd_output) < 0){
                    printf("Error: cannot close output file\n");
                    exit(-1);
                }

                exit(0);
            } else {
                // wait for the child processes to finish
                waitpid(-1, &status, 0);
                waitpid(-1, &status, 0);
            }
        }
    }


    if (close(pipes1[0]) < 0){
        printf("Error: cannot close pipe0\n");
        exit(-1);
    }
    if (close(pipes1[1]) < 0){
        printf("Error: cannot close pipe1\n");
        exit(-1);
    }


    return 0; /*



    if ((fd_output = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) {
        perror("Error: cannot open output file\n");
        exit(1);
    }









    if (pipe(fd_input) == -1) {
        perror("pipe");
        exit(1);
    }

    if (pipe(fd_output) == -1) {
        perror("pipe");
        exit(1);
    }

    pid_t pid_first, pid_second, pid_third;

    pid_first = fork();

    if (pid_first == -1) {
        perror("fork");
        exit(1);
    } else if (pid_first == 0) {
        close(fd_output);

        while ((read_bytes = read(pipes1[1], buffer, BUF_SIZE)) > 0) {
            if (write(fd_output, buffer, read_bytes) == -1) {
                perror("write");
                exit(1);
            }
        }
        if (read_bytes == -1) {
            perror("read");
            exit(1);
        }
        exit(0);
    } else {
        //
    }

    pid_second = fork();

    if (pid_second == -1) {
        perror("Error: fork");
        exit(1);
    } else if (pid_second == 0) {
        close(fd_input);
        close(fd_output);

        while ((read_bytes = read(pipes1[1], buffer, BUF_SIZE)) > 0) {
            for (int i = 0; i < read_bytes; i++) {
                char c = buffer[i];
                if (str[i] == 'a' || str[i] == 'e' || str[i] == 'i' || str[i] == 'o' || str[i] == 'u') {
                    str[i] = str[i] - 32; // convert to uppercase
                }
            }
            if (write(pipes1[2], buffer, read_bytes) == -1) {
                perror("write");
                exit(1);
            }
        }
        if (read_bytes == -1) {
            perror("read");
            exit(1);
        }
        exit(0);
    } else {
        //
    }

    pid_third = fork();

    if (pid_third == -1) {
        perror("fork");
        exit(1);
    } else if (pid_third == 0) {
        close(fd_input);

        while ((read_bytes = read(pipes1[0], buffer, BUF_SIZE)) > 0) {
            if (write(pipes1[3], buffer, read_bytes) == -1) {
                perror("write");
                exit(1);
            }
        }
        if (read_bytes == -1) {
            perror("read");
            exit(1);
        }

        exit(0);
    } else {
        //
    }

    if (close(fd_input) < 0){
        printf("Error: cannot close input file\n");
        exit(-1);
    }
    if (close(fd_output) < 0){
        printf("Error: cannot close output file\n");
        exit(-1);
    }

    if (close(pipes1[0]) < 0){
        printf("Error: cannot close pipe0\n");
        exit(-1);
    }
    if (close(pipes1[1]) < 0){
        printf("Error: cannot close pipe1\n");
        exit(-1);
    }
    if (close(pipes1[2]) < 0){
        printf("Error: cannot close pipe2\n");
        exit(-1);
    }
    if (close(pipes1[3]) < 0){
        printf("Error: cannot close pipe3\n");
        exit(-1);
    }

    exit(0);*/
}
