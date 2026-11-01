# GFOpenUp 0.8.2 (16 October 2026)

Workbench's look straight from the ROM. Dale, 16 October 2026: "make sure there aren't any legacy prefs or hacks
in place from before to make openbench look the way it needs to, it should be default, straight from the rom so
to speak, only the background will be the one that gets the attention".

- **GFOpenLook is no longer a part.** The Installer writes no C:GFOpenLook and no startup line for it, no
  ENVARC:GFOpenGadTools/Look or Theme, no Font prefs (CGTriumvirate 13) and no IControl prefs (title bars 2 pixels
  taller). The GFOpenKick ROM gives Workbench a 24-pixel bar in CGTriumvirate 21 on a 1080-line screen and draws the
  icons on the backdrop; on Kickstart 3.2.3 the look is 3.2.3's own.
- **An upgrade takes out what an earlier GFOpenUp wrote** (GFOpenUpTool OLDLOOK, run before the parts): C:GFOpenLook and
  its line, and each of those prefs files while its bytes are what GFOpenUp wrote, worked out again from the original
  it kept in SYS:Storage/GFOpenUp-Superseded (which comes back) or from none. A file you saved since (Font prefs,
  say) stays, and T:GFOpenUp.log says so.
- **Backgrounds** stays: the theme question now picks only the backdrop. Workbench as a backdrop and the slim
  drive windows (WBBACKDROP, WBGAUGE) come with it, as they came with GFOpenLook before.
- **GFOpenRTG writes no screen mode.** On the GFOpenKick ROM Workbench opens on GFOpenRTG's 16-bit screen by itself. A new
  install on Kickstart 3.2.3 asks, and only a Yes saves the 16-bit Native mode (Parts/GFOpenRTG/Mode). An upgrade
  never touches the mode in place. Parts/GFOpenRTG/Upgrade is gone.
