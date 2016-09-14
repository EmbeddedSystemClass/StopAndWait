/* Example use of Reed-Solomon library 
 *
 * Copyright Henry Minsky (hqm@alum.mit.edu) 1991-2009
 *
 * This software library is licensed under terms of the GNU GENERAL
 * PUBLIC LICENSE
 *
 * RSCODE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RSCODE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Rscode.  If not, see <http://www.gnu.org/licenses/>.

 * Commercial licensing is available under a separate license, please
 * contact author for details.
 *
 * This same code demonstrates the use of the encodier and 
 * decoder/error-correction routines. 
 *
 * We are assuming we have at least four bytes of parity (NPAR >= 4).
 * 
 * This gives us the ability to correct up to two errors, or 
 * four erasures. 
 *
 * In general, with E errors, and K erasures, you will need
 * 2E + K bytes of parity to be able to correct the codeword
 * back to recover the original message data.
 *
 * You could say that each error 'consumes' two bytes of the parity,
 * whereas each erasure 'consumes' one byte.
 *
 * Thus, as demonstrated below, we can inject one error (location unknown)
 * and two erasures (with their locations specified) and the 
 * error-correction routine will be able to correct the codeword
 * back to the original message.
 * */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rscode/ecc.h>

 
/* Some debugging routines to introduce errors or erasures
   into a codeword. 
   */

/* Introduce a byte error at LOC */
void
byte_err (int err, int loc, unsigned char *dst)
{
  printf("Adding Error at loc %d, data %#x\n", loc, dst[loc-1]);
  dst[loc-1] ^= err;
}

/* Pass in location of error (first byte position is
   labeled starting at 1, not 0), and the codeword.
*/
void
byte_erasure (int loc, unsigned char dst[], int cwsize, int erasures[]) 
{
  printf("Erasure at loc %d, data %#x\n", loc, dst[loc-1]);
  dst[loc-1] = 0;
}

void 
print_hex (unsigned char * p, int len)
{
  int i;
  for (i = 0; i < len; i++){
    printf("0x%02X ", p[i]);
  }
  printf("\n");
}

int
main (int argc, char *argv[])
{

  /* Initialization the ECC library */
 
  int codeword_len = 255;
  int payload_len = 239;
  int redundancy = codeword_len - payload_len;

  int i;

  unsigned char payload[payload_len];
  unsigned char codeword[codeword_len];
  unsigned char err_codeword[codeword_len];

  for (i = 0; i < payload_len; i++)
    payload[i] = (unsigned char) (rand()%256);

  print_hex(payload, payload_len);

  initialize_ecc (redundancy);
  
  /* ************** */
 
  /* Encode data into codeword, adding NPAR parity bytes */
  encode_data(payload, payload_len, codeword);
  
  printf("Codeword:\n");
  print_hex(codeword, codeword_len);
  memcpy(err_codeword, codeword, codeword_len);
  /* Add one error and two erasures */
  byte_err(0x35, 3, err_codeword);

  byte_err(0x23, 4, err_codeword);

  byte_err(0x34, 5, err_codeword);

  byte_err(0x34, 6, err_codeword);

  byte_err(0x34, 7, err_codeword);

  byte_err(0x34, 8, err_codeword);

  byte_err(0x34, 9, err_codeword);

  byte_err(0x34, 10, err_codeword);
 
  /* Now decode -- encoded codeword size must be passed */
  decode_data(err_codeword, codeword_len);
  /* check if syndrome is all zeros */
  if (check_syndrome () != 0) {
    printf("Syndrome founding errors, correcting\n");
    correct_errors_erasures (err_codeword, 
			     codeword_len,
			     0, 
			     NULL);
    printf("Mem compare: %d\n", memcmp(err_codeword, codeword, codeword_len));
  }else{
    printf("syndrome is zero\n");
  }
 
  exit(0);
}

