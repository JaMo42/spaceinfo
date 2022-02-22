#include "display.hh"
#include "options.hh"
#include <ncurses.h>

static std::string S_bar;
static usize S_cursor;

namespace display
{

static inline void
fill_line (int row)
{
  mvaddstr (row, 0, S_bar.c_str ());
}

static inline void
bar (f64 fullness, usize length, char fill='#', char empty=' ')
{
  auto print_n = [](char c, unsigned n) {
    while (n--)
      addch (c);
  };
  usize fill_amount = static_cast<usize> (std::round (fullness * length));
  print_n (fill, fill_amount);
  print_n (empty, length - fill_amount);
}

void
begin ()
{
  initscr ();
  curs_set (0);
  noecho ();
  cbreak ();
  nodelay (stdscr, FALSE);

  S_bar.assign (getmaxx (stdscr), ' ');
}

void
end ()
{
  endwin ();
}

void
refresh ()
{
  ::refresh ();
}

void
clear ()
{
  ::clear ();
  ::refresh ();
}

void
header (const fs::path &path)
{
  attron (A_REVERSE);
  fill_line (0);
  mvaddstr (0, 0, "Space info for ");
  attron (A_BOLD);
  if constexpr (std::is_same_v<fs::path::value_type, char>)
    addstr (path.c_str ());
  else
    addstr (path.generic_string ().c_str ());
  attroff (A_BOLD);
  attroff (A_REVERSE);
}

constexpr int
number_length (u64 n)
{
    if (n < 10ULL) return 1;
    if (n < 100ULL) return 2;
    if (n < 1000ULL) return 3;
    if (n < 10000ULL) return 4;
    if (n < 100000ULL) return 5;
    if (n < 1000000ULL) return 6;
    if (n < 10000000ULL) return 7;
    if (n < 100000000ULL) return 8;
    if (n < 1000000000ULL) return 9;
    if (n < 10000000000ULL) return 10;
    if (n < 100000000000ULL) return 11;
    if (n < 1000000000000ULL) return 12;
    if (n < 10000000000000ULL) return 13;
    if (n < 100000000000000ULL) return 14;
    if (n < 1000000000000000ULL) return 15;
    if (n < 10000000000000000ULL) return 16;
    if (n < 100000000000000000ULL) return 17;
    if (n < 1000000000000000000ULL) return 18;
    if (n < 10000000000000000000ULL) return 19;
    return 20;
}

template <bool GetLength = false>
static int
print_size (u64 size, int width = 0)
{
  if (Options::raw_size)
    {
      if constexpr (GetLength)
        return number_length (size);
      else
        printw ("%*" PRIu64, width, size);
    }
  else
    {
      constexpr std::string_view si_letters = "kMGTPEZY"sv;
      constexpr std::string_view letters = "KMGTPEZY"sv;
      const double scale = Options::si ? 1000.0 : 1024.0;
      double size_scaled = size;
      usize base = 0;
      while (size_scaled >= scale)
        {
          size_scaled /= scale;
          ++base;
        }
      const bool has_decimal = ((size_scaled - static_cast<usize> (size_scaled))
                                > std::numeric_limits<double>::epsilon ());
      if constexpr (GetLength)
        {
          return (number_length (static_cast<u64> (size_scaled))
                  + 2 * has_decimal
                  + (base > 0));
        }
      else
        {
          width -= base > 0;
          if (has_decimal)
            printw ("%*.1f", width, size_scaled);
          else
            printw ("%*" PRIu64, width, static_cast<u64> (size_scaled));
          if (base > 0)
            addch ((Options::si ? si_letters : letters)[base - 1]);
        }
    }
  return 0;
}

static int
size_column_width (const SpaceInfo &si)
{
  auto compare = [](const SpaceInfo::value_type &a, const SpaceInfo::value_type &b) {
    return print_size<true> (a.size) < print_size<true> (b.size);
  };
  return print_size<true> (std::max_element (si.begin (), si.end (), compare)->size);
}

static void
print_item (const SpaceInfo &si, usize idx, int row, int size_width,
            bool highlight)
{
  const SpaceInfo::value_type &item = si[idx];
  if (highlight)
    attron (A_REVERSE);
  fill_line (row);
  move (row, 0);
  addch (' ');
  print_size (item.size, size_width);
  addch (' ');
  addch ('[');
  bar (si.size_relative_to_biggest (item), 10);
  addch (']');
  addch (' ');
  if constexpr (std::is_same_v<fs::path::value_type, char>)
    addstr (item.path.filename ().c_str ());
  else
    addstr (item.path.filename ().generic_string ().c_str ());
  if (highlight)
    attroff (A_REVERSE);
}

static void
print_items (const SpaceInfo &si, usize from, usize to, int size_width,
             bool cursor)
{
  usize row = 2;
  if (cursor && S_cursor == 0)
    attron (A_REVERSE);
  fill_line (1);
  mvaddstr (1, 0, "..");
  if (cursor && S_cursor == 0)
    attroff (A_REVERSE);
  for (usize i = from; i <= to; ++i, ++row)
    {
      print_item (si, i, row, size_width, cursor && ((i + 1) == S_cursor));
    }
}

void
space_info (const SpaceInfo &si, bool show_cursor)
{
  const usize rows = static_cast<usize> (getmaxy (stdscr) - 3);
  const usize rows_2 = static_cast<usize> (rows / 2);
  const usize item_count = si.item_count ();
  const int size_width = size_column_width (si);
  int first, last;

  if (item_count <= rows)
    {
      first = 0;
      last = item_count - 1;
    }
  else if (S_cursor < rows_2)
    {
      first = 0;
      last = rows;
    }
  else if (S_cursor > (item_count - rows_2))
    {
      first = item_count - rows;
      last = item_count;
    }
  else
    {
      first = S_cursor - rows_2;
      last = S_cursor - (rows - rows_2);
    }

  print_items (si, first, last, size_width, show_cursor);
}

void
footer (const SpaceInfo &si)
{
  const int row = getmaxy (stdscr) - 1;
  attron (A_REVERSE);
  fill_line (row);
  mvaddstr (row, 0, "Total disk usage: ");
  print_size (si.total ());
  printw (", %" PRIu64 " Items", si.item_count ());
  attroff (A_REVERSE);
}

void
move_cursor (ssize by, usize end)
{
  if (S_cursor == 0 && by < 0)
    return;
  // would be `>= end` but we allow one more since the `..` is not part of the
  // space info
  if (S_cursor + by > end)
    return;
  S_cursor += by;
}

void
set_cursor (usize to)
{
  S_cursor = to + 1;
}

fs::path
select (const SpaceInfo &from, const fs::path &current)
{
  const auto root = fs::current_path().root_path();
  if (S_cursor == 0)
    {
      if (current == root)
        return root;
      return current.parent_path ();
    }
  else
    return from[S_cursor - 1].path;
}

}
