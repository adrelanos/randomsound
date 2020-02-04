#include "asoundrunner.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#define SAMPLERATE 8000
#define SAMPLERATESTR "8000"

int arecord_read_fd = -1;
int restart_arecord = 1;

static BitBuffer read_into_here;
static int signal_handler_installed = 0;

void
child_handler(int _ignored)
{
  int status;
  while (waitpid(-1, &status, WNOHANG) != -1) ;
  restart_arecord = 1;
}

int
start_arecord(BitBuffer read_into)
{
  pid_t child;
  int pipefd[2];
  int sparefd;
  read_into_here = read_into;
  if (signal_handler_installed == 0) {
    signal(SIGCHLD, child_handler);
    signal_handler_installed = 1;
  }
  if (arecord_read_fd != -1) {
    close(arecord_read_fd);
    arecord_read_fd = -1;
  }
  if (socketpair(AF_LOCAL, SOCK_STREAM, PF_LOCAL, pipefd) == -1)
    return -1;
  restart_arecord = 0;
  child = fork();
  switch (child) {
  case -1:
    return -1;
  case 0:
    sparefd = open("/dev/null", O_RDWR);
    close(0);
    close(1);
    close(2);
    close(pipefd[0]);
    dup2(pipefd[1], 1);
    close(pipefd[1]);
    dup2(sparefd, 0);
    dup2(sparefd, 2);
    close(sparefd);
    execlp("arecord", "arecord", "-c", "1", "-f", "S16_LE", "-r", SAMPLERATESTR, "-t", "raw", NULL);
    exit(1);
  default:
    close(pipefd[1]);
    arecord_read_fd = pipefd[0];
  }
  return child;
}

void
asound_do_read(void)
{
  /* Read in SAMPLERATE*2/10 bytes (1/10th of a second of sampling) */
  BitField buffer[SAMPLERATE*5];
  int xfered = 0;
  int readbytes = read(arecord_read_fd, buffer, SAMPLERATE*5);
  if (readbytes < 0) {
    perror("read");
    restart_arecord = 1;
    return;
  }
  while (xfered < (readbytes>>1) && bitbuffer_free_space(read_into_here) > 0) {
    BitField thisbit = buffer[xfered];
    bitbuffer_add_bits(read_into_here, thisbit, 8);
    xfered += 2;
  }
}
