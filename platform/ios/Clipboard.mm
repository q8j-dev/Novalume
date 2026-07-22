#include "rbx/platform/Clipboard.h"

#import <UIKit/UIKit.h>

namespace rbx::platform {

std::string readClipboardText()
{
    NSString* value = [[UIPasteboard generalPasteboard] string];
    return value ? std::string([value UTF8String]) : std::string();
}

}
