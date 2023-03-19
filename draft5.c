#define FIFO_FILE "MYFIFO"

FILE *fp;

umask(0);
mknod(FIFO_FILE, S_IFIFO|0666, 0)

fp = fopen(FIFO_FILE, "r")
fgets(buffer, BUF_SIZE, fp)
fclose(fp)
