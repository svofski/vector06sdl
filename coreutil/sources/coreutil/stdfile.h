#pragma once
#include <string>
#include <memory>
#include <limits>
#include <stdio.h>
#include <stdarg.h>

namespace coreutil {

struct StdioDefaultFileTraits;

template <class Traits>
class BasicStdioFile {
 public:
  typedef typename Traits::size_type size_type;
  typedef typename Traits::offset_type offset_type;

  static constexpr size_t size_max = std::numeric_limits<size_type>::max();

  BasicStdioFile();
  BasicStdioFile(const char *path, const char *mode);
  BasicStdioFile(int fd, const char *mode, bool owned);
  BasicStdioFile(const char *path, int oflag, int omode);

  void close();
  size_type read(void *ptr, size_type size);
  void write(const void *ptr, size_type size);
  int get();
  void put(char c);
  bool getline(std::string &linebuf, size_type linemax = size_max);
  void flush();
  offset_type tell();
  void seek(offset_type offset, int whence);
  void truncate();
  size_type length();

  [[gnu::format(printf, 2, 3)]] unsigned printf(const char *fmt, ...);
  unsigned vprintf(const char *fmt, va_list ap);

  [[gnu::format(scanf, 2, 3)]] unsigned scanf(const char *fmt, ...);
  unsigned vscanf(const char *fmt, va_list ap);

  explicit operator bool() const;

  FILE *get() const;
  FILE *release();
  int fd() const;

  static BasicStdioFile null();
  static BasicStdioFile temporary();
  static BasicStdioFile named_temporary(std::string &path);

  static BasicStdioFile command(const char *command, const char *mode);
  static BasicStdioFile command(const char *argv[], const char *mode);

 private:
  std::unique_ptr<FILE, int(*)(FILE *)> handle_;
};

typedef BasicStdioFile<StdioDefaultFileTraits> StdioFile;

}  // namespace coreutil

#include "stdfile.tcc"
