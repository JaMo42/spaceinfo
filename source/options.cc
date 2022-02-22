#include "options.hh"
#include "flag-cpp/flag.hh"

namespace Options
{
bool si = false;
bool raw_size = false;
int bar_length = 10;
}

const char *
parse_args (int argc, const char *const *argv)
{
  flag::add (Options::si, "si",
             "Use powers of 1000 not 1024 for human readable sizes.");
  flag::add (Options::raw_size, "r", "Do not print human readable sizes.");
  flag::add (Options::bar_length, "bar-length", "Length for the relative size bar.");

  flag::add_help ();

  std::vector<const char *> args = flag::parse (argc, argv);
  return args.empty () ? nullptr : args.front ();
}
