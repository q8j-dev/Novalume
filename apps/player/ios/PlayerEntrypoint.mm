#include "../PlayerMain.h"

#import <UIKit/UIKit.h>

#include <cstdlib>
#include <exception>

namespace {

int playerArgumentCount = 0;
char** playerArguments = nullptr;

}

@interface RbxPlayerApplicationDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation RbxPlayerApplicationDelegate

- (BOOL)application:(UIApplication*)application
    didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
    dispatch_async(dispatch_get_main_queue(), ^{
        int result = 1;
        @try {
            try {
                result = rbxPlayerMain(playerArgumentCount, playerArguments);
            } catch (const std::exception& error) {
                NSLog(@"RobloxPlayer: %s", error.what());
            } catch (...) {
                NSLog(@"RobloxPlayer: unknown fatal error");
            }
        } @catch (NSException* exception) {
            NSLog(@"RobloxPlayer: %@", exception);
        }
        std::exit(result);
    });
    return YES;
}

@end

int main(int argc, char** argv)
{
    playerArgumentCount = argc;
    playerArguments = argv;
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil,
            NSStringFromClass([RbxPlayerApplicationDelegate class]));
    }
}
