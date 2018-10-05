#pragma once
#include "stdfile.h"
#include "scope_guard.h"
#include <system_error>
#include <type_traits>
#include <fcntl.h>
#include <unistd.h>

namespace coreutil {

struct StdioDefaultFileTraits {
  typedef size_t size_type;
  typedef std::make_unsigned_t<off_t> offset_type;
  static constexpr FILE *(&open)(const char *, const char *) = ::fopen;
  static constexpr off_t (&tell)(FILE *) = ::ftello;
  static constexpr int (&seek)(FILE *, off_t, int) = ::fseeko;
};

template <class Traits>
BasicStdioFile<Traits>::BasicStdioFile()
    : handle_(nullptr, &::fclose) {
}

template <class Traits>
BasicStdioFile<Traits>::BasicStdioFile(const char *path, const char *mode)
    : handle_(nullptr, &::fclose) {
  FILE *handle = Traits::open(path, mode);
  if (!handle)
    throw std::system_error(errno, std::system_category());
  handle_.reset(handle);
}

template <class Traits>
BasicStdioFile<Traits>::BasicStdioFile(int fd, const char *mode, bool owned)
    : handle_(nullptr, &::fclose) {
  if (owned) {
    fd = ::dup(fd);
    if (fd == -1)
      throw std::system_error(errno, std::system_category());
  }

  scope(exit) { if (owned && fd != -1) ::close(fd); };

  FILE *handle = ::fdopen(fd, mode);
  if (!handle)
    throw std::system_error(errno, std::system_category());
  fd = -1;
  handle_.reset(handle);
}

template <class Traits>
BasicStdioFile<Traits>::BasicStdioFile(const char *path, int oflag, int omode)
    : handle_(nullptr, &::fclose) {
  int fd = ::open(path, oflag, omode);
  if (fd == -1)
    throw std::system_error(errno, std::system_category());

  scope(exit) { if (fd != -1) ::close(fd); };

  char mode[16];
  unsigned modei = 0;
  if (oflag & O_RDONLY) {
    mode[modei++] = 'r';
  } else if (oflag & O_WRONLY) {
    if (oflag & O_APPEND) {
      mode[modei++] = 'a';
    } else {
      mode[modei++] = 'w';
    }
  } else if (oflag & O_RDWR) {
    if (oflag & O_APPEND) {
      mode[modei++] = 'a';
      mode[modei++] = '+';
    } else if (oflag & O_CREAT) {
      mode[modei++] = 'w';
      mode[modei++] = '+';
    } else {
      mode[modei++] = 'r';
      mode[modei++] = '+';
    }
  }
  mode[modei++] = 'b';
  mode[modei++] = '\0';

  FILE *handle = ::fdopen(fd, mode);
  if (!handle)
    throw std::system_error(errno, std::system_category());
  fd = -1;
  handle_.reset(handle);
}

template <class Traits>
void BasicStdioFile<Traits>::close() {
  if (handle_.get_deleter()(handle_.release()) == -1)
    throw std::system_error(errno, std::system_category());
}

template <class Traits>
auto BasicStdioFile<Traits>::read(void *ptr, size_type size) -> size_type {
  size_type count = ::fread(ptr, 1, size, handle_.get());
  if (count != size) {
    int err = errno;
    if (::ferror(handle_.get()))
      throw std::system_error(err, std::system_category());
  }
  return count;
}

template <class Traits>
void BasicStdioFile<Traits>::write(const void *ptr, size_type size) {
  size_type count = ::fwrite(ptr, 1, size, handle_.get());
  if (count != size)
    throw std::system_error(errno, std::system_category());
}

template <class Traits>
int BasicStdioFile<Traits>::get() {
  int ret = ::fgetc(handle_.get());
  if (ret == -1) {
    int err = errno;
    if (::ferror(handle_.get()))
      throw std::system_error(err, std::system_category());
  }
  return ret;
}

template <class Traits>
void BasicStdioFile<Traits>::put(char c) {
  if (::fputc(c, handle_.get()) == -1)
    throw std::system_error(errno, std::system_category());
}

template <class Traits>
bool BasicStdioFile<Traits>::getline(std::string &linebuf, size_type linemax) {
  bool endl = false;
  linebuf.clear();
  while (!endl && linebuf.size() < linemax) {
    int c = get();
    if (c == -1)
      return !linebuf.empty();
    linebuf.push_back(c);
    endl = c == '\n';
  }
  return true;
}

template <class Traits>
void BasicStdioFile<Traits>::flush() {
  if (::fflush(handle_.get()) == -1)
    throw std::system_error(errno, std::system_category());
}

template <class Traits>
auto BasicStdioFile<Traits>::tell() -> offset_type {
  offset_type off = Traits::tell(handle_.get());
  if (off == offset_type(-1))
    throw std::system_error(errno, std::system_category());
  return off;
}

template <class Traits>
void BasicStdioFile<Traits>::seek(offset_type offset, int whence) {
  if (Traits::seek(handle_.get(), offset, whence) == -1)
    throw std::system_error(errno, std::system_category());
}

template <class Traits>
void BasicStdioFile<Traits>::truncate() {
  flush();
  seek(0, SEEK_SET);
  if (::ftruncate(::fileno(handle_.get()), 0) == -1)
    throw std::system_error(errno, std::system_category());
}

template <class Traits>
auto BasicStdioFile<Traits>::length() -> size_type {
  offset_type off = tell();
  seek(0, SEEK_END);
  size_type size = tell();
  seek(off, SEEK_SET);
  return size;
}

template <class Traits>
unsigned BasicStdioFile<Traits>::printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  unsigned n = vprintf(fmt, ap);
  va_end(ap);
  return n;
}

template <class Traits>
unsigned BasicStdioFile<Traits>::vprintf(const char *fmt, va_list ap) {
  errno = 0;
  unsigned n = ::vfprintf(handle_.get(), fmt, ap);
  int err = errno;
  if (err != 0)
    throw std::system_error(err, std::system_category());
  return n;
}

template <class Traits>
unsigned BasicStdioFile<Traits>::scanf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  unsigned n = scanf(fmt, ap);
  va_end(ap);
  return n;
}

template <class Traits>
unsigned BasicStdioFile<Traits>::vscanf(const char *fmt, va_list ap) {
  errno = 0;
  unsigned n = ::vfscanf(handle_.get(), fmt, ap);
  int err = errno;
  if (err != 0)
    throw std::system_error(err, std::system_category());
  return n;
}

template <class Traits>
BasicStdioFile<Traits>::operator bool() const {
  return bool(handle_);
}

template <class Traits>
FILE *BasicStdioFile<Traits>::get() const {
  return handle_.get();
}

template <class Traits>
FILE *BasicStdioFile<Traits>::release() {
  return handle_.release();
}

template <class Traits>
int BasicStdioFile<Traits>::fd() const {
  return ::fileno(handle_.get());
}

template <class Traits>
auto BasicStdioFile<Traits>::null() -> BasicStdioFile {
#if defined(_WIN32)
  const char *path = "nul";
#else
  const char *path = "/dev/null";
#endif
  return BasicStdioFile(path, "wb");
}

template <class Traits>
auto BasicStdioFile<Traits>::temporary() -> BasicStdioFile {
  BasicStdioFile file;
  FILE *handle = ::tmpfile();
  if (!handle)
    throw std::system_error(errno, std::system_category());
  file.handle_.reset(handle);
  return file;
}

template <class Traits>
auto BasicStdioFile<Traits>::named_temporary(std::string &path) -> BasicStdioFile {
  while (true) {
    int fd = -1;
    char *pathbuf = tempnam(nullptr, nullptr);
    if (!pathbuf)
      throw std::system_error(errno, std::system_category());

    scope(exit)  {
      ::free(pathbuf);
      if (fd != -1) ::close(fd);
    };

    fd = ::open(pathbuf, O_CREAT|O_EXCL|O_RDWR, 0600);
    if (fd == -1 && errno != EEXIST)
      throw std::system_error(errno, std::system_category());

    if (fd != -1) {
      BasicStdioFile file(fd, "w+b", true);
      fd = -1;
      path.assign(pathbuf);
      path.c_str();
      return file;
    }
  }
}

template <class Traits>
auto BasicStdioFile<Traits>::command(const char *command, const char *mode) -> BasicStdioFile {
  BasicStdioFile file;
  FILE *handle = ::popen(command, mode);
  if (!handle)
    throw std::system_error(errno, std::system_category());
  file.handle_ = std::unique_ptr<FILE, int(*)(FILE *)>(handle, &::pclose);
  return file;
}

template <class Traits>
auto BasicStdioFile<Traits>::command(const char *argv[], const char *mode) -> BasicStdioFile {
  std::string cmd;
  cmd.reserve(1024);
  for (const char *arg; (arg = *argv); ++argv) {
    if (!cmd.empty())
      cmd.push_back(' ');
#if defined(_WIN32)
    cmd.push_back('"');
    for (char c;; ++arg) {
      unsigned numbackslash = 0;
      while (c == '\\') {
        c = *++arg;
        ++numbackslash;
      }
      if (!c) {
        cmd.append(numbackslash * 2, '\\');
        break;
      }
      if (c == '"')
        cmd.append(numbackslash * 2 + 1, '\\');
      else
        cmd.append(numbackslash, '\\');
      cmd.push_back(c);
    }
    cmd.push_back('"');
#else
    for (char c; (c = *arg); ++arg) {
      bool alphanumeric = (c >= '0' && c <= '9') ||
                          (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
      if (!alphanumeric)
        cmd.push_back('\\');
      cmd.push_back(c);
    }
#endif
  }
  return command(cmd.c_str(), mode);
}

}  // namespace coreutil
