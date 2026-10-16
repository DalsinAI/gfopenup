# OpenUp 0.8.2 (16 October 2026)

Workbench's look straight from the ROM. Dale, 16 October 2026: "make sure there aren't any legacy prefs or hacks
in place from before to make openbench look the way it needs to, it should be default, straight from the rom so
to speak, only the background will be the one that gets the attention".

- **OpenLook is no longer a part.** The Installer writes no C:OpenLook and no startup line for it, no
  ENVARC:OpenGadTools/Look or Theme, no Font prefs (CGTriumvirate 13) and no IControl prefs (title bars 2 pixels
  taller). The OpenKick ROM gives Workbench a 24-pixel bar in CGTriumvirate 21 on a 1080-line screen and draws the
  icons on the backdrop; on Kickstart 3.2.3 the look is 3.2.3's own.
- **An upgrade takes out what an earlier OpenUp wrote** (OpenUpTool OLDLOOK, run before the parts): C:OpenLook and
  its line, and each of those prefs files while its bytes are what OpenUp wrote, worked out again from the original
  it kept in SYS:Storage/OpenUp-Superseded (which comes back) or from none. A file you saved since (Font prefs,
  say) stays, and T:OpenUp.log says so.
- **Backgrounds** stays: the theme question now picks only the backdrop. Workbench as a backdrop and the slim
  drive windows (WBBACKDROP, WBGAUGE) come with it, as they came with OpenLook before.
- **OpenRTG writes no screen mode.** On the OpenKick ROM Workbench opens on OpenRTG's 16-bit screen by itself. A new
  install on Kickstart 3.2.3 asks, and only a Yes saves the 16-bit Native mode (Parts/OpenRTG/Mode). An upgrade
  never touches the mode in place. Parts/OpenRTG/Upgrade is gone.
