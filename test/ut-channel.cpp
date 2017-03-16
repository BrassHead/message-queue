/* ut-channel.cc - channel unit test

Copyright © 2013 Brian Bray


*/

#define TRACING 1
#include "selftest.hpp"
using selftest::selftest_error;
using selftest::over_reasonable_limit;
using selftest::trace;

#include "channel.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <future>

using std::cerr;
using std::chrono::milliseconds;
using std::async;
using std::launch;
using std::this_thread::sleep_for;
using std::future_status;
using std::auto_ptr;
template< class T >
const T& max( const T& a, const T& b )
{
    return a<b ? b : a;
}

/*
Self verifying channel. Note: not thread safe, so it can only be used
for tests in a single thread.
*/
template<typename Msg>
class utchannel : public channel<Msg> {
public:
    utchannel( int maxQueueLength=1 ) : channel<Msg>(maxQueueLength) {
        // No preconditions
        // Postconditions
        CHECKIF( this->capacity==maxQueueLength );
        CHECKIF( size()==0 );
    }

    void send(Msg msg) {
        // No Preconditions
        ASSERT( this->valid() );
        channel<Msg>::send(std::move(msg));
        CHECKIF( this->valid() );
    }

    Msg get() {
        ASSERT( this->valid() );
        Msg rc = channel<Msg>::get();
        CHECKIF( this->valid() );
        return rc;
    }

    int size() {
        if (this->empty())
            return 0;
        else
            return (this->front+this->capacity-this->back) % this->capacity;
    }

    void invalidate() {
        this->capacity = -1;
    }

};


namespace {

class CInt {		// Counted integer class to test unique_ptr channel.
public:
    CInt(int i) : i_(i) {
        ++totalCInts;
    };
    CInt(const CInt &) = delete;
    CInt operator=(const CInt &) = delete;
    ~CInt() {
        --totalCInts;
    };

    operator int() {
        return i_;
    };

    static int totalCInts;
    int i_;
};

int CInt::totalCInts = {0};

TEST_FUNCTION( unique_ptr_single_thread_test )
{
    trace << "unique_ptr_single_thread_test\n";
    utchannel<std::unique_ptr<CInt>>	ch(5);
    CHECKIF( CInt::totalCInts==0 );

    for (int i=2; i<10; ++i) {
        // Send
        for (int j=0; j<i/2; ++j)
            ch.send( std::unique_ptr<CInt>(new CInt(j)) );

        CHECKIF( CInt::totalCInts==i/2 );

        // Receive
        for (int j=0; j<i/2; ++j)
            CHECKIF( *ch.get()==j );

        CHECKIF( CInt::totalCInts==0 );
    }
    CHECKIF( CInt::totalCInts==0 );
}

TEST_FUNCTION( shared_ptr_single_thread_test )
{
    trace << "shared_ptr_single_thread_test\n";
    utchannel<std::shared_ptr<CInt>>	ch(5);
    CHECKIF( CInt::totalCInts==0 );

    for (int i=2; i<10; ++i) {
        // Send
        for (int j=0; j<i/2; ++j)
            ch.send( std::shared_ptr<CInt>(new CInt(j)) );

        CHECKIF( CInt::totalCInts==i/2 );

        // Receive
        for (int j=0; j<i/2; ++j)
            CHECKIF( *ch.get()==j );

        CHECKIF( CInt::totalCInts==0 );
    }
    CHECKIF( CInt::totalCInts==0 );
}

TEST_FUNCTION( constructor_tests )
{
    trace << "constructor_tests\n";
    CHECKIFTHROWS(	utchannel<int>(0),
                    std::invalid_argument );
    CHECKIFTHROWS(	utchannel<int>(channel<int>::maxReasonableLength+1),
                    std::invalid_argument );
}

TEST_FUNCTION( destructor_tests )
{
    trace << "destructor_tests\n";
    CHECKIFTHROWS(	utchannel<int> bad; bad.invalidate(); bad.send(1),
                    selftest_error );
    CHECKIFTHROWS(	utchannel<int> bad; bad.invalidate(); bad.get(),
                    selftest_error );
}


static constexpr int bucketRange {1000000};
static constexpr int maxBuckets {500};
static constexpr int maxNumProducers {500};
static constexpr int maxNumConsumers {500};

typedef channel<int> IChannel;

void int_producer(
    int numBursts,
    int msgsPerBurst,
    int delayms,
    int instance,
    IChannel &och)
{
    // Send bursts of messages with a delay between bursts
    // Each "message" is just an int of increasing value within a range
    milliseconds sleepytime( delayms );
    int msg = instance*bucketRange;
    for (int burstCount=0; burstCount<numBursts; ++burstCount) {
        for (int msgCount=0; msgCount<msgsPerBurst; ++msgCount) {
            och.send(msg);
            ++msg;
        }
        sleep_for( sleepytime );
    }
}

void int_consumer(
    int numBursts,
    int msgsPerBurst,
    int delayms,
    IChannel &ich)
{
    // Receive bursts of messages with a delay between bursts
    // Make sure that messages received are increasing in value for each sender
    milliseconds sleepytime( delayms );
    int lastseen[maxBuckets];
    for (int i=0; i<maxBuckets; ++i)
        lastseen[i] = i*bucketRange-1;

    for (int burstCount=0; burstCount<numBursts; ++burstCount) {
        for (int msgCount=0; msgCount<msgsPerBurst; ++msgCount) {
            int got = ich.get();
            int bucket = got/bucketRange;
            CHECKIF( lastseen[bucket]<got );
            lastseen[bucket] = got;
        }
        sleep_for( sleepytime );
    }
}


TEST_FUNCTION( producer_consumer_example )
{
    trace << "producer_consumer_example\n";
    IChannel ch(20);
    auto sendfuture = async(launch::async,
                            int_producer, 10, 100, 100, 0, std::ref(ch) );
    auto recvfuture = async(launch::async,
                            int_consumer, 5, 200, 5, std::ref(ch) );

    constexpr int timeEst = 1010;  // 10 bursts@100ms + a little
    auto status = recvfuture.wait_for(milliseconds(timeEst));
    CHECKIF( status!=future_status::timeout );
    sendfuture.get();
    recvfuture.get();
}

void pc_test(
    int totalMessages,

    int numProducers,
    int producerBursts,
    int producerDelay,

    int numConsumers,
    int consumerBursts,
    int consumerDelay
)
{
    IChannel ch(20);

    std::future<void> producers[maxNumProducers];
    std::future<void> consumers[maxNumConsumers];

    int producerMsgsPerBurst = (totalMessages / numProducers) / producerBursts;
    int consumerMsgsPerBurst = (totalMessages / numConsumers) / consumerBursts;

    // Make sure we have room
    CHECKIF( numConsumers<=maxNumConsumers);
    CHECKIF( numProducers<=maxNumProducers);
    
    // Make sure rounding doesn't mismatch producers/consumers
    CHECKIF( totalMessages == numProducers*producerBursts*producerMsgsPerBurst );
    CHECKIF( totalMessages == numConsumers*consumerBursts*consumerMsgsPerBurst );

    for (int pIndex=0; pIndex<numProducers; ++pIndex) {
        producers[pIndex] = async(launch::async, int_producer,
                                  producerBursts,
                                  producerMsgsPerBurst,
                                  producerDelay,
                                  pIndex,
                                  std::ref(ch) );
    }
    for (int cIndex=0; cIndex<numConsumers; ++cIndex) {
        consumers[cIndex] = async(launch::async, int_consumer,
                                  consumerBursts,
                                  consumerMsgsPerBurst,
                                  consumerDelay,
                                  std::ref(ch) );
    }

    int minProducerTime = producerBursts*producerDelay;
    int minConsumerTime = consumerBursts*consumerDelay;
    int messageTime = totalMessages/15;	// 15,000 messages/second (Raspberry Pi)
    int timeEst = max(minProducerTime,minConsumerTime) + messageTime;
    trace << "Channel<> test time estimate: " << timeEst << " ms" << std::endl;

    auto status = consumers[numConsumers-1].wait_for(milliseconds(timeEst));
    CHECKIF( status!=future_status::timeout );

}

TEST_FUNCTION( competing_producers )
{
    trace << "competing_producers\n";
    //         msgs     Producers     Consumers
    //            #       #  #b  ms     #  #b   ms
    pc_test(  10000,    100,  1,  0,    1, 10,  10 );
}

TEST_FUNCTION( competing_consumers )
{
    trace << "competing_consumers\n";
    //         msgs     Producers     Consumers
    //            #       #  #b  ms     #  #b   ms
    pc_test(  10000,      1, 10, 10,  100,  1,   0 );
}


} // unnamed namespace

int main( int argc, char *argv[] )
{
    trace << "Starting test sequence.\n\n\n";
    auto fails = selftest::runUnitTests<0>();

    if ( 0==fails.numFailedTests ) {
        cerr << "\n\nchannel.hpp testing completed successfully\n\n";
        return 0;
    } else {
        cerr << "\n\nUnit testing of channel.hpp failed\n\n";
        return 1;
    }
}
