/* channel.h - Inter-thread message queues

Copyright © 2013-2017 Brian Bray

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <mutex>
#include <condition_variable>
#include <vector>
#include <stdexcept>


template<typename Msg>
class utchannel;

template<typename Msg>
class channel {
public:
    // NO justiifcation for this number, just a sanity check
    static constexpr int    maxReasonableLength = 1000000;

    channel(int maxQueueLength=1)
      : queue(maxQueueLength),
        front(-1),
        back(0),
        capacity(maxQueueLength)
    {
        if( maxQueueLength<=0 )
            throw std::invalid_argument( "Queue too short" );
		if( maxQueueLength>maxReasonableLength )
			throw std::invalid_argument( "Queue too long" );
    }

    ~channel() noexcept {
    }

    channel(const channel&) = delete;
    channel(channel&&) = delete;
    channel& operator=(const channel&) = delete;
    channel& operator=(channel&&) = delete;

    void send(Msg msg) {
        std::unique_lock<std::mutex> lck(mtx);
        while (full())
            fullwait.wait(lck);
        queue[back] = std::move(msg);
        nextback();
        lck.unlock();
        emptywait.notify_one();
    }

    Msg get() {
        std::unique_lock<std::mutex> lck(mtx);
        while (empty())
            emptywait.wait(lck);
        Msg msg = std::move(queue[front]);
        nextfront();
        lck.unlock();
        fullwait.notify_one();
        return msg;
    }

    bool valid() {
        std::unique_lock<std::mutex> lck(mtx);
        return unsyncedSelftest();
    }

private:
    friend class utchannel<Msg>;
    
    std::mutex					mtx;
    std::condition_variable		fullwait, emptywait;
    std::vector<Msg>			queue;
    int							front, back, capacity;

    bool empty() {
        return front==-1;
    }

    bool full() {
        return front==back;
    }

    void nextfront() {
        front = (front+1) % capacity;
        if (front==back)
            front = -1;
    }
    void nextback() {
        if (front==-1)
            front = back;
        back = (back+1) % capacity;
    }

    bool unsyncedSelftest() {
        bool valid  = (  0 <  capacity ) & ( capacity <= maxReasonableLength );
             valid &= (  0 <=     back ) & ( back     <  capacity );
             valid &= ( -1 <=    front ) & ( front    <  capacity );
        return valid && ( capacity == int(queue.size()) );
    }
};
