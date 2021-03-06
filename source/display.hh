#pragma once
#include "stdafx.hh"
#include "space_info.hh"

namespace Display
{
void begin ();
void end ();

void refresh_size ();
int width ();
int height ();
std::pair<int/*width*/, int/*height*/> size ();
int page_move_amount ();

void refresh ();
void clear ();

void set_space_info (const SpaceInfo *);
void set_path (const fs::path &path);

void header ();
void space_info (bool show_cursor = true);
void footer ();
void format_footer (const char *fmt, ...);

void move_cursor (ssize by);
void set_cursor (usize to);
fs::path select (const SpaceInfo &from, const fs::path &current);
}
