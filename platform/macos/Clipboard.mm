#include "rbx/platform/Clipboard.h"

#import <AppKit/AppKit.h>

namespace rbx::platform {

std::string readClipboardText()
{
    NSString* value = [[NSPasteboard generalPasteboard]
        stringForType:NSPasteboardTypeString];
    return value ? std::string([value UTF8String]) : std::string();
}

} // namespace rbx::platform
