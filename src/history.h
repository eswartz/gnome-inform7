/* Copyright  2007  P.F. Chimento  <philip.chimento@gmail.com>
 * This file is part of GNOME Inform 7.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
 
#ifndef _HISTORY_H
#define _HISTORY_H

#include "story.h"

#ifndef LEFT
# define LEFT 0
#endif
#ifndef RIGHT
# define RIGHT 1
#endif

void history_push_tab(Story *thestory, int side, int tab);
void history_push_subtab(Story *thestory, int side, int tab, int subtab);
void history_push_docpage(Story *thestory, int side, const gchar *page);
void go_back(Story *thestory, int side);
void go_forward(Story *thestory, int side);
void history_block_handlers(Story *thestory, int side);
void history_unblock_handlers(Story *thestory, int side);
gchar *history_get_last_docpage(Story *thestory, int side);

#endif /* _HISTORY_H */