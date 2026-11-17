#include "../src/key_policy.h"
#include <cassert>
#include <cstdio>
int main() {
    capsime::KeyPolicy p;
    auto d=p.process(0x13,false,false); assert(!d.consume && !d.toggle);
    d=p.process(0x14,true,false); assert(!d.consume && !d.toggle);
    d=p.process(0x13,true,true); assert(d.consume && !d.toggle);
    d=p.process(0x13,true,false); assert(d.consume && d.toggle);
    d=p.process(0x13,true,false); assert(d.consume && !d.toggle);
    d=p.process(0x41,true,false); assert(!d.consume && !d.toggle);
    d=p.process(0x13,true,true); assert(d.consume && !d.toggle);
    d=p.process(0x13,true,false); assert(d.consume && d.toggle);
    puts("PASS: hardware Caps/Pause pass through; injected Pause toggles once per press");
}
