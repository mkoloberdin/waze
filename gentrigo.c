#include <math.h>
#include <stdio.h>

int main (int argc, char **argv) {

   int degree;
   int x;
   int y;
   double radian;
   double Half_PI = asin(1);

   for (degree = 0; degree <= 45; degree += 1) {

      radian = (degree * Half_PI) / 90.0;

      x = (int) ((32768 * sin(radian))+0.5);
      if (x >= 32768) x = 32767;
      else if (x < -32768) x = -32768;

      y = (int) ((32768 * cos(radian))+0.5);
      if (y >= 32768) y = 32767;
      else if (y < -32768) y = -32768;

      printf ("   {%5d,%5d},          /* %d degrees. */\n", x, y, degree);
   }

   return 0;
}

