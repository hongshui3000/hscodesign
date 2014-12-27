#include <alt_types.h>
//#include "sys/alt_stdio.h"
#include "sys/alt_irq.h"
#include "sys/alt_flash.h"
#include <io.h>
#include <system.h>
#include <malloc.h>
#include <ctype.h>
#include "flac_dec.h"
#include "flac_dsp.h"

#include <sys/alt_alarm.h>

//#define printf printf
#define fprintf(shit, fmt, ...) printf(fmt, ##__VA_ARGS__)

#include <stdio.h>
//#include <stdlib.h>
#include <stdint.h>
#include <string.h>
//#include <errno.h>

#include "../adc128s022.h"

#include "Altera_UP_SD_Card_Avalon_Interface.h"
#include "altera_up_avalon_audio.h"
#include "altera_up_avalon_audio_regs.h"
#include "altera_up_avalon_audio_and_video_config.h"
#include "altera_avalon_performance_counter.h"

#define READ_BLOCK 17

#define SD_CARD_STATUS      0x0234
#define SD_CARD_ARGUMENT    0x022C
#define SD_CARD_COMMAND     0x0230

static int32_t *decoded_l[2];
static int32_t *decoded_r[2];
static int decoded_idx, decoded_pos, decoded_len, decoded_chmod;

static uint32_t sdcard_buf[512 / 4];

alt_up_sd_card_dev *sd_card_dev;
static int *sdcard_com_arg_reg;
static short int *sdcard_com_reg;
static short int *sdcard_aux_stat_reg;

alt_up_audio_dev *audio_dev;

static int decode_main();
static int decode_frame(GetBitContext *gb, int32_t *out_l, int32_t *out_r, int *ch_mode);

static void read_block(GetBitContext *gb)
{
    int status, i;

    while ((status = IORD_16DIRECT(sdcard_aux_stat_reg, 0)) & 0x04)
        if ((status & 0x02) == 0) {
            printf("sd card was removed\n");
            exit(1);
        }

    // swap bytes
    for (i = 0; i < 128; i++) {
        gb->buf[i] = ((int32_t *)sd_card_dev->base)[i];
        gb->buf[i] = (gb->buf[i]<<24) | (gb->buf[i]<<8&0x00FF0000) | (gb->buf[i]>>8&0x0000FF00) | (gb->buf[i]>>24);
    }

    *sdcard_com_arg_reg = ++gb->blk_cnt * 512;
    *sdcard_com_reg = READ_BLOCK;

    gb->pos -= 4096;
}

static void audio_write_isr(void *ctx, alt_u32 id)
{
    register int32_t left, right;
    register unsigned int fifo_space;

    while (decoded_pos < decoded_len) {
        left = decoded_l[decoded_idx][decoded_pos];
        right = decoded_r[decoded_idx][decoded_pos];

        fifo_space = IORD_ALT_UP_AUDIO_FIFOSPACE(audio_dev->base);
        fifo_space &= ALT_UP_AUDIO_FIFOSPACE_WSLC_MSK | ALT_UP_AUDIO_FIFOSPACE_WSRC_MSK;

        if (fifo_space) {
            IOWR_ALT_UP_AUDIO_LEFTDATA(audio_dev->base, left);
            IOWR_ALT_UP_AUDIO_RIGHTDATA(audio_dev->base, right);
            decoded_pos++;
        } else
            return;
    }

    alt_up_audio_disable_write_interrupt(audio_dev);
}

int main()
{
    sd_card_dev = alt_up_sd_card_open_dev(SDCARD_NAME);

    sdcard_com_arg_reg = ((int *)(sd_card_dev->base + SD_CARD_ARGUMENT));
    sdcard_com_reg = ((short int *)(sd_card_dev->base + SD_CARD_COMMAND));
    sdcard_aux_stat_reg = ((short int *)(sd_card_dev->base + SD_CARD_STATUS));

    audio_dev = alt_up_audio_open_dev(AUDIO_NAME);

    if (audio_dev == NULL) {
        printf("audio dev could not be opened\n");
        return 1;
    }

    printf("waiting for sd card\n");
    while ((IORD_16DIRECT(sdcard_aux_stat_reg, 0) & 0x02) == 0);
    printf("sd card connected\n");

    alt_up_audio_disable_write_interrupt(audio_dev);
    alt_irq_register(AUDIO_IRQ, NULL, audio_write_isr);

    PERF_RESET (PERFORMANCE_COUNTER_BASE);

    *sdcard_com_arg_reg = 0;
    *sdcard_com_reg = READ_BLOCK;

    PERF_START_MEASURING (PERFORMANCE_COUNTER_BASE);
    decode_main();
    PERF_STOP_MEASURING (PERFORMANCE_COUNTER_BASE);

    perf_print_formatted_report(PERFORMANCE_COUNTER_BASE, 100000000, 1, "heidi");

    return 0;
}

static int decode_main()
{
    uint32_t buf;
    int len, chmod;

    GetBitContext gb;
    gb.read_block = read_block;
    gb.blk_cnt = 0;
    gb.buf = sdcard_buf;
    gb.pos = 4096;

    buf = get_bits(&gb, 32);

    if (strncmp((char *)&buf, "CaLf", 4)) {
        fprintf(stderr, "error: wrong file type\n");
        exit(1);
    }

    do {
        buf = get_bits1(&gb);
        skip_bits(&gb, 7);

        len = get_bits(&gb, 24);
        printf("reading metadata block of size %d%s\n", len, buf ? ", last block" : "");

        skip_bits(&gb, len * 8);
    }
    while (!buf);

    decoded_l[0] = malloc(4608 * 4);
    decoded_l[1] = malloc(4608 * 4);
    decoded_r[0] = malloc(4608 * 4);
    decoded_r[1] = malloc(4608 * 4);

    decoded_idx = 1;

    if (alt_sysclk_init(100) < 0)
        printf("no system clock available\n");

    while ((len = decode_frame(&gb, decoded_l[decoded_idx ^ 1], decoded_r[decoded_idx ^ 1], &chmod)) >= 0) {
        //int tick1 = alt_nticks();

        PERF_BEGIN (PERFORMANCE_COUNTER_BASE, 1);
        while (IORD_ALT_UP_AUDIO_CONTROL(audio_dev->base) & ALT_UP_AUDIO_CONTROL_WE_MSK);
        PERF_END (PERFORMANCE_COUNTER_BASE, 1);

        //int tick2 = alt_nticks();

        decoded_idx ^= 1;
        decoded_pos = 0;
        decoded_len = len;
        decoded_chmod = chmod;

        alt_up_audio_enable_write_interrupt(audio_dev);

        //printf("system waited from %d to %d\n", tick1, tick2);
    }

    return 0;
}

static int decode_frame(GetBitContext *gb, int32_t *out_l, int32_t *out_r, int *ch_mode)
{
    int ret, l_cnt, r_cnt;
    FLACFrameInfo fi;

    if ((ret = ff_flac_decode_frame_header(gb, &fi)) < 0) {
        fprintf(stderr, "invalid frame header\n");
        return ret;
    }

    if (fi.blocksize > 4608) {
        fprintf(stderr, "invalid block size (must be <= 4608, current is: %d)\n", fi.blocksize);
        return -1;
    }

    if (fi.channels == 0 || fi.channels > 2) {
        fprintf(stderr, "error: invalid channel count (must be 1 or 2, current is %d)\n", fi.channels);
        return -1;
    }

    if ((ret = decode_subframe(gb, &fi, out_l, 0)) < 0)
        return ret;

    if (fi.channels == 2)
        if ((ret = decode_subframe(gb, &fi, out_r, 1)) < 0)
            return ret;

    align_get_bits(gb);

    /* frame footer */
    skip_bits(gb, 16); /* data crc */

    switch (fi.ch_mode) {
        case FLAC_CHMODE_INDEPENDENT: flac_decorrelate_indep_c(out_l, out_r, fi.channels, fi.blocksize); break;
        case FLAC_CHMODE_LEFT_SIDE:   flac_decorrelate_ls_c(out_l, out_r, fi.channels, fi.blocksize); break;
        case FLAC_CHMODE_RIGHT_SIDE:  flac_decorrelate_rs_c(out_l, out_r, fi.channels, fi.blocksize); break;
        case FLAC_CHMODE_MID_SIDE:    flac_decorrelate_ms_c(out_l, out_r, fi.channels, fi.blocksize); break;
    }
    *ch_mode = fi.ch_mode;

    /*
    l_cnt = r_cnt = 0;

    while (l_cnt < fi.blocksize || r_cnt < fi.blocksize) {
        if (l_cnt < fi.blocksize)
            l_cnt += alt_up_audio_write_fifo(audio_dev, (unsigned int *)(&decoded_l[l_cnt]), fi.blocksize - l_cnt, ALT_UP_AUDIO_LEFT);

        if (r_cnt < fi.blocksize)
            r_cnt += alt_up_audio_write_fifo(audio_dev, (unsigned int *)(&decoded_r[r_cnt]), fi.blocksize - r_cnt, ALT_UP_AUDIO_RIGHT);
    }
    */

    /*
    int cnt = 0;
    int32_t left, right;
    unsigned int fifo_base, space_l, space_r;

    while (cnt < fi.blocksize) {
        switch (fi.ch_mode) {
            case FLAC_CHMODE_INDEPENDENT:
                left = decoded_l[cnt];
                right = decoded_r[cnt];
                break;

            case FLAC_CHMODE_LEFT_SIDE:
                left = decoded_l[cnt];
                right = left - decoded_r[cnt];
                break;

            case FLAC_CHMODE_RIGHT_SIDE:
                right = decoded_r[cnt];
                left = right - decoded_l[cnt];
                break;

            case FLAC_CHMODE_MID_SIDE:
                right = decoded_l[cnt];
                left = decoded_r[cnt];
                right -= left >> 1;
                left += right;
                break;
        }

        fifo_base = IORD_ALT_UP_AUDIO_FIFOSPACE(audio_dev->base);

        space_l = (fifo_base & ALT_UP_AUDIO_FIFOSPACE_WSLC_MSK); // >> ALT_UP_AUDIO_FIFOSPACE_WSLC_OFST;
        space_r = (fifo_base & ALT_UP_AUDIO_FIFOSPACE_WSRC_MSK); // >> ALT_UP_AUDIO_FIFOSPACE_WSRC_OFST;

        if (space_l > 0 && space_r > 0) {
                IOWR_ALT_UP_AUDIO_LEFTDATA(audio_dev->base, left);
                IOWR_ALT_UP_AUDIO_RIGHTDATA(audio_dev->base, right);
                cnt++;
        }
    }
    */

    return fi.blocksize;
}
