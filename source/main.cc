#include "stdafx.hh"
#include "options.hh"
#include "space_info.hh"
#include "display.hh"
#include "select.hh"

static void
show_progress (const SpaceInfo &si)
{
  Display::set_space_info (&si);
  Display::space_info (false);
  Display::footer ();
  Display::refresh ();
}

static void
fail ()
{
  std::fputs (G_error.message ().c_str (), stderr);
  std::fputc ('\n', stderr);
  std::exit (1);
}

static int
my_getch ()
{
  int ch = getch ();
  if (ch == 27)
    {
      (void)getch ();
      switch (ch = getch ())
        {
          case 'A':  // ^[[A
            return KEY_UP;
          case 'B':  // ^[[B
            return KEY_DOWN;
          case 'H':  // ^[[H
            return KEY_HOME;
          case 'F':  // ^[[F
            return KEY_END;
          case '5':  // ^[[5~
            (void)getch ();
            return KEY_PPAGE;
          case '6':  // ^[[6~
            (void)getch ();
            return KEY_NPAGE;
          default:
            return 0;
        }
    }
  return ch;
}

static int
help ()
{
  static constexpr std::array text = {
    "k/↑     Move cursor up"sv,
    "j/↓     Move cursor down"sv,
    "K/PgUp  Move cursor up multiple items"sv,
    "J/PgDn  Move cursor down multiple items"sv,
    "g/Home  Move cursor to the start"sv,
    "G/End   Move cursor to the bottom"sv,
    "Enter/Space"sv,
    "        Enter the directory under the cursor"sv,
    "r/i     Reverse sorting order"sv,
    "'/'     Begin search"sv,
    "n       Select the next search result"sv,
    "N       Select the previous search result"sv,
    "c       Clear search"sv,
    "h       Go to a specific path"sv,
    "R       Reload the current directory"sv
  };
  static constexpr usize text_width = []() consteval -> usize {
    return std::max_element (
      std::begin (text), std::end (text),
      [](const std::string_view &a, const std::string_view &b) {
        return a.size () < b.size ();
      }
    )->size ();
  } ();
  const auto [width, height] = Display::size ();
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
      ch = my_getch ();
      delwin (win);
      return ch;
    }

  const int max_pos = text.size () - content_height;
  const int page_move_amount = std::max (5, (content_height - 1) / 2);
  bool stop = false;
  int pos = 0;
  while (!stop)
    {
      switch (ch = my_getch ())
        {
          case KEY_UP:
          case 'k':
            if (pos > 0)
              --pos;
            break;
          case KEY_DOWN:
          case 'j':
            if (pos < max_pos)
              ++pos;
            break;
          case KEY_PPAGE:
          case 'K':
            if (pos < page_move_amount)
              pos = 0;
            else
              pos -= page_move_amount;
            break;
          case KEY_NPAGE:
          case 'J':
            if (pos + page_move_amount > max_pos)
              pos = max_pos;
            else
              pos += page_move_amount;
            break;
          case KEY_HOME:
          case 'g':
            pos = 0;
            break;
          case KEY_END:
          case 'G':
            pos = max_pos;
            break;
          case 0:  // unhandeled escape in my_getch
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
  return ch;
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
        Display::set_path (path);
        Display::header ();
        si = process_dir (path, show_progress);
        if (si == nullptr)
          {
            path.swap (pending_path);
            Display::set_path (path);
            Display::header ();
            si = process_dir (path);
            Display::set_space_info (si);
          }
        else
          {
            // Display::set_space_info (si) already happened in show_progress
            Display::set_cursor (0);
            Select::select (search, *si);
          }
        Display::set_space_info (si);
        Display::footer ();
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
  Display::set_path (path);
  Display::header ();
  si = process_dir (path, show_progress);
  if (si == nullptr)
    {
      Display::end ();
      fail ();
    }
  Display::space_info ();
  Display::footer ();
  Display::refresh ();

  int ch;
  bool stop = false;
  while (!stop)
    {
      ch = my_getch ();
main_loop_repeat:
      switch (ch)
        {
          case KEY_UP:
          case 'k':
            Display::move_cursor (-1);
            break;
          case KEY_DOWN:
          case 'j':
            Display::move_cursor (1);
            break;
          case KEY_HOME:
          case 'g':
            Display::set_cursor (0);
            break;
          case KEY_END:
          case 'G':
            Display::set_cursor (si->item_count ());
            break;
          case KEY_PPAGE:
          case 'K':
            Display::move_cursor (-Display::page_move_amount ());
            break;
          case KEY_NPAGE:
          case 'J':
            Display::move_cursor (Display::page_move_amount ());
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
            Display::footer ();
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
              Display::footer ();
            break;
          case 'R':
            G_dirs.erase (path);
            Display::clear ();
            Display::header ();
            si = process_dir (path, show_progress);
            Display::space_info ();
            Display::footer ();
            sort_ascending = false;
            break;
          case '?':
            ch = help ();
            Display::clear ();
            Display::header ();
            Display::space_info ();
            Display::footer ();
            Display::refresh ();
            goto main_loop_repeat;
          case 'q':
            stop = true;
            break;
          case KEY_RESIZE:
            Display::refresh_size ();
            Display::clear ();
            Display::header ();
            Display::footer ();
            break;
        }
      Display::space_info ();
      Display::refresh ();
    }
  Display::end ();
}
