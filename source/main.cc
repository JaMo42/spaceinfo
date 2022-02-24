#include "stdafx.hh"
#include "options.hh"
#include "space_info.hh"
#include "display.hh"
#include "select.hh"

static void
show_progress (const SpaceInfo &si)
{
  Display::space_info (si, false);
  Display::footer (si);
  Display::refresh ();
}

static void
fail ()
{
  std::fputs (G_error.message ().c_str (), stderr);
  std::fputc ('\n', stderr);
  std::exit (1);
}

static bool
help ()
{
  static constexpr std::array text = {
    "k/↑  Move cursor up"sv,
    "j/↓  Move cursor down"sv,
    "g    Move cursor to the start"sv,
    "G    Move cursor to the bottom"sv,
    "Enter/Space"sv,
    "     Enter the directory under the cursor"sv,
    "r/i  Reverse sroting order"sv,
    "'/'  Begin search"sv,
    "n    Select the next search result"sv,
    "N    Select the previous search result"sv,
    "c    Clear search"sv,
    "h    Go to a specific path"sv,
    "R    Reload the current directory"sv
  };
  static constexpr usize text_width = []() consteval -> usize {
    return std::max_element (
      std::begin (text), std::end (text),
      [](const std::string_view &a, const std::string_view &b) {
        return a.size () < b.size ();
      }
    )->size ();
  } ();
  const int width = getmaxx (stdscr);
  const int height = getmaxy (stdscr);
  const int content_height = std::min (static_cast<int> (text.size ()),
                                       height - 6);
  const int x = (width - text_width) / 2;
  const int y = (height - content_height - 2) / 2;
  WINDOW *win = newwin (content_height + 2, text_width + 2, y, x);

  auto put_lines = [&win](usize begin, usize end) {
    for (int line = 1; begin != end; ++begin, ++line)
    mvwaddstr (win, line, 1, text[begin].data ());
  };

  box (win, 0, 0);
  put_lines (0, content_height);
  wrefresh (win);

  int ch;
  if (static_cast<usize> (content_height) == text.size ())
    {
      ch = getch ();
      delwin (win);
      return ch == 'q';
    }

  const int max_pos = text.size () - content_height;
  bool stop = false;
  int pos = 0;
  while (!stop)
    {
      ch = getch ();
      switch (ch)
        {
          case KEY_UP:
          case 'k':
help_key_up:
            if (pos > 0)
              --pos;
            break;
          case KEY_DOWN:
          case 'j':
help_key_down:
            if (pos < max_pos)
              ++pos;
            break;
          case 'q':
            stop = true;
            break;
          case 27:
            (void)getch ();
            ch = getch ();
            if (ch == 'A')
              goto help_key_up;
            else if (ch == 'B')
              goto help_key_down;
            [[fallthrough]];
          default:
            stop = true;
            break;
        }
      wclear (win);
      box (win, 0, 0);
      put_lines (pos, pos + content_height);
      wrefresh (win);
    }
  delwin (win);
  return ch == 'q';
}

int
main (const int argc, const char **argv)
{
  const fs::path dev_path = "/dev";
  SpaceInfo *si;
  const char *const arg = parse_args (argc, argv);
  fs::path path = (arg
                   ? fs::canonical (fs::path (arg))
                   : fs::current_path ());
  fs::path pending_path;
  bool sort_ascending = false;
  std::string_view search = ""sv;

  auto maybe_goto_pending = [&]() {
    if (fs::is_directory (pending_path) && pending_path != dev_path)
      {
        path.swap (pending_path);
        Display::clear ();
        Display::header (path);
        si = process_dir (path, show_progress);
        if (si == nullptr)
          {
            path.swap (pending_path);
            Display::header (path);
            si = process_dir (path);
          }
        else
          {
            Display::set_cursor (0);
            Select::select (search, *si);
          }
        Display::footer (*si);
        si->sort (sort_ascending = false);
      }
  };

  // The program gets stuck when entering the /dev/ directory so we do not
  // permit entering it. It also reports a bogus size when showing the root
  // directory so we display 'Not supported' instead of the size.
  if (path == dev_path)
    {
      std::fputs ("The /dev directory is not supported.\n", stderr);
      return 1;
    }

  Select::clear_selection ();

  Display::begin ();
  Display::header (path);
  si = process_dir (path, show_progress);
  if (si == nullptr)
    {
      Display::end ();
      fail ();
    }
  Display::space_info (*si);
  Display::footer (*si);
  Display::refresh ();

  int ch;
  while ((ch = getch ()) != 'q')
    {
      switch (ch)
        {
          case KEY_UP:
          case 'k':
key_up:
            Display::move_cursor (-1, si->item_count () + 1);
            break;
          case KEY_DOWN:
          case 'j':
key_down:
            Display::move_cursor (1, si->item_count () + 1);
            break;
          case 27:
            (void)getch ();
            ch = getch ();
            if (ch == 'A')
              goto key_up;
            else if (ch == 'B')
              goto key_down;
            break;
          case 'g':
            Display::set_cursor (0);
            break;
          case 'G':
            Display::set_cursor (si->item_count ());
            break;
          case 10: // Enter
          case ' ':
            pending_path = Display::select (*si, path);
            maybe_goto_pending ();
            break;
          case 'r':
          case 'i':
            sort_ascending = !sort_ascending;
            si->sort (sort_ascending);
            break;
          case '/':
            search = Display::input ("Search");
            Display::footer (*si);
            Select::select (search, *si);
            break;
          case 'n':
            Select::next ();
            break;
          case 'N':
            Select::prev ();
            break;
          case 'c':
            Select::clear_selection ();
            break;
          case 'h':
            pending_path = Display::input ("Go to");
            if (!pending_path.is_absolute ())
              pending_path = fs::canonical (path / pending_path, G_error);
            if (!G_error)
              maybe_goto_pending ();
            else
              Display::footer (*si);
            break;
          case 'R':
            G_dirs.erase (path);
            Display::clear ();
            Display::header (path);
            si = process_dir (path, show_progress);
            Display::space_info (*si);
            Display::footer (*si);
            sort_ascending = false;
            break;
          case '?':
            if (help ())
              goto break_main_loop;
            Display::clear ();
            Display::header (path);
            Display::footer (*si);
            break;
        }
      Display::space_info (*si);
      Display::refresh ();
    }
break_main_loop:
  Display::end ();
}
