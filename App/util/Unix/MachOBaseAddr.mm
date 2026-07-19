//
//  MachOBaseAddr.m
//  App
//
//  Created on 11/10/14.
//
//

#import <Foundation/Foundation.h>
#include <mach-o/dyld.h>

#include <cstring>

namespace {

const segment_command_64* textSegment()
{
    const uint32_t imageCount = _dyld_image_count();
    for (uint32_t index = 0; index < imageCount; ++index) {
        const mach_header* header = _dyld_get_image_header(index);
        if (!header || header->filetype != MH_EXECUTE || header->magic != MH_MAGIC_64)
            continue;

        const auto* header64 = reinterpret_cast<const mach_header_64*>(header);
        const auto* command = reinterpret_cast<const load_command*>(header64 + 1);
        for (uint32_t commandIndex = 0; commandIndex < header64->ncmds; ++commandIndex) {
            if (command->cmd == LC_SEGMENT_64) {
                const auto* segment = reinterpret_cast<const segment_command_64*>(command);
                if (std::strncmp(segment->segname, SEG_TEXT,
                        sizeof(segment->segname)) == 0)
                    return segment;
            }
            command = reinterpret_cast<const load_command*>(
                reinterpret_cast<const char*>(command) + command->cmdsize);
        }
    }
    return nullptr;
}

} // namespace

uintptr_t staticBaseAddress(void)
{
    const segment_command_64* segment = textSegment();
    return segment ? static_cast<uintptr_t>(segment->vmaddr) : 0;
}

intptr_t imageSlide(void)
{
    int image_count = _dyld_image_count();
    const struct mach_header *header;
    
    for (int index = 0; index < image_count; ++index)
    {
        header = _dyld_get_image_header(index);
        if (header->filetype == MH_EXECUTE)
        {
            return _dyld_get_image_vmaddr_slide(index);
        }
    }
    return 0;
}

uintptr_t machODynamicBaseAddress(void)
{
    return staticBaseAddress() + imageSlide();
}

size_t machOTextSize(void)
{
    const segment_command_64* segment = textSegment();
    return segment ? static_cast<size_t>(segment->vmsize) : 0;
}
