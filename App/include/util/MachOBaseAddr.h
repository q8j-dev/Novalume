//
//  MachOBaseAddr.h
//  App
//
//  Created by David Stahl on 11/10/14.
//
//

#ifndef App_MachOBaseAddr_h
#define App_MachOBaseAddr_h

#include <cstddef>
#include <cstdint>

uintptr_t machODynamicBaseAddress(void);

size_t machOTextSize(void);

#endif
