// Ref.: https://github.com/DhrBaksteen/ArduinoOPL2/blob/master/src/OPL2.cpp

#include <io.h>
#include <stdint.h>
#include <stdbool.h>

#define OPL_ADDR 0x40000000
#define OPL_DATA 0x40000004

#define CLOCK_VALUE 0x20000004

#define ADLIB_REG_WSE 0x01
#define ADLIB_REG_CSM_SEL 0x08
#define ADLIB_REG_AM_VIB_EG_KSR_MULTI 0x20
#define ADLIB_REG_KSL_TL 0x40
#define ADLIB_REG_AR_DR 0x60
#define ADLIB_REG_SL_RR 0x80
#define ADLIB_REG_FNL 0xA0
#define ADLIB_REG_KON_BLOCK_FNH 0xB0
#define ADLIB_REG_DRUM 0xBD
#define ADLIB_REG_FB_C 0xC0
#define ADLIB_REG_WS 0xE0

#define ADLIB_BASSDRUM 0x10
#define ADLIB_SNARE 0x08
#define ADLIB_TOMTOM 0x04
#define ADLIB_CYMBAL 0x02
#define ADLIB_HIHAT 0x01

typedef uint8_t byte;

static char adlib_reg_drum = 0x00;
static int adlib_osc_off[] =
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x08, 0x09, 0x0a,
     0x0b, 0x0c, 0x0d, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15};
static int adlib_voice_osc[6][2] =
    {{0x00, 0x03}, {0x01, 0x04}, {0x02, 0x05}, {0x08, 0x0b}, {0x09, 0x0c}, {0x0a, 0x0d}};

/* frequency numbers are the same for all octaves, EXCEPT F8, which is 5Hz off. */
const unsigned int adlib_notes[] =
    {172, 182, 193, 205, 217, 230, 243, 258, 273, 290, 307, 325};

typedef struct
{
    char AR, DR, SL, RR; /* ADSR curve */
    char AM, VIB;        /* Amplitude modulation, vibrato */
    char SUS;            /* Sustain until key off */
    char KSL, TL;        /* key sustain level, total level */
    char WS;             /* wave select */
} adlib_oscillator;

typedef struct
{
    adlib_oscillator CAR, MOD; /* carrier and modulator */
    char C, FB;                /* connection, feedback */
} adlib_instrument;

void delay(uint32_t ms)
{
    uint32_t now = MEM_READ(CLOCK_VALUE);
    while(abs(MEM_READ(CLOCK_VALUE) - now) < ms);
}

void adlib_reg(byte reg, byte val)
{
    MEM_WRITE(OPL_ADDR, reg);
    for (int i = 0; i < 6; ++i)
        MEM_READ(OPL_ADDR);
    MEM_WRITE(OPL_DATA, val);
    for (int i = 0; i < 35; ++i)
        MEM_READ(OPL_DATA);
}

void adlib_reset()
{
    int i;
    const char block = 3;        /* frequency block 3 */
    const unsigned int A5 = 290; /* eq 440Hz in Block 3 */

    /* set Wave Select Enable (WSE) bit */
    adlib_reg(ADLIB_REG_WSE, 0x20);
    /* disable speech mode, disable split */
    adlib_reg(ADLIB_REG_CSM_SEL, 0x00);
    /* rhythm mode */
    adlib_reg(ADLIB_REG_DRUM, 0x20);
    /* configure ADSR for all oscillators to a medium curve */
    for (i = 0; i < 18; ++i)
    {
        adlib_reg(ADLIB_REG_AR_DR + adlib_osc_off[i], 0x99);
        adlib_reg(ADLIB_REG_SL_RR + adlib_osc_off[i], 0x66);
    }
    /* set all channels to A5 */
    for (i = 0; i < 9; ++i)
    {
        adlib_reg(ADLIB_REG_FNL + i, A5 & 0xFF);
        adlib_reg(ADLIB_REG_KON_BLOCK_FNH + i, (block << 2) | (A5 >> 8));
    }
}

void adlib_rhythm_mode()
{
    int A4 = 70, A7 = 870, block = 3;
    /* enable R bit */
    adlib_reg_drum |= 0x20;
    adlib_reg(ADLIB_REG_DRUM, adlib_reg_drum);
    /* set tom tom to a higher note for better sound */
    adlib_reg(ADLIB_REG_FNL + 8, A7 & 0xFF);
    adlib_reg(ADLIB_REG_KON_BLOCK_FNH + 8, (block << 2) | (A7 >> 8));
    /* set bassdrum to a lower note for better sound */
    adlib_reg(ADLIB_REG_FNL + 6, A4 & 0xFF);
    adlib_reg(ADLIB_REG_KON_BLOCK_FNH + 6, (block << 2) | (A4 >> 8));
    /* set ADSR for CY and HH to something slower */
    adlib_reg(ADLIB_REG_AR_DR + adlib_osc_off[13], 0x88);
    adlib_reg(ADLIB_REG_SL_RR + adlib_osc_off[13], 0x55);
    adlib_reg(ADLIB_REG_AR_DR + adlib_osc_off[17], 0x88);
    adlib_reg(ADLIB_REG_SL_RR + adlib_osc_off[17], 0x55);
}

void adlib_set_instrument(adlib_instrument *instr, char voice)
{
    /* set ADSR for both oscillators */
    adlib_reg(ADLIB_REG_AR_DR + adlib_voice_osc[voice][0], ((instr->MOD.AR & 0xF) << 4) | (instr->MOD.DR & 0xF));
    adlib_reg(ADLIB_REG_SL_RR + adlib_voice_osc[voice][0], ((instr->MOD.SL & 0xF) << 4) | (instr->MOD.RR & 0xF));
    adlib_reg(ADLIB_REG_AR_DR + adlib_voice_osc[voice][1], ((instr->CAR.AR & 0xF) << 4) | (instr->CAR.DR & 0xF));
    adlib_reg(ADLIB_REG_SL_RR + adlib_voice_osc[voice][1], ((instr->CAR.SL & 0xF) << 4) | (instr->CAR.RR & 0xF));
    /* Sustain, AM and vibrato */
    adlib_reg(ADLIB_REG_AM_VIB_EG_KSR_MULTI + adlib_voice_osc[voice][0], (instr->MOD.AM << 7) | ((instr->MOD.VIB) << 6) | ((instr->MOD.SUS) << 5));
    adlib_reg(ADLIB_REG_AM_VIB_EG_KSR_MULTI + adlib_voice_osc[voice][1], (instr->CAR.AM << 7) | ((instr->CAR.VIB) << 6) | ((instr->CAR.SUS) << 5));
    /* KSL and TL */
    adlib_reg(ADLIB_REG_KSL_TL + adlib_voice_osc[voice][0], (instr->MOD.KSL << 6) | (instr->MOD.TL & 0x3F));
    adlib_reg(ADLIB_REG_KSL_TL + adlib_voice_osc[voice][1], (instr->CAR.KSL << 6) | (instr->CAR.TL & 0x3F));
    /* waveform select */
    adlib_reg(ADLIB_REG_WS + adlib_voice_osc[voice][0], instr->MOD.WS & 0x03);
    adlib_reg(ADLIB_REG_WS + adlib_voice_osc[voice][1], instr->CAR.WS & 0x03);
    /* connection and feedback */
    adlib_reg(ADLIB_REG_FB_C + voice, (instr->FB << 1) | (instr->C & 0x01));
}

void adlib_play_drums(char drums)
{
    adlib_reg(ADLIB_REG_DRUM, adlib_reg_drum);
    adlib_reg(ADLIB_REG_DRUM, adlib_reg_drum | drums);
}

void adlib_play_note(unsigned int note, char voice)
{
    static unsigned int voices[6] = {0, 0, 0, 0, 0, 0};
    /* key off */
    adlib_reg(ADLIB_REG_KON_BLOCK_FNH + voice, (voices[voice] >> 8) & 0x1F);
    /* key on */
    adlib_reg(ADLIB_REG_FNL + voice, note & 0xFF);
    adlib_reg(ADLIB_REG_KON_BLOCK_FNH + voice, 0x20 | (note >> 8));
    voices[voice] = note;
}

int main()
{
    adlib_instrument instr;
    adlib_reset();

    adlib_rhythm_mode();

    instr.CAR.AM = 1;
    instr.CAR.AR = 0x6;
    instr.CAR.DR = 0x4;
    instr.CAR.SL = 0xb;
    instr.CAR.RR = 0x6;
    instr.CAR.KSL = 0;
    instr.CAR.SUS = 0;
    instr.CAR.TL = 0x00;
    instr.CAR.VIB = 1;
    instr.CAR.WS = 3;
    instr.MOD.AM = 1;
    instr.MOD.AR = 0x6;
    instr.MOD.DR = 0x6;
    instr.MOD.SL = 0x6;
    instr.MOD.RR = 0x1;
    instr.MOD.KSL = 0;
    instr.MOD.SUS = 0;
    instr.MOD.TL = 0x3f;
    instr.MOD.VIB = 1;
    instr.MOD.WS = 3;
    instr.C = 0;
    instr.FB = 0;
    for (int i = 0; i < 6; ++i)
    {
        adlib_set_instrument(&instr, i);
    }

    char block = 4;
    char cur_voice = 0;
    print("Play notes...\r\n");
    for (int i = 0; i < 12; i++) {
        adlib_play_drums(ADLIB_SNARE);
        adlib_play_note(((block & 0x7)<<10) | (adlib_notes[i] & 0x3FF), cur_voice);
        delay(1000);
    }
    adlib_reset();
    print("Done.\r\n");
    return 0;
}