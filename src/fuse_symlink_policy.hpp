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

#pragma once

#include "fuse_symlink_policy_base.hpp"
#include "fuse_symlink_policy_factory.hpp"

#include "toml.hpp"


namespace FUSE::SYMLINK
{
  class Policy
  {
  public:
    Policy(const toml::value &toml_)
    {
      _symlink = POLICY::factory(toml_);
    }

  public:
    int
    operator()(const char      *target_,
               const gfs::path &linkpath_)
    {
      return (*_symlink)(target_,linkpath_);
    }

  private:
    POLICY::Base::Ptr _symlink;
  };
}