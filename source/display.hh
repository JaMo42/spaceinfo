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

void refresh ();
void clear ();

void set_space_info (const SpaceInfo *);
void set_path (const fs::path &path);

void header ();
void space_info (bool show_cursor = true);
void footer ();

void move_cursor (ssize by, usize end);
void set_cursor (usize to);
fs::path select (const SpaceInfo &from, const fs::path &current);

std::string_view input (std::string_view purpose);
}
