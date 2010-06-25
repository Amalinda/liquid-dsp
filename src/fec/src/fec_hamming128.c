/*
 * Copyright (c) 2007, 2008, 2009, 2010 Joseph Gaeddert
 * Copyright (c) 2007, 2008, 2009, 2010 Virginia Polytechnic
 *                                      Institute & State University
 *
 * This file is part of liquid.
 *
 * liquid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * liquid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with liquid.  If not, see <http://www.gnu.org/licenses/>.
 */

//
// 2/3-rate (8,12) Hamming code
//
//  bit position    1   2   3   4   5   6   7   8   9   10  11  12
//  encoded bits    P1  P2  1   P4  2   3   4   P8  5   6   7   8
//
//  parity bit  P1  x   .   x   .   x   .   x   .   x   .   x   .
//  coveratge   P2  .   x   x   .   .   x   x   .   .   x   x   .
//              P4  .   .   .   x   x   x   x   .   .   .   .   x
//              P8  .   .   .   .   .   .   .   x   x   x   x   x

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "liquid.internal.h"

#define DEBUG_FEC_HAMMING128 0

// parity bit coverage mask for encoder (collapsed version of figure
// above, stripping out parity bits P1, P2, P4, P8 and only including
// data bits 1:8)
//
// bit position     3   5   6   7   9   10  11  12
//
//  parity bit  P1  x   x   .   x   x   .   x   .   =   1101 1010
//  coverage    P2  x   .   x   x   .   x   x   .   =   1011 0110
//              P4  .   x   x   x   .   .   .   x   =   0111 0001
//              P8  .   .   .   .   x   x   x   x   =   0000 1111
#define HAMMING128_M1   0xda    // 1101 1010
#define HAMMING128_M2   0xb6    // 1011 0110
#define HAMMING128_M4   0x71    // 0111 0001
#define HAMMING128_M8   0x0f    // 0000 1111

// parity bit coverage mask for decoder; used to compute syndromes
// for decoding a received message (see first figure, above).
#define HAMMING128_S1   0x0aaa  // .... 1010 1010 1010
#define HAMMING128_S2   0x0666  // .... 0110 0110 0110
#define HAMMING128_S4   0x01e1  // .... 0001 1110 0001
#define HAMMING128_S8   0x001f  // .... 0000 0001 1111

// binary dot-product (count ones modulo 2)
// same as
//  #define bdotprod(x,y) (count_ones_static((x)&(y)) & 0x0001)
// but much faster
#define bdotprod(x,y) ( (c_ones_mod2[ (x & y & 0x00ff)>>0] +  \
                         c_ones_mod2[ (x & y & 0x0f00)>>8]    \
                        ) & 0x0001)

unsigned int fec_hamming128_encode_symbol(unsigned int _sym_dec)
{
    // validate input
    if (_sym_dec >= (1<<8)) {
        fprintf(stderr,"error, fec_hamming128_encode(), input symbol too large\n");
        exit(1);
    }

    // compute parity bits
    unsigned int p1 = bdotprod(_sym_dec, HAMMING128_M1);
    unsigned int p2 = bdotprod(_sym_dec, HAMMING128_M2);
    unsigned int p4 = bdotprod(_sym_dec, HAMMING128_M4);
    unsigned int p8 = bdotprod(_sym_dec, HAMMING128_M8);

#if DEBUG_FEC_HAMMING128
    printf("parity bits (p1,p2,p4,p8) : (%1u,%1u,%1u,%1u)\n", p1, p2, p4, p8);
#endif

    // encode symbol by inserting parity bits with data bits to
    // make a 12-bit symbol
    unsigned int sym_enc = ((_sym_dec & 0x000f) << 0) |
                           ((_sym_dec & 0x0070) << 1) |
                           ((_sym_dec & 0x0080) << 2) |
                           ( p1 << 11 ) |
                           ( p2 << 10 ) |
                           ( p4 << 8  ) |
                           ( p8 << 4  );

    return sym_enc;
}

unsigned int fec_hamming128_decode_symbol(unsigned int _sym_enc)
{
    // validate input
    if (_sym_enc >= (1<<12)) {
        fprintf(stderr,"error, fec_hamming128_decode(), input symbol too large\n");
        exit(1);
    }

    // compute syndrome bits
    unsigned int s1 = bdotprod(_sym_enc, HAMMING128_S1);
    unsigned int s2 = bdotprod(_sym_enc, HAMMING128_S2);
    unsigned int s4 = bdotprod(_sym_enc, HAMMING128_S4);
    unsigned int s8 = bdotprod(_sym_enc, HAMMING128_S8);

    // index
    unsigned int z = (s8<<3) | (s4<<2) | (s2<<1) | s1;

#if DEBUG_FEC_HAMMING128
    printf("syndrome bits (s1,s2,s4,s8) : (%1u,%1u,%1u,%1u)\n", s1, s2, s4, s8);
    printf("syndrome z : %u\n", z);
#endif

    // flip bit at this position
    if (z) {
        if (z > 12) {
            // if this happens there are likely too many errors
            // to correct; just pass without trying to do anything
            //fprintf(stderr,"warning, fec_hamming128_decode_symbol(), syndrome index exceeds 12\n");
        } else {
            //printf("error detected!\n");
            _sym_enc ^= 1 << (12-z);
        }
    }

    // strip data bits (x) from encoded symbol with parity bits (.)
    //      symbol:  [..x. xxx. xxxx]
    //                0000 0000 1111     >  0x000f
    //                0000 1110 0000     >  0x00e0
    //                0010 0000 0000     >  0x0200
    unsigned int sym_dec = ((_sym_enc & 0x000f)     )   |
                           ((_sym_enc & 0x00e0) >> 1)   |
                           ((_sym_enc & 0x0200) >> 2);


    return sym_dec;
}

// create Hamming(12,8) codec object
fec fec_hamming128_create(void * _opts)
{
    fec q = (fec) malloc(sizeof(struct fec_s));

    // set scheme
    q->scheme = FEC_HAMMING128;
    q->rate = fec_get_rate(q->scheme);

    // set internal function pointers
    q->encode_func = &fec_hamming128_encode;
    q->decode_func = &fec_hamming128_decode;

    return q;
}

// destroy Hamming(12,8) object
void fec_hamming128_destroy(fec _q)
{
    free(_q);
}

// encode block of data using Hamming(12,8) encoder
//
//  _q              :   encoder/decoder object
//  _dec_msg_len    :   decoded message length (number of bytes)
//  _msg_dec        :   decoded message [size: 1 x _dec_msg_len]
//  _msg_enc        :   encoded message [size: 1 x 2*_dec_msg_len]
void fec_hamming128_encode(fec _q,
                          unsigned int _dec_msg_len,
                          unsigned char *_msg_dec,
                          unsigned char *_msg_enc)
{
    unsigned int i, j=0;    // input/output symbol counters
    unsigned char s0, s1;   // input 8-bit symbols
    unsigned int m0, m1;    // output 12-bit symbols

    // determine if input length is odd
    unsigned int r = _dec_msg_len % 2;

    for (i=0; i<_dec_msg_len-r; i+=2) {
        // strip two input bytes
        s0 = _msg_dec[i+0];
        s1 = _msg_dec[i+1];

        // encode each byte into 12-bit symbols
        m0 = fec_hamming128_encode_symbol(s0);
        m1 = fec_hamming128_encode_symbol(s1);

        // append both 12-bit symbols to output (three 8-bit bytes)
        _msg_enc[j+0] = m0 & 0xff;              // lower 8 bits of m0
        _msg_enc[j+1] = m1 & 0xff;              // lower 8 bits of m1
        _msg_enc[j+2] = ((m0 & 0x0f00) >> 8) |  // upper 4 bits of m0
                        ((m1 & 0x0f00) >> 4);   // upper 4 bits of m1

        j += 3;
    }

    // if input length is even, encode last symbol by itself
    if (r) {
        // strip last input symbol
        s0 = _msg_dec[_dec_msg_len-1];

        // encode into 12-bit symbol
        m0 = fec_hamming128_encode_symbol(s0);

        // append to output
        _msg_enc[j+0] = m0 & 0xff;  // lower 8 bits of m0
        _msg_enc[j+1] = m0 >> 8;    // upper 4 bits of m0

        j += 2;
    }

    assert(j== fec_get_enc_msg_length(FEC_HAMMING128,_dec_msg_len));
}

// decode block of data using Hamming(12,8) decoder
//
//  _q              :   encoder/decoder object
//  _dec_msg_len    :   decoded message length (number of bytes)
//  _msg_enc        :   encoded message [size: 1 x 2*_dec_msg_len]
//  _msg_dec        :   decoded message [size: 1 x _dec_msg_len]
//
//unsigned int
void fec_hamming128_decode(fec _q,
                          unsigned int _dec_msg_len,
                          unsigned char *_msg_enc,
                          unsigned char *_msg_dec)
{
    unsigned int i=0,j=0;
    unsigned int r = _dec_msg_len % 2;
    unsigned char r0, r1, r2;
    unsigned int m0, m1;

    for (i=0; i<_dec_msg_len-r; i+=2) {
        // strip three input symbols
        r0 = _msg_enc[j+0];
        r1 = _msg_enc[j+1];
        r2 = _msg_enc[j+2];

        // combine three 8-bit symbols into two 12-bit symbols
        m0 = r0 | ((r2 & 0x0f) << 8);
        m1 = r1 | ((r2 & 0xf0) << 4);

        // decode each symbol into an 8-bit byte
        _msg_dec[i+0] = fec_hamming128_decode_symbol(m0);
        _msg_dec[i+1] = fec_hamming128_decode_symbol(m1);

        j += 3;
    }

    // if input length is even, decode last symbol by itself
    if (r) {
        // strip last two input bytes (last byte should only contain
        // for bits)
        r0 = _msg_enc[j+0];
        r1 = _msg_enc[j+1];

        // pack into 12-bit symbol
        m0 = r0 | ((r1 & 0x0f) << 8);

        // decode symbol into an 8-bit byte
        _msg_dec[i++] = fec_hamming128_decode_symbol(m0);

        j += 2;
    }

    assert(j== fec_get_enc_msg_length(FEC_HAMMING128,_dec_msg_len));

    //return num_errors;
}

