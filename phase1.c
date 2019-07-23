

#include "mypng.c"
#include "stdio.h"
#include "time.h"

/*
* Read Png
*/
void testrun()
{
  png_byte **read_rows = readpng("test-image.png");
  float **read_floats = tofloatArray(read_rows);
  float **write_floats = fillfloats();
  cleanpngbytes(read_rows);
  double fsum = 0;
  double esum = 0;
  int count = 10;
  for (int i = 0; i < count; i++)
  {
    struct measure ex = stencilfilterapply(read_floats, write_floats);
    fsum += ex.flops;
    esum += ex.estimation;
    printf("-> Computaton Time(IO ignored) elapsed : %f ms and %f Gflop/s \n", ex.estimation, ex.flops);
  }

  printf("-------------------------------------------------------\n");
  printf("Averaged elapsed time for %d cycle(s) : %f ms\n", count, esum / count);
  printf("Averaged FLOPS for %d cycle(s) : %f  GFLOP/s\n", count, fsum / count);
  writepng("output.png", write_floats);
  cleanfloats(write_floats);
  printf("-> finished!\n");
}
int main()
{

 
  testrun();
  return 0;
}