/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2014 - Daniel De Matteis
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef DSPFILTER_API_H__
#define DSPFILTER_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#define DSPFILTER_SIMD_SSE      (1 << 0)
#define DSPFILTER_SIMD_SSE2     (1 << 1)
#define DSPFILTER_SIMD_VMX      (1 << 2)
#define DSPFILTER_SIMD_VMX128   (1 << 3)
#define DSPFILTER_SIMD_AVX      (1 << 4)
#define DSPFILTER_SIMD_NEON     (1 << 5)
#define DSPFILTER_SIMD_SSE3     (1 << 6)
#define DSPFILTER_SIMD_SSSE3    (1 << 7)
#define DSPFILTER_SIMD_MMX      (1 << 8)
#define DSPFILTER_SIMD_MMXEXT   (1 << 9)
#define DSPFILTER_SIMD_SSE4     (1 << 10)
#define DSPFILTER_SIMD_SSE42    (1 << 11)
#define DSPFILTER_SIMD_AVX2     (1 << 12)
#define DSPFILTER_SIMD_VFPU     (1 << 13)
#define DSPFILTER_SIMD_PS       (1 << 14)

/* A bit-mask of all supported SIMD instruction sets.
 * Allows an implementation to pick different 
 * dspfilter_implementation structs.
 */
typedef unsigned dspfilter_simd_mask_t;

/* Dynamic library endpoint. */
typedef const struct dspfilter_implementation *(
      *dspfilter_get_implementation_t)(dspfilter_simd_mask_t mask);

/* The same SIMD mask argument is forwarded to create() callback 
 * as well to avoid having to keep lots of state around. */
const struct dspfilter_implementation *dspfilter_get_implementation(
      dspfilter_simd_mask_t mask);

#define DSPFILTER_API_VERSION 1

struct dspfilter_info
{
   /* Input sample rate that the DSP plugin receives. */
   float input_rate;
};

struct dspfilter_output
{
   /* The DSP plugin has to provide the buffering for the 
    * output samples or reuse the input buffer directly.
    *
    * The samples are laid out in interleaving order: LRLRLRLR
    * The range of the samples are [-1.0, 1.0]. 
    *
    * It is not necessary to manually clip values. */
   float *samples;

   /* Frames which the DSP plugin outputted for the current process.
    *
    * One frame is here defined as a combined sample of 
    * left and right channels. 
    *
    * (I.e. 44.1kHz, 16bit stereo will have 
    * 88.2k samples/sec and 44.1k frames/sec.)
    */
   unsigned frames;
};

struct dspfilter_input 
{
   /* Input data for the DSP. The samples are interleaved in order: LRLRLRLR
    *
    * It is valid for a DSP plug to use this buffer for output as long as 
    * the output size is less or equal to the input.
    *
    * This is useful for filters which can output one sample for each 
    * input sample and do not need to maintain its own buffers.
    *
    * Block based filters must provide their own buffering scheme.
    *
    * The input size is not bound, but it can be safely assumed that it 
    * will not exceed ~100ms worth of audio at a time. */
   float *samples;

   /* Number of frames for input data.
    * One frame is here defined as a combined sample of 
    * left and right channels. 
    *
    * (I.e. 44.1kHz, 16bit stereo will have 
    * 88.2k samples/sec and 44.1k frames/sec.)
    */
   unsigned frames;
};

/* Returns true if config key was found. Otherwise, 
 * returns false, and sets value to default value.
 */
typedef int (*dspfilter_config_get_float_t)(void *userdata,
      const char *key, float *value, float default_value);

typedef int (*dspfilter_config_get_int_t)(void *userdata,
      const char *key, int *value, int default_value);

/* Allocates an array with values. free() with dspfilter_config_free_t. */
typedef int (*dspfilter_config_get_float_array_t)(void *userdata, 
      const char *key, float **values, unsigned *out_num_values,
      const float *default_values, unsigned num_default_values);

typedef int (*dspfilter_config_get_int_array_t)(void *userdata,
      const char *key, int **values, unsigned *out_num_values,
      const int *default_values, unsigned num_default_values);

typedef int (*dspfilter_config_get_string_t)(void *userdata,
      const char *key, char **output, const char *default_output);

/* Calls free() in host runtime. Sometimes needed on Windows. 
 * free() on NULL is fine. */
typedef void (*dspfilter_config_free_t)(void *ptr);

struct dspfilter_config
{
   dspfilter_config_get_float_t get_float;
   dspfilter_config_get_int_t get_int;

   dspfilter_config_get_float_array_t get_float_array;
   dspfilter_config_get_int_array_t get_int_array;

   dspfilter_config_get_string_t get_string;
   /* Avoid problems where DSP plug and host are 
    * linked against different C runtimes. */
   dspfilter_config_free_t free; 
};

/* Creates a handle of the plugin. Returns NULL if failed. */
typedef void *(*dspfilter_init_t)(const struct dspfilter_info *info,
      const struct dspfilter_config *config, void *userdata);

/* Frees the handle. */
typedef void (*dspfilter_free_t)(void *data);

/* Processes input data. 
 * The plugin is allowed to return variable sizes for output data. */
typedef void (*dspfilter_process_t)(void *data,
      struct dspfilter_output *output, const struct dspfilter_input *input);

struct dspfilter_implementation
{
   dspfilter_init_t     init;
   dspfilter_process_t  process;
   dspfilter_free_t     free;

   /* Must be DSPFILTER_API_VERSION */
   unsigned api_version;

   /* Human readable identifier of implementation. */
   const char *ident;

   /* Computer-friendly short version of ident.
    * Lower case, no spaces and special characters, etc. */
   const char *short_ident; 
};

#ifdef __cplusplus
}
#endif

#endif
