/*
  ISC License

  Copyright (c) 2023, Antonio SJ Musumeci <trapexit@spawn.link>

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

#include <stdarg.h>
#include <syslog.h>
#include <stdbool.h>


static bool g_SYSLOG_ENABLED = false;

void
syslog_open()
{
  const char *ident = "mergerfs";
  const int   option = (LOG_CONS|LOG_PID);
  const int   facility = LOG_USER;

  openlog(ident,option,facility);
  g_SYSLOG_ENABLED = true;
}

void
syslog_close()
{
  closelog();
  g_SYSLOG_ENABLED = false;
}

static
void
syslog_vlog(const int   priority_,
            const char *format_,
            va_list     valist_)
{
  if(g_SYSLOG_ENABLED == false)
    return;

  vsyslog(priority_,format_,valist_);
}

void
syslog_log(const int   priority_,
           const char *format_,
           ...)
{
  va_list valist;

  va_start(valist,format_);
  syslog_vlog(priority_,format_,valist);
  va_end(valist);
}

void
syslog_info(const char *format_,
            ...)
{
  va_list valist;

  va_start(valist,format_);
  syslog_vlog(LOG_INFO,format_,valist);
  va_end(valist);
}

void
syslog_notice(const char *format_,
              ...)
{
  va_list valist;

  va_start(valist,format_);
  syslog_vlog(LOG_NOTICE,format_,valist);
  va_end(valist);
}

void
syslog_warning(const char *format_,
               ...)
{
  va_list valist;

  va_start(valist,format_);
  syslog_vlog(LOG_WARNING,format_,valist);
  va_end(valist);
}

void
syslog_error(const char *format_,
             ...)
{
  va_list valist;

  va_start(valist,format_);
  syslog_vlog(LOG_ERR,format_,valist);
  va_end(valist);
}
