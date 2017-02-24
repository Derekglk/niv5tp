
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>


int main()
{
  int fc;
  unsigned int chenillard[3];
  unsigned int config[3] = {1, 15, 0};

  fc = open("/dev/chenillard",O_RDWR);
  read(fc, chenillard, sizeof(chenillard));
  printf("********** mode=%d speed=%d ************\n",chenillard[0],chenillard[1]);

  write(fc, config, sizeof(config));
  read(fc, chenillard, sizeof(chenillard));
  printf("********** mode=%d speed=%d ************\n",chenillard[0],chenillard[1]);

  close(fc);
  return 0;
}
