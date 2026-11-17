#include "../src/key_policy.h"
#include <cassert>
#include <cstdio>
int main() {
    capsime::KeyPolicy p;
    auto d=p.process(0xe7,0xe0fe,false,false); assert(!d.consume && !d.toggle);
    d=p.process(0x14,0,true,false); assert(!d.consume && !d.toggle);
    d=p.process(0x13,0,true,false); assert(!d.consume && !d.toggle);
    d=p.process(0xe7,0x3042,true,false); assert(!d.consume && !d.toggle);
    d=p.process(0xe7,0xe0fe,true,true); assert(d.consume && !d.toggle);
    d=p.process(0xe7,0xe0fe,true,false); assert(d.consume && d.toggle);
    d=p.process(0xe7,0xe0fe,true,false); assert(d.consume && !d.toggle);
    d=p.process(0x41,0,true,false); assert(!d.consume && !d.toggle);
    d=p.process(0xe7,0xe0fe,true,true); assert(d.consume && !d.toggle);
    d=p.process(0xe7,0xe0fe,true,false); assert(d.consume && d.toggle);
    puts("PASS: only injected private Unicode marker is consumed; physical keys and normal Unicode pass through");
}
