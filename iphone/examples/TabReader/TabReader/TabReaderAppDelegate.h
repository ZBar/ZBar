//
//  TabReaderAppDelegate.h
//  TabReader
//
//  Created by spadix on 5/3/11.
//

#import <UIKit/UIKit.h>

@interface TabReaderAppDelegate
    : NSObject
    < UIApplicationDelegate,
      UITabBarControllerDelegate,
      ZBarReaderDelegate >
{
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet UITabBarController *tabBarController;

@end
