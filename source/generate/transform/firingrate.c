//**** This is not part of the DynaSys project ****
//Compile with gcc -std=c11 -O3 -lm -Wno-unused-result firingrate.c -o firingrate.out

#include "stdio.h"
#include "math.h"
#include "stdlib.h"
#include "string.h" //Need for strcmp, would like to avoid

#define MAX_NUM_SPIKES 1024 * 1024

#ifndef bool
typedef int bool;
#endif
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

//#define DS_DEBUG

int GetSpikes(const double* data, const int vidx, const int num_fields, const int num_samples,
               const double thresh, int* spikes)
{
#ifdef DS_DEBUG
    fprintf(stderr, "GetSpikes\n");
#endif
    int spike_now = false;
    int spike_ct = 0;
    for (int i=0; i<num_samples; ++i)
    {
        double v = data[i*num_fields + vidx];
        if (spike_now==true)
        {
            if (v>thresh) continue;
            spike_now = false;
        }
        else if (v>thresh)
        {
#ifdef DS_DEBUG
    fprintf(stderr, "%f, %d, %d\n", v, spike_ct, i);
#endif
            spikes[spike_ct++] = i;
            spike_now = true;
        }
    }
    return spike_ct;
}

void CountSpikes(const int* spikes, const int num_spikes,
                 const int num_divs, const int samps_per_div, double* fr)
{
    if (num_spikes==0)
    {
        for (int i=0; i<num_divs; ++i) fr[i] = 0.0;
        return;
    }

    int spk_idx = 0;
    double* spike_cts = (double*)malloc(num_divs*sizeof(double));
    int spike = spikes[spk_idx++];
    for (int i=0; i<num_divs; ++i)
    {
        int div_end = (i+1)*samps_per_div;
        while (spike<div_end)
        {
            ++spike_cts[i];
            if (spk_idx>num_spikes) break;
            spike = spikes[spk_idx++];
        }
        if (spk_idx>num_spikes) break;
    }

    for (int i=0; i<num_divs; ++i)
        fr[i] = (1000.0/(double)samps_per_div)*spike_cts[i];
}

int main(int argc, char* argv[])
{
    fprintf(stdout, "%s\n", argv[0]);

    if (argc<4) return 2;

    FILE* fp = fopen(argv[1], "rb");
    if (fp==0) return 1;

    const int samps_per_div = atoi(argv[2]);
    const double thresh = atof(argv[3]);
    fprintf(stdout, "samples per division: %d\nthreshold: %f\n", samps_per_div, thresh);

    size_t br;
    int vnum;
    br = fread(&vnum, sizeof(int), 1, fp);
    fprintf(stdout, "DynaSys version %d\n", vnum);

    int num_fields;
    br = fread(&num_fields, sizeof(int), 1, fp);
    char** fields = (char**)malloc(num_fields*sizeof(char*));
    fprintf(stdout, "Fields (%d):\n", num_fields);

    for (int i=0; i<num_fields; ++i)
    {
        int len;
        br = fread(&len, sizeof(int), 1, fp);
        fields[i] = (char*)malloc(len*sizeof(char));
        fgets(fields[i], len+1, fp);
        fprintf(stdout, "\t%s\n", fields[i]);
    }

    int nth_sample, num_samples;
    br = fread(&nth_sample, sizeof(int), 1, fp);
    br = fread(&num_samples, sizeof(int), 1, fp);
    fprintf(stdout, "Number of samples saved: %d\n", num_samples);

    double* data = (double*)malloc((num_fields*num_samples) * sizeof(double));
    br = fread(data, sizeof(double), num_fields*num_samples, fp);
    fprintf(stdout, "%u bytes of data read.\n", br*sizeof(double));
    fclose(fp);

    int vidx=0;
    while (strcmp(fields[vidx], "V")) ++vidx;
#ifdef DS_DEBUG
    fprintf(stdout, "vidx: %d, %s\n", vidx, fields[vidx]);
#endif

    int* spikes = (int*)malloc(MAX_NUM_SPIKES*sizeof(int));
    int num_spikes = GetSpikes(data, vidx, num_fields, num_samples, thresh, spikes);

    const int num_divs = num_samples / samps_per_div;
    double* fr = (double*)malloc(num_divs*sizeof(double));
    CountSpikes(spikes, num_spikes, num_divs, samps_per_div, fr);

    FILE* fp_out = fopen("error.dsfdat", "wb");
    fwrite(&num_divs, sizeof(int), 1, fp_out);
    fwrite(fr, sizeof(double), num_divs, fp_out);
    fclose(fp_out);

    free(data);
    free(spikes);
    for (int i=0; i<num_fields; ++i) free(fields[i]);
    free(fields);

    fprintf(stderr, "firingrate exited without crashing\n");

    return 0;
}
