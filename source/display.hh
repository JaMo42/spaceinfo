#pragma once
#include "stdafx.hh"
#include "space_info.hh"

namespace display
{
void begin ();
void end ();
void refresh ();
void clear ();

void header (const fs::path &path);
void space_info (const SpaceInfo &si, bool show_cursor = true);
void footer (const SpaceInfo &si);

void move_cursor (ssize by, usize end);
void set_cursor (usize to);
fs::path select (const SpaceInfo &from);
}
