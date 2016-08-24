#ifndef __S3_BUCKET_READER__
#define __S3_BUCKET_READER__

#include <string>

#include "reader.h"
#include "s3interface.h"
#include "s3key_reader.h"

using std::string;

// S3BucketReader read multiple files in a bucket.
class S3BucketReader : public Reader {
   public:
    S3BucketReader();
    virtual ~S3BucketReader();

    void open(const ReaderParams &params);
    uint64_t read(char *buf, uint64_t count);
    void close();

    void setS3interface(S3Interface *s3) {
        this->s3interface = s3;
    }

    void setUpstreamReader(Reader *reader) {
        this->upstreamReader = reader;
    }

    void parseURL();
    void parseURL(const string &url) {
        this->url = url;
        parseURL();
    };

    ListBucketResult *getKeyList() {
        return keyList;
    }

    const string &getRegion() {
        return region;
    }
    const string &getBucket() {
        return bucket;
    }
    const string &getPrefix() {
        return prefix;
    }

   protected:
    // Get URL for a S3 object/file.
    string getKeyURL(const string &key);

   private:
    uint64_t segId;   // segment id
    uint64_t segNum;  // total number of segments
    uint64_t chunkSize;
    uint64_t numOfChunks;

    string url;
    string schema;
    string region;
    string bucket;
    string prefix;

    S3Credential cred;
    S3Interface *s3interface;

    // upstreamReader is where we get data from.
    Reader *upstreamReader;
    bool needNewReader;

    ListBucketResult *keyList;  // List of matched keys/files.
    uint64_t keyIndex;          // BucketContent index of keylist->contents.

    void SetSchema();
    void SetRegion();
    void SetBucketAndPrefix();
    BucketContent *getNextKey();
    ReaderParams getReaderParams(BucketContent *key);
};

#endif
