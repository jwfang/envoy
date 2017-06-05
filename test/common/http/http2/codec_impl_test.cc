#include <cstdint>
#include <string>

#include "common/http/exception.h"
#include "common/http/header_map_impl.h"
#include "common/http/http2/codec_impl.h"
#include "common/stats/stats_impl.h"

#include "test/common/http/common.h"
#include "test/mocks/http/mocks.h"
#include "test/mocks/network/mocks.h"
#include "test/test_common/printers.h"
#include "test/test_common/utility.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace Envoy {

using testing::_;
using testing::AtLeast;
using testing::InSequence;
using testing::Invoke;
using testing::NiceMock;

namespace Http {
namespace Http2 {

typedef ::testing::tuple<uint32_t, uint32_t, uint32_t, uint32_t> Http2SettingsTuple;
typedef ::testing::tuple<Http2SettingsTuple, Http2SettingsTuple> Http2SettingsTestParam;

namespace {
Http2Settings Http2SettingsFromTuple(const Http2SettingsTuple& tp) {
  Http2Settings ret;
  ret.hpack_table_size_ = ::testing::get<0>(tp);
  ret.max_concurrent_streams_ = ::testing::get<1>(tp);
  ret.initial_stream_window_size_ = ::testing::get<2>(tp);
  ret.initial_connection_window_size_ = ::testing::get<3>(tp);
  return ret;
}
}

class Http2CodecImplTest : public testing::TestWithParam<Http2SettingsTestParam> {
public:
  struct ConnectionWrapper {
    void dispatch(const Buffer::Instance& data, ConnectionImpl& connection) {
      buffer_.add(data);
      if (!dispatching_) {
        while (buffer_.length() > 0) {
          dispatching_ = true;
          connection.dispatch(buffer_);
          dispatching_ = false;
        }
      }
    }

    bool dispatching_{};
    Buffer::OwnedImpl buffer_;
  };

  Http2CodecImplTest()
      : client_http2settings_(Http2SettingsFromTuple(::testing::get<0>(GetParam()))),
        client_(client_connection_, client_callbacks_, stats_store_, client_http2settings_),
        server_http2settings_(Http2SettingsFromTuple(::testing::get<1>(GetParam()))),
        server_(server_connection_, server_callbacks_, stats_store_, server_http2settings_),
        request_encoder_(client_.newStream(response_decoder_)) {
    setupDefaultConnectionMocks();

    EXPECT_CALL(server_callbacks_, newStream(_))
        .WillOnce(Invoke([&](StreamEncoder& encoder) -> StreamDecoder& {
          response_encoder_ = &encoder;
          encoder.getStream().addCallbacks(callbacks_);
          return request_decoder_;
        }));
  }

  void setupDefaultConnectionMocks() {
    ON_CALL(client_connection_, write(_))
        .WillByDefault(Invoke([&](Buffer::Instance& data)
                                  -> void { server_wrapper_.dispatch(data, server_); }));
    ON_CALL(server_connection_, write(_))
        .WillByDefault(Invoke([&](Buffer::Instance& data)
                                  -> void { client_wrapper_.dispatch(data, client_); }));
  }

  Stats::IsolatedStoreImpl stats_store_;
  const Http2Settings client_http2settings_;
  NiceMock<Network::MockConnection> client_connection_;
  MockConnectionCallbacks client_callbacks_;
  ClientConnectionImpl client_;
  ConnectionWrapper client_wrapper_;
  const Http2Settings server_http2settings_;
  NiceMock<Network::MockConnection> server_connection_;
  MockServerConnectionCallbacks server_callbacks_;
  ServerConnectionImpl server_;
  ConnectionWrapper server_wrapper_;
  MockStreamDecoder response_decoder_;
  StreamEncoder& request_encoder_;
  MockStreamDecoder request_decoder_;
  StreamEncoder* response_encoder_{};
  MockStreamCallbacks callbacks_;
};

TEST_P(Http2CodecImplTest, ExpectContinueHeadersOnlyResponse) {
  TestHeaderMapImpl request_headers;
  request_headers.addViaCopy("expect", "100-continue");
  HttpTestUtility::addDefaultHeaders(request_headers);
  TestHeaderMapImpl expected_headers;
  HttpTestUtility::addDefaultHeaders(expected_headers);
  EXPECT_CALL(request_decoder_, decodeHeaders_(HeaderMapEqual(&expected_headers), false));

  TestHeaderMapImpl continue_headers{{":status", "100"}};
  EXPECT_CALL(response_decoder_, decodeHeaders_(HeaderMapEqual(&continue_headers), false));
  request_encoder_.encodeHeaders(request_headers, false);

  EXPECT_CALL(request_decoder_, decodeData(_, true));
  Buffer::OwnedImpl hello("hello");
  request_encoder_.encodeData(hello, true);

  TestHeaderMapImpl response_headers{{":status", "200"}};
  EXPECT_CALL(response_decoder_, decodeHeaders_(HeaderMapEqual(&response_headers), true));
  response_encoder_->encodeHeaders(response_headers, true);
}

TEST_P(Http2CodecImplTest, ExpectContinueTrailersResponse) {
  TestHeaderMapImpl request_headers;
  request_headers.addViaCopy("expect", "100-continue");
  HttpTestUtility::addDefaultHeaders(request_headers);
  EXPECT_CALL(request_decoder_, decodeHeaders_(_, false));

  TestHeaderMapImpl continue_headers{{":status", "100"}};
  EXPECT_CALL(response_decoder_, decodeHeaders_(HeaderMapEqual(&continue_headers), false));
  request_encoder_.encodeHeaders(request_headers, false);

  EXPECT_CALL(request_decoder_, decodeData(_, true));
  Buffer::OwnedImpl hello("hello");
  request_encoder_.encodeData(hello, true);

  TestHeaderMapImpl response_headers{{":status", "200"}};
  EXPECT_CALL(response_decoder_, decodeHeaders_(HeaderMapEqual(&response_headers), false));
  response_encoder_->encodeHeaders(response_headers, false);

  TestHeaderMapImpl response_trailers{{"foo", "bar"}};
  EXPECT_CALL(response_decoder_, decodeTrailers_(HeaderMapEqual(&response_trailers)));
  response_encoder_->encodeTrailers(response_trailers);
}

TEST_P(Http2CodecImplTest, ShutdownNotice) {
  TestHeaderMapImpl request_headers;
  HttpTestUtility::addDefaultHeaders(request_headers);
  EXPECT_CALL(request_decoder_, decodeHeaders_(_, true));
  request_encoder_.encodeHeaders(request_headers, true);

  EXPECT_CALL(client_callbacks_, onGoAway());
  server_.shutdownNotice();
  server_.goAway();

  TestHeaderMapImpl response_headers{{":status", "200"}};
  EXPECT_CALL(response_decoder_, decodeHeaders_(_, true));
  response_encoder_->encodeHeaders(response_headers, true);
}

TEST_P(Http2CodecImplTest, RefusedStreamReset) {
  TestHeaderMapImpl request_headers;
  HttpTestUtility::addDefaultHeaders(request_headers);
  EXPECT_CALL(request_decoder_, decodeHeaders_(_, false));
  request_encoder_.encodeHeaders(request_headers, false);

  MockStreamCallbacks callbacks;
  request_encoder_.getStream().addCallbacks(callbacks);
  EXPECT_CALL(callbacks_, onResetStream(StreamResetReason::LocalRefusedStreamReset));
  EXPECT_CALL(callbacks, onResetStream(StreamResetReason::RemoteRefusedStreamReset));
  response_encoder_->getStream().resetStream(StreamResetReason::LocalRefusedStreamReset);
}

TEST_P(Http2CodecImplTest, InvalidFrame) {
  ON_CALL(client_connection_, write(_))
      .WillByDefault(
          Invoke([&](Buffer::Instance& data) -> void { server_wrapper_.buffer_.add(data); }));
  request_encoder_.encodeHeaders(TestHeaderMapImpl{}, true);
  EXPECT_THROW(server_wrapper_.dispatch(Buffer::OwnedImpl(), server_), CodecProtocolException);
}

TEST_P(Http2CodecImplTest, TrailingHeaders) {
  TestHeaderMapImpl request_headers;
  HttpTestUtility::addDefaultHeaders(request_headers);
  EXPECT_CALL(request_decoder_, decodeHeaders_(_, false));
  request_encoder_.encodeHeaders(request_headers, false);
  EXPECT_CALL(request_decoder_, decodeData(_, false));
  Buffer::OwnedImpl hello("hello");
  request_encoder_.encodeData(hello, false);
  EXPECT_CALL(request_decoder_, decodeTrailers_(_));
  request_encoder_.encodeTrailers(TestHeaderMapImpl{{"trailing", "header"}});

  TestHeaderMapImpl response_headers{{":status", "200"}};
  EXPECT_CALL(response_decoder_, decodeHeaders_(_, false));
  response_encoder_->encodeHeaders(response_headers, false);
  EXPECT_CALL(response_decoder_, decodeData(_, false));
  Buffer::OwnedImpl world("world");
  response_encoder_->encodeData(world, false);
  EXPECT_CALL(response_decoder_, decodeTrailers_(_));
  response_encoder_->encodeTrailers(TestHeaderMapImpl{{"trailing", "header"}});
}

TEST_P(Http2CodecImplTest, TrailingHeadersLargeBody) {
  // Buffer server data so we can make sure we don't get any window updates.
  ON_CALL(client_connection_, write(_))
      .WillByDefault(
          Invoke([&](Buffer::Instance& data) -> void { server_wrapper_.buffer_.add(data); }));

  TestHeaderMapImpl request_headers;
  HttpTestUtility::addDefaultHeaders(request_headers);
  EXPECT_CALL(request_decoder_, decodeHeaders_(_, false));
  request_encoder_.encodeHeaders(request_headers, false);
  EXPECT_CALL(request_decoder_, decodeData(_, false)).Times(AtLeast(1));
  Buffer::OwnedImpl body(std::string(1024 * 1024, 'a'));
  request_encoder_.encodeData(body, false);
  EXPECT_CALL(request_decoder_, decodeTrailers_(_));
  request_encoder_.encodeTrailers(TestHeaderMapImpl{{"trailing", "header"}});

  // Flush pending data.
  setupDefaultConnectionMocks();
  server_wrapper_.dispatch(Buffer::OwnedImpl(), server_);

  TestHeaderMapImpl response_headers{{":status", "200"}};
  EXPECT_CALL(response_decoder_, decodeHeaders_(_, false));
  response_encoder_->encodeHeaders(response_headers, false);
  EXPECT_CALL(response_decoder_, decodeData(_, false));
  Buffer::OwnedImpl world("world");
  response_encoder_->encodeData(world, false);
  EXPECT_CALL(response_decoder_, decodeTrailers_(_));
  response_encoder_->encodeTrailers(TestHeaderMapImpl{{"trailing", "header"}});
}

TEST_P(Http2CodecImplTest, DeferredReset) {
  InSequence s;

  // Buffer server data so we can make sure we don't get any window updates.
  ON_CALL(client_connection_, write(_))
      .WillByDefault(
          Invoke([&](Buffer::Instance& data) -> void { server_wrapper_.buffer_.add(data); }));

  // This will send a request that is bigger than the initial window size. When we then reset it,
  // after attempting to flush we should reset the stream and drop the data we could not send.
  TestHeaderMapImpl request_headers;
  HttpTestUtility::addDefaultHeaders(request_headers);
  request_encoder_.encodeHeaders(request_headers, false);
  Buffer::OwnedImpl body(std::string(1024 * 1024, 'a'));
  request_encoder_.encodeData(body, true);
  request_encoder_.getStream().resetStream(StreamResetReason::LocalReset);

  EXPECT_CALL(request_decoder_, decodeHeaders_(_, false));
  EXPECT_CALL(request_decoder_, decodeData(_, false)).Times(AtLeast(1));
  EXPECT_CALL(callbacks_, onResetStream(StreamResetReason::RemoteReset));
  server_wrapper_.dispatch(Buffer::OwnedImpl(), server_);
}

// we seperate default/edge cases here to avoid combinatorial explosion

#define HTTP2SETTINGS_DEFAULT_COMBIME                                                              \
  ::testing::Combine(::testing::Values(Http2Settings::DEFAULT_HPACK_TABLE_SIZE),                   \
                     ::testing::Values(Http2Settings::DEFAULT_MAX_CONCURRENT_STREAMS),             \
                     ::testing::Values(Http2Settings::DEFAULT_INITIAL_STREAM_WINDOW_SIZE),         \
                     ::testing::Values(Http2Settings::DEFAULT_INITIAL_CONNECTION_WINDOW_SIZE))

INSTANTIATE_TEST_CASE_P(Http2CodecImplTestDefaultSettings, Http2CodecImplTest,
                        ::testing::Combine(HTTP2SETTINGS_DEFAULT_COMBIME,
                                           HTTP2SETTINGS_DEFAULT_COMBIME));

#define HTTP2SETTINGS_EDGE_COMBIME                                                                 \
  ::testing::Combine(                                                                              \
      ::testing::Values(Http2Settings::MIN_HPACK_TABLE_SIZE, Http2Settings::MAX_HPACK_TABLE_SIZE), \
      ::testing::Values(Http2Settings::MIN_MAX_CONCURRENT_STREAMS,                                 \
                        Http2Settings::MAX_MAX_CONCURRENT_STREAMS),                                \
      ::testing::Values(Http2Settings::MIN_INITIAL_STREAM_WINDOW_SIZE,                             \
                        Http2Settings::MAX_INITIAL_STREAM_WINDOW_SIZE),                            \
      ::testing::Values(Http2Settings::MIN_INITIAL_CONNECTION_WINDOW_SIZE,                         \
                        Http2Settings::MAX_INITIAL_CONNECTION_WINDOW_SIZE))

INSTANTIATE_TEST_CASE_P(Http2CodecImplTestEdgeSettings, Http2CodecImplTest,
                        ::testing::Combine(HTTP2SETTINGS_EDGE_COMBIME, HTTP2SETTINGS_EDGE_COMBIME));

TEST(Http2CodecUtility, reconstituteCrumbledCookies) {
  {
    HeaderString key;
    HeaderString value;
    HeaderString cookies;
    EXPECT_FALSE(Utility::reconstituteCrumbledCookies(key, value, cookies));
    EXPECT_TRUE(cookies.empty());
  }

  {
    HeaderString key(Headers::get().ContentLength);
    HeaderString value;
    value.setInteger(5);
    HeaderString cookies;
    EXPECT_FALSE(Utility::reconstituteCrumbledCookies(key, value, cookies));
    EXPECT_TRUE(cookies.empty());
  }

  {
    HeaderString key(Headers::get().Cookie);
    HeaderString value;
    value.setCopy("a=b", 3);
    HeaderString cookies;
    EXPECT_TRUE(Utility::reconstituteCrumbledCookies(key, value, cookies));
    EXPECT_EQ(cookies, "a=b");

    HeaderString key2(Headers::get().Cookie);
    HeaderString value2;
    value2.setCopy("c=d", 3);
    EXPECT_TRUE(Utility::reconstituteCrumbledCookies(key2, value2, cookies));
    EXPECT_EQ(cookies, "a=b; c=d");
  }
}

} // Http2
} // Http
} // Envoy
