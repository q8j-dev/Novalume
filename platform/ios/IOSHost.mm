#include "rbx/platform/IOSHost.h"
#include "rbx/platform/RecentDocuments.h"

#import <GameController/GameController.h>
#import <QuartzCore/CAMetalLayer.h>
#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <stdexcept>
#include <utility>

typedef void (*RbxTouchCallback)(void*, std::uintptr_t, float, float, bool, bool, bool);
typedef void (*RbxDocumentCallback)(void*, const char*);

@interface RbxIOSMetalView : UIView {
@public
    void* context;
    RbxTouchCallback callback;
}
@end

@interface RbxIOSDocumentDelegate : NSObject <UIDocumentPickerDelegate> {
@public
    void* context;
    RbxDocumentCallback callback;
}
@end

@implementation RbxIOSMetalView
+ (Class)layerClass
{
    return [CAMetalLayer class];
}

- (void)forwardTouches:(NSSet<UITouch*>*)touches ended:(BOOL)ended moved:(BOOL)moved cancelled:(BOOL)cancelled
{
    for (UITouch* touch in touches) {
        CGPoint point = [touch locationInView:self];
        if (callback)
            callback(context, reinterpret_cast<std::uintptr_t>(touch), point.x,
                point.y, ended, moved, cancelled);
    }
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    [self forwardTouches:touches ended:NO moved:NO cancelled:NO];
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    [self forwardTouches:touches ended:NO moved:YES cancelled:NO];
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    [self forwardTouches:touches ended:YES moved:NO cancelled:NO];
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    [self forwardTouches:touches ended:NO moved:NO cancelled:YES];
}
@end

@implementation RbxIOSDocumentDelegate
- (void)documentPicker:(UIDocumentPickerViewController*)controller
    didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls
{
    for (NSURL* url in urls) {
        const char* path = [[url path] fileSystemRepresentation];
        if (callback && path)
            callback(context, path);
    }
}
@end

namespace rbx::platform {
namespace {

class IOSHost final : public Host {
public:
    IOSHost(std::uint32_t width, std::uint32_t height, bool visible)
    {
        application_ = [UIApplication sharedApplication];
        const CGRect screenBounds = [[UIScreen mainScreen] bounds];
        window_ = [[UIWindow alloc] initWithFrame:screenBounds];
        controller_ = [[UIViewController alloc] init];
        view_ = [[RbxIOSMetalView alloc] initWithFrame:screenBounds];
        view_->context = this;
        view_->callback = &IOSHost::queueTouch;
        [view_ setMultipleTouchEnabled:YES];
        [view_ setUserInteractionEnabled:YES];
        [controller_ setView:view_];
        [window_ setRootViewController:controller_];
        if (visible) {
            [window_ makeKeyAndVisible];
            focused_ = true;
            events_.push_back(InputEvent{.kind = InputEvent::Kind::focusGained});
        }
        documentDelegate_ = [[RbxIOSDocumentDelegate alloc] init];
        documentDelegate_->context = this;
        documentDelegate_->callback = &IOSHost::queueDocument;
        [GCController setShouldMonitorBackgroundEvents:YES];
        [GCController startWirelessControllerDiscoveryWithCompletionHandler:nil];
        if (width != 0 && height != 0)
            [view_ setContentScaleFactor:[[UIScreen mainScreen] scale]];
    }

    ~IOSHost() override
    {
        [GCController stopWirelessControllerDiscovery];
        for (ControllerState& controller : controllers_)
            [controller.controller release];
        [window_ setRootViewController:nil];
        [view_ release];
        [controller_ release];
        [documentDelegate_ release];
        [window_ release];
    }

    NativeSurface nativeSurface() const noexcept override
    {
        const CGRect bounds = [view_ bounds];
        const CGFloat scale = [view_ contentScaleFactor];
        const UIEdgeInsets safeArea = [view_ safeAreaInsets];
        CAMetalLayer* layer = (CAMetalLayer*)[view_ layer];
        [layer setContentsScale:scale];
        [layer setDrawableSize:CGSizeMake(bounds.size.width * scale,
                                          bounds.size.height * scale)];
        DisplayOrientation orientation = displayOrientationForDimensions(
            static_cast<std::uint32_t>(bounds.size.width),
            static_cast<std::uint32_t>(bounds.size.height));
        UIWindowScene* scene = [window_ windowScene];
        if (scene) {
            switch ([scene interfaceOrientation]) {
            case UIInterfaceOrientationLandscapeRight:
                orientation = DisplayOrientation::landscapeRight;
                break;
            case UIInterfaceOrientationLandscapeLeft:
                orientation = DisplayOrientation::landscapeLeft;
                break;
            case UIInterfaceOrientationPortrait:
            case UIInterfaceOrientationPortraitUpsideDown:
                orientation = DisplayOrientation::portrait;
                break;
            default:
                break;
            }
        }
        return NativeSurface{
            .window = reinterpret_cast<std::uintptr_t>(view_),
            .display = 0,
            .graphicsContext = 0,
            .width = static_cast<std::uint32_t>(bounds.size.width * scale),
            .height = static_cast<std::uint32_t>(bounds.size.height * scale),
            .logicalWidth = static_cast<std::uint32_t>(bounds.size.width),
            .logicalHeight = static_cast<std::uint32_t>(bounds.size.height),
            .pixelDensity = static_cast<float>(scale),
            .safeArea = {
                .left = static_cast<float>(safeArea.left),
                .top = static_cast<float>(safeArea.top),
                .right = static_cast<float>(safeArea.right),
                .bottom = static_cast<float>(safeArea.bottom)},
            .orientation = orientation};
    }

    std::filesystem::path resourceRoot() const override
    {
        NSString* path = [[NSBundle mainBundle] resourcePath];
        return path ? std::filesystem::path([path fileSystemRepresentation])
                    : std::filesystem::current_path();
    }

    std::filesystem::path writableDataRoot() const override
    {
        NSArray<NSURL*>* urls = [[NSFileManager defaultManager]
            URLsForDirectory:NSApplicationSupportDirectory
                   inDomains:NSUserDomainMask];
        NSURL* url = [urls firstObject];
        return url ? std::filesystem::path([[url path] fileSystemRepresentation])
                   : std::filesystem::temp_directory_path();
    }

    bool pumpEvents() override
    {
        const bool focused = [application_ applicationState] == UIApplicationStateActive;
        if (focused != focused_) {
            focused_ = focused;
            events_.push_back(InputEvent{
                .kind = focused ? InputEvent::Kind::focusGained
                                : InputEvent::Kind::focusLost});
        }
        pollGamepads();
        [[NSRunLoop mainRunLoop] runMode:NSDefaultRunLoopMode
                              beforeDate:[NSDate date]];
        return true;
    }

    std::vector<InputEvent> takeInputEvents() override
    {
        return std::exchange(events_, {});
    }

    void requestOpenDocument() override
    {
        UIDocumentPickerViewController* picker = [[UIDocumentPickerViewController alloc]
            initForOpeningContentTypes:@[UTTypeData] asCopy:YES];
        [picker setDelegate:documentDelegate_];
        [picker setAllowsMultipleSelection:NO];
        [controller_ presentViewController:picker animated:YES completion:nil];
        [picker release];
    }

    std::vector<std::filesystem::path> takeOpenedDocuments() override
    {
        return std::exchange(openedDocuments_, {});
    }

    std::vector<std::filesystem::path> recentDocuments() const override
    {
        return loadRecentDocuments(writableDataRoot());
    }

    bool launchDocument(const std::filesystem::path& path) override
    {
        std::error_code error;
        if (!std::filesystem::is_regular_file(path, error) || error)
            return false;
        NSString* value = [NSString stringWithUTF8String:path.string().c_str()];
        if (!value)
            return false;
        NSURL* url = [NSURL fileURLWithPath:value];
        if (![application_ canOpenURL:url])
            return false;
        recordRecentDocument(writableDataRoot(), path);
        [application_ openURL:url options:@{} completionHandler:nil];
        return true;
    }

    bool openExternalUri(std::string_view uri) override
    {
        NSString* value = [[NSString alloc] initWithBytes:uri.data()
                                                   length:uri.size()
                                                 encoding:NSUTF8StringEncoding];
        NSURL* url = value ? [NSURL URLWithString:value] : nil;
        const bool accepted = url && [application_ canOpenURL:url];
        if (accepted)
            [application_ openURL:url options:@{} completionHandler:nil];
        [value release];
        return accepted;
    }

    void setClipboardText(std::string_view text) override
    {
        NSString* value = [[NSString alloc] initWithBytes:text.data()
                                                   length:text.size()
                                                 encoding:NSUTF8StringEncoding];
        if (value)
            [[UIPasteboard generalPasteboard] setString:value];
        [value release];
    }

    void setPointerLock(bool locked) override
    {
        (void)locked;
    }

private:
    struct ControllerState {
        GCController* controller = nil;
        std::array<bool, 14> buttons{};
        float leftX = 0.0F;
        float leftY = 0.0F;
        float rightX = 0.0F;
        float rightY = 0.0F;
        float leftTrigger = 0.0F;
        float rightTrigger = 0.0F;
    };

    static void queueTouch(void* context, std::uintptr_t touchId, float x, float y,
                           bool ended, bool moved, bool cancelled)
    {
        IOSHost* host = static_cast<IOSHost*>(context);
        host->events_.push_back(InputEvent{
            .kind = cancelled ? InputEvent::Kind::touchCancel
                              : ended ? InputEvent::Kind::touchUp
                                      : moved ? InputEvent::Kind::touchMove
                                              : InputEvent::Kind::touchDown,
            .touchId = touchId,
            .x = x,
            .y = y});
    }

    static void queueDocument(void* context, const char* path)
    {
        IOSHost* host = static_cast<IOSHost*>(context);
        host->openedDocuments_.emplace_back(path);
    }

    void syncGamepads()
    {
        NSArray<GCController*>* available = [GCController controllers];
        for (ControllerState& state : controllers_) {
            if (state.controller && ![available containsObject:state.controller]) {
                [state.controller release];
                state = {};
            }
        }
        for (GCController* controller in available) {
            bool known = false;
            for (const ControllerState& state : controllers_)
                known |= state.controller == controller;
            if (known)
                continue;
            for (ControllerState& state : controllers_) {
                if (!state.controller) {
                    state.controller = [controller retain];
                    break;
                }
            }
        }
    }

    void updateButton(std::size_t slot, std::size_t index,
                      InputEvent::GamepadControl control, bool pressed)
    {
        ControllerState& state = controllers_[slot];
        if (state.buttons[index] == pressed)
            return;
        state.buttons[index] = pressed;
        events_.push_back(InputEvent{
            .kind = pressed ? InputEvent::Kind::gamepadButtonDown
                            : InputEvent::Kind::gamepadButtonUp,
            .gamepadControl = control,
            .gamepadIndex = static_cast<std::uint8_t>(slot),
            .x = pressed ? 1.0F : 0.0F});
    }

    void updateAxis(std::size_t slot, InputEvent::GamepadControl control,
                    float x, float y = 0.0F)
    {
        events_.push_back(InputEvent{
            .kind = InputEvent::Kind::gamepadAxis,
            .gamepadControl = control,
            .gamepadIndex = static_cast<std::uint8_t>(slot),
            .x = x,
            .y = y});
    }

    void pollGamepads()
    {
        syncGamepads();
        using Control = InputEvent::GamepadControl;
        for (std::size_t slot = 0; slot < controllers_.size(); ++slot) {
            ControllerState& state = controllers_[slot];
            GCExtendedGamepad* pad = [state.controller extendedGamepad];
            if (!pad)
                continue;
            updateButton(slot, 0, Control::buttonA, [[pad buttonA] isPressed]);
            updateButton(slot, 1, Control::buttonB, [[pad buttonB] isPressed]);
            updateButton(slot, 2, Control::buttonX, [[pad buttonX] isPressed]);
            updateButton(slot, 3, Control::buttonY, [[pad buttonY] isPressed]);
            updateButton(slot, 4, Control::leftShoulder, [[pad leftShoulder] isPressed]);
            updateButton(slot, 5, Control::rightShoulder, [[pad rightShoulder] isPressed]);
            updateButton(slot, 6, Control::leftStick, [[pad leftThumbstickButton] isPressed]);
            updateButton(slot, 7, Control::rightStick, [[pad rightThumbstickButton] isPressed]);
            updateButton(slot, 8, Control::start, [[pad buttonMenu] isPressed]);
            updateButton(slot, 9, Control::select, [[pad buttonOptions] isPressed]);
            updateButton(slot, 10, Control::dpadLeft, [[[pad dpad] left] isPressed]);
            updateButton(slot, 11, Control::dpadRight, [[[pad dpad] right] isPressed]);
            updateButton(slot, 12, Control::dpadUp, [[[pad dpad] up] isPressed]);
            updateButton(slot, 13, Control::dpadDown, [[[pad dpad] down] isPressed]);

            const float leftX = [[[pad leftThumbstick] xAxis] value];
            const float leftY = [[[pad leftThumbstick] yAxis] value];
            if (std::abs(leftX - state.leftX) > 0.0001F ||
                std::abs(leftY - state.leftY) > 0.0001F) {
                state.leftX = leftX;
                state.leftY = leftY;
                updateAxis(slot, Control::leftStick, leftX, leftY);
            }
            const float rightX = [[[pad rightThumbstick] xAxis] value];
            const float rightY = [[[pad rightThumbstick] yAxis] value];
            if (std::abs(rightX - state.rightX) > 0.0001F ||
                std::abs(rightY - state.rightY) > 0.0001F) {
                state.rightX = rightX;
                state.rightY = rightY;
                updateAxis(slot, Control::rightStick, rightX, rightY);
            }
            const float leftTrigger = [[pad leftTrigger] value];
            if (std::abs(leftTrigger - state.leftTrigger) > 0.0001F) {
                state.leftTrigger = leftTrigger;
                updateAxis(slot, Control::leftTrigger, leftTrigger);
            }
            const float rightTrigger = [[pad rightTrigger] value];
            if (std::abs(rightTrigger - state.rightTrigger) > 0.0001F) {
                state.rightTrigger = rightTrigger;
                updateAxis(slot, Control::rightTrigger, rightTrigger);
            }
        }
    }

    UIApplication* application_ = nil;
    UIWindow* window_ = nil;
    UIViewController* controller_ = nil;
    RbxIOSMetalView* view_ = nil;
    RbxIOSDocumentDelegate* documentDelegate_ = nil;
    bool focused_ = false;
    std::vector<InputEvent> events_;
    std::vector<std::filesystem::path> openedDocuments_;
    std::array<ControllerState, 8> controllers_{};
};

}

std::unique_ptr<Host> createIOSHost(std::uint32_t width, std::uint32_t height,
                                    bool visible)
{
    return std::make_unique<IOSHost>(width, height, visible);
}

std::unique_ptr<Host> createHost(std::uint32_t width, std::uint32_t height,
                                 bool visible)
{
    return createIOSHost(width, height, visible);
}

}
