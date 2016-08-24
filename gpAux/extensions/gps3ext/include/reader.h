#ifndef __GP_EXT_READER_H__
#define __GP_EXT_READER_H__

#include "reader_params.h"

class Reader {
   public:
    virtual ~Reader() {
    }

    virtual void open(const ReaderParams &params) = 0;

    // read() attempts to read up to count bytes into the buffer.
    // Always return 0 if EOF, no matter how many times it's invoked. Throw exception if encounters
    // errors.
    virtual uint64_t read(char *buf, uint64_t count) = 0;

    // This should be reentrant, has no side effects when called multiple times.
    virtual void close() = 0;
};

#endif
