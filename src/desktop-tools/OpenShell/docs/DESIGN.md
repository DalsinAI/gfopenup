# OpenShell 0.1 - OpenUp Shell Window

OpenShell is not a replacement command language. It is a modern Workbench
front-end for the Amiga Shell.

The command interpreter remains the system Shell. OpenShell improves the
window, input, history, navigation, output handling and session management
around it.

## Design rule

**Do not break Amiga Shell behaviour to obtain a modern-looking terminal.**

Scripts, aliases, assigns, command search paths, return codes and normal Shell
commands remain Amiga semantics.

## Window layout

    +------------------------------------------------------------------+
    | [DH0:System] [DH1:Projects] [+]                     OpenShell     |
    +------------------------------------------------------------------+
    | DH1:Projects/OpenUp                                                |
    |------------------------------------------------------------------|
    | 07:41  > list src                                                  |
    |         opencalc.c        4217                                     |
    |         openinput.c       8912                                     |
    |                                                                    |
    | 07:41  > make                                                      |
    |         Compiling...                                               |
    |         0 errors                                                   |
    |                                                                    |
    |------------------------------------------------------------------|
    | DH1:Projects/OpenUp > _                                            |
    +------------------------------------------------------------------+
    | Ready | RC 0 | 18 commands | UTF-8 display | 80 x 34             |
    +------------------------------------------------------------------+

The content area is the Shell transcript. The input line is visually
separated at the bottom but still submits ordinary Shell commands.

## Tabs

Each tab owns an independent Shell process and state:

- current directory;
- local aliases/environment inherited at launch;
- transcript;
- command history;
- return code;
- running command state.

A tab title defaults to the current drawer/path but can be renamed.

Closing a busy tab must ask before terminating its process.

## Path bar

The current path appears immediately above the transcript.

Functions:

- click a path component to `CD` there;
- copy path;
- paste a path into the command line;
- open the location as a Workbench drawer;
- drag a Workbench file/drawer into the Shell to insert its quoted path.

The path bar reflects Shell state; it must not maintain a second imaginary
working directory.

## Command line

The editable command line supports:

- normal Amiga Shell syntax;
- cursor navigation;
- Home/End;
- word-wise movement;
- command history Up/Down;
- incremental history search;
- cut/copy/paste;
- undo for current edit buffer;
- filename/path completion;
- command completion from the current command path;
- selected-history insertion without immediate execution.

Return sends the line to the Shell.

Shift+Return inserts a line break only when multiline-edit mode is enabled.

## Transcript

Output is selectable text rather than a dumb painted terminal surface.

Required operations:

- normal drag selection;
- Select All;
- Copy;
- Find;
- Find Next/Previous;
- clear transcript without resetting the Shell;
- save transcript;
- copy an individual command and its output;
- visible error/return-code markers without recolouring command output.

The Shell output bytes remain authoritative. OpenShell may provide a Unicode
presentation layer, but it must not mutate command data sent to or received
from the Shell.

## History drawer

A lightweight side panel, hidden by default:

    History
    Today
      07:41 make
      07:40 list src
      07:39 cd DH1:Projects/OpenUp

Selecting an item places it in the command editor. Double activation can run
it, but a single selection must never unexpectedly execute a command.

History is per-user and can be disabled.

## Completion

Tab completion operates in this order:

1. current command token;
2. command search path;
3. current/explicit Amiga path;
4. assigns/devices where appropriate.

If one result exists, insert it.
If several exist, show a compact completion popup anchored to the command
line.

Never print pages of completion candidates into the transcript unless the
user explicitly asks for that classic behaviour.

## Status line

The bottom status line shows only useful state:

- Ready / Running / Waiting;
- last return code;
- command count in this tab;
- optional dimensions;
- optional current text/display encoding.

Do not waste permanent space on decorative status.

## Keyboard

Suggested defaults:

- Ctrl+Shift+T: new tab
- Ctrl+Shift+W: close tab
- Ctrl+Tab / Ctrl+Shift+Tab: switch tabs
- Ctrl+L: focus/clear command editor
- Ctrl+F: transcript find
- Ctrl+R: history search
- Ctrl+C: copy selection if one exists; otherwise send break only after a
  deliberate Shell-compatible mapping
- Ctrl+Shift+C / Ctrl+Shift+V may be offered for users who prefer terminal
  conventions
- Amiga key shortcuts should also be provided where normal Workbench practice
  calls for them.

Keyboard mappings must be configurable because Amiga Shell break semantics
must not be sacrificed for copy/paste convenience.

## Menus

Project:
- New Tab
- New Window
- Save Transcript
- Close Tab
- Quit

Edit:
- Cut
- Copy
- Paste
- Select All
- Find
- History

Shell:
- Break
- Clear Transcript
- Reset Session
- Open Current Drawer
- Snapshot Window

View:
- History Panel
- Status Bar
- Font
- Theme
- Wrap Long Lines

## Workbench integration

- Drop files/directories to insert properly quoted Amiga paths.
- "Open Shell Here" from a drawer should open a new OpenShell tab at that
  exact Amiga path.
- A path-bar action can open the current directory in Workbench.
- Window geometry is snapshot-compatible.
- AppWindow support is preferred for drag/drop integration.

## Sessions and persistence

Persist UI preferences separately from Shell state.

Safe to remember:
- window geometry;
- font;
- wrapping;
- history-panel visibility;
- keyboard preferences;
- optional command history.

Do not silently resurrect running processes or unfinished commands after a
crash.

## Architecture

    Workbench window / MUI UI
              |
              v
       OpenShell controller
        /       |        \
       /        |         \
 command     transcript    tabs
 editor       model        model
       \        |         /
        \       |        /
          Shell transport
               |
               v
          Amiga Shell

The transport is the only layer that talks to the real Shell process.

This keeps the presentation replaceable and makes transport behaviour
testable.

## Non-goals for 0.1

- replacing the AmigaDOS Shell language;
- POSIX emulation;
- pretending `/home/foo` is an Amiga path;
- embedded SSH;
- a full IDE;
- terminal escape emulation unrelated to Amiga Shell.

Those can be separate components later.

## Suggested repository location

    OpenUp/
      apps/
        OpenShell/
