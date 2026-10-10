/*
 * OpenShell UI integration notes / skeleton.
 *
 * The concrete GUI should use the OpenUp project's standard MUI/OpenGUI
 * wrapper. Keep these logical widgets:
 *
 * Window
 *   TabStrip
 *   PathBar
 *   TranscriptView
 *   CommandEditor
 *   StatusBar
 *
 * Optional side pane:
 *   HistoryView
 *
 * Crucial behaviour:
 * - one transport per tab
 * - transcript output is append-only data plus selection/search metadata
 * - command editor is separate from transcript display
 * - current path is queried from the real Shell after relevant commands
 * - drag/drop inserts quoted paths into CommandEditor
 * - UI never parses/reimplements Amiga Shell command semantics
 */

#include "openshell_model.h"

int main(void) {
    /* Repository GUI adapter lands here. */
    return 0;
}
