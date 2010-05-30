
#include <windows.h>
#include "libmad.h"
#include "stream.h"
#include "frame.h"
#include "synth.h"
#include "fixed.h"

struct audio_dither {
  mad_fixed_t error[3];
  mad_fixed_t random;
};

struct audio_stats {
  unsigned long clipped_samples;
  mad_fixed_t peak_clipping;
  mad_fixed_t peak_sample;
};

typedef struct
{
	struct mad_stream stream;
	struct mad_frame frame;
	struct mad_synth synth;
	struct audio_stats stats;

	int equalizer;
	int left;
	mad_fixed_t eqfactor[32];
}dec_struct;

void equalizer_init(mad_fixed_t eqfactor[32])
{
	int i;
	for (i = 0; i < 32; i++)
		eqfactor[i] = 0x10000000;
}

HANDLE mad_init()
{
	dec_struct* dec = malloc(sizeof(dec_struct));
	if (!dec)
		return NULL;

	memset(dec, 0, sizeof(dec_struct));
	mad_stream_init(&dec->stream);
	mad_frame_init(&dec->frame);
	mad_synth_init(&dec->synth);

	equalizer_init(dec->eqfactor);
	dec->equalizer = 0;
	dec->left = 0;

	dec->stats.clipped_samples = 0;
	dec->stats.peak_clipping = 0;
	dec->stats.peak_sample = 0;

	return dec;
}

static __inline
void clip(mad_fixed_t *sample, struct audio_stats *stats)
{
	enum {
		MIN = -MAD_F_ONE,
		MAX =  MAD_F_ONE - 1
	};

	if (*sample >= stats->peak_sample) {
		if (*sample > MAX) {
			++stats->clipped_samples;
			if (*sample - MAX > stats->peak_clipping)
				stats->peak_clipping = *sample - MAX;

			*sample = MAX;
		}
		stats->peak_sample = *sample;
	}
	else if (*sample < -stats->peak_sample) {
		if (*sample < MIN) {
			++stats->clipped_samples;
			if (MIN - *sample > stats->peak_clipping)
				stats->peak_clipping = MIN - *sample;

			*sample = MIN;
		}
		stats->peak_sample = -*sample;
	}
}

static __inline
signed long audio_linear_round(unsigned int bits, mad_fixed_t sample,
			       struct audio_stats *stats)
{
	/* round */
	sample += (1L << (MAD_F_FRACBITS - bits));

	/* clip */
	clip(&sample, stats);

	/* quantize and scale */
	return sample >> (MAD_F_FRACBITS + 1 - bits);
}

static __inline
unsigned long prng(unsigned long state)
{
  return (state * 0x0019660dL + 0x3c6ef35fL) & 0xffffffffL;
}

static __inline
signed long audio_linear_dither(unsigned int bits, mad_fixed_t sample,
				struct audio_dither *dither,
				struct audio_stats *stats)
{
	unsigned int scalebits;
	mad_fixed_t output, mask, random;

	enum {
		MIN = -MAD_F_ONE,
		MAX =  MAD_F_ONE - 1
	};

	/* noise shape */
	sample += dither->error[0] - dither->error[1] + dither->error[2];

	dither->error[2] = dither->error[1];
	dither->error[1] = dither->error[0] / 2;

	/* bias */
	output = sample + (1L << (MAD_F_FRACBITS + 1 - bits - 1));

	scalebits = MAD_F_FRACBITS + 1 - bits;
	mask = (1L << scalebits) - 1;

	/* dither */
	random  = prng(dither->random);
	output += (random & mask) - (dither->random & mask);

	dither->random = random;

	/* clip */
	if (output >= stats->peak_sample) {
		if (output > MAX) {
			++stats->clipped_samples;
			if (output - MAX > stats->peak_clipping)
				stats->peak_clipping = output - MAX;

			output = MAX;

			if (sample > MAX)
				sample = MAX;
		}
		stats->peak_sample = output;
	}
	else if (output < -stats->peak_sample) {
		if (output < MIN) {
			++stats->clipped_samples;
			if (MIN - output > stats->peak_clipping)
				stats->peak_clipping = MIN - output;

			output = MIN;

			if (sample < MIN)
				sample = MIN;
		}
		stats->peak_sample = -output;
	}

	/* quantize */
	output &= ~mask;

	/* error feedback */
	dither->error[0] = sample - output;

	/* scale */
	return output >> scalebits;
}

static
unsigned int pack_pcm(unsigned char *data, unsigned int nsamples,
		      mad_fixed_t const *left, mad_fixed_t const *right,
		      int resolution, struct audio_stats *stats)
{
	static struct audio_dither left_dither, right_dither;
	unsigned char const *start;
	register signed long sample0, sample1;
	int effective, bytes;

	start     = data;
	effective = (resolution > 24) ? 24 : resolution;
	bytes     = resolution / 8;

	if (right) {  /* stereo */
		while (nsamples--) {
#ifdef LINEAR_DITHER
			sample0 = audio_linear_dither(effective, *left++, &left_dither, stats);
			sample1 = audio_linear_dither(effective, *right++, &right_dither, stats);
#else
			sample0 = audio_linear_round(effective, *left++, stats);
			sample1 = audio_linear_round(effective, *right++, stats);
#endif

			switch (resolution) {
			case 8:
				data[0] = sample0 + 0x80;
				data[1] = sample1 + 0x80;
				break;
			case 32:
				sample0 <<= 8;
				sample1 <<= 8;
				data[        3] = sample0 >> 24;
				data[bytes + 3] = sample1 >> 24;
			case 24:
				data[        2] = sample0 >> 16;
				data[bytes + 2] = sample1 >> 16;
			case 16:
				data[        1] = sample0 >>  8;
				data[bytes + 1] = sample1 >>  8;
				data[        0] = sample0 >>  0;
				data[bytes + 0] = sample1 >>  0;
			}
			data += bytes * 2;
		}
	}
	else {  /* mono */
		while (nsamples--) {
#ifdef LINEAR_DITHER
			sample0 = audio_linear_dither(effective, *left++, &left_dither, stats);
#else
			sample0 = audio_linear_round(effective, *left++, stats);
#endif
			switch (resolution) {
			case 8:
				data[0] = sample0 + 0x80;
				break;
			case 32:
				sample0 <<= 8;
				data[3] = sample0 >> 24;
			case 24:
				data[2] = sample0 >> 16;
			case 16:
				data[1] = sample0 >>  8;
				data[0] = sample0 >>  0;
			}
			data += bytes;
		}
	}
	return data - start;
}

int mad_decode(HANDLE hMad, char *inmemory,int inmemsize, char *outmemory, 
			   int outmemsize, int* read, int *done, int resolution, int halfsamplerate)
{
	int size;
	dec_struct* dec = (dec_struct*)hMad;
	
	*done = 0;
	*read = 0;

	if (dec->left) {
		size = min(outmemsize / (resolution * dec->synth.pcm.channels / 8), dec->left);
		pack_pcm(outmemory, size, &dec->synth.pcm.samples[0][dec->synth.pcm.length - dec->left],
			(dec->synth.pcm.channels == 1) ? NULL : &dec->synth.pcm.samples[1][dec->synth.pcm.length - dec->left], 
			resolution, &dec->stats);
		*done += size * resolution * dec->synth.pcm.channels / 8;
		outmemsize -= size * resolution * dec->synth.pcm.channels / 8;
	
		dec->left -= size;
		if (dec->left)
			return MAD_NEED_MORE_OUTPUT;
		if (!inmemsize)
			return MAD_OK;
	}

	mad_stream_buffer(&dec->stream, inmemory, inmemsize);
	
	if (mad_frame_decode(&dec->frame, &dec->stream) == -1) {
		if (dec->stream.error == MAD_ERROR_BUFLEN)
			return MAD_NEED_MORE_INPUT;
		return MAD_RECOVERABLE(dec->stream.error) ? MAD_ERR : MAD_FATAL_ERR;
	}

	if (dec->equalizer)
	{
		unsigned int nch, ch, ns, s, sb;

		nch = MAD_NCHANNELS(&dec->frame.header);
		ns  = MAD_NSBSAMPLES(&dec->frame.header);
		for (ch = 0; ch < nch; ++ch) {
			for (s = 0; s < ns; ++s) {
				for (sb = 0; sb < 32; ++sb) {
					dec->frame.sbsample[ch][s][sb] =
					mad_f_mul(dec->frame.sbsample[ch][s][sb], dec->eqfactor[sb]);
				}
			}
		}
	}

	if (halfsamplerate)
		dec->frame.options |= MAD_OPTION_HALFSAMPLERATE;
	else
		dec->frame.options &= ~MAD_OPTION_HALFSAMPLERATE;
	mad_synth_frame(&dec->synth, &dec->frame);

	if (outmemsize < dec->synth.pcm.length * resolution * dec->synth.pcm.channels / 8) {
		size = outmemsize / (resolution * dec->synth.pcm.channels / 8);
		pack_pcm(outmemory + *done, size, dec->synth.pcm.samples[0],
			(dec->synth.pcm.channels == 1) ? NULL : dec->synth.pcm.samples[1], resolution, &dec->stats);
		*done += size * resolution * dec->synth.pcm.channels / 8;
		*read += dec->stream.next_frame - inmemory;
		dec->left = dec->synth.pcm.length - size;
		return MAD_NEED_MORE_OUTPUT;
	}

	pack_pcm(outmemory + *done, dec->synth.pcm.length, dec->synth.pcm.samples[0],
		(dec->synth.pcm.channels == 1) ? NULL : dec->synth.pcm.samples[1], resolution, &dec->stats);
	*done += dec->synth.pcm.length * resolution * dec->synth.pcm.channels / 8;
	*read += dec->stream.next_frame - inmemory;
	return MAD_OK;
}

void mad_uninit(HANDLE hMad)
{
	dec_struct* dec = (dec_struct*)hMad;

	mad_synth_finish(&dec->synth);
	mad_frame_finish(&dec->frame);
	mad_stream_finish(&dec->stream);

	free(dec);
}

double eq_decibels(int value)
{
	/* 0-63, 0 == +20 dB, 31 == 0 dB, 63 == -20 dB */
	return (value == 31) ? 0.0 : 20.0 - (20.0 / 31.5) * value;
}

mad_fixed_t eq_factor(double db)
{
	if (db > 18)
		db = 18;

	return mad_f_tofixed(pow(10, db / 20));
}

void mad_seteq(HANDLE hMad, equalizer_value* eq)
{
	double base;
	static unsigned char const map[32] = {
	0, 1, 2, 3, 4, 5, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8,
	8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9
	};
	int i;
	dec_struct* dec = (dec_struct*)hMad;

	/* 60, 170, 310, 600, 1k, 3k, 6k, 12k, 14k, 16k */

	base = eq_decibels(eq->preamp);

	for (i = 0; i < 32; ++i)
		dec->eqfactor[i] = eq_factor(base + eq_decibels(eq->data[map[i]]));
	dec->equalizer = eq->enable;
}
