#include <cassert>
#include <cstdio>
#include <iterator>
#include <vector>

using std::vector;

struct op {
    int cmd;
    vector<int> data;
    int delay;

    op(int cmd)
    : cmd(cmd),
      delay(0)
    {}

};

vector<op> ops;

void writecommand(int cmd)
{
    ops.push_back(op(cmd));
}

void writedata(int datum)
{
    assert(!ops.empty());
    ops.back().data.push_back(datum);
}

void delay(int delay)
{
    assert(!ops.empty());
    assert(!ops.back().delay);
    ops.back().delay = delay;
}

int main()
{
    writecommand(0x01);
    delay(150);

    #include "GC9A01_Init.h"

    printf("{\n");
    printf("    %zu,\n", ops.size());
    for (auto& op : ops) {
        printf("    %#02x,", op.cmd);
        if (op.delay) {
            printf(" DELAY_BIT");
            if (!op.data.empty())
                printf(" | %zu", op.data.size());
            printf(",");
        } else {
            printf(" %zu,", op.data.size());
        }
        for (auto datum : op.data)
            printf(" %#02x,", datum);
        if (op.delay) {
            assert(op.delay < 255);
            printf(" %d,", op.delay);
        }
        printf("\n");
    }
    printf("};\n");
    return 0;
}