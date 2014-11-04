#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* size in bytes, irgendeine dateigroesse daweil wurscht*/
#define FLACFILESIZE 18896786
/*whole size -> 18896786*/

const char path[]="../matlab/flac/marvin.flac";
const char mode = 'r';

FILE *flac_music;

unsigned int sizecounter = FLACFILESIZE;
char content[FLACFILESIZE];

uint32_t get_utf8(unsigned char *get_byte)
{
	uint32_t val = get_byte[0];
	uint32_t top = (val & 128) >> 1;
	if((val & 0xc0) == 0x80 || val >= 0xFE)
	{
			printf("Error UTF-8\n");
			return 1;
	}	
	while(val & top)
	{
		int tmp = get_byte[0] - 128;
		if(tmp>>6)
		{
			printf("Error UTF-8\n");
		}
		val= (val<<6) + tmp;
		top <<= 5;
	}
	val &= (top << 1) - 1;
	return val;
}

/* binary view */
void printbincharpad(char c)
{
    for (int i = 7; i >= 0; --i)
    {
        putchar( (c & (1 << i)) ? '1' : '0' );
    }
    putchar('\n');
}
/* decode header and skip footer */
char *flac_decode_frame_header(unsigned char *buffer)
{
	unsigned int blocking_strategy = 0;
	unsigned int bs_code = 0;
	unsigned int sr_code = 0;
	unsigned int channel_assignment = 0;	
	unsigned int samplesisze = 0;
	uint16_t samplerate = 0;	
	uint16_t blocksize = 0;
	/* frame sync code */
	while(! ( (( (buffer[0]& 0xFF) == 0xFF )&&( (buffer[1]& 0xF8) == 0xF8 )) || (sizecounter == 0)) )
	{
		buffer++;	
		sizecounter--;
	}
	/* 0x02 - Reserved is wurscht*/
	if(buffer[1]&0x01 == 0x01)
	{
		blocking_strategy = 1;
	}else{
		blocking_strategy = 0;
	}
	if(blocking_strategy)
	{
		printf("Variable blocksize\n");
	}else{		
		printf("Fixed blocksize\n");
	}
	/*jump over sync, reserved and blocking strategy*/
	buffer += 2; 	
	/*Block size in inter-channel samples*/
	bs_code = buffer[0] >> 4;
	printf("Blocksize: %d\n",blocksize);
	/*Sample rate (should be 0x09 - 44100)*/
	sr_code = buffer[0]&0x0F;
	if(sr_code != 0x09)
	{
		printf("not 44100khz!!\n");
		return 0;
	}else{
		printf("Sample Rate: 44100kHz\n");
	}
	buffer++; /* next byte */
	/* Channel assignment, Sample size in bits, Reserved */
	channel_assignment = buffer[0] >> 4;
	samplesisze = buffer[0]&0x0E >> 1;
	buffer++; /* next byte */
	
	uint32_t val = get_utf8(buffer);
	if(blocking_strategy)
	{
		printf("Sample number in UTF-8:%d\n",val);
	}else{
		printf("Frame number in UTF-8:%d\n",val);		
	}	
	buffer++;
	if((bs_code& 0x0111)==0x0110)				/*if(blocksize bits == 011x)*/
	{
		blocksize = buffer[0];
		buffer++;
	}else if((bs_code& 0x0111)==0x0111){
		blocksize = 8 << buffer[0];
		blocksize |= buffer[1];
		buffer += 2;
	}
	if((sr_code& 0x1101)==0x1100)				/*if(sample rate bits == 11xx)*/
	{
		samplerate = buffer[0];
		buffer++;
	}else if((sr_code& 0x1101)==0x1101){
		samplerate = 8 << buffer[0];
		samplerate |= buffer[1];
		buffer += 2;
	}
	/* skip crc check */
	buffer++;
	return buffer;
}

int main (void)
{
	/* File variables */
	unsigned char * filestream = 0;	
	long length;
	FILE * f = fopen (path, "rb");	

	if (f)
	{
	  fseek (f, 0, SEEK_END);
	  length = ftell (f);
	  fseek (f, 0, SEEK_SET);
	  filestream = malloc (length);
	  if (filestream)
	  {
		fread (filestream, 1, length, f);
	  }
	  fclose (f);
	}

	filestream = flac_decode_frame_header(filestream);

	uint8_t subframe_type = 0;
	uint8_t subframe_lpc_order = 0;
	uint8_t subframe_fixed_order = 0;
	
	subframe_type = (filestream[0]&01111110) >> 1;
	printbincharpad(subframe_type);
	printf("da");
	if(subframe_type&0x20 == 0x20)			/* SUBFRAME_LPC, xxxxx=order-1  */
	{
		printf("SUBFRAME LPC\n");
		subframe_lpc_order = (subframe_type&0x1F)+1;		
	}else if((subframe_type&0x38 == 0x08)&&(subframe_type&0x07 <= 4) ){	/* if(xxx <= 4) SUBFRAME_FIXED, xxx=order ; else reserved  */
		subframe_fixed_order = subframe_fixed_order&0x07;
		printf("SUBFRAME FIXED\n");
	}else if(subframe_type&0x3F == 0x01){	/* SUBFRAME_VERBATIM */
		
		printf("SUBFRAME VERBATIM\n");
	}else if(subframe_type&0x3F == 0x00){	/* SUBFRAME_CONSTANT */
		
		printf("SUBFRAME CONSTANT\n");
	}
	/* skip frame footer */
//	printbincharpad(buffer[0]);
//	printbincharpad(buffer[1]);
//	buffer += 2;
	
	for(int i=0;i<600;i++)
	{
		printbincharpad(filestream[i]);
		//printf("%x\n",buffer[i]);
	}	
	return 0;
}