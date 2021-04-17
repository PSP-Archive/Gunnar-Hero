#ifdef _PSP
#include <pspgu.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <psprtc.h>
#endif
#include <stdio.h>
#include <setjmp.h>
#include <jpeglib.h>
#include "mjpegplayer.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#ifdef _PSP
static unsigned int __attribute__((aligned(16))) list[512];
#endif

struct my_error_mgr {
	struct jpeg_error_mgr pub;    /* "public" fields */

	jmp_buf setjmp_buffer;        /* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

METHODDEF(void) my_error_exit (j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr myerr = (my_error_ptr) cinfo->err;

	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	(*cinfo->err->output_message) (cinfo);

	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}

int playMJPEG(const char *filename,int fps)
{
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	JSAMPARRAY buffer;
	int row_stride;
	char *target;
	//int frameNo=0;

	if(fps<1) fps=1;

	FILE *in=fopen(filename,"rb");
	if(!in) return -1;
	printf("Opened %s\n",filename);

	cinfo.err=jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit=my_error_exit;
	jpeg_create_decompress(&cinfo);

	if(setjmp(jerr.setjmp_buffer)) {
#ifdef _PSP
		sceGuDisplay(GU_FALSE);
#endif
		jpeg_destroy_decompress(&cinfo);
		fclose(in);
		return -2;
	}

	jpeg_stdio_src(&cinfo,in);

#ifdef _PSP
	sceGuInit();
	sceGuStart(GU_DIRECT,list);
	sceGuDrawBuffer(GU_PSM_8888,(void *)(512*272*4),512);
	sceGuDispBuffer(480,272,(void *)0,512);
	sceGuAmbientColor(0xffffffff);
	sceGuClear(GU_COLOR_BUFFER_BIT);
	int count=sceGuFinish();
	printf("Used %d entries from the list.\n",count);
	sceGuSync(0,0);
	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);

	target=(char *)sceGuSwapBuffers()+0x4000000;
#endif

	// Read a sequence of jpegs:
	while(1) {
#ifdef _PSP		
		u64 start,end;
		sceRtcGetCurrentTick(&start);
#endif
		jpeg_read_header(&cinfo,TRUE);
		// Print the header parameters...
		//printf("Image is %dx%d\n",cinfo.image_width,cinfo.image_height);
		// Set the desired target color space
		cinfo.out_color_space=JCS_RGB;

		jpeg_start_decompress(&cinfo);
		/*
		 * output_width            image width and height, as scaled
		 * output_height
		 * out_color_components    # of color components in out_color_space
		 * output_components       # of color components returned per pixel
		 * colormap                the selected colormap, if any
		 * actual_number_of_colors         number of entries in colormap
		 */                
		// Allocate buffers for data...
		row_stride=cinfo.output_width*cinfo.output_components;
		buffer=(*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo,JPOOL_IMAGE,row_stride,1);
		while(cinfo.output_scanline<cinfo.output_height) {
			jpeg_read_scanlines(&cinfo,buffer,1);
#ifdef _PSP
			if(cinfo.output_scanline*2>=272) continue;
			int i;
			for(i=0;i<cinfo.output_width && i*2<480;i++) {
				// Should do pixel doubling here. -- bilinear would be nice.
				target[i*8+0]=buffer[0][i*3+0];
				target[i*8+1]=buffer[0][i*3+1];
				target[i*8+2]=buffer[0][i*3+2];
				target[i*8+3]=0xff;
				target[i*8+4]=buffer[0][i*3+0];
				target[i*8+5]=buffer[0][i*3+1];
				target[i*8+6]=buffer[0][i*3+2];
				target[i*8+7]=0xff;
				target[512*4+i*8+0]=buffer[0][i*3+0];
				target[512*4+i*8+1]=buffer[0][i*3+1];
				target[512*4+i*8+2]=buffer[0][i*3+2];
				target[512*4+i*8+3]=0xff;
				target[512*4+i*8+4]=buffer[0][i*3+0];
				target[512*4+i*8+5]=buffer[0][i*3+1];
				target[512*4+i*8+6]=buffer[0][i*3+2];
				target[512*4+i*8+7]=0xff;
			}
			target+=512*4*2;
#endif			
		}
		jpeg_finish_decompress(&cinfo);
		//printf("Finished image %d\n",++frameNo);
#ifdef _PSP
		sceKernelDcacheWritebackInvalidateAll();
		sceDisplayWaitVblankStart();
		while(1) {
			sceRtcGetCurrentTick(&end);
			if(end-start>=1000000/fps) break;
			sceDisplayWaitVblankStart();
		}
		target=(char *)sceGuSwapBuffers()+0x44000000;
#endif
	}
	fclose(in);
	jpeg_destroy_decompress(&cinfo);

	return 0;
}

#ifndef _PSP
int main(int argc,char **argv) {
	if(argc<2) { printf("Usage: %s <imagefile.mjpeg>\n",argv[0]); return 5; }
	printf("Decompressing %s\n",argv[1]);
	playMJPEG(argv[1]);
	printf("Exiting...\n");
	return 0;
}
#endif
