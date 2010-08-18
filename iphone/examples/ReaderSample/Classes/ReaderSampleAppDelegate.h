//
//  ReaderSampleAppDelegate.h
//  ReaderSample
//
//  Created by spadix on 8/4/10.
//

#import <UIKit/UIKit.h>

@class ReaderSampleViewController;

@interface ReaderSampleAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    ReaderSampleViewController *viewController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet ReaderSampleViewController *viewController;

@end

