#include "stream_reassembler.hh"

#include <iostream>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

// Node::Node(const uint32_t &dataIndex, const uint32_t &length) { DUMMY_CODE(dataIndex, length); }
StreamReassembler::StreamReassembler(const size_t capacity)
    :  // buffer(capacity, std::pair<char,bool>(char(" "),false)),
    buffer(capacity, std::make_pair(char(' '), false))
    , _output(capacity)
    , _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // 如果收到了最后一个数据包，先确定结束边界
    if (eof) {
        _eof = true;
        _eof_index = data.length() + index;
    };
    // 定义一个减少的长度
    int sub_length = 0;
    /*
    这里出现一个问题，就是有一种可能情况:
    当first_unassembled <= index < first_unassembled() && index + data.length() >= first_unassembled()
    如果这个数据segment中的eof == true会导致递归调用push_substring()，并且此时eof == true
    再次调用这个函数的时候，修改_eof_index的值，导致结果出错

    怎么去解决这个问题？
    如果eof == true就不进行递归调用，只是直接把数据切割————无法解决
    */

    // 分析输入substring与窗口的可能边界条件
    if (index >= first_unacceptable() || index < first_unassembled() ||
        (index < first_unacceptable() && index + data.length() > first_unacceptable())) {
        if (index < first_unassembled() && index + data.length() >= first_unassembled())
            // 这里注意一个问题：就是从index = first_unassembled()
            // 的地方开始并不意味着，在该字符串中的索引为first_unassembled()的子字符串开始
            return push_substring(data.substr(first_unassembled() - index, data.length()), first_unassembled(), eof);
        else if (index < first_unacceptable() && index + data.length() > first_unacceptable()) {
            if (eof == true) {
                sub_length = data.length() - (first_unacceptable() - index);
            } else
                return push_substring(data.substr(0, first_unacceptable() - index), index, eof);
        } else {
            return;
        }
    };
    // 如果第一个数据包已经到达
    if (index == first_unassembled())
        first_in = true;
    for (size_t i = 0; i < data.length() - sub_length; i++) {
        // 如果出现数据不一致的情况，panic
        if (buffer[(index + i) % _capacity].second == true) {
            if (buffer[(index + i) % _capacity].first == data[i])
                continue;
            else
                throw "datagrame is inconsistent";
        }
        buffer[(index + i) % _capacity].first = data[i];
        buffer[(index + i) % _capacity].second = true;
        _unassembled_bytes++;
    };
    // 这里根据是都第一个数据包到达来决定是都写入缓存
    // 如果第一个数据包已经到达，就把连续的字节加入到操作系统的缓存中
    if (first_in) {
        size_t i = 0;
        std::string output_string = "";
        std::pair<char, bool> nowPair = buffer[(first_unassembled() + i) % _capacity];
        while (nowPair.second) {
            buffer[(first_unassembled() + i) % _capacity].second = false;
            output_string += nowPair.first;
            // i++;
            // nowPair = buffer[(first_unassembled()+ i)%_capacity];
            nowPair = buffer[(first_unassembled() + ++i) % _capacity];
            // 注意：下面这个语句的含义跟上面这行语句的含义不一样，会导致不同的结果
            // 建议看文章[必须知道的C语言知识细节：i++和++i](https://zhuanlan.zhihu.com/p/162282210)
            // nowPair = buffer[(first_unassembled()+ i++)%_capacity];
        };
        _unassembled_bytes -= i;
        _output.write(output_string);
    };
    if ((first_unassembled() == _eof_index) && _eof) {
        _output.end_input();
    };
}
// 第一个没有read的序列号
size_t StreamReassembler::first_unread() { return _output.bytes_read(); }
// 第一个没有组装完成的序列号
size_t StreamReassembler::first_unassembled() { return _output.bytes_written(); }
// 第一个不能接受的序列号(滑动窗口之外)
size_t StreamReassembler::first_unacceptable() { return _output.bytes_read() + _capacity; }

size_t StreamReassembler::unassembled_bytes() const { return {_unassembled_bytes}; }

bool StreamReassembler::empty() const { return {_unassembled_bytes == 0}; }

// 将第一个测试用例写入代码,通过GDB进行调试
/*
int main() {
    // 先初始化一下 StreamReassembler
    StreamReassembler streamReassembler = StreamReassembler(8);
    // 写入测试用例的相关操作
    streamReassembler.push_substring("abc", 0, false);
    cout << streamReassembler.stream_out().eof() << endl;
    streamReassembler.push_substring("ghX", 6, true);
    cout << streamReassembler.stream_out().eof() << endl;
    streamReassembler.push_substring("cdefg", 2, false);
    streamReassembler.stream_out().read(8);
    cout << streamReassembler.stream_out().eof() << endl;
}
*/

/*
      {
            ReassemblerTestHarness test{8};

            test.execute(SubmitSegment{"abc", 0});
            test.execute(BytesAssembled(3));
            test.execute(NotAtEof{});

            test.execute(SubmitSegment{"ghX", 6}.with_eof(true));
            test.execute(BytesAssembled(3));
            test.execute(NotAtEof{});

            test.execute(SubmitSegment{"cdefg", 2});
            test.execute(BytesAssembled(8));
            test.execute(BytesAvailable{"abcdefgh"});
            test.execute(NotAtEof{});
        }*/