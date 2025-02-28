//
//  defines.h
//  xESP
//
//  Created by ocfu on 12.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#ifndef defines_h
#define defines_h

// escape sequences

// text colors
#define ESC_TEXT_BLACK      "\033[30m"
#define ESC_TEXT_RED        "\033[31m"
#define ESC_TEXT_GREEN      "\033[32m"
#define ESC_TEXT_YELLOW     "\033[33m"
#define ESC_TEXT_BLUE       "\033[34m"
#define ESC_TEXT_MAGENTA    "\033[35m"
#define ESC_TEXT_CYAN       "\033[36m"
#define ESC_TEXT_WHITE      "\033[37m"
#define ESC_TEXT_DEFAULT    "\033[39m"

// background colors
#define ESC_BG_BLACK        "\033[40m"
#define ESC_BG_RED          "\033[41m"
#define ESC_BG_GREEN        "\033[42m"
#define ESC_BG_YELLOW       "\033[43m"
#define ESC_BG_BLUE         "\033[44m"
#define ESC_BG_MAGENTA      "\033[45m"
#define ESC_BG_CYAN         "\033[46m"
#define ESC_BG_WHITE        "\033[47m"
#define ESC_BG_DEFAULT      "\033[49m"

// text attributes
#define ESC_ATTR_RESET          "\033[0m"
#define ESC_ATTR_BOLD           "\033[1m"
#define ESC_ATTR_DIM            "\033[2m"
#define ESC_ATTR_ITALIC         "\033[3m"
#define ESC_ATTR_UNDERLINE      "\033[4m"
#define ESC_ATTR_BLINK          "\033[5m"
#define ESC_ATTR_REVERSE        "\033[7m"
#define ESC_ATTR_HIDDEN         "\033[8m"
#define ESC_ATTR_STRIKETHROUGH  "\033[9m"

// bright text colors
#define ESC_TEXT_BRIGHT_BLACK   "\033[90m"
#define ESC_TEXT_BRIGHT_RED     "\033[91m"
#define ESC_TEXT_BRIGHT_GREEN   "\033[92m"
#define ESC_TEXT_BRIGHT_YELLOW  "\033[93m"
#define ESC_TEXT_BRIGHT_BLUE    "\033[94m"
#define ESC_TEXT_BRIGHT_MAGENTA "\033[95m"
#define ESC_TEXT_BRIGHT_CYAN    "\033[96m"
#define ESC_TEXT_BRIGHT_WHITE   "\033[97m"

// bright background colors
#define ESC_BG_BRIGHT_BLACK     "\033[100m"
#define ESC_BG_BRIGHT_RED       "\033[101m"
#define ESC_BG_BRIGHT_GREEN     "\033[102m"
#define ESC_BG_BRIGHT_YELLOW    "\033[103m"
#define ESC_BG_BRIGHT_BLUE      "\033[104m"
#define ESC_BG_BRIGHT_MAGENTA   "\033[105m"
#define ESC_BG_BRIGHT_CYAN      "\033[106m"
#define ESC_BG_BRIGHT_WHITE     "\033[107m"

// Cursor-control
#define ESC_CURSOR_UP(n)        "\033[" #n "A"
#define ESC_CURSOR_DOWN(n)      "\033[" #n "B"
#define ESC_CURSOR_FORWARD(n)   "\033[" #n "C"
#define ESC_CURSOR_BACKWARD(n)  "\033[" #n "D"

// screen-control
#define ESC_CLEAR_SCREEN        "\033[2J"
#define ESC_CLEAR_LINE          "\033[2K"
#define ESC_RESET_CURSOR        "\033[H"


// default commands
#define USR_CMD_Q    "?"
#define USR_CMD_HELP "help"
#define USR_CMD_USR  "usr"
#define USR_CMD_INFO "info"
#define USR_CMD_APP  "app"

#define USR_CMD_DEFAULT USR_CMD_INFO // default usr command, should be supported by each component and app

#define USR_USAGE ESC_ATTR_BOLD "Usage: " ESC_ATTR_RESET

// prompt formats, used as argument to call printf
#define FMT_PROMPT_ESP  ESC_ATTR_BOLD "\resp:/> " ESC_ATTR_RESET
#define FMT_PROMPT_USER_HOST  ESC_ATTR_BOLD "\r%s@%s:/> " ESC_ATTR_RESET, getUserName(), getHostNameForPrompt()
#define FMT_PROMPT_USER_HOST_OFFLINE  ESC_ATTR_BOLD ESC_TEXT_BRIGHT_RED "\r%s@%s:/> " ESC_ATTR_RESET, getUserName(), getHostNameForPrompt()
#define FMT_PROMPT_USER_HOST_APMODE   ESC_ATTR_BOLD ESC_TEXT_BRIGHT_MAGENTA "\r%s@%s:/> " ESC_ATTR_RESET, getUserName(), getHostNameForPrompt()

#define FMT_PROMPT_DEFAULT FMT_PROMPT_USER_HOST

#define FMT_PROMPT_USER_YN ESC_ATTR_BOLD ESC_TEXT_BRIGHT_WHITE "\r%s [y/n]:\n" ESC_ATTR_RESET


// logging
#ifdef DEBUG_BUILD
#  define _LOG_DEBUG(...) debug(__VA_ARGS__)
#  define _LOG_DEBUG_EXT(...) debug_ext(__VA_ARGS__)
#else
#  define _LOG_DEBUG(...) ((void)0)
#  define _LOG_DEBUG_EXT(...) ((void)0)
#endif

#ifndef debug
#ifdef DEBUG_BUILD
#define _CONSOLE_DEBUG(...) console.debug(__VA_ARGS__)
#else
#define _CONSOLE_DEBUG(...) ((void)0)
#endif
#endif


#define LOGLEVEL_OFF       0
#define LOGLEVEL_ERROR     1
#define LOGLEVEL_WARN      2
#define LOGLEVEL_INFO      3
#define LOGLEVEL_DEBUG     4
#define LOGLEVEL_DEBUG_EXT 5

#define LOGLEVEL_MAX LOGLEVEL_DEBUG_EXT


#endif /* defines_h */
