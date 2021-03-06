#include "display.hh"
#include "options.hh"
#include "select.hh"
#include <ncurses.h>

constexpr int SELECTION_COLOR = 10;

static std::string S_bar;
static usize S_cursor;

static const SpaceInfo *S_si;
static fs::path S_current_path;

static int S_display_width;
static int S_display_height;
static int S_page_move_amount;

namespace Display
{

static inline void
fill_line (int row)
{
  mvaddstr (row, 0, S_bar.c_str ());
}

static inline void
bar (f64 fullness, int length, char fill='#', char empty=' ')
{
  auto print_n = [](char c, unsigned n) {
    while (n--)
      addch (c);
  };
  int fill_amount = std::lround (fullness * length);
  print_n (fill, fill_amount);
  print_n (empty, length - fill_amount);
}

void
begin ()
{
  setlocale (LC_CTYPE, "");
  initscr ();
  curs_set (0);
  noecho ();
  cbreak ();
  nodelay (stdscr, FALSE);
  start_color ();
  use_default_colors ();

  init_pair (SELECTION_COLOR, COLOR_BLACK, COLOR_YELLOW);

  refresh_size ();
  S_bar.assign (S_display_width, ' ');
}

void
end ()
{
  endwin ();
}

int
width ()
{
  return S_display_width;
}

void
refresh_size ()
{
  getmaxyx (stdscr, S_display_height, S_display_width);
  S_bar.assign (S_display_width, ' ');
  S_page_move_amount = std::max (5, (S_display_height - 3) / 2);
}

int
height ()
{
  return S_display_height;
}

std::pair<int, int>
size ()
{
  return std::make_pair (S_display_width, S_display_height);
}

int
page_move_amount ()
{
  return S_page_move_amount;
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
}

void
set_space_info (const SpaceInfo *si)
{
  S_si = si;
}

void
set_path (const fs::path &path)
{
  S_current_path = path;
}

void
header ()
{
  attron (A_REVERSE);
  fill_line (0);
  mvaddstr (0, 0, "Space info for ");
  attron (A_BOLD);
  if constexpr (std::is_same_v<fs::path::value_type, char>)
    addstr (S_current_path.c_str ());
  else
    addstr (S_current_path.generic_string ().c_str ());
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
  auto get_width = [](const SpaceInfo::value_type &item) {
    return (item.error
            ? std::max (0, static_cast<int> (strlen (item.error)) - (Options::bar_length + 2))
            : print_size<true> (item.size));
  };
  auto compare = [&get_width](const SpaceInfo::value_type &a,
                              const SpaceInfo::value_type &b) {
    return get_width (a) < get_width (b);
  };
  return get_width (*std::max_element (si.begin (), si.end (), compare));
}

static void
print_item (const SpaceInfo &si, usize idx, int row, int size_width,
            bool highlight, bool do_highlight)
{
  const SpaceInfo::value_type &item = si[idx];
  const bool is_selected = do_highlight ? Select::is_selected (idx) : false;
  if (highlight)
    attron (A_REVERSE);
  else if (is_selected)
    attron (COLOR_PAIR (SELECTION_COLOR));
  fill_line (row);
  move (row, 0);
  addch (' ');
  if (item.display_name)
    addstr (item.display_name);
  else
    {
      if (item.error)
        {
          printw ("%-*s", size_width + Options::bar_length + 3, item.error);
        }
      else
        {
          print_size (item.size, size_width);
          addch (' ');
          addch ('[');
          bar (si.size_relative_to_biggest (item), Options::bar_length);
          addch (']');
        }
      addch (' ');
      if constexpr (std::is_same_v<fs::path::value_type, char>)
        addstr (item.path.c_str ());
      else
        addstr (item.path.generic_string ().c_str ());
      if (item.is_directory)
        addch ('/' | (A_DIM * !highlight));
    }
  if (highlight)
    attroff (A_REVERSE);
  else if (is_selected)
    attroff (COLOR_PAIR (SELECTION_COLOR));
}

static void
print_items (const SpaceInfo &si, usize from, usize to, int size_width,
             bool cursor)
{
  usize row = 1;
  for (usize i = from; i <= to; ++i, ++row)
    {
      print_item (si, i, row, size_width, cursor && (i == S_cursor), cursor);
    }
}

void
space_info (bool show_cursor)
{
  const usize rows = static_cast<usize> (S_display_height - 3);
  const usize rows_2 = static_cast<usize> (rows / 2);
  const usize item_count = S_si->item_count ();
  const int size_width = size_column_width (*S_si);
  int first, last;

  if (item_count <= rows)
    {
      first = 0;
      last = item_count;
    }
  else if (S_cursor < rows_2)
    {
      first = 0;
      last = rows;
    }
  else if (S_cursor >= (item_count - rows_2))
    {
      first = item_count - rows;
      last = item_count;
    }
  else
    {
      first = S_cursor - rows_2;
      last = S_cursor + (rows - rows_2);
    }

  print_items (*S_si, first, last, size_width, show_cursor);
}

void
footer ()
{
  constexpr const char help_info[] = "Press ? for help";
  const int row = S_display_height - 1;
  attron (A_REVERSE);
  fill_line (row);
  mvaddstr (row, 0, "Total disk usage: ");
  print_size (S_si->total ());
  printw (", %" PRIu64 " Items, %" PRIu64 " Files total, ",
          S_si->item_count (), S_si->total_file_count ());
  print_size (file_system_free);
  addstr (" Free");
  move (row, S_display_width - sizeof (help_info));
  addstr (help_info);
  attroff (A_REVERSE);
}

void
format_footer (const char *fmt, ...)
{
  va_list ap;
  attron (A_REVERSE);
  const int row = S_display_height - 1;
  fill_line (row);
  move (row, 0);
  va_start (ap, fmt);
  vw_printw (stdscr, fmt, ap);
  va_end (ap);
  attroff (A_REVERSE);
}

void
move_cursor (ssize by)
{
  if (by < 0 && static_cast<usize> (-by) > S_cursor)
    S_cursor = 0;
  else if (S_cursor + by >= S_si->item_count () + 1)
    S_cursor = S_si->item_count ();
  else
    S_cursor += by;
}

void
set_cursor (usize to)
{
  S_cursor = to;
}

fs::path
select (const SpaceInfo &from, const fs::path &current)
{
  const auto &p = from[S_cursor].path;
  if (p.is_absolute ())
    return p;
  return current / p;
}

}
