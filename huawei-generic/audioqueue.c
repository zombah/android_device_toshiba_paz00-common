#include "audioqueue.h"
#include <string.h>
#include <stdio.h>

#define LOG_NDEBUG 0
#define LOG_TAG "RILAudioQueue"
#include <utils/Log.h>

#define f_mul(a,b) 	(((long long)(a))*(b) >> 28)
#define f_div(a,b) 	((((long long)(a)) << 28) / (b))
#define f_fract(a) 	((a) & ((1<<28)-1))
#define f_intp(a)  	((a) >> 28)
#define F_ONE 		(1<<28)
#define F_NBR(x)	((int)((1<<28) * (x)))

//#define CHECK_MEM_OVERRUN
#define  AUDIOQUEUE_DEBUG 0
#if AUDIOQUEUE_DEBUG
#  define  D(...)   ALOGD(__VA_ARGS__)
#else
#  define  D(...)   ((void)0)
#endif 


/* Return count in buffer.  */
#define CIRC_CNT(wr_pos,rd_pos,size) (((wr_pos) - (rd_pos)) & ((size)-1))

/* Return space available, 0..size-1.  We always leave one free char
   as a completely full buffer has wr_pos == rd_pos, which is the same as
   empty.  */
#define CIRC_SPACE(wr_pos,rd_pos,size) CIRC_CNT((rd_pos),((wr_pos)+1),(size))

/* Return count up to the end of the buffer.  Carefully avoid
   accessing wr_pos and rd_pos more than once, so they can change
   underneath us without returning inconsistent results.  */
#define CIRC_CNT_TO_END(wr_pos,rd_pos,size) \
	({int end = (size) - (rd_pos); \
	  int n = ((wr_pos) + end) & ((size)-1); \
	  n < end ? n : end;})

/* Return space available up to the end of the buffer.  */
#define CIRC_SPACE_TO_END(wr_pos,rd_pos,size) \
	({int end = (size) - 1 - (wr_pos); \
	  int n = (end + (rd_pos)) & ((size)-1); \
	  n <= end ? n : end+1;}) 


	  
// Init the audio queue
int AudioQueue_init(struct AudioQueue* ctx,unsigned int p2maxsamples, unsigned int sample_sz)
{
	unsigned int maxsamples = 1U << p2maxsamples;
	memset(ctx,0,sizeof(*ctx));
	
	ctx->size = maxsamples;
	ctx->low  = maxsamples / 4;       // 1/4 or less means low
	ctx->high = (maxsamples * 3) / 4; // 3/4 or more means high
	ctx->sample_sz = sample_sz;
	
	/* Linear resampler */
	ctx->ratio = F_ONE;
	
	ALOGD("[%p] Initializing audio queue: size: %d, low: %d, high: %d, sample_sz: %d",ctx,ctx->size,ctx->low,ctx->high,ctx->sample_sz);
	
#ifdef CHECK_MEM_OVERRUN
	ctx->data = malloc(8 + maxsamples * sample_sz);
	if (!ctx->data) {
		ALOGE("{%p} Failed to allocate %d memory",ctx, 8 + maxsamples * sample_sz);
		return -1;
	}
	ctx->data = (int*)ctx->data + 1;
	((int*)ctx->data)[-1                           ] = 0x1A2B6C7D;
	((int*)ctx->data)[(ctx->size*ctx->sample_sz)>>2] = 0xD7C6B2A1;
#else
	ctx->data = malloc(maxsamples * sample_sz);
	if (!ctx->data) {
		ALOGE("{%p} Failed to allocate %d memory",ctx, maxsamples * sample_sz);
		return -1;
	}
#endif
	
	// Init audio AGC
	agc_init(&ctx->agc,31000);
	
	ctx->running = 1;
	
	return 0;
}

int AudioQueue_isrunning(struct AudioQueue* ctx)
{
	return ctx->running;
}

int AudioQueue_add(struct AudioQueue* ctx, void* data,unsigned int samples)
{
	unsigned int samples_todo = samples;
	
	// If exited, avoid adding
	if (!ctx->running)
		return 0;
		
	D("add[%p]: begin: Store %d samples",ctx,samples);
	
	// Not filled, add to the queue.
	while (ctx->running && samples_todo) {
	
		// Calculate remaining space until end of buffer. We always leave a byte free 
		//  so we can differenciate between empty and full buffers
		unsigned int rem = CIRC_SPACE_TO_END(ctx->wr_pos,ctx->rd_pos,ctx->size);
				
		D("add[%p]: samples_todo: %u, rem: %u, rd: %u, wr: %u, sz: %u",ctx, samples_todo, rem, ctx->rd_pos, ctx->wr_pos, ctx->size);
		
		if (rem == 0) {
			/* not enough data... Ignore the part we can't store */			
			D("add[%p]: Not enough space on queue...",ctx);
			break; 
			
		} else {

			// Do not store more than needed
			if (rem > samples_todo) {
				rem = samples_todo;
			}
			
			// Store data in queue
			memcpy( (char*)ctx->data + ctx->wr_pos * ctx->sample_sz, data, rem * ctx->sample_sz);
			data = (char*) data + rem * ctx->sample_sz;
			ctx->wr_pos = (ctx->wr_pos + rem) & (ctx->size-1);
			samples_todo -= rem;
		}
	};
	
	D("add[%p]: end: Stored %d samples, size %d, rd:%d, wr:%d",ctx,samples - samples_todo,ctx->size, ctx->rd_pos, ctx->wr_pos);

	// Adjust ratio if queue is getting full, to keep fullness under control
	if (CIRC_CNT(ctx->wr_pos,ctx->rd_pos,ctx->size) > ctx->high) {
		unsigned int ratio = ctx->ratio;
	
		// Adjust ratio to avoid this the next time 
		ratio += ratio/200;
	
		// Limit to sensible values
		if (ratio > F_NBR(1.05)) {
			ratio = F_NBR(1.05);
		}
		ctx->ratio = ratio;
		D("add[%p]: Adjusting ratio to keep queue 3/4 full: New ratio: %u",ctx, ratio);
	}

#ifdef CHECK_MEM_OVERRUN
	if (((int*)ctx->data)[-1                      ] != 0x1A2B6C7D) {
		ALOGE("add[%p] Memory corruption at start: Found: %08x",ctx, ((int*)ctx->data)[-1                      ]);
	}
	
	if (((int*)ctx->data)[(ctx->size*ctx->sample_sz)>>2] != 0xD7C6B2A1) {
		ALOGE("add[%p] Memory corruption at end: Found: %08x",ctx, ((int*)ctx->data)[ctx->size*ctx->sample_sz]);
	}
#endif
	
	// Return the count of stored samples
	return samples - samples_todo;
}

int AudioQueue_get(struct AudioQueue* ctx, void* data,unsigned int samples)
{
	unsigned int samples_todo = samples;
	void* pdata = data;

	// If exited, avoid adding
	if (!ctx->running)
		return 0;
		
	D("get[%p]: begin: Read of %d samples",ctx,samples);
		
	// While samples to be read
	while (ctx->running && samples_todo) {
	
		// Read the first free position
		int av = CIRC_CNT(ctx->wr_pos,ctx->rd_pos,ctx->size);
		unsigned int step = ctx->step, mask = ctx->size-1;
		unsigned int rd_pos = ctx->rd_pos;
		
		D("get[%p]: [1] samples_todo: %u, rd: %u, wr: %u, sz: %u",ctx, samples_todo, ctx->rd_pos, ctx->wr_pos, ctx->size);
		
		// linear interpolation resampling until all requested data is providedno more
		if (ctx->sample_sz == 2 ) {
			
			short const *psrc = ctx->data;
			short *pdst = (short*)pdata;
	
			/* We use the current one and next one samples */
			while (samples_todo) {
				int ipart = f_intp(step);
				rd_pos  = (rd_pos + ipart) & mask;
				av 	   -= ipart;
				step    = f_fract(step);

				// If not enough data, break now
				if (av < 2) 
					break;
				
				*pdst++ = psrc[rd_pos] + f_mul(psrc[(rd_pos+1) & mask] - psrc[rd_pos], step);
				samples_todo--;
				step   += ctx->ratio;
				
				// Update buffer pointers and linear resampler step
				ctx->step = step;
				ctx->rd_pos = rd_pos;
			}
			pdata = pdst;
			
		} else {
			
			unsigned char const *psrc = ctx->data;
			unsigned char *pdst = (unsigned char*)pdata;
	
			/* We use the current one and next one samples */
			while (samples_todo) {
				int ipart = f_intp(step);
				rd_pos  = (rd_pos + ipart) & mask;
				av 	   -= ipart;
				step    = f_fract(step);

				// If not enough data, break now
				if (av < 2) 
					break;

				*pdst++ = psrc[rd_pos] + f_mul(psrc[(rd_pos+1) & mask] - psrc[rd_pos], step);
				samples_todo--;
				step   += ctx->ratio;
				
				// Update buffer pointers and linear resampler step
				ctx->step = step;
				ctx->rd_pos = rd_pos;
			}
			pdata = pdst;
		}
		
		D("get[%p]: [2] samples_todo: %u, rd: %u, wr: %u, sz: %u",ctx, samples_todo, ctx->rd_pos, ctx->wr_pos, ctx->size);			
		if (samples_todo) {
		
			// No more samples to provide....
			D("get[%p]: Not enough data on queue...",ctx);		

			// Return what we have...
			break;
		}
	};

	D("get[%p]: end: got %d samples, total: %d, rd:%d, wr:%d",ctx, samples - samples_todo, ctx->size,ctx->rd_pos, ctx->wr_pos);

	// Adjust ratio if queue is getting empty, to keep fullness under control
	if (CIRC_CNT(ctx->wr_pos,ctx->rd_pos,ctx->size) < ctx->low) {
	
		unsigned int ratio = ctx->ratio;
		ratio -= ratio / 200;
	
		// Limit to sensible values
		if (ratio < F_NBR(0.95)) {
			ratio = F_NBR(0.95);
		}
		ctx->ratio = ratio;
		
		D("get[%p]: Adjusting rate to keep queue at least 1/4 full: New ratio: %u",ctx,ratio);
	}

	// Apply AGC to the samples
	if (ctx->sample_sz == 2 ) {
		agc_process_16bit(&ctx->agc,(short*) data, samples - samples_todo);
	} else {
		agc_process_8bit(&ctx->agc,(unsigned char*) data, samples - samples_todo);
	}
	
#ifdef CHECK_MEM_OVERRUN
	if (((int*)ctx->data)[-1                      ] != 0x1A2B6C7D) {
		ALOGE("get[%p] Memory corruption at start: Found: %08x",ctx, ((int*)ctx->data)[-1                      ]);
	}
	
	if (((int*)ctx->data)[(ctx->size*ctx->sample_sz)>>2] != 0xD7C6B2A1) {
		ALOGE("get[%p] Memory corruption at end: Found: %08x",ctx, ((int*)ctx->data)[ctx->size*ctx->sample_sz]);
	}
#endif


	// Return the count of read samples
	return samples - samples_todo;
}

int AudioQueue_end(struct AudioQueue* ctx)
{
	
	// If exited, avoid reexecuting deinitialization
	if (!ctx->running)
		return 0;
		
	// Signal end
	ctx->running = 0;

	// Some delay to let add and get end...
	sleep(1);
	
#ifdef CHECK_MEM_OVERRUN
	free(((int*)ctx->data)-1);
#else
	free(ctx->data);
#endif

	memset(ctx,0,sizeof(struct AudioQueue));
	return 0;
}
