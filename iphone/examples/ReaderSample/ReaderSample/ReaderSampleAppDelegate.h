//
//  ReaderSampleAppDelegate.h
//  ReaderSample
//
//  Created by spadix on 4/14/11.
//

#import <UIKit/UIKit.h>

@class ReaderSampleViewController;

@interface ReaderSampleAppDelegate : NSObject <UIApplicationDelegate> {

}

@property (nonatomic, retain) IBOutlet UIWindow *window;

@property (nonatomic, retain) IBOutlet ReaderSampleViewController *viewController;

@end
