#ifndef UTIL_H
#define UTIL_H

void eprintf(const char *fmt, ...);

void *emalloc(size_t size);

FILE *efopen(const char *path, const char *mode);

#endif
