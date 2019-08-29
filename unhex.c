#include <stdio.h>
#include <stdlib.h>

char decode(char x) {              /* Function to decode a hex character */
  if (x >= '0' && x <= '9')         /* 0-9 is offset by hex 30 */
    return (x - 0x30);
  else if (x >= 'A' && x <= 'F')    /* A-F offset by hex 37 */
    return(x - 0x37);
  else if (x >= 'a' && x <= 'f')    /* a-f offset by hex 37 */
    return(x - 0x57);
  else {                            /* Otherwise, an illegal hex digit */
    // fprintf(stderr,"\nInput is not in legal hex format\n");
    return 0;
  }
}

unsigned char * unhex(const unsigned char * x, int length) {
  unsigned char *data = malloc(length / 2 + 1);
  for (int i = 0; i < length; i = i + 2) {
    data[i / 2] = ((decode(x[i]) << 4) & 0xF0) + (decode(x[i+1]) & 0xF);
  }

  return data;
}
