/*
 * Copyright (c) 2007, 2008, 2009, 2010, 2011, 2012 Joseph Gaeddert
 * Copyright (c) 2007, 2008, 2009, 2010, 2011, 2012 Virginia Polytechnic
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
// fft.common.c
//

#include "liquid.internal.h"

struct FFT(plan_s) {
    unsigned int nfft;  // fft size
    TC * twiddle;       // twiddle factors
    TC * x;             // input array pointer (not allocated)
    TC * y;             // output array pointer (not allocated)
    int direction;      // forward/reverse
    int flags;
    liquid_fft_kind kind;
    liquid_fft_method method;

    void (*execute)(FFT(plan)); // function pointer

    // real even/odd DFT parameters (DCT/DST)
    T * xr; // input array (real)
    T * yr; // output array (real)
};

// execute fft : simply calls internal function pointer
void FFT(_execute)(FFT(plan) _q)
{
    _q->execute(_q);
}

// perform _n-point fft shift
void FFT(_shift)(TC *_x, unsigned int _n)
{
    unsigned int i, n2;
    if (_n%2)
        n2 = (_n-1)/2;
    else
        n2 = _n/2;

    TC tmp;
    for (i=0; i<n2; i++) {
        tmp = _x[i];
        _x[i] = _x[i+n2];
        _x[i+n2] = tmp;
    }
}

