/*
  ISC License

  Copyright (c) 2022, Antonio SJ Musumeci <trapexit@spawn.link>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "fuse_symlink_policy_ff.hpp"

#include "fs_clonepath.hpp"
#include "fs_clonepath_branches.hpp"
#include "fs_symlink.hpp"
#include "ugid.hpp"


FUSE::SYMLINK::POLICY::FF::FF(const toml::value &toml_)
  : _branches(toml_)
{

}

int
FUSE::SYMLINK::POLICY::FF::operator()(const char      *target_,
                                      const gfs::path &linkpath_)
{
  int rv;
  gfs::path fullpath;

  for(const auto &branch_group : _branches)
    {
      for(const auto &branch : branch_group)
        {
          fullpath  = branch.path / linkpath_;

          rv = fs::symlink(target_,fullpath);
          if(rv == -ENOENT)
            {
              rv = fs::clonepath_as_root(_branches,branch.path,linkpath_);
              if(rv >= 0)
                rv = fs::symlink(target_,fullpath);
            }

          return rv;
        }
    }

  return -ENOENT;
}