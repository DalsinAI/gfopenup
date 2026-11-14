/* DETECT's SlimROM answer, in plain C (no OS calls), shared with its host
 * test (tests/test_openup_slimrom.c).
 *
 * The slim (thin) ROM has kickload and no workbench.library of its own, so
 * it needs the Kickstart set on the disk. kickload used to be a RomTag in
 * the ROM window; since the packed boot image it is unpacked and runs from
 * Fast RAM, as is every packed module, so "where is the RomTag" no longer
 * tells the ROM's modules from the set's. The ROM's own header does: our
 * ROMs (tools/openkickrom, amicradle-guest) carry 'OKR1' at offset 20 and
 * at offset $18 the id "GFOpenKick V.R <rom name>-<profile>" (profile.py
 * rom_id), and the slim ROM's profile is "thin" (profiles/thin.json).
 *
 *   kickload    a "kickload" resident is there (FindResident)
 *   rom         the first 64 bytes of the ROM at $F80000
 *   kick_in_rom, wb_in_rom, wb_found
 *               where kickload's and workbench.library's RomTags are: for a
 *               ROM without our header (an older one of ours), the earlier
 *               rule: kickload in the ROM window and workbench.library not
 *
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OPENUP_SLIMROM_H
#define OPENUP_SLIMROM_H

#define OUS_MARK    0x4F4B5231UL        /* 'OKR1' */
#define OUS_MARK_AT 20
#define OUS_ID_AT   0x18
#define OUS_ID_END  0x40                /* the scan list starts here */

static unsigned long ous_be32(const unsigned char *b)
{
    return (unsigned long)b[0] << 24 | (unsigned long)b[1] << 16 | (unsigned long)b[2] << 8 | b[3];
}

/* 1 when the ROM's id (NUL ends it, at most up to $40) ends in "-thin" */
static int ous_thin_id(const unsigned char *rom)
{
    static const char tail[] = "-thin";
    unsigned n = 0, t = sizeof tail - 1, i;
    while (OUS_ID_AT + n < OUS_ID_END && rom[OUS_ID_AT + n]) n++;
    if (OUS_ID_AT + n == OUS_ID_END || n < t) return 0;     /* no NUL: no id */
    for (i = 0; i < t; i++)
        if (rom[OUS_ID_AT + n - t + i] != (unsigned char)tail[i]) return 0;
    return 1;
}

static int ous_slim_rom(int kickload, const unsigned char *rom, int kick_in_rom, int wb_found, int wb_in_rom)
{
    if (!kickload) return 0;                    /* OS 3.2.3, or any ROM not ours */
    if (ous_be32(rom + OUS_MARK_AT) == OUS_MARK)
        return ous_thin_id(rom);
    return kick_in_rom && (!wb_found || !wb_in_rom);
}

#endif
