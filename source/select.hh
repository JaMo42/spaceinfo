#pragma once
#include "stdafx.hh"
#include "space_info.hh"

namespace Select
{
void clear_selection ();
void select (std::string_view word, const SpaceInfo &si);
void re_select (const SpaceInfo &si);
void next ();
void prev ();
bool is_selected (usize idx);
}
