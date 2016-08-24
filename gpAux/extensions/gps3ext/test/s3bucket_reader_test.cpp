#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mock_classes.h"
#include "reader_params.h"
#include "s3bucket_reader.cpp"

using ::testing::AtLeast;
using ::testing::Return;
using ::testing::Throw;
using ::testing::_;

class MockS3Reader : public Reader {
   public:
    MOCK_METHOD1(open, void(const ReaderParams& params));
    MOCK_METHOD2(read, uint64_t(char*, uint64_t));
    MOCK_METHOD0(close, void());
};

// ================== S3BucketReaderTest ===================

class S3BucketReaderTest : public testing::Test {
   protected:
    // Remember that SetUp() is run immediately before a test starts.
    virtual void SetUp() {
        bucketReader = new S3BucketReader();
        bucketReader->setS3interface(&s3interface);
    }

    // TearDown() is invoked immediately after a test finishes.
    virtual void TearDown() {
        bucketReader->close();
        delete bucketReader;
    }

    S3BucketReader* bucketReader;
    ReaderParams params;
    char buf[64];

    MockS3Interface s3interface;
    MockS3Reader s3reader;
};

TEST_F(S3BucketReaderTest, OpenInvalidURL) {
    string url = "https://s3-us-east-2.amazon.com/s3test.pivotal.io/whatever";
    params.setUrlToLoad(url);
    EXPECT_THROW(bucketReader->open(params), std::runtime_error);
}

TEST_F(S3BucketReaderTest, OpenURL) {
    ListBucketResult* result = new ListBucketResult();

    EXPECT_CALL(s3interface, listBucket(_, _, _, _, _)).Times(1).WillOnce(Return(result));

    string url = "https://s3-us-east-2.amazonaws.com/s3test.pivotal.io/whatever";
    params.setUrlToLoad(url);

    EXPECT_NO_THROW(bucketReader->open(params));
}

TEST_F(S3BucketReaderTest, OpenThrowExceptionWhenS3InterfaceIsNULL) {
    bucketReader->setS3interface(NULL);

    string url = "https://s3-us-east-2.amazonaws.com/s3test.pivotal.io/whatever";
    params.setUrlToLoad(url);
    EXPECT_THROW(bucketReader->open(params), std::runtime_error);
}

TEST_F(S3BucketReaderTest, ParseURL_normal) {
    EXPECT_NO_THROW(this->bucketReader->parseURL(
        "s3://s3-us-west-2.amazonaws.com/s3test.pivotal.io/dataset1/normal"));
    EXPECT_EQ("us-west-2", this->bucketReader->getRegion());
    EXPECT_EQ("s3test.pivotal.io", this->bucketReader->getBucket());
    EXPECT_EQ("dataset1/normal", this->bucketReader->getPrefix());
}

TEST_F(S3BucketReaderTest, ParseURL_NoPrefixAndSlash) {
    EXPECT_NO_THROW(
        this->bucketReader->parseURL("s3://s3-us-west-2.amazonaws.com/s3test.pivotal.io"));
    EXPECT_EQ("s3test.pivotal.io", this->bucketReader->getBucket());
    EXPECT_EQ("", this->bucketReader->getPrefix());
}

TEST_F(S3BucketReaderTest, ParseURL_NoPrefix) {
    EXPECT_NO_THROW(
        this->bucketReader->parseURL("s3://s3-us-west-2.amazonaws.com/s3test.pivotal.io/"));
    EXPECT_EQ("s3test.pivotal.io", this->bucketReader->getBucket());
    EXPECT_EQ("", this->bucketReader->getPrefix());
}

TEST_F(S3BucketReaderTest, ParseURL_default) {
    EXPECT_NO_THROW(
        this->bucketReader->parseURL("s3://s3.amazonaws.com/s3test.pivotal.io/dataset1/normal"));
    EXPECT_EQ("external-1", this->bucketReader->getRegion());
    EXPECT_EQ("s3test.pivotal.io", this->bucketReader->getBucket());
    EXPECT_EQ("dataset1/normal", this->bucketReader->getPrefix());
}

TEST_F(S3BucketReaderTest, ParseURL_useast1) {
    EXPECT_NO_THROW(this->bucketReader->parseURL(
        "s3://s3-us-east-1.amazonaws.com/s3test.pivotal.io/dataset1/normal"));
    EXPECT_EQ("external-1", this->bucketReader->getRegion());
    EXPECT_EQ("s3test.pivotal.io", this->bucketReader->getBucket());
    EXPECT_EQ("dataset1/normal", this->bucketReader->getPrefix());
}

TEST_F(S3BucketReaderTest, ParseURL_eucentral1) {
    EXPECT_NO_THROW(this->bucketReader->parseURL(
        "s3://s3.eu-central-1.amazonaws.com/s3test.pivotal.io/dataset1/normal"));
    EXPECT_EQ("eu-central-1", this->bucketReader->getRegion());
    EXPECT_EQ("s3test.pivotal.io", this->bucketReader->getBucket());
    EXPECT_EQ("dataset1/normal", this->bucketReader->getPrefix());
}

TEST_F(S3BucketReaderTest, ParseURL_eucentral11) {
    EXPECT_NO_THROW(this->bucketReader->parseURL(
        "s3://s3-eu-central-1.amazonaws.com/s3test.pivotal.io/dataset1/normal"));
    EXPECT_EQ("eu-central-1", this->bucketReader->getRegion());
    EXPECT_EQ("s3test.pivotal.io", this->bucketReader->getBucket());
    EXPECT_EQ("dataset1/normal", this->bucketReader->getPrefix());
}

TEST_F(S3BucketReaderTest, ParseURL_apnortheast2) {
    EXPECT_NO_THROW(this->bucketReader->parseURL(
        "s3://s3.ap-northeast-2.amazonaws.com/s3test.pivotal.io/dataset1/normal"));
    EXPECT_EQ("ap-northeast-2", this->bucketReader->getRegion());
    EXPECT_EQ("s3test.pivotal.io", this->bucketReader->getBucket());
    EXPECT_EQ("dataset1/normal", this->bucketReader->getPrefix());
}

TEST_F(S3BucketReaderTest, ParseURL_apnortheast21) {
    EXPECT_NO_THROW(this->bucketReader->parseURL(
        "s3://s3-ap-northeast-2.amazonaws.com/s3test.pivotal.io/dataset1/normal"));
    EXPECT_EQ("ap-northeast-2", this->bucketReader->getRegion());
    EXPECT_EQ("s3test.pivotal.io", this->bucketReader->getBucket());
    EXPECT_EQ("dataset1/normal", this->bucketReader->getPrefix());
}

TEST_F(S3BucketReaderTest, ReaderThrowExceptionWhenUpstreamReaderIsNULL) {
    EXPECT_THROW(bucketReader->read(buf, sizeof(buf)), std::runtime_error);
}

TEST_F(S3BucketReaderTest, ReaderReturnZeroForEmptyBucket) {
    ListBucketResult* result = new ListBucketResult();
    EXPECT_CALL(s3interface, listBucket(_, _, _, _, _)).Times(1).WillOnce(Return(result));

    params.setUrlToLoad("https://s3-us-east-2.amazonaws.com/s3test.pivotal.io/whatever");
    bucketReader->open(params);
    bucketReader->setUpstreamReader(&s3reader);
    EXPECT_EQ(0, bucketReader->read(buf, sizeof(buf)));
}

TEST_F(S3BucketReaderTest, ReadBucketWithSingleFile) {
    ListBucketResult* result = new ListBucketResult();
    BucketContent* item = new BucketContent("foo", 456);
    result->contents.push_back(item);

    EXPECT_CALL(s3interface, listBucket(_, _, _, _, _)).Times(1).WillOnce(Return(result));

    EXPECT_CALL(s3reader, read(_, _))
        .Times(3)
        .WillOnce(Return(256))
        .WillOnce(Return(200))
        .WillOnce(Return(0));

    EXPECT_CALL(s3reader, open(_)).Times(1);
    EXPECT_CALL(s3reader, close()).Times(1);

    params.setSegId(0);
    params.setSegNum(1);
    params.setUrlToLoad("https://s3-us-east-2.amazonaws.com/s3test.pivotal.io/whatever");
    bucketReader->open(params);
    bucketReader->setUpstreamReader(&s3reader);

    EXPECT_EQ(256, bucketReader->read(buf, sizeof(buf)));
    EXPECT_EQ(200, bucketReader->read(buf, sizeof(buf)));
    EXPECT_EQ(0, bucketReader->read(buf, sizeof(buf)));
}

TEST_F(S3BucketReaderTest, ReadBuckeWithOneEmptyFileOneNonEmptyFile) {
    ListBucketResult* result = new ListBucketResult();
    BucketContent* item = new BucketContent("foo", 0);
    result->contents.push_back(item);
    item = new BucketContent("bar", 456);
    result->contents.push_back(item);

    EXPECT_CALL(s3interface, listBucket(_, _, _, _, _)).Times(1).WillOnce(Return(result));

    EXPECT_CALL(s3reader, read(_, _))
        .Times(3)
        .WillOnce(Return(0))
        .WillOnce(Return(256))
        .WillOnce(Return(0));

    EXPECT_CALL(s3reader, open(_)).Times(2);
    EXPECT_CALL(s3reader, close()).Times(2);

    params.setSegId(0);
    params.setSegNum(1);
    params.setUrlToLoad("https://s3-us-east-2.amazonaws.com/s3test.pivotal.io/whatever");
    bucketReader->open(params);
    bucketReader->setUpstreamReader(&s3reader);

    EXPECT_EQ(256, bucketReader->read(buf, sizeof(buf)));
    EXPECT_EQ(0, bucketReader->read(buf, sizeof(buf)));
}

TEST_F(S3BucketReaderTest, ReaderShouldSkipIfFileIsNotForThisSegment) {
    ListBucketResult* result = new ListBucketResult();
    BucketContent* item = new BucketContent("foo", 456);
    result->contents.push_back(item);

    EXPECT_CALL(s3interface, listBucket(_, _, _, _, _)).Times(1).WillOnce(Return(result));

    params.setSegId(10);
    params.setSegNum(16);
    params.setUrlToLoad("https://s3-us-east-2.amazonaws.com/s3test.pivotal.io/whatever");
    bucketReader->open(params);
    bucketReader->setUpstreamReader(&s3reader);

    EXPECT_EQ(0, bucketReader->read(buf, sizeof(buf)));
}

TEST_F(S3BucketReaderTest, UpstreamReaderThrowException) {
    ListBucketResult* result = new ListBucketResult();
    BucketContent* item = new BucketContent("foo", 0);
    result->contents.push_back(item);
    item = new BucketContent("bar", 456);
    result->contents.push_back(item);

    EXPECT_CALL(s3interface, listBucket(_, _, _, _, _)).Times(1).WillOnce(Return(result));

    EXPECT_CALL(s3reader, read(_, _))
        .Times(AtLeast(1))
        .WillRepeatedly(Throw(std::runtime_error("")));

    EXPECT_CALL(s3reader, open(_)).Times(1);
    EXPECT_CALL(s3reader, close()).Times(0);

    params.setSegId(0);
    params.setSegNum(1);
    params.setUrlToLoad("https://s3-us-east-2.amazonaws.com/s3test.pivotal.io/whatever");
    bucketReader->open(params);
    bucketReader->setUpstreamReader(&s3reader);

    EXPECT_THROW(bucketReader->read(buf, sizeof(buf)), std::runtime_error);
    EXPECT_THROW(bucketReader->read(buf, sizeof(buf)), std::runtime_error);
}
