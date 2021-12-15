#define TEST_RINGBUFFER

#include "ringBuffer.h"
#include <iostream>

using namespace leaf;
using namespace std;


void print(RingBuffer<int>& ring_buffer) {
    cout << "capacity_ = " << ring_buffer.capacity()
        << ", head_ = " << ring_buffer.head()
        << ", tail_ = " << ring_buffer.tail() << endl;
    if ( ring_buffer.tail() > ring_buffer.head() ) {
        for ( size_t i = 0; i < ring_buffer.head(); ++i ) {
            ring_buffer.buffer()[i] = 0;
        }
        for ( size_t i = ring_buffer.tail(); i < ring_buffer.capacity(); ++i ) {
            ring_buffer.buffer()[i] = 0;
        }
    }
    else {
        for ( size_t i = ring_buffer.tail(); i < ring_buffer.head(); ++i ) {
            ring_buffer.buffer()[i] = 0;
        }
    }

    for ( size_t i = 0; i < ring_buffer.capacity(); ++i ) {
        cout << ring_buffer.buffer()[i] << " ";
    }
    cout << endl;

}

void init(RingBuffer<int>& ring_buffer) {
    uint32_t i = 1;
    for ( auto it = ring_buffer.begin(); it != ring_buffer.end(); ++it ) {
        *it = i++;
    }
}

int main(int argc, const char *argv[])
{
    RingBuffer<int> ring_buffer = {1, 2, 3, 4, 5, 6, 7, 8};

    for ( auto i : ring_buffer) {
        cout << i << " ";
    }
    cout << endl;
    {
        RingBuffer<int> test1(8, 2, 7);
        init(test1);
        print(test1);
        test1.push_back(test1.begin(), test1.end()-1);
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1(16, 3, 8);
        init(test1);
        print(test1);

        RingBuffer<int> test2(16, 3, 8);
        init(test2);
        print(test2);

        test1.push_back(test2.begin(), test2.end());
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1(16, 8, 14);
        init(test1);
        print(test1);

        RingBuffer<int> test2(16, 3, 8);
        init(test2);
        print(test2);

        test1.push_back(test2.begin(), test2.end());
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1(16, 3, 8);
        init(test1);
        print(test1);

        RingBuffer<int> test2(16, 14, 3);
        init(test2);
        print(test2);

        test1.push_back(test2.begin(), test2.end());
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1(16, 8, 14);
        init(test1);
        print(test1);

        RingBuffer<int> test2(16, 15, 3);
        init(test2);
        print(test2);

        test1.push_back(test2.begin(), test2.end());
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1(16, 8, 14);
        init(test1);
        print(test1);

        RingBuffer<int> test2(16, 12, 3);
        init(test2);
        print(test2);

        test1.push_back(test2.begin(), test2.end());
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1(16, 14, 4);
        init(test1);
        print(test1);

        RingBuffer<int> test2(16, 3, 6);
        init(test2);
        print(test2);

        test1.push_back(test2.begin(), test2.end());
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1(16, 14, 4);
        init(test1);
        print(test1);

        RingBuffer<int> test2(16, 12, 3);
        init(test2);
        print(test2);

        test1.push_back(test2.begin(), test2.end());
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1;
        print(test1);

        RingBuffer<int> test2(16, 3, 8);
        init(test2);
        print(test2);

        test1.push_back(test2.begin(), test2.end());
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1;
        print(test1);

        RingBuffer<int> test2(16, 12, 3);
        init(test2);
        print(test2);

        test1.push_back(test2.begin(), test2.end());
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1(16, 3, 3);
        print(test1);

        RingBuffer<int> test2(16, 3, 8);
        init(test2);
        print(test2);

        test1.push_back(test2.begin(), test2.end());
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1(16, 3, 3);
        print(test1);

        RingBuffer<int> test2(16, 12, 3);
        init(test2);
        print(test2);

        test1.push_back(test2.begin(), test2.end());
        print(test1);
    }

    cout << endl;
    cout << "----------- push_front ---------------" << endl;
    {
        RingBuffer<int> test1(8, 2, 7);
        init(test1);
        print(test1);
        test1.push_front(test1.begin(), test1.end()-1);
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1(16, 8, 14);
        init(test1);
        print(test1);

        RingBuffer<int> test2(16, 3, 10);
        init(test2);
        print(test2);

        test1.push_front(test2.begin(), test2.end());
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1(16, 3, 8);
        init(test1);
        print(test1);

        RingBuffer<int> test2(16, 3, 10);
        init(test2);
        print(test2);

        test1.push_front(test2.begin(), test2.end());
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1(16, 8, 12);
        init(test1);
        print(test1);

        RingBuffer<int> test2(16, 13, 5);
        init(test2);
        print(test2);

        test1.push_front(test2.begin(), test2.end());
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1(16, 3, 8);
        init(test1);
        print(test1);

        RingBuffer<int> test2(16, 9, 2);
        init(test2);
        print(test2);

        test1.push_front(test2.begin(), test2.end());
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1(16, 3, 8);
        init(test1);
        print(test1);

        RingBuffer<int> test2(16, 14, 5);
        init(test2);
        print(test2);

        test1.push_front(test2.begin(), test2.end());
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1(16, 14, 4);
        init(test1);
        print(test1);

        RingBuffer<int> test2(16, 3, 6);
        init(test2);
        print(test2);

        test1.push_front(test2.begin(), test2.end());
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1(16, 14, 4);
        init(test1);
        print(test1);

        RingBuffer<int> test2(16, 12, 3);
        init(test2);
        print(test2);

        test1.push_front(test2.begin(), test2.end());
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1;
        print(test1);

        RingBuffer<int> test2(16, 3, 8);
        init(test2);
        print(test2);

        test1.push_front(test2.begin(), test2.end());
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1;
        print(test1);

        RingBuffer<int> test2(16, 12, 3);
        init(test2);
        print(test2);

        test1.push_front(test2.begin(), test2.end());
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1(16, 3, 3);
        print(test1);

        RingBuffer<int> test2(16, 3, 8);
        init(test2);
        print(test2);

        test1.push_front(test2.begin(), test2.end());
        print(test1);
    }
    cout << "--------------------------------------" << endl;
    {
        RingBuffer<int> test1(16, 3, 3);
        print(test1);

        RingBuffer<int> test2(16, 12, 3);
        init(test2);
        print(test2);

        test1.push_front(test2.begin(), test2.end());
        print(test1);
    }
    return 0;
}

