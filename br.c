#include <stdio.h>

int main (int argc, char *argv[])
{
  int i, j;

  printf ("static const u8 bit_reverse_byte [0x100] =\n");
  printf ("{\n");
  for (i = 0; i < 0x100; i++)
    {
      if ((i & 7) == 0)
	printf ("  ");
      j = (((i & 0x01) << 7) |
	   ((i & 0x02) << 5) |
	   ((i & 0x04) << 3) |
	   ((i & 0x08) << 1) |
	   ((i & 0x10) >> 1) |
	   ((i & 0x20) >> 3) |
	   ((i & 0x40) >> 5) |
	   ((i & 0x80) >> 7));
      printf ("0x%02x", j);
      if (i != 0xff)
	printf (",");
      if ((i & 7) == 7)
	printf ("\n");
      else
	printf (" ");
    }
  printf ("};\n");
  return (0);
}
