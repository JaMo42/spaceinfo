#include "stdafx.hh"
#include "options.hh"
#include "space_info.hh"
#include "display.hh"
#include "select.hh"
#include "input.hh"
#include "nc-help/help.h"

static void
show_progress (const SpaceInfo &si)
{
  Display::set_space_info (&si);
  Display::space_info (false);
  Display::footer ();
  Display::refresh ();
}

void
fail ()
{
  std::fputs (G_error.message ().c_str (), stderr);
  std::fputc ('\n', stderr);
  std::exit (1);
}

static bool
help ()
{
  static help_text_type help_text = {
    {"k/↑",         "Move cursor up"},
    {"j/↓",         "Move cursor down"},
    {"K/PgUp",      "Move cursor up multiple items"},
    {"J/PgDn",      "Move cursor down multiple items"},
    {"g/Home",      "Move cursor to the start"},
    {"G/End",       "Move cursor to the bottom"},
    {"Enter/Space", "Enter the directory under the cursor"},
    {"r/i",         "Reverse sorting order"},
    {"'/'",         "Begin search"},
    {"n",           "Select the next search result"},
    {"N",           "Select the previous search result"},
    {"c",           "Clear search"},
    {"h",           "Go to a specific path"},
    {"R",           "Reload the current directory"}
  };
  static nc_help::Help help (help_text);

  help.resize_offset (2, 2);
  help.draw ();
  box (help.window (), 0, 0);
  help.refresh ();

  if (!help.can_scroll ())
    return Input::get_char () == KEY_RESIZE;

  const int page_move_amount = std::max (5, (getmaxy (help.window ()) - 3) / 2);
  int ch;
  bool stop = false;
  bool propagate_resize = false;
  while (!stop)
    {
      switch (ch = Input::get_char ())
        {
          case KEY_UP:
          case 'k':
            help.move_cursor (-1);
            break;
          case KEY_DOWN:
          case 'j':
            help.move_cursor (1);
            break;
          case KEY_PPAGE:
          case 'K':
            help.move_cursor (-page_move_amount);
            break;
          case KEY_NPAGE:
          case 'J':
            help.move_cursor (page_move_amount);
            break;
          case KEY_HOME:
          case 'g':
            help.set_cursor (0);
            break;
          case KEY_END:
          case 'G':
            help.set_cursor (static_cast<unsigned> (-1));
            break;
          default:
            propagate_resize = ch == KEY_RESIZE;
            stop = true;
            break;
        }
      help.draw ();
      box (help.window (), 0, 0);
      help.refresh ();
    }
  return propagate_resize;
}

void
search (const SpaceInfo &si)
{
  static Input::History S_history;
  Display::format_footer ("Search: ");
  auto text = Input::get_line (&S_history, [&](const std::string &text_) {
    Select::select (text_, si);
    Display::space_info ();
    Display::format_footer ("Search: %s", text_.c_str ());
    Display::refresh ();
  });
  if (text.empty ())
    Select::clear_selection ();
  Display::footer ();
}

std::string_view
get_go_to_path ()
{
  static Input::History S_history;
  Display::format_footer ("Go to: ");
  return Input::get_line (&S_history, [&](const std::string &text) {
    Display::format_footer ("Go to: %s", text.c_str ());
    Display::refresh ();
  });
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
            Display::set_cursor (0);
            Select::re_select (*si);
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
      ch = Input::get_char ();
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
            search (*si);
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
            pending_path = get_go_to_path ();
            if (pending_path.empty ())
              {
                Display::footer ();
                break;
              }
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
            if (help ())
              goto do_resize;
            Display::clear ();
            Display::header ();
            Display::footer ();
            break;
          case 'q':
            stop = true;
            break;
          case KEY_RESIZE:
do_resize:
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
