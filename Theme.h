#ifndef HEADER_Theme
#define HEADER_Theme
/*
htop - Theme.h
(C) 2026 htop contributors
Released under the GNU GPLv2+, see the COPYING file
in the source distribution for its full text.
*/

#include <stdbool.h>
#include <stddef.h>

#include "CRT.h"


typedef struct ThemeInfo_ {
   char* id;    /* basename without .theme; used in htoprc */
   char* name;  /* display name from file or id */
   char* path;  /* absolute or relative filesystem path */
} ThemeInfo;

typedef struct ThemeList_ {
   ThemeInfo* items;
   size_t count;
} ThemeList;

/* Load a theme file into out[LAST_COLORELEMENT].
 * Missing elements keep the current DEFAULT scheme values.
 * Returns true on success. forceBlackBg is set when the theme requests it. */
bool Theme_loadFile(const char* path, int out[LAST_COLORELEMENT], bool* forceBlackBg);

/* Discover theme files from config and data directories.
 * Later directories override earlier ones with the same id (user wins). */
ThemeList* Theme_scan(void);

void Theme_listDelete(ThemeList* list);

/* Find a theme by id in a previously scanned list. */
const ThemeInfo* Theme_listFind(const ThemeList* list, const char* id);

/* Convenience: scan, load by id, free the list. Returns true on success. */
bool Theme_loadById(const char* id, int out[LAST_COLORELEMENT], bool* forceBlackBg);

/* Element name for diagnostics / documentation (may be NULL for unused). */
const char* Theme_elementName(ColorElements element);

#endif
