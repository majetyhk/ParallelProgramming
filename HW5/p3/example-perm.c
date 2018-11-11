#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "jemalloc/jemalloc.h"

#define BACK_FILE "/tmp/app.back"
#define MMAP_FILE "/tmp/app.mmap"
#define MMAP_SIZE ((size_t)1 << 30)

#define NITERATIONS       20

PERM int iteration;
PERM double values[NITERATIONS];

int main(int argc, char *argv[])
{
  int do_restore = argc > 1 && strcmp("-r", argv[1]) == 0;
  const char *mode = (do_restore) ? "r+" : "w+";
  
  // Persistent memory initialization
  perm(PERM_START, PERM_SIZE);
  mopen(MMAP_FILE, mode, MMAP_SIZE);
  bopen(BACK_FILE, mode);

  // Init persistent variables
  if (!do_restore) {
    iteration = 0;
    memset(values, 0, sizeof(values));	
    mflush(); /* a flush is needed to save some global state */
    backup();
  }
  else {
    printf("restarting...\n");
    restore();
  }

  // Main loop
  for (; iteration < NITERATIONS; iteration++){
	double sum = 0;
	values[iteration] = 1;
	for (int i = 0; i < NITERATIONS; i++)
		sum += values[i];
	printf("Iteration: %d, sum:%f\n", iteration+1, sum);
	backup();
	sleep(1);
  }

  // Cleanup
  mclose();
  bclose();
  remove(BACK_FILE);
  remove(MMAP_FILE);
  
  return 0;
}
