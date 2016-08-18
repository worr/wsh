/* Copyright (c) 2014 William Orr <will@worrbase.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __MOCK_NCURSES_H
#define __MOCK_NCURSES_H

#include <stdbool.h>

#ifdef CURSES_HAVE_NCURSES_H
#include <ncurses.h>
#define HAVE_CURSES
#endif
#ifdef CURSES_HAVE_NCURSES_NCURSES_H
#include <ncurses/ncurses.h>
#define HAVE_CURSES
#endif
#ifdef CURSES_HAVE_NCURSES_CURSES_H
#include <ncurses/curses.h>
#define HAVE_CURSES
#endif
#ifdef CURSES_HAVE_CURSES_H
#include <curses.h>
#define HAVE_CURSES
#endif
#ifdef HAVE_TERM_H
#include <term.h>
#endif

#ifndef ERR
#define ERR -1
#endif

#ifndef NCURSES_CONST
#define NCURSES_CONST
#endif

int setupterm(NCURSES_CONST char* term, int filedes, int* err);
bool has_colors(void);

void set_setupterm_ret(int ret);
void set_has_colors_ret(bool ret);

#endif

