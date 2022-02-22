#include "select.hh"

static std::vector<usize> S_selection;
static usize S_selection_current;
static constexpr usize S_selection_npos = static_cast<usize> (-1);

namespace Select
{
void
clear_selection ()
{
  S_selection.clear ();
  S_selection_current = S_selection_npos;
}

void
select (std::string_view word, const SpaceInfo &si)
{
  clear_selection ();
  if (word.empty ())
    return;
  usize i = 0;
  for (const auto &item : si)
    {
      if (item.path.native ().starts_with (word))
        S_selection.push_back (i);
      ++i;
    }
}

void
next ()
{
  if (S_selection.empty ())
    return;
  if (S_selection_current == S_selection_npos)
    S_selection_current = 0;
  else
    S_selection_current = (S_selection_current + 1) % S_selection.size ();
  Display::set_cursor (S_selection[S_selection_current]);
}

void
prev ()
{
  if (S_selection.empty ())
    return;
  if (S_selection_current == S_selection_npos || S_selection_current == 0)
    S_selection_current = S_selection.size () - 1;
  else
    --S_selection_current;
  Display::set_cursor (S_selection[S_selection_current]);
}

bool
is_selected (usize idx)
{
  if (S_selection.empty ())
    return false;
  return std::binary_search (S_selection.begin (),
                             S_selection.end (),
                             idx);
}

}
