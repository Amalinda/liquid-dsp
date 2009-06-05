//
//
//

#include <stdio.h>

#include "liquid.h"

#define OUTPUT_FILENAME "fir_farrow_rrrf_example.m"

int main() {
    // options
    unsigned int h_len=17;  // filter length
    unsigned int p=5;       // polynomial order
    float fc=0.9f;          // filter cutoff
    float slsl=60.0f;       // sidelobe suppression level
    unsigned int m=7;       // number of delays to evaluate

    // coefficients array
    float h[h_len];
    float tao = ((float)h_len-1)/2.0f;  // nominal filter delay

    fir_farrow_rrrf f = fir_farrow_rrrf_create(h_len, p, slsl);

    FILE*fid = fopen(OUTPUT_FILENAME,"w");
    fprintf(fid,"%% fir_filter_rrrf_example.m: auto-generated file\n\n");
    fprintf(fid,"clear all;\nclose all;\n\n");
    fprintf(fid,"m = %u;\n", m);
    fprintf(fid,"h_len = %u;\n", h_len);
    fprintf(fid,"h = zeros(%u,%u);\n", m, h_len);
    fprintf(fid,"mu = zeros(1,%u);\n", m);
    fprintf(fid,"tao = %12.8f;\n", tao);

    unsigned int i, j;
    float mu_vect[m];
    for (i=0; i<m; i++) {
        mu_vect[i] = ((float)i)/((float)m-1) - 0.5f;
        printf("mu[%3u] = %12.8f\n", i, mu_vect[i]);
        fprintf(fid,"mu(%3u) = %12.8f;\n", i+1, mu_vect[i]);
    }

    for (i=0; i<m; i++) {
        fir_farrow_rrrf_set_delay(f,mu_vect[i]);
        fir_farrow_rrrf_get_coefficients(f,h);
        for (j=0; j<h_len; j++)
            fprintf(fid,"  h(%3u,%3u) = %12.4e;\n", i+1, j+1, h[j]);
    }

   
    // compute delay, print results
    fprintf(fid,"nfft=512;\n");
    fprintf(fid,"g = 0:(h_len-1);\n");
    fprintf(fid,"D = zeros(m,nfft);\n");
    fprintf(fid,"for i=1:m,\n");
    fprintf(fid,"    d = real( fft(h(i,:).*g,2*nfft) ./ fft(h(i,:),2*nfft) );\n");
    fprintf(fid,"    D(i,:) = d(1:nfft);\n");
    fprintf(fid,"end;\n");

    fprintf(fid,"f = [0:(nfft-1)]/(2*nfft);\n");
    fprintf(fid,"figure;\n");
    fprintf(fid,"plot(f,D,'-k','LineWidth',2);\n");
    fprintf(fid,"axis([0 0.5 (tao-1) (tao+1)]);\n");
    fprintf(fid,"for i=1:m,\n");
    fprintf(fid,"    text(0.3,tao+mu(i)+0.025,['\mu =' num2str(mu(i))]);\n");
    fprintf(fid,"end;\n");

    fclose(fid);
    printf("results written to %s\n", OUTPUT_FILENAME);

    fir_farrow_rrrf_destroy(f);

    printf("done.\n");
    return 0;
}

