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

static int ous_slim_rom(int kickload, const unsigned char *rom, int kick_in_rom, int wb_found, int wb_in_rom)
{
    (void)rom;
    return kickload && kick_in_rom && (!wb_found || !wb_in_rom);   /* main's rule, moved here unchanged */
}

#endif
