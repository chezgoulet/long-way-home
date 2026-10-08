// lwh_ui.h -- Long Way Home's screens in the UI module. The one name the patched upstream UI calls.

#ifndef LWH_UI_H
#define LWH_UI_H

// Handles our UI console commands (ui_lwh_engineering, ...). True if the command was ours.
qboolean LWH_UI_ConsoleCommand( const char *cmd );

// The command console and character creation (ui_lwh_command.cpp); called by the above.
qboolean LWH_UI_CommandScreens( const char *cmd );

// The way in: the "Long Way Home" line on the main menu and the start-state selector it opens
// (ui_lwh_start.cpp). Called by the above.
qboolean LWH_UI_StartScreens( const char *cmd );

// Adds our line to the main menu. Implemented in ui_lwh_start.cpp and called from the patched
// upstream ui_menu.cpp (an attach point: the menu itself is upstream's). `menu` is a
// menuframework_s, passed as void* so the hook header needs no UI type.
void LWH_UI_MainMenuAdd( void *menu );

#endif
