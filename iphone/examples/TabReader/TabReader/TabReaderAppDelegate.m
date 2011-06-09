//
//  TabReaderAppDelegate.m
//  TabReader
//
//  Created by spadix on 5/3/11.
//

#import "TabReaderAppDelegate.h"
#import "ResultsViewController.h"

@implementation TabReaderAppDelegate

@synthesize window=_window;
@synthesize tabBarController=_tabBarController;

- (BOOL)            application: (UIApplication*) application
  didFinishLaunchingWithOptions: (NSDictionary*) options
{
    self.window.rootViewController = self.tabBarController;
    [self.window makeKeyAndVisible];

    // force class to load so it may be referenced directly from nib
    [ZBarReaderViewController class];

    ZBarReaderViewController *reader =
        [self.tabBarController.viewControllers objectAtIndex: 0];
    reader.readerDelegate = self;
    reader.showsZBarControls = NO;
    reader.supportedOrientationsMask = ZBarOrientationMaskAll;

    return(YES);
}

- (void) dealloc
{
    [_window release];
    [_tabBarController release];
    [super dealloc];
}


// ZBarReaderDelegate

- (void)  imagePickerController: (UIImagePickerController*) picker
  didFinishPickingMediaWithInfo: (NSDictionary*) info
{
    // do something useful with results
    UITabBarController *tabs = self.tabBarController;
    tabs.selectedIndex = 1;
    ResultsViewController *results = [tabs.viewControllers objectAtIndex: 1];
    UIImage *image = [info objectForKey: UIImagePickerControllerOriginalImage];
    results.resultImage.image = image;

    id <NSFastEnumeration> syms =
    [info objectForKey: ZBarReaderControllerResults];
    for(ZBarSymbol *sym in syms) {
        results.resultText.text = sym.data;
        break;
    }
}

@end
