#include "platform_menu.h"
#import <AppKit/AppKit.h>



@interface FreeGAGMenuTarget : NSObject
- (void)showSettings:(id)sender;
- (void)showControls:(id)sender;
- (void)returnToLauncher:(id)sender;
@end

@implementation FreeGAGMenuTarget
- (void)showSettings:(id)sender
{
    (void)sender;
    freegag::post_platform_menu_command(freegag::PlatformMenuCommand::SETTINGS);
}

- (void)showControls:(id)sender
{
    (void)sender;
    freegag::post_platform_menu_command(freegag::PlatformMenuCommand::CONTROLS);
}

- (void)returnToLauncher:(id)sender
{
    (void)sender;
    freegag::post_platform_menu_command(freegag::PlatformMenuCommand::RETURN_TO_LAUNCHER);
}
@end

namespace freegag
{

FreeGAGMenuTarget *platform_menu_target;

static NSMenuItem *menu_item(NSString *title, SEL action, NSString *key, id target)
{
    NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:title action:action keyEquivalent:key];
    item.target = target;
    return item;
}

bool install_native_platform_menu()
{
    @autoreleasepool
    {
        NSApplication *application = NSApplication.sharedApplication;
        if(application == nil)
            return false;

        platform_menu_target = [FreeGAGMenuTarget new];
        NSMenu *main_menu = [[NSMenu alloc] initWithTitle:@""];
        NSMenuItem *application_root = [[NSMenuItem alloc] initWithTitle:@"FreeGAG" action:nil keyEquivalent:@""];
        NSMenu *application_menu = [[NSMenu alloc] initWithTitle:@"FreeGAG"];
        [application_menu addItem:menu_item(@"About FreeGAG", @selector(orderFrontStandardAboutPanel:), @"", application)];
        [application_menu addItem:NSMenuItem.separatorItem];
        NSMenuItem *settings = menu_item(@"Settings…", @selector(showSettings:), @",", platform_menu_target);
        settings.keyEquivalentModifierMask = NSEventModifierFlagCommand;
        [application_menu addItem:settings];
        [application_menu addItem:menu_item(@"Controls…", @selector(showControls:), @"", platform_menu_target)];
        [application_menu addItem:NSMenuItem.separatorItem];
        [application_menu addItem:menu_item(@"Hide FreeGAG", @selector(hide:), @"h", application)];
        NSMenuItem *hide_others = menu_item(@"Hide Others", @selector(hideOtherApplications:), @"h", application);
        hide_others.keyEquivalentModifierMask = NSEventModifierFlagCommand | NSEventModifierFlagOption;
        [application_menu addItem:hide_others];
        [application_menu addItem:menu_item(@"Show All", @selector(unhideAllApplications:), @"", application)];
        [application_menu addItem:NSMenuItem.separatorItem];
        [application_menu addItem:menu_item(@"Quit FreeGAG", @selector(terminate:), @"q", application)];
        application_root.submenu = application_menu;
        [main_menu addItem:application_root];

        NSMenuItem *game_root = [[NSMenuItem alloc] initWithTitle:@"Game" action:nil keyEquivalent:@""];
        NSMenu *game_menu = [[NSMenu alloc] initWithTitle:@"Game"];
        [game_menu addItem:menu_item(@"Stop Game", @selector(returnToLauncher:), @"", platform_menu_target)];
        game_root.submenu = game_menu;
        [main_menu addItem:game_root];

        application.mainMenu = main_menu;
        return true;
    }
}

void uninstall_native_platform_menu()
{
    @autoreleasepool
    {
        NSApplication.sharedApplication.mainMenu = nil;
        platform_menu_target = nil;
    }
}

}
