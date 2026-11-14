#!/usr/bin/env python3
"""The icon library in the Gorilla style: every icon in icons/LIST.csv that
is an icon (not a title bar glyph or a pointer), drawn from the things in
tools/gvocab.py, both frames, and written as an icon collection for
AmiCradle's icon library (PNG icons, the OS 4 format, which the new icon
set turns into GlowIcons for OS 3.x). MIT, Copyright (c) 2026 AmiCradle Team.

    python3 tools/icon_library_art.py OUT [--sheet] [--only TEXT]
        OUT/png/<folder>/<Name>.info   PNG icons, 64 x 64, both frames
        OUT/collection.json            the collection's name, source and licence
        OUT/sheets/<folder>.png        contact sheets, light and dark (--sheet)

A recipe says what an icon is made of: a thing, then things on it, each
"name(option=value, ...)" with "@face" (on it) or "@badge" (at its foot on
the right, the default). Prefs editors get the
prefs mark at their foot on the left. "file(NAME)" is one of our own icons
in icons/open, as it is. The recipes follow LIST.csv's sections and names.
"""
from __future__ import annotations

import argparse
import csv
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
sys.path.insert(0, str(HERE))
import gstyle as g          # noqa: E402
import gvocab as v          # noqa: E402
from PIL import Image, ImageDraw   # noqa: E402

LIST = ROOT / "icons" / "LIST.csv"
OPEN_ART = ROOT / "icons" / "open"

RECIPES = r"""
[Disks and devices]
System = harddisk + power
Work = harddisk
Hard disk (any) = harddisk
Ram Disk = ramstick
Recoverable RAM disk = ramstick + arrows(kind=circle)
Floppy (DD) = floppy
Floppy (HD) = floppy(hd=1)
Kickstart disk = floppy + chip
Unreadable disk = floppy + question
Unreadable hard disk = harddisk + question
Unformatted disk = floppy(tint=grey)
Busy disk = floppy + light
Busy hard disk = harddisk + light
CD-ROM = disc
Busy CD-ROM = disc + light
Unreadable CD = disc + question
Audio CD = disc + note
DVD = disc(tint=purple) + text(s=DVD,tint=purple)
CD image mounted = dashed + disc@face
ADF mounted = dashed + floppy@face
HDF mounted = dashed + harddisk@face
CompactFlash = flashcard(kind=cf)
SD card = flashcard(kind=sd)
PCMCIA card = flashcard(kind=pcmcia)
USB stick = usbstick
USB hard disk = harddisk + usbplug
Zip disk = cartridge(tint=blue)
Jaz disk = cartridge(tint=teal)
SyQuest = cartridge(tint=dark)
Magneto-optical disk = cartridge(tint=purple)
Tape = tape
PC (FAT) floppy = floppy(tint=pcgrey)
PC (FAT) hard disk = harddisk(tint=pcgrey)
FFS partition = harddisk + tag(label=FFS)
PFS partition = harddisk + tag(label=PFS,tint=blue)
SFS partition = harddisk + tag(label=SFS,tint=green)
Network share = harddisk + netplug
NFS share = harddisk + tag(label=NFS,tint=teal)
FTP mount = harddisk + globe
This PC (AmiCradle) = pctower + mark
Emulator folder = drawer + chip
Parallel link = network
Other computer = pctower
Amiga on the network = computer + netplug
Trashcan (empty) = bin
Trashcan (full) = bin(full=1)
Printer = printer
Unknown device = box + question
Pipe = pipe
Audio device = harddisk + speaker
Serial port = dsub(pins=9)
Parallel port = dsub(pins=25,tint=blue)
Printer port = dsub(pins=25) + printer
PC floppy driver = floppy(tint=pcgrey) + tag(label=PC)

[Workbench drawers (SYS:)]
C = drawer + prompt
Classes = drawer + puzzle
Classes/Gadgets = drawer + switch
Classes/Images = drawer + landscape
Classes/DataTypes = drawer + tag
Devs = drawer + cogs
Devs/DataTypes = drawer + tag
Devs/DOSDrivers = drawer + harddisk
Devs/Keymaps = drawer + key(letter=A)
Devs/Monitors = drawer + monitor
Devs/Printers = drawer + printer
Devs/AHI = drawer + speaker
Devs/Midi = drawer + din
Expansion = drawer + board
Fonts = drawer + text(s=Aa)
L = drawer + cog
Libs = drawer + book
Locale = drawer + globe
Locale/Catalogs = drawer + bubble(two=1)
Locale/Countries = drawer + flag
Locale/Languages = drawer + text(s=Aé,tint=blue)
Locale/Help = drawer + lifebuoy
Prefs = drawer + prefs
Prefs/Presets = drawer + bookmark
Prefs/Presets/Themes = drawer + swatches
Prefs/Presets/Backdrops = drawer + landscape
Prefs/Presets/Patterns = drawer + tiles
Prefs/Env-Archive = drawer + cog(tint=blue)
Rexxc = drawer + crown
S = drawer + scroll
Storage = drawer + box
Storage/DOSDrivers = drawer + harddisk
Storage/Monitors = drawer + monitor
Storage/Printers = drawer + printer
Storage/Keymaps = drawer + key(letter=A)
Storage/DataTypes = drawer + tag
System = drawer + wrench
T = drawer + clock
Tools = drawer + hammer
Tools/Commodities = drawer + bolt
Utilities = drawer + cabinet
WBStartup = drawer + rocket
Devs/Drivers = drawer + chip
Trashcan drawer = bin

[Your drawers (Work:)]
Drawer (plain) = drawer
Documents = drawer + page(lines_on=1)
Pictures = drawer + photo
Music = drawer + note
Video = drawer + film
Downloads = drawer + arrow(dir=down)
Games = drawer + joystick
Demos = drawer + star
Programs = drawer + window
Development = drawer + brackets
Internet = drawer + globe
Graphics = drawer + brush
Sound = drawer + speaker
Office = drawer + pen
Emulation = drawer + computer
Archives = drawer + box
Backup = drawer + safe
Home = drawer + house
Shared = drawer + netplug
Send Out = drawer + pctower
Disk images = drawer + floppy
Kickstarts = drawer + chip(label=ROM)
Fonts (user) = drawer + text(s=Aa,tint=blue)
Projects = drawer + ruler
Utilities (user) = drawer + wrench

[Prefs editors: OS 3.2.3]
Asl = requester + drawer
Font = page + text(s=Aa)@face
IControl = gadgets
Input = keyboard + mouse
Locale = globe + flag
Overscan = monitor + dashed@face
Palette = colourwheel
Pointer = pointer + pencil
Printer = printer
PrinterGfx = printer + landscape
PrinterPS = printer + text(s=PS)
ScreenMode = monitor + tiles@face
Serial = dsub(pins=9)
Sound = speaker
Time = clock
WBPattern = tiles + landscape
Workbench = window + tick@face
DefIcons = iconpic + tag
Presets = bookmark
Trackdisk = floppydrive

[Prefs editors: GFOpenUp and AmiCradle]
Look = file(Look)
Windows = file(Windows)
Dock = file(Dock)
Menus = file(Menus)
Title bar = file(TitleBar)
Sound (GFOpenUp) = file(Sound)
GFOpenTypes = file(GFOpenTypes)
GFOpenUp Setup = file(GFOpenUp-Setup)
GFOpenPrint = printer + page
GFOpenSocket = netplug
Classic = window(tint=orange)
GFOpenBlanker = monitor(dark=1) + star@face
GFOpenTata = file(GFOpenTata)
GFOpenRTG = monitor + card
All Prefs = cog + drawer
Wi-Fi = wifi
AmiCradle Sharing = clipboard + arrows

[Prefs editors: other people's]
MUI = gadgets + cog
Zune = gadgets + cog(tint=gold)
AHI = speaker + card(kind=sound)
Picasso96Mode = monitor + card
CyberGraphX = monitor + card(kind=graphics)
ReqTools = requester
Poseidon / Trident = usbplug + cog
Roadshow = netplug + cog
Miami = netplug + dot(tint=gold)
Genesis = netplug
OpenURL = chain
NewIcons = iconpic + star
VisualPrefs = window + pencil
MCP = cog + wrench
MagicMenu = menu + wand
Birdie = window + bird@face
PopupMenu = menu
TurboPrint = printer + bolt
Boot = bootscreen
Keymap editor = keyboard + pencil

[Utilities, Tools and System: OS 3.2.3]
Shell = terminal
Clock = clock
MultiView = landscape + magnifier
Installer = box + arrow(dir=down)
Calculator = calculator
CMD = printer + floppy
GraphicDump = printer + monitor
HDToolBox = harddisk + wrench
IconEdit = iconpic + pencil
InitPrinter = printer + power
KeyShow = keyboard
Lacer = monitor + lines@face
PrepCard = flashcard(kind=pcmcia) + broom
PrintFiles = printer + page
ShowConfig = board + magnifier
TextEdit = page(lines_on=1) + pencil
UnArc = box + arrow(dir=up)
Format = floppy + broom
DiskCopy = floppy + floppy(tint=blue)
FixFonts = text(s=Aa) + wrench
Intellifont = text(s=Aa,outline_only=1)
NoFastMem = ramstick + cross
RexxMast = crown
Find = magnifier
More = page(lines_on=1)
Help = lifebuoy
About = monitor + info@face
Screen grabber = camera + monitor
Memory monitor = monitor + graph@face

[Commodities]
Exchange = cogs + switch
AutoPoint = pointer + target
Blanker = monitor(dark=1)
ClickToFront = window + pointer@face
CrossDOS = floppy(tint=pcgrey) + arrows
FKey = key(letter=F1)
MouseBlanker = pointer(faded=1)
NoCapsLock = key(letter=Caps) + cross
DepthMenu = windows + menu
ASCIITable = letters
GFOpenWindows = windows + magnet
GFOpenSocketControl = netplug + switch
ACNetControl = netplug + pctower
ToolsDaemon = menu + hammer
Yak = yak

[Open apps and AmiCradle]
GFOpenUp = cabinet + openup
Install-GFOpenUp = file(Install-GFOpenUp)
Uninstall-GFOpenUp = file(Uninstall-GFOpenUp)
GFOpenFiles = file(GFOpenFiles)
GFOpenView = file(GFOpenView)
GFOpenEdit = file(GFOpenEdit)
GFOpenWrite = file(GFOpenWrite)
GFOpenPlay = file(GFOpenPlay)
OpenBrowser = file(OpenBrowser)
GFOpenMail = envelope
Telegram = file(Telegram)
GFOpenCompress = file(GFOpenCompress)
GFOpenPrint queue = file(Printers and Queue)
GFOpenSocket = netplug + pctower
GFOpenService = board + bell
GFOpenSpeaker = speaker
MUIExample = file(MUIExample)
GFOpenMUI = gadgets
Theme Builder = swatches + ruler
ACKitchen = code + knife
GFOpenRTG = card(kind=graphics)
GFOpenGames = joystick + play
ACDrop = pctower + arrow(dir=down)
ACClip = clipboard + arrows
Send Out = pctower + arrow(dir=right)
GFOpenUp update = box + star
Dock buttons: Start = mark

[File types: documents and text]
Text = page(lines_on=1)
Document = page
ReadMe = page + info@face
AmigaGuide = page + lifebuoy@face
PDF = page + bookmark(tint=red)@face
PostScript = page + text(s=PS)@face
Word document = page(tint=blue,lines_on=1)
OpenDocument text = page(tint=teal,lines_on=1)
Rich text = page(tint=purple,lines_on=1)
Spreadsheet = grid
Presentation = easel
CSV = grid(commas=1)
Markdown = page + text(s=#)@face
Licence = page + seal
Authors = page + person(two=1)
Final Writer / Wordworth document = page(lines_on=1) + quill
PageStream document = newspaper
e-book = book
Log = page(lines_on=1) + clock

[File types: pictures]
Picture = landscape
ILBM = landscape + palette
JPEG = photo
PNG = landscape(checks=1)
GIF = landscape(dots=1)
GIF animation = landscape + film
BMP = landscape + tag(label=BMP)
TIFF = photostack
WebP = landscape + tag(label=WebP,tint=green)
SVG = curve
ICO = iconpic
PCX / TGA = landscape + tag(label=TGA,tint=blue)
XCF = photostack + brush
HAM picture = landscape(rainbow=1)
Icon (.info) = iconpic + page
Brush = brush
Font preview = text(s=Aa)

[File types: sound and music]
Sound = speaker
Music = note
8SVX = waveform
AIFF = waveform + tag(label=AIFF)
WAV = waveform + tag(label=WAV,tint=blue)
MP3 = note + tag(label=MP3)
MP3 with tags = note + tag
Ogg Vorbis = note(two=0)
FLAC = note(two=0) + tag(label=FLAC,tint=teal)
AAC / M4A = note(two=0) + tag(label=AAC,tint=purple)
Module = patterngrid + note
MED module = patterngrid(notes=1)
XM / S3M / IT = patterngrid + tag(label=XM,tint=blue)
AHX / HivelyTracker = chip + note
SID tune = chip(label=SID) + note
MIDI = keyboard + note
Instrument = waveform + note
Playlist = page(lines_on=1) + note
Speech = mouth

[File types: video and animation]
Movie = film(frame_only=1)
ANIM = flipbook
CDXL = disc + film
MPEG = film(play=1)
AVI = film + tag(label=AVI)
MP4 = film + tag(label=MP4,tint=blue)
QuickTime = film + tag(label=MOV,tint=teal)
Matroska = film + tag(label=MKV,tint=purple)
FLI/FLC = flipbook + tag(label=FLI)

[File types: archives and disk images]
Archive = box
LhA = box + text(s=LHA)@face
LZX = box + text(s=LZX)@face
Zip = box + text(s=ZIP)@face
7-Zip = box + text(s=7z)@face
Tar = box + tape
gzip / bzip2 / xz = page + arrows
DMS = floppy + arrows
ADF = floppy + tag(label=ADF)
HDF = harddisk + tag(label=HDF)
ISO = disc + tag(label=ISO)
CUE/BIN = disc + page
WHDLoad slave = joystick + floppy
Patch = page + plaster
Installer package = box + arrow(dir=down)
Kickstart ROM = chip(label=ROM)

[File types: code and scripts]
Program = window + cog@face
Project = page + cog
Script = scroll + prompt@face
ARexx script = scroll + crown@face
Installer script = scroll + box@face
C source = page + text(s=C,tint=blue)@face
C header = page + text(s=H,tint=purple)@face
C++ source = page + text(s=C++,tint=blue)@face
Assembler = page + text(s=ASM,tint=red)@face
E source = page + text(s=E,tint=green)@face
AMOS source = page + text(s=AMOS,tint=orange)@face
Blitz Basic source = page + text(s=BB,tint=red)@face
Hollywood script = page + star@face
Python = page + text(s=Py,tint=blue)@face
Lua = page + moon@face
Makefile = page + cogs@face
Object file = cube
Link library = books
Include drawer = drawer + text(s=H,tint=purple)
Diff = page + plusminus@face
JavaScript = page + text(s=JS,tint=gold)@face
PowerPC program = window + chip(label=PPC)@face
Linux program = window + penguin@face
Windows program = window + pctower

[File types: system files]
Library = book + cog
Device = chip + dsub
Datatype = tag
Class = puzzle
Handler / file system = cog + harddisk
Bitmap font = text(s=Aa,pixels=1)
Outline font = text(s=Aa,outline_only=1)
TrueType font = text(s=Aa) + tag(label=TT)
OpenType font = text(s=Aa) + tag(label=O,tint=blue)
Catalog = bubble
Country = flag
Keymap = key(letter=A)
Prefs file = page + cog@face
Preset = bookmark
Theme = swatches
Desktop profile = monitor + person@face
Monitor driver = monitor
Printer driver = printer + cog
Mount list = harddisk + page
Startup-Sequence = scroll + power@face
Commodity = cog + bolt
Docky = dock
Pointer file = pointer
Sound prefs sample = speaker + waveform

[File types: internet and data]
Web page = page + globe@face
Stylesheet = page + brush@face
XML = page + brackets(kind=</>)@face
JSON = page + text(s={ },tint=blue)@face
Link = chain
Email message = envelope
Mailbox = envelope + envelope
Contact = cardindex + person
Calendar event = calendar
Certificate = seal
Torrent = seed
Download in progress = page + progress

[States and situations]
Selected frame = -
Disk busy = light
Write-protected disk = floppy + lock
Disk nearly full = harddisk + gauge
Trashcan full = bin(full=1)
AppIcon = -
Iconified window = window + arrow(dir=down)
Leave Out = -
Link = arrow(dir=upright)
Read-only file = lock
No icon / unknown = page + question@face
Network connected = network
Network off = network(broken=1)
Wi-Fi signal = wifi
Mail: new mail = envelope(dot=1)
Mail: none = envelope
Printing = printer
Print error = printer + exclaim
Clipboard synced = clipboard + tick
Update available = box + star
Running (dock dot) = -
Sound muted = speaker(muted=1)
Battery = battery
Online / offline (dial-up) = modem

[Apps: desktop and file managers]
Directory Opus 4 = panes + floppy
Directory Opus 5 / Magellan = panes + magnifier
DiskMaster = panes + harddisk
FileMaster = panes + page
SID = panes + star
Scalos = window + star@face
ToolManager = dock + hammer
AmiDock = dock
AmiStart = startbutton
MagicMenu = menu + wand
NewIcons = iconpic + star
ClassAction = page + arrows
PowerWindows = window + bolt
iGame = page(lines_on=1) + joystick
TinyLauncher = joystick + rocket

[Apps: internet]
IBrowse = globe + flag
AWeb = globe + chain
Voyager = globe + rocket
NetSurf = globe + cloud
OWB = globe(ring=1)
YAM = envelope + seed
SimpleMail = envelope
Thor = envelope + hammer
AmIRC = bubble(two=1)
WookieChat = bubble(two=1) + star
AmiFTP = drawer + arrows
AmiTradeCenter = drawer + arrows(tint=blue)
Roadshow = netplug
Miami = netplug + dot(tint=gold)
AmiTCP / Genesis = netplug + cog
Term / NComm = terminal + modem
SSH client = terminal + keylock
Telnet = terminal + globe
News reader = newspaper
Download manager = drawer + arrow(dir=down)
Jabber / chat = bubble

[Apps: office and text]
GoldED / Cubic IDE = page(lines_on=1) + pen(tint=gold)
CygnusEd = page(lines_on=1) + bird
Annotate = page(lines_on=1) + pen(tint=blue)
FrexxEd = page(lines_on=1) + pencil
MicroEmacs = page(lines_on=1) + prompt
Wordworth = page(lines_on=1) + quill
Final Writer = page(tint=blue,lines_on=1) + quill
PageStream = newspaper + pen
ProPage = newspaper + ruler
TurboCalc = grid
Final Calc = grid(tint=blue)
Datastore = cardindex
Organiser = book(tint=red) + calendar
Calendar = calendar
Notepad = notepad
Contacts = cardindex + person
PDF viewer = page + bookmark(tint=red)@face + magnifier
Dictionary / spell checker = book + text(s=abc,tint=white)@face
Units = ruler + scales

[Apps: graphics and 3D]
Deluxe Paint = palette + brush
Personal Paint = brush
Brilliance = brush + sparkle
TVPaint = monitor + brush
ImageFX = photo + wand
ArtEffect = photo + brush
Photogenics = photostack
ADPro = photo + arrows
Lunapaint = brush + moon
Grafx2 = tiles + pencil
Lightwave = wireframe
Imagine = wireframe(kind=cube)
Cinema 4D = wireframe + camera
Real 3D = wireframe(kind=cube) + sparkle
Sculpt = wireframe + hammer
Picture viewer = photo + eye
Scanner = scanner
Screen grabber = camera
Font editor = text(s=Aa) + pencil
Icon editor = iconpic + pencil
Colour picker = dropper

[Apps: music and audio]
ProTracker = patterngrid
OctaMED = patterngrid(notes=1)
DigiBooster = patterngrid + star
HivelyTracker = patterngrid + chip
HippoPlayer = player
EaglePlayer = player + bird
DeliTracker = player + note
AmigaAMP = player(wave=1)
Bars&Pipes = pipe + note
Music-X = keyboard + note
Audio editor = waveform + scissors
Audio recorder = microphone
Mixer = faders
CD player = disc + play
Internet radio = wifi + note
Speech = mouth + bubble

[Apps: video and multimedia]
Scala = monitor + lines@face
Video Toaster = monitor + bolt
Video player = film(play=1)
Animation player = flipbook + play
Genlock / titling = monitor + text(s=Aa,tint=white)@face
Slide show = photostack + play
Webcam = camera + globe

[Apps: development]
SAS/C = book + text(s=C,tint=white)@face
StormC = cloud(storm=1) + text(s=C,tint=blue)
GCC = cog + text(s=C,tint=blue)
vbcc = text(s=C,tint=blue) + cog
DICE = die
Devpac = text(s=ASM,tint=red) + cog
AsmOne / AsmPro = text(s=ASM,tint=red)
Blitz Basic / AmiBlitz = text(s=BB,tint=red) + bolt
AMOS = text(s=AMOS,tint=orange) + star
Hollywood = film + code
AmigaE = text(s=E,tint=green)
ARexx = crown
Lua = moon
Python = text(s=Py,tint=blue)
Debugger = bug + magnifier
Enforcer / MuForce = shield + exclaim
SnoopDOS = harddisk + magnifier
Scout = chip + magnifier
Sashimi = fish
Hex editor = hexgrid
Disassembler = code + magnifier
Version control = branches
ACKitchen = code + knife

[Apps: system and disk tools]
SysInfo = gauge
WHDLoad = joystick + floppy
JST = joystick
Executive = stopwatch
SnoopDOS = magnifier
Task manager = page(lines_on=1) + cross
Memory monitor = gauge + ramstick
CPU monitor = graph
Partitioner = pie
Disk salvage = harddisk + lifebuoy
Backup = harddisk + tape
Disk copier = floppy + floppy(tint=blue)
ADF transfer = floppy + arrow(dir=right)
Disk image mounter = floppydrive + disc
File system tools = harddisk + wrench
Defragmenter = harddisk + tiles
Virus checker = shield + bug
Boot selector = menu + harddisk
Early Startup = menu + power
Mounter = harddisk + usbplug
Log viewer = page(lines_on=1) + magnifier
Virtual memory = ramstick + harddisk
Clipboard viewer = clipboard
Screen blanker = monitor(dark=1) + star@face
Keyboard viewer = keyboard + eye

[Apps: archives and transfer]
LhA = box + text(s=LHA)@face
LZX = box + text(s=LZX)@face
UnZip / Zip = box + text(s=ZIP)@face
XAD tools = box + arrows
DMS = floppy + arrows
Aminet browser = globe + box
Package installer = box + arrow(dir=down)
Update tool = box + arrows(kind=circle)

[Apps: emulation and games]
UAE = monitor + computer@face
ShapeShifter = monitor + floppy
Fusion = box + monitor
PC-Task = pctower
C64 emulator = computer(kind=breadbox)
ScummVM = joystick + bubble
DOSBox = pctower + joystick
MAME = arcade
Spectrum emulator = computer(kind=rubber)
Game (WHDLoad install) = joystick + floppy
Game (disk image) = floppy + joystick
Game (CD32) = disc + gamepad
Demo = starburst
Intro / cracktro = star
Disk magazine = newspaper + floppy
Saved game = floppy + star

[Apps: hardware and drivers]
Picasso96 = card(kind=graphics)
CyberGraphX = card(kind=graphics) + monitor
Graphics card tools = card + wrench
AHI = card(kind=sound)
Poseidon = usbplug
Network card = card(kind=network)
Accelerator card = board(tint=blue)
PiStorm = board(chips=1,tint=red)
CPU control = chip(label=CPU)
MMU tools = chip + tiles
Real-time clock = clock + battery
Printer driver tool = printer + cog
Joystick / gamepad = gamepad
Mouse = mouse
Tablet = tablet
Scanner = scanner
Modem = modem
Monitor = monitor
Keyboard = keyboard
Gotek / floppy emulator = floppydrive(screen=1)
IDE / SCSI = board + harddisk
AmiCradle board = board + mark
"""

SKIP = {"Title bar, dock and gadget glyphs", "Pointers"}       # drawn by GFOpenLook and GFOpenWindows, not icons
PREFS = ("Prefs editors",)

# where each section's icons go in the collection, and their Workbench type
FOLDERS = [
    ("Disks and devices", "disks", 1), ("Workbench drawers", "drawers", 2), ("Your drawers", "drawers", 2),
    ("Prefs editors", "prefs", 3), ("Utilities, Tools and System", "system", 3), ("Commodities", "system", 3),
    ("Open apps", "open", 3), ("File types", "filetypes", 4), ("States", "states", 4), ("Apps:", "apps", 3),
]
DEVICES = {"Printer", "Unknown device", "Pipe", "Audio device", "Serial port", "Parallel port", "Printer port", "PC floppy driver",
           "Parallel link", "Other computer", "Amiga on the network"}


def parse_recipes(text: str) -> dict[tuple[str, str], str]:
    out, section = {}, ""
    for line in text.splitlines():
        line = line.strip()
        if not line:
            continue
        if line.startswith("[") and line.endswith("]"):
            section = line[1:-1]
            continue
        name, _, recipe = line.partition(" = ")
        out[(section, name.strip())] = recipe.strip()
    return out


PART = re.compile(r"^(\w+)(?:\((.*)\))?(?:@(\w+))?$")


def value(s: str):
    s = s.strip()
    if re.fullmatch(r"-?\d+", s):
        return int(s)
    if re.fullmatch(r"-?\d+\.\d+", s):
        return float(s)
    return s


def parse_part(s: str) -> tuple[str, dict, str | None]:
    m = PART.match(s.strip())
    if not m:
        raise ValueError(f"can't read {s!r}")
    name, args, slot = m.group(1), m.group(2), m.group(3)
    opts = {}
    if args:
        for kv in re.split(r",(?=\w+=)", args):
            k, _, val = kv.partition("=")
            opts[k.strip()] = value(val)
    for k in ("hd", "full", "lines_on", "two", "checks", "dots", "rainbow", "commas", "notes", "frame_only", "play", "dark",
              "muted", "dot", "broken", "ring", "wave", "storm", "screen", "outline_only", "pixels", "faded"):
        if k in opts:
            opts[k] = bool(opts[k])
    return name, opts, slot


def draw(recipe: str, sel: bool, prefs: bool) -> Image.Image:
    parts = [parse_part(x) for x in recipe.split(" + ")]
    ic = g.Icon()
    pen = v.Pen(ic)
    name, opts, _ = parts[0]
    slots = v.OBJECTS[name](pen, sel=sel, **opts)
    for n, o, slot in parts[1:]:
        slot = slot or "badge"
        v.OBJECTS[n](pen.slot(slots, slot), sel=sel, **o)
    if prefs:
        v.OBJECTS["prefs"](pen.slot(slots, "bl"), sel=sel)
    return ic.render()


def frames(recipe: str, prefs: bool) -> list[Image.Image]:
    m = re.fullmatch(r"file\((.+)\)", recipe)
    if m:
        return [Image.open(OPEN_ART / f"{m.group(1)}{s}.png").convert("RGBA") for s in ("", "-selected")]
    return g.finish([draw(recipe, False, prefs), draw(recipe, True, prefs)])


def folder_for(section: str) -> tuple[str, int]:
    for prefix, folder, itype in FOLDERS:
        if section.startswith(prefix):
            return folder, itype
    raise ValueError(section)


def file_names(icon: str, section: str) -> list[str]:
    """The names an icon is saved under: what Workbench calls it (the last part
    of a SYS: path), and each of "A / B"'s names."""
    if "drawers" in section and "/" in icon:
        return [icon.rsplit("/", 1)[1]]
    names = [n.strip() for n in icon.split(" / ")] if " / " in icon else [icon]
    return [re.sub(r'[/:"*?<>|]', "-", n) for n in names]


def rows() -> list[dict]:
    with LIST.open(newline="") as f:
        return [r for r in csv.DictReader(f) if r["Category"] not in SKIP]


def sheet(items: list[tuple[str, list[Image.Image]]], out: Path) -> None:
    cell, cols = 74, 8
    n = len(items)
    nrows = (n + cols - 1) // cols
    W, H = cols * cell * 2, nrows * (cell + 14)
    img = Image.new("RGB", (W, H * 2), (236, 239, 242))
    d = ImageDraw.Draw(img)
    d.rectangle([0, H, W, 2 * H], fill=(31, 41, 52))
    for half in (0, 1):
        for i, (name, fr) in enumerate(items):
            x, y = (i % cols) * cell * 2, half * H + (i // cols) * (cell + 14)
            for k, f in enumerate(fr):
                img.paste(f, (x + k * cell + 4, y + 2), f)
            d.text((x + 4, y + cell - 2), name[:24], fill=(110, 110, 110) if not half else (170, 180, 190))
    img.save(out)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("out", type=Path)
    ap.add_argument("--sheet", action="store_true", help="contact sheets, light and dark, per folder")
    ap.add_argument("--only", default="", help="only icons whose name holds this text")
    ap.add_argument("--amigachrome", type=Path, help="an amicradle checkout (for amiga_icon); default ../amicradle")
    a = ap.parse_args(argv)
    ac = a.amigachrome or ROOT.parent / "amicradle"
    sys.path.insert(0, str(ac / "scripts"))
    import amiga_icon

    recipes = parse_recipes(RECIPES)
    a.out.mkdir(parents=True, exist_ok=True)
    made, skipped, missing = {}, [], []
    by_folder: dict[str, list] = {}
    for r in rows():
        section, icon = r["Category"], r["Icon"]
        recipe = recipes.get((section, icon))
        if recipe is None:
            missing.append(f"{section} | {icon}")
            continue
        if recipe == "-" or (a.only and a.only.lower() not in icon.lower()):
            skipped.append(icon)
            continue
        folder, itype = folder_for(section)
        if folder == "disks":
            itype = 5 if icon.startswith("Trashcan") else 6 if icon in DEVICES else 1
        if icon == "Trashcan drawer" or (folder == "states" and icon == "Trashcan full"):
            itype = 5
        fr = frames(recipe, prefs=section.startswith(PREFS) and not recipe.startswith("file("))
        for name in file_names(icon, section):
            path = a.out / "png" / folder / f"{name}.info"
            if path in made:
                continue                                  # the same name in a folder: the first one stands
            path.parent.mkdir(parents=True, exist_ok=True)
            pics = [(f.width, f.height, f.tobytes()) for f in fr]
            meta = {"type": itype, "defaultTool": None, "toolTypes": [], "toolWindow": None, "stack": 4096,
                    "x": amiga_icon.NO_POSITION, "y": amiga_icon.NO_POSITION,
                    "drawer": {"left": 50, "top": 50, "width": 400, "height": 200} if itype in amiga_icon.DRAWER_TYPES else None}
            path.write_bytes(amiga_icon.write_png_icon(meta, pics))
            made[path] = recipe
            by_folder.setdefault(folder, []).append((name, fr))
    (a.out / "collection.json").write_text(json.dumps({
        "name": "GFOpenUp (Gorilla style)",
        "source": "gfopenup tools/icon_library_art.py, drawn from icons/LIST.csv with tools/gvocab.py",
        "licence": "MIT",
        "notes": "Copyright (c) 2026 AmiCradle Team. Our own drawing in the Gorilla style (bold outlines, light from the top left, "
                 "a soft shadow), nothing traced. png/: PNG icons, 64 pixels, two frames (the second moves something).",
        "rank": 3,
    }, indent=2) + "\n")
    if a.sheet:
        (a.out / "sheets").mkdir(parents=True, exist_ok=True)
        for folder, items in by_folder.items():
            sheet(items, a.out / "sheets" / f"{folder}.png")
    print(f"{len(made)} icons in {a.out / 'png'}; {len(skipped)} skipped; {len(missing)} without a recipe")
    for m in missing:
        print("  no recipe:", m)
    return 0 if not missing else 1


if __name__ == "__main__":
    sys.exit(main())
