#include <allocation.hpp>
#include <cstdio>

extern "C" void ironclad_mark_address(void * address) {
  if((unsigned long)address > 1000){
    allocation_base* allocation = (allocation_base*)address;
    allocation->mark(address);
  }
}
