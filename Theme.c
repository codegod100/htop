/*
htop - Theme.c
(C) 2026 htop contributors
Released under the GNU GPLv2+, see the COPYING file
in the source distribution for its full text.
*/

#include "config.h" // IWYU pragma: keep

#include "Theme.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ProvideCurses.h"
#include "XUtils.h"


/* Match CRT.c pair encoding so file themes produce identical attr values. */
#define ColorIndex(i, j) ((7 - (i)) * 8 + (j))
#define ColorPair(i, j)  COLOR_PAIR(ColorIndex(i, j))

#define Black   COLOR_BLACK
#define Red     COLOR_RED
#define Green   COLOR_GREEN
#define Yellow  COLOR_YELLOW
#define Blue    COLOR_BLUE
#define Magenta COLOR_MAGENTA
#define Cyan    COLOR_CYAN
#define White   COLOR_WHITE

#define ColorPairGrayBlack ColorPair(Magenta, Magenta)

/* Special color name sentinels used only while parsing. */
#define THEME_COLOR_DEFAULT (-2)
#define THEME_COLOR_GRAY    (-3)

static const char* const Theme_elementNames[LAST_COLORELEMENT] = {
   [RESET_COLOR] = "RESET_COLOR",
   [DEFAULT_COLOR] = "DEFAULT_COLOR",
   [FUNCTION_BAR] = "FUNCTION_BAR",
   [FUNCTION_KEY] = "FUNCTION_KEY",
   [FAILED_SEARCH] = "FAILED_SEARCH",
   [FAILED_READ] = "FAILED_READ",
   [PAUSED] = "PAUSED",
   [PANEL_HEADER_FOCUS] = "PANEL_HEADER_FOCUS",
   [PANEL_HEADER_UNFOCUS] = "PANEL_HEADER_UNFOCUS",
   [PANEL_SELECTION_FOCUS] = "PANEL_SELECTION_FOCUS",
   [PANEL_SELECTION_FOLLOW] = "PANEL_SELECTION_FOLLOW",
   [PANEL_SELECTION_UNFOCUS] = "PANEL_SELECTION_UNFOCUS",
   [LARGE_NUMBER] = "LARGE_NUMBER",
   [METER_SHADOW] = "METER_SHADOW",
   [METER_TEXT] = "METER_TEXT",
   [METER_VALUE] = "METER_VALUE",
   [METER_VALUE_ERROR] = "METER_VALUE_ERROR",
   [METER_VALUE_IOREAD] = "METER_VALUE_IOREAD",
   [METER_VALUE_IOWRITE] = "METER_VALUE_IOWRITE",
   [METER_VALUE_NOTICE] = "METER_VALUE_NOTICE",
   [METER_VALUE_OK] = "METER_VALUE_OK",
   [METER_VALUE_WARN] = "METER_VALUE_WARN",
   [LED_COLOR] = "LED_COLOR",
   [UPTIME] = "UPTIME",
   [BATTERY] = "BATTERY",
   [TASKS_RUNNING] = "TASKS_RUNNING",
   [SWAP] = "SWAP",
   [SWAP_CACHE] = "SWAP_CACHE",
   [SWAP_FRONTSWAP] = "SWAP_FRONTSWAP",
   [PROCESS] = "PROCESS",
   [PROCESS_SHADOW] = "PROCESS_SHADOW",
   [PROCESS_TAG] = "PROCESS_TAG",
   [PROCESS_MEGABYTES] = "PROCESS_MEGABYTES",
   [PROCESS_GIGABYTES] = "PROCESS_GIGABYTES",
   [PROCESS_TREE] = "PROCESS_TREE",
   [PROCESS_RUN_STATE] = "PROCESS_RUN_STATE",
   [PROCESS_D_STATE] = "PROCESS_D_STATE",
   [PROCESS_BASENAME] = "PROCESS_BASENAME",
   [PROCESS_HIGH_PRIORITY] = "PROCESS_HIGH_PRIORITY",
   [PROCESS_LOW_PRIORITY] = "PROCESS_LOW_PRIORITY",
   [PROCESS_NEW] = "PROCESS_NEW",
   [PROCESS_TOMB] = "PROCESS_TOMB",
   [PROCESS_THREAD] = "PROCESS_THREAD",
   [PROCESS_THREAD_BASENAME] = "PROCESS_THREAD_BASENAME",
   [PROCESS_COMM] = "PROCESS_COMM",
   [PROCESS_THREAD_COMM] = "PROCESS_THREAD_COMM",
   [PROCESS_PRIV] = "PROCESS_PRIV",
   [BAR_BORDER] = "BAR_BORDER",
   [BAR_SHADOW] = "BAR_SHADOW",
   [GRAPH_1] = "GRAPH_1",
   [GRAPH_2] = "GRAPH_2",
   [MEMORY_1] = "MEMORY_1",
   [MEMORY_2] = "MEMORY_2",
   [MEMORY_3] = "MEMORY_3",
   [MEMORY_4] = "MEMORY_4",
   [MEMORY_5] = "MEMORY_5",
   [MEMORY_6] = "MEMORY_6",
   [HUGEPAGE_1] = "HUGEPAGE_1",
   [HUGEPAGE_2] = "HUGEPAGE_2",
   [HUGEPAGE_3] = "HUGEPAGE_3",
   [HUGEPAGE_4] = "HUGEPAGE_4",
   [LOAD] = "LOAD",
   [LOAD_AVERAGE_FIFTEEN] = "LOAD_AVERAGE_FIFTEEN",
   [LOAD_AVERAGE_FIVE] = "LOAD_AVERAGE_FIVE",
   [LOAD_AVERAGE_ONE] = "LOAD_AVERAGE_ONE",
   [CHECK_BOX] = "CHECK_BOX",
   [CHECK_MARK] = "CHECK_MARK",
   [CHECK_TEXT] = "CHECK_TEXT",
   [CLOCK] = "CLOCK",
   [DATE] = "DATE",
   [DATETIME] = "DATETIME",
   [HELP_BOLD] = "HELP_BOLD",
   [HELP_SHADOW] = "HELP_SHADOW",
   [HOSTNAME] = "HOSTNAME",
   [CPU_NICE] = "CPU_NICE",
   [CPU_NICE_TEXT] = "CPU_NICE_TEXT",
   [CPU_NORMAL] = "CPU_NORMAL",
   [CPU_SYSTEM] = "CPU_SYSTEM",
   [CPU_IOWAIT] = "CPU_IOWAIT",
   [CPU_IRQ] = "CPU_IRQ",
   [CPU_SOFTIRQ] = "CPU_SOFTIRQ",
   [CPU_STEAL] = "CPU_STEAL",
   [CPU_GUEST] = "CPU_GUEST",
   [GPU_ENGINE_1] = "GPU_ENGINE_1",
   [GPU_ENGINE_2] = "GPU_ENGINE_2",
   [GPU_ENGINE_3] = "GPU_ENGINE_3",
   [GPU_ENGINE_4] = "GPU_ENGINE_4",
   [GPU_RESIDUE] = "GPU_RESIDUE",
   [PANEL_EDIT] = "PANEL_EDIT",
   [SCREENS_OTH_BORDER] = "SCREENS_OTH_BORDER",
   [SCREENS_OTH_TEXT] = "SCREENS_OTH_TEXT",
   [SCREENS_CUR_BORDER] = "SCREENS_CUR_BORDER",
   [SCREENS_CUR_TEXT] = "SCREENS_CUR_TEXT",
   [PRESSURE_STALL_TEN] = "PRESSURE_STALL_TEN",
   [PRESSURE_STALL_SIXTY] = "PRESSURE_STALL_SIXTY",
   [PRESSURE_STALL_THREEHUNDRED] = "PRESSURE_STALL_THREEHUNDRED",
   [FILE_DESCRIPTOR_USED] = "FILE_DESCRIPTOR_USED",
   [FILE_DESCRIPTOR_MAX] = "FILE_DESCRIPTOR_MAX",
   [ZFS_MFU] = "ZFS_MFU",
   [ZFS_MRU] = "ZFS_MRU",
   [ZFS_ANON] = "ZFS_ANON",
   [ZFS_HEADER] = "ZFS_HEADER",
   [ZFS_OTHER] = "ZFS_OTHER",
   [ZFS_COMPRESSED] = "ZFS_COMPRESSED",
   [ZFS_RATIO] = "ZFS_RATIO",
   [ZRAM_COMPRESSED] = "ZRAM_COMPRESSED",
   [ZRAM_UNCOMPRESSED] = "ZRAM_UNCOMPRESSED",
   [DYNAMIC_GRAY] = "DYNAMIC_GRAY",
   [DYNAMIC_DARKGRAY] = "DYNAMIC_DARKGRAY",
   [DYNAMIC_RED] = "DYNAMIC_RED",
   [DYNAMIC_GREEN] = "DYNAMIC_GREEN",
   [DYNAMIC_BLUE] = "DYNAMIC_BLUE",
   [DYNAMIC_CYAN] = "DYNAMIC_CYAN",
   [DYNAMIC_MAGENTA] = "DYNAMIC_MAGENTA",
   [DYNAMIC_YELLOW] = "DYNAMIC_YELLOW",
   [DYNAMIC_WHITE] = "DYNAMIC_WHITE",
};

const char* Theme_elementName(ColorElements element) {
   if (element < 0 || element >= LAST_COLORELEMENT)
      return NULL;
   return Theme_elementNames[element];
}

static int Theme_findElement(const char* name) {
   for (int i = 0; i < LAST_COLORELEMENT; i++) {
      if (Theme_elementNames[i] && String_eq(Theme_elementNames[i], name))
         return i;
   }
   return -1;
}

static void Theme_tolowerInPlace(char* s) {
   for (; *s; s++)
      *s = (char) tolower((unsigned char) *s);
}

static int Theme_parseColorName(const char* name) {
   if (String_eq(name, "black"))
      return Black;
   if (String_eq(name, "red"))
      return Red;
   if (String_eq(name, "green"))
      return Green;
   if (String_eq(name, "yellow"))
      return Yellow;
   if (String_eq(name, "blue"))
      return Blue;
   if (String_eq(name, "magenta") || String_eq(name, "purple"))
      return Magenta;
   if (String_eq(name, "cyan"))
      return Cyan;
   if (String_eq(name, "white"))
      return White;
   if (String_eq(name, "gray") || String_eq(name, "grey"))
      return THEME_COLOR_GRAY;
   if (String_eq(name, "default") || String_eq(name, "none"))
      return THEME_COLOR_DEFAULT;
   return -1;
}

static int Theme_parseAttr(const char* name) {
   if (String_eq(name, "bold"))
      return A_BOLD;
   if (String_eq(name, "reverse") || String_eq(name, "standout"))
      return A_REVERSE;
   if (String_eq(name, "dim") || String_eq(name, "dark"))
      return A_DIM;
   if (String_eq(name, "underline"))
      return A_UNDERLINE;
   if (String_eq(name, "normal") || String_eq(name, "none"))
      return A_NORMAL;
   if (String_eq(name, "blink"))
      return A_BLINK;
   return -1;
}

static int Theme_compose(int fg, int bg, int attrs) {
   if (fg == THEME_COLOR_GRAY)
      return attrs | ColorPairGrayBlack;

   /* Terminal default maps onto the black slot; CRT_setColors maps black bg to -1. */
   if (fg == THEME_COLOR_DEFAULT)
      fg = White;
   if (bg == THEME_COLOR_DEFAULT || bg == THEME_COLOR_GRAY)
      bg = Black;

   return attrs | ColorPair(fg, bg);
}

static bool Theme_parseValue(char* value, int* outAttr) {
   char* tokens[16];
   size_t n = 0;

   for (char* p = value; *p && n < ARRAYSIZE(tokens); ) {
      while (*p && isspace((unsigned char) *p))
         p++;
      if (!*p)
         break;
      tokens[n++] = p;
      while (*p && !isspace((unsigned char) *p) && *p != ',')
         p++;
      if (*p) {
         *p++ = '\0';
      }
   }

   if (n < 1)
      return false;

   /* Attribute-only form: "bold" / "reverse" / "normal" */
   if (n == 1) {
      int attr = Theme_parseAttr(tokens[0]);
      if (attr < 0)
         return false;
      *outAttr = attr;
      return true;
   }

   int fg = Theme_parseColorName(tokens[0]);
   int bg = Theme_parseColorName(tokens[1]);
   if (fg == -1 || bg == -1)
      return false;

   int attrs = A_NORMAL;
   for (size_t i = 2; i < n; i++) {
      int a = Theme_parseAttr(tokens[i]);
      if (a < 0)
         return false;
      attrs |= a;
   }

   *outAttr = Theme_compose(fg, bg, attrs);
   return true;
}

bool Theme_loadFile(const char* path, int out[LAST_COLORELEMENT], bool* forceBlackBg) {
   FILE* fp = fopen(path, "r");
   if (!fp)
      return false;

   /* Seed with the built-in default scheme so partial themes work. */
   const int* defaults = CRT_getColorScheme(COLORSCHEME_DEFAULT);
   memcpy(out, defaults, sizeof(int) * LAST_COLORELEMENT);

   if (forceBlackBg)
      *forceBlackBg = false;

   bool ok = true;
   char* line;

   while ((line = String_readLine(fp)) != NULL) {
      char* p = line;
      while (*p && isspace((unsigned char) *p))
         p++;
      if (*p == '\0' || *p == '#' || *p == ';') {
         free(line);
         continue;
      }

      char* cr = strchr(p, '\r');
      if (cr)
         *cr = '\0';

      char* eq = strchr(p, '=');
      if (!eq) {
         free(line);
         ok = false;
         break;
      }
      *eq = '\0';
      char* key = p;
      char* val = eq + 1;

      /* Trim key */
      char* keyEnd = key + strlen(key);
      while (keyEnd > key && isspace((unsigned char) keyEnd[-1]))
         *--keyEnd = '\0';
      while (*val && isspace((unsigned char) *val))
         val++;
      char* valEnd = val + strlen(val);
      while (valEnd > val && isspace((unsigned char) valEnd[-1]))
         *--valEnd = '\0';

      char keyLower[128];
      String_safeStrncpy(keyLower, key, sizeof(keyLower));
      Theme_tolowerInPlace(keyLower);

      if (String_eq(keyLower, "name") || String_eq(keyLower, "force_black_bg") ||
          String_eq(keyLower, "force-black-bg") || String_eq(keyLower, "black_bg")) {
         if (String_eq(keyLower, "force_black_bg") || String_eq(keyLower, "force-black-bg") ||
             String_eq(keyLower, "black_bg")) {
            if (forceBlackBg) {
               *forceBlackBg = (String_eq(val, "1") || String_eq(val, "true") ||
                                String_eq(val, "yes") || String_eq(val, "on"));
            }
         }
         free(line);
         continue;
      }

      /* Accept element names case-insensitively. */
      char elemKey[128];
      String_safeStrncpy(elemKey, key, sizeof(elemKey));
      for (char* c = elemKey; *c; c++) {
         if (*c == '-')
            *c = '_';
         else
            *c = (char) toupper((unsigned char) *c);
      }

      int elem = Theme_findElement(elemKey);
      if (elem < 0) {
         /* Unknown keys are ignored so themes can grow with new elements. */
         free(line);
         continue;
      }

      char valCopy[256];
      String_safeStrncpy(valCopy, val, sizeof(valCopy));
      Theme_tolowerInPlace(valCopy);

      int attr = 0;
      if (!Theme_parseValue(valCopy, &attr)) {
         free(line);
         ok = false;
         break;
      }
      out[elem] = attr;
      free(line);
   }

   fclose(fp);
   return ok;
}

static char* Theme_idFromFilename(const char* filename) {
   const char* base = strrchr(filename, '/');
   base = base ? base + 1 : filename;
   size_t len = strlen(base);
   if (len > 6 && String_eq(base + len - 6, ".theme"))
      len -= 6;
   return xStrndup(base, len);
}

static char* Theme_readDisplayName(const char* path, const char* fallbackId) {
   FILE* fp = fopen(path, "r");
   if (!fp)
      return xStrdup(fallbackId);

   char* name = NULL;
   char* line;

   while ((line = String_readLine(fp)) != NULL) {
      char* p = line;
      while (*p && isspace((unsigned char) *p))
         p++;
      if (*p == '#' || *p == ';' || *p == '\0') {
         free(line);
         continue;
      }
      if (!String_startsWith(p, "name=") && !String_startsWith(p, "Name=")) {
         free(line);
         continue;
      }
      p = strchr(p, '=');
      if (!p) {
         free(line);
         break;
      }
      p++;
      while (*p && isspace((unsigned char) *p))
         p++;
      char* cr = strchr(p, '\r');
      if (cr)
         *cr = '\0';
      if (*p)
         name = xStrdup(p);
      free(line);
      break;
   }

   fclose(fp);
   return name ? name : xStrdup(fallbackId);
}

static void Theme_listAddOrReplace(ThemeList* list, const char* path) {
   char* id = Theme_idFromFilename(path);
   char* name = Theme_readDisplayName(path, id);

   for (size_t i = 0; i < list->count; i++) {
      if (String_eq(list->items[i].id, id)) {
         free(list->items[i].name);
         free(list->items[i].path);
         free(list->items[i].id);
         list->items[i].id = id;
         list->items[i].name = name;
         list->items[i].path = xStrdup(path);
         return;
      }
   }

   list->items = xReallocArray(list->items, list->count + 1, sizeof(ThemeInfo));
   list->items[list->count].id = id;
   list->items[list->count].name = name;
   list->items[list->count].path = xStrdup(path);
   list->count++;
}

static void Theme_scanDir(ThemeList* list, const char* dirPath) {
   DIR* dir = opendir(dirPath);
   if (!dir)
      return;

   struct dirent* de;
   while ((de = readdir(dir)) != NULL) {
      if (de->d_name[0] == '.')
         continue;
      size_t len = strlen(de->d_name);
      if (len < 7 || !String_eq(de->d_name + len - 6, ".theme"))
         continue;

      char path[PATH_MAX];
      xSnprintf(path, sizeof(path), "%s/%s", dirPath, de->d_name);

      struct stat st;
      if (stat(path, &st) != 0 || !S_ISREG(st.st_mode))
         continue;

      Theme_listAddOrReplace(list, path);
   }
   closedir(dir);
}

static int Theme_compareInfo(const void* a, const void* b) {
   const ThemeInfo* ta = (const ThemeInfo*) a;
   const ThemeInfo* tb = (const ThemeInfo*) b;
   return SPACESHIP_NULLSTR(ta->name, tb->name);
}

ThemeList* Theme_scan(void) {
   ThemeList* list = xCalloc(1, sizeof(ThemeList));

   /* System data first, then sysconf, then user (later wins on same id). */
#ifdef HTOP_DATADIR
   {
      char path[PATH_MAX];
      xSnprintf(path, sizeof(path), "%s/themes", HTOP_DATADIR);
      Theme_scanDir(list, path);
   }
#endif
#ifdef SYSCONFDIR
   {
      char path[PATH_MAX];
      xSnprintf(path, sizeof(path), "%s/htop/themes", SYSCONFDIR);
      Theme_scanDir(list, path);
   }
#endif

   const char* home = getenv("HOME");
   if (!home || home[0] != '/') {
      const struct passwd* pw = getpwuid(getuid());
      home = (pw && pw->pw_dir && pw->pw_dir[0] == '/') ? pw->pw_dir : NULL;
   }

   const char* xdg = getenv("XDG_CONFIG_HOME");
   if (xdg && xdg[0] == '/') {
      char path[PATH_MAX];
      xSnprintf(path, sizeof(path), "%s/htop/themes", xdg);
      Theme_scanDir(list, path);
   } else if (home) {
      char path[PATH_MAX];
      xSnprintf(path, sizeof(path), "%s" CONFIGDIR "/htop/themes", home);
      Theme_scanDir(list, path);
   }

   /* Developer override for running from the build tree. */
   const char* override = getenv("HTOP_THEME_DIR");
   if (override && override[0])
      Theme_scanDir(list, override);

   if (list->count > 1)
      qsort(list->items, list->count, sizeof(ThemeInfo), Theme_compareInfo);

   return list;
}

void Theme_listDelete(ThemeList* list) {
   if (!list)
      return;
   for (size_t i = 0; i < list->count; i++) {
      free(list->items[i].id);
      free(list->items[i].name);
      free(list->items[i].path);
   }
   free(list->items);
   free(list);
}

const ThemeInfo* Theme_listFind(const ThemeList* list, const char* id) {
   if (!list || !id)
      return NULL;
   for (size_t i = 0; i < list->count; i++) {
      if (String_eq(list->items[i].id, id))
         return &list->items[i];
   }
   return NULL;
}

bool Theme_loadById(const char* id, int out[LAST_COLORELEMENT], bool* forceBlackBg) {
   ThemeList* list = Theme_scan();
   const ThemeInfo* info = Theme_listFind(list, id);
   bool ok = false;
   if (info)
      ok = Theme_loadFile(info->path, out, forceBlackBg);
   Theme_listDelete(list);
   return ok;
}
