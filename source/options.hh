#pragma once
#include "stdafx.hh"

namespace Options
{
extern bool si;
extern bool raw_size;
extern int bar_length;
}

const char *
parse_args (int argc, const char *const *argv);
