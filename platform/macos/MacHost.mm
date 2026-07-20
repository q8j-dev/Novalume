#include "rbx/platform/MacHost.h"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <QuartzCore/CAMetalLayer.h>

#include <algorithm>
#include <stdexcept>
#include <utility>

typedef void (*RbxWindowCloseCallback)(void* context);
typedef void (*RbxDocumentOpenCallback)(void* context, const char* path);

@interface RbxMacWindowDelegate : NSObject <NSWindowDelegate> {
@public
    void* context;
    RbxWindowCloseCallback callback;
    BOOL allowClose;
}
@end

@interface RbxMacApplicationDelegate : NSObject <NSApplicationDelegate> {
@public
    void* context;
    RbxDocumentOpenCallback callback;
}
@end

@implementation RbxMacApplicationDelegate
- (void)application:(NSApplication*)application openFiles:(NSArray<NSString*>*)filenames
{
    BOOL accepted = NO;
    for (NSString* filename in filenames) {
        const char* path = [filename fileSystemRepresentation];
        if (callback && path) {
            callback(context, path);
            accepted = YES;
        }
    }
    [application replyToOpenOrPrint:accepted
        ? NSApplicationDelegateReplySuccess
        : NSApplicationDelegateReplyFailure];
}
@end

@implementation RbxMacWindowDelegate
- (BOOL)windowShouldClose:(id)sender
{
    if (allowClose)
        return YES;
    if (callback)
        callback(context);
    return NO;
}
@end

namespace rbx::platform {
namespace {

InputEvent::Key translateKey(NSEvent* event)
{
    NSString* characters = [event charactersIgnoringModifiers];
    if ([characters length] == 0)
        return InputEvent::Key::unknown;

    const unichar value = [characters characterAtIndex:0];
    if (value >= 'a' && value <= 'z')
        return static_cast<InputEvent::Key>(
            static_cast<unsigned int>(InputEvent::Key::a) + value - 'a');
    if (value >= 'A' && value <= 'Z')
        return static_cast<InputEvent::Key>(
            static_cast<unsigned int>(InputEvent::Key::a) + value - 'A');
    if (value >= '0' && value <= '9')
        return static_cast<InputEvent::Key>(
            static_cast<unsigned int>(InputEvent::Key::zero) + value - '0');

    switch (value) {
    case NSBackspaceCharacter: return InputEvent::Key::backspace;
    case NSTabCharacter: return InputEvent::Key::tab;
    case NSCarriageReturnCharacter:
    case NSEnterCharacter: return InputEvent::Key::enter;
    case 0x1b: return InputEvent::Key::escape;
    case ' ': return InputEvent::Key::space;
    case '\'': return InputEvent::Key::quote;
    case ',': return InputEvent::Key::comma;
    case '-': return InputEvent::Key::minus;
    case '.': return InputEvent::Key::period;
    case '/': return InputEvent::Key::slash;
    case ';': return InputEvent::Key::semicolon;
    case '=': return InputEvent::Key::equals;
    case '[': return InputEvent::Key::leftBracket;
    case '\\': return InputEvent::Key::backslash;
    case ']': return InputEvent::Key::rightBracket;
    case '`': return InputEvent::Key::backquote;
    case NSLeftArrowFunctionKey: return InputEvent::Key::left;
    case NSRightArrowFunctionKey: return InputEvent::Key::right;
    case NSUpArrowFunctionKey: return InputEvent::Key::up;
    case NSDownArrowFunctionKey: return InputEvent::Key::down;
    case NSF1FunctionKey: return InputEvent::Key::f1;
    case NSF2FunctionKey: return InputEvent::Key::f2;
    case NSF3FunctionKey: return InputEvent::Key::f3;
    case NSF4FunctionKey: return InputEvent::Key::f4;
    case NSF5FunctionKey: return InputEvent::Key::f5;
    case NSF6FunctionKey: return InputEvent::Key::f6;
    case NSF7FunctionKey: return InputEvent::Key::f7;
    case NSF8FunctionKey: return InputEvent::Key::f8;
    case NSF9FunctionKey: return InputEvent::Key::f9;
    case NSF10FunctionKey: return InputEvent::Key::f10;
    case NSF11FunctionKey: return InputEvent::Key::f11;
    case NSF12FunctionKey: return InputEvent::Key::f12;
    default: return InputEvent::Key::unknown;
    }
}

std::uint32_t translateModifiers(NSEventModifierFlags flags)
{
    std::uint32_t result = 0;
    result |= (flags & NSEventModifierFlagShift) ? 1U << 0U : 0U;
    result |= (flags & NSEventModifierFlagControl) ? 1U << 1U : 0U;
    result |= (flags & NSEventModifierFlagOption) ? 1U << 2U : 0U;
    result |= (flags & NSEventModifierFlagCommand) ? 1U << 3U : 0U;
    result |= (flags & NSEventModifierFlagCapsLock) ? 1U << 4U : 0U;
    return result;
}

class MacHost final : public Host {
public:
    MacHost(std::uint32_t width, std::uint32_t height, bool visible)
        : visible_(visible) {
        [NSApplication sharedApplication];
        applicationDelegate_ = [[RbxMacApplicationDelegate alloc] init];
        applicationDelegate_->context = this;
        applicationDelegate_->callback = &MacHost::queueOpenedDocument;
        [NSApp setDelegate:applicationDelegate_];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        [NSApp finishLaunching];
        const NSRect rect = NSMakeRect(0, 0, static_cast<CGFloat>(width),
                                      static_cast<CGFloat>(height));
        window_ = [[NSWindow alloc]
            initWithContentRect:rect
                      styleMask:(NSWindowStyleMaskTitled |
                                 NSWindowStyleMaskClosable |
                                 NSWindowStyleMaskResizable)
                        backing:NSBackingStoreBuffered
                          defer:NO];
        if (window_ == nil) {
            throw std::runtime_error("unable to create macOS player window");
        }
        windowDelegate_ = [[RbxMacWindowDelegate alloc] init];
        windowDelegate_->context = this;
        windowDelegate_->callback = &MacHost::requestNativeClose;
        windowDelegate_->allowClose = NO;
        [window_ setDelegate:windowDelegate_];
        [window_ setTitle:@"Roblox"];
        [window_ center];
        [[window_ contentView] setWantsLayer:YES];
        CAMetalLayer* metalLayer = [CAMetalLayer layer];
        [metalLayer setOpaque:YES];
        CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
        [metalLayer setColorspace:colorSpace];
        CGColorSpaceRelease(colorSpace);
        [[window_ contentView] setLayer:metalLayer];
        if (visible) {
            [window_ makeKeyAndOrderFront:nil];
            [NSApp activateIgnoringOtherApps:YES];
            [window_ makeFirstResponder:[window_ contentView]];
            events_.push_back(InputEvent{.kind = InputEvent::Kind::focusGained});
        }
    }

    ~MacHost() override {
        setPointerLock(false);
        setCursorHidden(false);
        windowDelegate_->allowClose = YES;
        [window_ close];
        [window_ setDelegate:nil];
        [windowDelegate_ release];
        [window_ release];
        [NSApp setDelegate:nil];
        [applicationDelegate_ release];
    }

    NativeSurface nativeSurface() const noexcept override {
        const NSView* view = [window_ contentView];
        const NSRect points = [view bounds];
        const CGFloat scale = [window_ backingScaleFactor];
        const NSEdgeInsets safeArea = [view safeAreaInsets];
        return NativeSurface{
            .window = reinterpret_cast<std::uintptr_t>(const_cast<NSView*>(view)),
            .display = 0,
            .graphicsContext = 0,
            .width = static_cast<std::uint32_t>(points.size.width * scale),
            .height = static_cast<std::uint32_t>(points.size.height * scale),
            .logicalWidth = static_cast<std::uint32_t>(points.size.width),
            .logicalHeight = static_cast<std::uint32_t>(points.size.height),
            .pixelDensity = static_cast<float>(scale),
            .safeArea = {
                .left = static_cast<float>(safeArea.left),
                .top = static_cast<float>(safeArea.top),
                .right = static_cast<float>(safeArea.right),
                .bottom = static_cast<float>(safeArea.bottom)}};
    }

    std::filesystem::path resourceRoot() const override {
        NSString* path = [[NSBundle mainBundle] resourcePath];
        return path ? std::filesystem::path([path fileSystemRepresentation])
                    : std::filesystem::current_path();
    }

    std::filesystem::path writableDataRoot() const override {
        NSArray<NSURL*>* urls = [[NSFileManager defaultManager]
            URLsForDirectory:NSApplicationSupportDirectory
                   inDomains:NSUserDomainMask];
        return std::filesystem::path([[[urls firstObject] path] fileSystemRepresentation]);
    }

    std::filesystem::path existingClientSettingsRoot() const override {
        NSString* home = NSHomeDirectory();
        if (!home)
            return {};
        return std::filesystem::path([home fileSystemRepresentation]) /
            "Library" / "Roblox" / "ClientSettings";
    }

    bool pumpEvents() override {
        for (;;) {
            NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                                untilDate:[NSDate distantPast]
                                                   inMode:NSDefaultRunLoopMode
                                                  dequeue:YES];
            if (event == nil) {
                break;
            }
            const NSEventType type = [event type];
            if (type == NSEventTypeKeyDown || type == NSEventTypeKeyUp) {
                const bool down = type == NSEventTypeKeyDown;
                if (([event modifierFlags] & NSEventModifierFlagCommand) != 0 &&
                    translateKey(event) == InputEvent::Key::o) {
                    if (down && ![event isARepeat])
                        requestOpenDocument();
                    continue;
                }
                NSString* text = [event characters];
                events_.push_back(InputEvent{
                    .kind = down ? InputEvent::Kind::keyDown : InputEvent::Kind::keyUp,
                    .key = translateKey(event),
                    .modifiers = translateModifiers([event modifierFlags]),
                    .text = static_cast<char>(
                        down && [text length] == 1 && [text characterAtIndex:0] < 128
                            ? [text characterAtIndex:0] : 0),
                    .repeat = static_cast<bool>([event isARepeat])});
                // AppKit beeps when an otherwise valid game key has no native
                // text responder. Player owns these events, so do not forward them.
                continue;
            }
            if (type == NSEventTypeMouseMoved || type == NSEventTypeLeftMouseDragged ||
                type == NSEventTypeRightMouseDragged || type == NSEventTypeOtherMouseDragged) {
                enqueuePointer(event, InputEvent::Kind::pointerMove,
                    InputEvent::PointerButton::none);
            } else if (type == NSEventTypeLeftMouseDown || type == NSEventTypeRightMouseDown ||
                       type == NSEventTypeOtherMouseDown) {
                enqueuePointer(event, InputEvent::Kind::pointerDown, pointerButton(type));
            } else if (type == NSEventTypeLeftMouseUp || type == NSEventTypeRightMouseUp ||
                       type == NSEventTypeOtherMouseUp) {
                enqueuePointer(event, InputEvent::Kind::pointerUp, pointerButton(type));
            } else if (type == NSEventTypeScrollWheel) {
                enqueuePointer(event, InputEvent::Kind::scroll,
                    InputEvent::PointerButton::none);
            } else if (type == NSEventTypeAppKitDefined) {
                if ([NSApp isActive] && !focused_) {
                    focused_ = true;
                    events_.push_back(InputEvent{.kind = InputEvent::Kind::focusGained});
                } else if (![NSApp isActive] && focused_) {
                    focused_ = false;
                    events_.push_back(InputEvent{.kind = InputEvent::Kind::focusLost});
                }
            }
            [NSApp sendEvent:event];
        }
        updatePointerLock();
        return !visible_ || [window_ isVisible];
    }

    std::vector<InputEvent> takeInputEvents() override {
        std::vector<InputEvent> result;
        result.swap(events_);
        return result;
    }

    void requestOpenDocument() override {
        setPointerLock(false);
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];
        [panel setResolvesAliases:YES];
        [panel setAllowedFileTypes:@[@"rbxl", @"rbxlx", @"rbxm", @"rbxmx", @"rbxlp"]];
        if ([panel runModal] != NSModalResponseOK)
            return;
        NSURL* url = [[panel URLs] firstObject];
        if (url && [url isFileURL])
            queueOpenedDocument(this, [[url path] fileSystemRepresentation]);
    }

    std::vector<std::filesystem::path> takeOpenedDocuments() override {
        std::vector<std::filesystem::path> result;
        result.swap(openedDocuments_);
        return result;
    }

    bool launchDocument(const std::filesystem::path& path) override {
        if (!std::filesystem::is_regular_file(path))
            return false;
        NSString* executable = [[NSBundle mainBundle] executablePath];
        const std::string nativePath = path.string();
        NSString* document = [[NSString alloc]
            initWithBytes:nativePath.data()
                   length:nativePath.size()
                 encoding:NSUTF8StringEncoding];
        if (!executable || !document) {
            [document release];
            return false;
        }

        NSTask* task = [[NSTask alloc] init];
        [task setLaunchPath:executable];
        [task setArguments:@[@"--place", document]];
        NSError* error = nil;
        const BOOL launched = [task launchAndReturnError:&error];
        if (!launched)
            NSLog(@"Unable to launch Roblox document %@: %@", document,
                [error localizedDescription]);
        [task release];
        [document release];
        return launched == YES;
    }

    void setClipboardText(std::string_view text) override {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard clearContents];
        NSString* value = [[NSString alloc] initWithBytes:text.data()
                                                   length:text.size()
                                                 encoding:NSUTF8StringEncoding];
        [pasteboard setString:value forType:NSPasteboardTypeString];
        [value release];
    }

    void setPointerLock(bool locked) override {
        pointerLockRequested_ = locked;
        updatePointerLock();
    }

private:
    static void requestNativeClose(void* context) {
        MacHost* host = static_cast<MacHost*>(context);
        host->events_.push_back(InputEvent{
            .kind = InputEvent::Kind::nativeCloseRequested});
    }

    static void queueOpenedDocument(void* context, const char* path) {
        if (!context || !path)
            return;
        static_cast<MacHost*>(context)->openedDocuments_.emplace_back(path);
    }

    void setCursorHidden(bool hidden) {
        if (cursorHidden_ == hidden)
            return;
        cursorHidden_ = hidden;
        // NSCursor's hide count can be undone by AppKit cursor-rect updates
        // while the Metal view is first responder.  CoreGraphics owns the
        // actual hardware cursor and remains hidden across those updates.
        NSScreen* screen = [window_ screen] ?: [NSScreen mainScreen];
        NSNumber* screenNumber = [[screen deviceDescription]
            objectForKey:@"NSScreenNumber"];
        const CGDirectDisplayID display = screenNumber
            ? static_cast<CGDirectDisplayID>([screenNumber unsignedIntValue])
            : CGMainDisplayID();
        if (hidden) {
            hiddenDisplay_ = display;
            CGDisplayHideCursor(display);
        } else {
            CGDisplayShowCursor(hiddenDisplay_ ? hiddenDisplay_ : display);
            hiddenDisplay_ = 0;
        }
    }

    void updatePointerLock() {
        const bool appFocused = [NSApp isActive] && [window_ isKeyWindow];
        if (focused_ != appFocused) {
            focused_ = appFocused;
            events_.push_back(InputEvent{.kind = focused_
                ? InputEvent::Kind::focusGained
                : InputEvent::Kind::focusLost});
        }
        const bool shouldLock = pointerLockRequested_ && focused_ &&
            [NSApp isActive] && [window_ isKeyWindow];
        if (pointerLocked_ != shouldLock) {
            if (shouldLock) {
                NSView* contentView = [window_ contentView];
                pointerLockPoint_ = [contentView
                    convertPoint:[window_ mouseLocationOutsideOfEventStream] fromView:nil];
                pointerRestorePoint_ = [window_ convertPointToScreen:
                    [contentView convertPoint:pointerLockPoint_ toView:nil]];
                pointerRestoreValid_ = true;
                CGAssociateMouseAndMouseCursorPosition(false);
                const NSRect contentBounds = [[window_ contentView] bounds];
                const NSPoint centerInWindow = [[window_ contentView]
                    convertPoint:NSMakePoint(NSMidX(contentBounds), NSMidY(contentBounds))
                    toView:nil];
                const NSPoint centerOnScreen = [window_ convertPointToScreen:centerInWindow];
                warpCursorToScreenPoint(centerOnScreen);
            } else {
                if (pointerRestoreValid_)
                    warpCursorToScreenPoint(pointerRestorePoint_);
                CGAssociateMouseAndMouseCursorPosition(true);
                pointerRestoreValid_ = false;
            }
            pointerLocked_ = shouldLock;
            suppressNextWarpMotion_ = true;
        }
        const NSPoint point = [[window_ contentView]
            convertPoint:[window_ mouseLocationOutsideOfEventStream] fromView:nil];
        const bool insideContent = NSPointInRect(point, [[window_ contentView] bounds]);
        // Roblox renders its own cursor through the 2D adorn pass. Suppress the
        // AppKit cursor over the game surface even when movement is not locked.
        setCursorHidden(visible_ && focused_ && (shouldLock || insideContent));
    }

    void warpCursorToScreenPoint(NSPoint point) {
        NSScreen* screen = [window_ screen] ?: [NSScreen mainScreen];
        NSNumber* screenNumber = [[screen deviceDescription]
            objectForKey:@"NSScreenNumber"];
        const CGDirectDisplayID display = screenNumber
            ? static_cast<CGDirectDisplayID>([screenNumber unsignedIntValue])
            : CGMainDisplayID();
        const CGRect displayBounds = CGDisplayBounds(display);
        const NSRect screenFrame = [screen frame];
        const CGPoint globalPoint = CGPointMake(
            displayBounds.origin.x + point.x - NSMinX(screenFrame),
            displayBounds.origin.y + NSMaxY(screenFrame) - point.y);
        CGWarpMouseCursorPosition(globalPoint);
    }

    static InputEvent::PointerButton pointerButton(NSEventType type) {
        if (type == NSEventTypeLeftMouseDown || type == NSEventTypeLeftMouseUp)
            return InputEvent::PointerButton::primary;
        if (type == NSEventTypeRightMouseDown || type == NSEventTypeRightMouseUp)
            return InputEvent::PointerButton::secondary;
        return InputEvent::PointerButton::middle;
    }

    void enqueuePointer(NSEvent* event, InputEvent::Kind kind,
                        InputEvent::PointerButton button) {
        if ([event window] != window_)
            return;
        if (kind == InputEvent::Kind::pointerMove && suppressNextWarpMotion_) {
            suppressNextWarpMotion_ = false;
            return;
        }
        NSPoint point = [[window_ contentView] convertPoint:[event locationInWindow]
                                                   fromView:nil];
        const CGFloat height = [[window_ contentView] bounds].size.height;
        if (kind == InputEvent::Kind::pointerMove && !pointerLocked_ &&
            !NSPointInRect(point, [[window_ contentView] bounds]))
            return;
        if (pointerLocked_ && kind == InputEvent::Kind::pointerMove)
            point = pointerLockPoint_;
        const float scrollY = kind == InputEvent::Kind::scroll
            ? static_cast<float>([event hasPreciseScrollingDeltas]
                ? std::clamp([event scrollingDeltaY] * 0.1, -1.0, 1.0)
                : std::clamp([event scrollingDeltaY], -1.0, 1.0))
            : static_cast<float>([event deltaY]);
        events_.push_back(InputEvent{
            .kind = kind,
            .button = button,
            .x = static_cast<float>(point.x),
            .y = static_cast<float>(height - point.y),
            .deltaX = static_cast<float>(kind == InputEvent::Kind::scroll
                ? [event scrollingDeltaX] : [event deltaX]),
            .deltaY = scrollY,
            .modifiers = translateModifiers([event modifierFlags])});
    }

    NSWindow* window_ = nil;
    RbxMacWindowDelegate* windowDelegate_ = nil;
    RbxMacApplicationDelegate* applicationDelegate_ = nil;
    bool visible_ = false;
    bool focused_ = true;
    bool pointerLockRequested_ = false;
    bool pointerLocked_ = false;
    bool cursorHidden_ = false;
    bool pointerRestoreValid_ = false;
    bool suppressNextWarpMotion_ = false;
    NSPoint pointerLockPoint_ = NSZeroPoint;
    NSPoint pointerRestorePoint_ = NSZeroPoint;
    CGDirectDisplayID hiddenDisplay_ = 0;
    std::vector<InputEvent> events_;
    std::vector<std::filesystem::path> openedDocuments_;
};

} // namespace

std::unique_ptr<Host> createMacHost(std::uint32_t width, std::uint32_t height,
                                    bool visible) {
    return std::make_unique<MacHost>(width, height, visible);
}

std::unique_ptr<Host> createHost(std::uint32_t width, std::uint32_t height,
                                 bool visible) {
    return createMacHost(width, height, visible);
}

} // namespace rbx::platform
