#include <algorithm>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <string.h>

#include "decompress_reader.h"
#include "s3log.h"
#include "s3macros.h"

uint64_t S3_ZIP_CHUNKSIZE = 1024 * 1024 * 2;

DecompressReader::DecompressReader() {
    this->reader = NULL;
    this->in = new char[S3_ZIP_CHUNKSIZE];
    this->out = new char[S3_ZIP_CHUNKSIZE];
    this->outOffset = 0;
}

DecompressReader::~DecompressReader() {
    delete this->in;
    delete this->out;
}

// Used for unit test to adjust buffer size
void DecompressReader::resizeDecompressReaderBuffer(uint64_t size) {
    delete this->in;
    delete this->out;
    this->in = new char[size];
    this->out = new char[size];
    this->outOffset = 0;
    this->zstream.avail_out = size;
}

void DecompressReader::setReader(Reader *reader) {
    this->reader = reader;
}

void DecompressReader::open(const ReaderParams &params) {
    // allocate inflate state for zlib
    zstream.zalloc = Z_NULL;
    zstream.zfree = Z_NULL;
    zstream.opaque = Z_NULL;
    zstream.next_in = Z_NULL;
    zstream.next_out = (Byte *)this->out;

    zstream.avail_in = 0;
    zstream.avail_out = S3_ZIP_CHUNKSIZE;

    this->outOffset = 0;

    // 47 is the number of windows bits, to make sure zlib could recognize and decode gzip stream.
    int ret = inflateInit2(&zstream, 47);
    CHECK_OR_DIE_MSG(ret == Z_OK, "%s", "failed to initialize zlib library");

    this->reader->open(params);
}

uint64_t DecompressReader::read(char *buf, uint64_t bufSize) {
    uint64_t remainingOutLen = this->getDecompressedBytesNum() - this->outOffset;

    // S3DEBUG("has = %" PRIu64 ", offset = %d, chunksize = %u, avail_out = %u, count = %"
    // PRIu64,
    //     remainingOutLen, outOffset, S3_ZIP_CHUNKSIZE, this->zstream.avail_out, bufSize);
    if (remainingOutLen == 0) {
        this->decompress();
        this->outOffset = 0;  // reset cursor for out buffer to read from beginning.
        remainingOutLen = this->getDecompressedBytesNum();
    }

    uint64_t count = std::min(remainingOutLen, bufSize);
    memcpy(buf, this->out + outOffset, count);

    this->outOffset += count;

    return count;
}

// Read compressed data from underlying reader and decompress to this->out buffer.
// If no more data to consume, this->zstream.avail_out == S3_ZIP_CHUNKSIZE;
void DecompressReader::decompress() {
    if (this->zstream.avail_in == 0) {
        this->zstream.avail_out = S3_ZIP_CHUNKSIZE;
        this->zstream.next_out = (Byte *)this->out;

        // reader S3_ZIP_CHUNKSIZE data from underlying reader and put into this->in buffer.
        // read() might happen more than one time when it's EOF, make sure every time read() will
        // return 0.
        uint64_t hasRead = this->reader->read(this->in, S3_ZIP_CHUNKSIZE);

        // EOF, no more data to decompress.
        if (hasRead == 0) {
            S3DEBUG(
                "No more data to decompress: avail_in = %u, avail_out = %u, total_in = %u, "
                "total_out = %u",
                zstream.avail_in, zstream.avail_out, zstream.total_in, zstream.total_out);
            return;
        }

        // Fill this->in as possible as it could, otherwise data in this->in might not be able to be
        // inflated.
        while (hasRead < S3_ZIP_CHUNKSIZE) {
            uint64_t count = this->reader->read(this->in + hasRead, S3_ZIP_CHUNKSIZE - hasRead);

            if (count == 0) {
                break;
            }

            hasRead += count;
        }

        this->zstream.next_in = (Byte *)this->in;
        this->zstream.avail_in = hasRead;
    } else {
        // Still have more data in 'in' buffer to decode.
        this->zstream.avail_out = S3_ZIP_CHUNKSIZE;
        this->zstream.next_out = (Byte *)this->out;
    }

    // S3DEBUG("Before decompress: avail_in = %u, avail_out = %u, total_in = %u, total_out = %u",
    //      zstream.avail_in, zstream.avail_out, zstream.total_in, zstream.total_out);

    int status = inflate(&this->zstream, Z_NO_FLUSH);
    if (status == Z_STREAM_END) {
        S3DEBUG("Compression finished: Z_STREAM_END.");
    } else if (status < 0 || status == Z_NEED_DICT) {
        inflateEnd(&this->zstream);
        CHECK_OR_DIE_MSG(false, "Failed to decompress data: %d", status);
    }

    // S3DEBUG("After decompress: avail_in = %u, avail_out = %u, total_in = %u, total_out = %u",
    //     zstream.avail_in, zstream.avail_out, zstream.total_in, zstream.total_out);

    return;
}

void DecompressReader::close() {
    inflateEnd(&zstream);
    this->reader->close();
}
