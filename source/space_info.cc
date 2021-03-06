#include "space_info.hh"

std::error_code G_error;

u64 file_system_free;

void
SpaceInfo::add_parent (const fs::path &parent)
{
  items_.emplace_back (parent, 0, true, nullptr, "..");
}

void
SpaceInfo::add (const fs::path &full_path, u64 size, u64 file_count,
                bool is_directory, const char *error)
{
  const fs::path path = full_path.filename ();
  file_count_ += file_count;
  insert_sorted (Item { path, size, is_directory, error });
  total_ += size;
  if (size > biggest_)
    biggest_ = size;
}

void
SpaceInfo::sort (bool ascending)
{
  auto comp = [ascending] (const Item &a, const Item &b) {
    return (a.size == b.size
            ? a.path < b.path
            : (a.size > b.size) ^ ascending);
  };
  std::sort (items_.begin () + 1, items_.end (), comp);
}

void
SpaceInfo::insert_sorted (Item &&item)
{
  if (items_.size () == 1)
    {
      items_.push_back (item);
      return;
    }
  items_.insert (
    std::upper_bound (items_.begin () + 1, items_.end (), item,
                      [](const Item &a, const Item &b) {
                        return (a.size == b.size
                                ? a.path < b.path
                                : a.size > b.size);
                      }),
    std::move (item)
  );
}

bool
can_get_size (const fs::file_status &stat)
{
  switch (stat.type ())
    {
      break; case fs::file_type::none: return true;
      break; case fs::file_type::unknown: return true;
      break; case fs::file_type::regular: return true;
      break; case fs::file_type::symlink: return true;
      break; default: return false;
    }
}

template <class IteratorType = fs::directory_iterator, class UnaryFunction>
static bool
safe_directory_iterator (const fs::path &path, UnaryFunction f)
{
  auto dir_it = IteratorType (path, G_error);
  if (G_error)
    return false;
  const auto end = fs::end (dir_it);
  for (auto it = fs::begin (dir_it); it != end; it.increment (G_error))
    {
      if (G_error)
        return false;
      f (*it);
    }
  return true;
}

u64
file_size (const fs::directory_entry &entry)
{
  struct stat sb;
  if (::lstat (entry.path ().c_str (), &sb) == -1)
    {
      // Hacky but this *should* never fail since we already handeled
      // permission errors and checked that we can get the size
      extern void fail ();
      G_error = std::error_code (errno, std::system_category ());
      fail ();
      return 0;
    }
  return sb.st_size;
}

bool
directory_size_and_file_count (const fs::path &path, u64 &size, u64 &count)
{
  size = count = 0;
  return safe_directory_iterator<fs::recursive_directory_iterator> (
    path,
    [&](const fs::directory_entry &entry) {
      if (entry.exists () && can_get_size (entry.status ()))
        {
          size += file_size (entry);
          ++count;
        }
    }
  );
}

SpaceInfo *
process_dir (const fs::path &path, ProcessingCallback callback)
{
  const fs::path dev_path = "/dev";
  u64 size, count;

  if (G_dirs.contains (path))
    {
      file_system_free = fs::space (path).free;
      return &G_dirs[path];
    }

  SpaceInfo *const si
    = &G_dirs.emplace (std::make_pair (path, SpaceInfo {})).first->second;
  si->add_parent (path.parent_path ());

  if (!safe_directory_iterator (
        path,
        [&](const fs::directory_entry &entry) {
          // See comment in the main function for why this is not supported
          if (entry.path () == dev_path)
            si->add (entry.path (), 0, 0, true, "Not supported");
          else if (entry.is_directory ())
            {
              if (!directory_size_and_file_count (entry.path (), size, count))
                // ToDo: get the actual error message
                si->add (entry.path (), 0, 1, true, "Permission denied");
              else
                si->add (entry.path (), size, count, true);
            }
          else if (entry.exists () && can_get_size (entry.status ()))
            si->add (entry.path (), file_size (entry));
          if (callback)
            callback (*si);
        }
      ))
    {
      G_dirs.erase (path);
      return nullptr;
    }
  file_system_free = fs::space (path).free;
  return si;
}
