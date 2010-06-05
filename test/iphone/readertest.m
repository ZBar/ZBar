#import <zbar/ZBarReaderViewController.h>
#import <zbar/ZBarReaderView.h>

#if 0
# define VALGRIND "/usr/local/bin/valgrind"
#endif

enum {
    CLASS_SECTION = 0,
    SOURCE_SECTION,
    CAMODE_SECTION,
    CONFIG_SECTION,
    SYMBOL_SECTION,
    RESULT_SECTION,
    NUM_SECTIONS
};

static NSString* const section_titles[] = {
    @"Classes",
    @"SourceType",
    @"CameraMode",
    @"Reader Configuration",
    @"Enabled Symbologies",
    @"Decode Results",
};

@interface AppDelegate
    : UITableViewController
    < UIApplicationDelegate,
      UINavigationControllerDelegate,
      UITableViewDelegate,
      UITableViewDataSource,
      ZBarReaderDelegate >
{
    UIWindow *window;
    UINavigationController *nav;

    NSMutableArray *sections, *symbolEnables;

    BOOL found, paused;
    NSInteger dataHeight;
    UILabel *typeLabel, *dataLabel;
    UIImageView *imageView;

    ZBarReaderViewController *reader;
    UIView *overlay;
    NSArray *masks;
}

@end


@implementation AppDelegate

- (id) init
{
    // force these classes to load
    [ZBarReaderViewController class];
    [ZBarReaderController class];

    return([super initWithStyle: UITableViewStyleGrouped]);
}

- (void) initReader: (NSString*) clsName
{
    [reader release];
    reader = [NSClassFromString(clsName) new];
    reader.readerDelegate = self;
}

- (void) initOverlay
{
    overlay = [[UIView alloc]
                  initWithFrame: CGRectMake(0, 0, 320, 480)];
    overlay.backgroundColor = [UIColor clearColor];

    masks = [[NSArray alloc]
                initWithObjects:
                    [[[UIView alloc]
                         initWithFrame: CGRectMake(0, 0, 320, 0)]
                        autorelease],
                    [[[UIView alloc]
                         initWithFrame: CGRectMake(0, 0, 0, 426)]
                        autorelease],
                    [[[UIView alloc]
                         initWithFrame: CGRectMake(0, 426, 320, 0)]
                        autorelease],
                    [[[UIView alloc]
                         initWithFrame: CGRectMake(320, 0, 0, 426)]
                        autorelease],
                nil];
    for(UIView *mask in masks) {
        mask.backgroundColor = [UIColor colorWithWhite: 0
                                        alpha: .5];
        [overlay addSubview: mask];
    }

    UILabel *label =
        [[UILabel alloc]
            initWithFrame: CGRectMake(0, 0, 320, 48)];
    label.backgroundColor = [UIColor clearColor];
    label.textColor = [UIColor whiteColor];
    label.font = [UIFont boldSystemFontOfSize: 24];
    label.text = @"Custom Overlay";
    [overlay addSubview: label];
    [label release];

    UIToolbar *toolbar =
        [[UIToolbar alloc]
            initWithFrame: CGRectMake(0, 426, 320, 54)];
    toolbar.tintColor = [UIColor colorWithRed: .5
                                     green: 0
                                     blue: 0
                                     alpha: 1];
    toolbar.items =
        [NSArray arrayWithObjects:
            [[[UIBarButtonItem alloc]
                 initWithTitle: @"X"
                 style: UIBarButtonItemStylePlain
                 target: self
                 action: @selector(imagePickerControllerDidCancel:)]
                autorelease],
            [[[UIBarButtonItem alloc]
                 initWithBarButtonSystemItem: UIBarButtonSystemItemFlexibleSpace
                 target: nil
                 action: nil]
                autorelease],
            [[[UIBarButtonItem alloc]
                 initWithBarButtonSystemItem: UIBarButtonSystemItemPause
                 target: self
                 action: @selector(pause)]
                autorelease],
            nil];
    [overlay addSubview: toolbar];
    [toolbar release];
}

- (void) setCheck: (BOOL) state
          forCell: (UITableViewCell*) cell
{
    cell.accessoryType =
        ((state)
         ? UITableViewCellAccessoryCheckmark
         : UITableViewCellAccessoryNone);
}

- (void) setCheckForTag: (int) tag
              inSection: (int) section
{
    for(UITableViewCell *cell in [sections objectAtIndex: section])
        [self setCheck: (cell.tag == tag)
              forCell: cell];
}

- (void) setCheckForName: (NSString*) name
               inSection: (int) section
{
    for(UITableViewCell *cell in [sections objectAtIndex: section])
        [self setCheck: [name isEqualToString: cell.textLabel.text]
              forCell: cell];
}

- (void) applicationDidFinishLaunching: (UIApplication*) application
{
    self.title = @"ZBar Reader Test";

    nav = [[UINavigationController alloc]
              initWithRootViewController: self];
    nav.delegate = self;

    window = [[UIWindow alloc] initWithFrame: [[UIScreen mainScreen] bounds]];
    [window addSubview: nav.view];
    [window makeKeyAndVisible];

    [self initReader: @"ZBarReaderViewController"];
}

- (UITableViewCell*) cellWithTitle: (NSString*) title
                               tag: (NSInteger) tag
                           checked: (BOOL) checked
{
    UITableViewCell *cell = [UITableViewCell new];
    cell.textLabel.text = title;
    cell.tag = tag;
    [self setCheck: checked
          forCell: cell];
    return([cell autorelease]);
}

- (void) initControlCells
{
    // NB don't need SourceTypeSavedPhotosAlbum
    static NSString* const sourceNames[] = {
        @"Library", @"Camera", @"Album", nil
    };
    NSMutableArray *sources = [NSMutableArray array];
    for(int i = 0; sourceNames[i]; i++)
        if([[reader class] isSourceTypeAvailable: i])
            [sources addObject:
                [self cellWithTitle: sourceNames[i]
                      tag: i
                      checked: (reader.sourceType == i)]];
    [sections replaceObjectAtIndex: SOURCE_SECTION
              withObject: sources];

    static NSString* const modeNames[] = {
        @"Default", @"Sampling", @"Sequence", nil
    };
    NSMutableArray *modes = [NSMutableArray array];
    for(int i = 0; modeNames[i]; i++)
        [modes addObject:
            [self cellWithTitle: modeNames[i]
                  tag: i
                  checked: (reader.cameraMode == i)]];
    [sections replaceObjectAtIndex: CAMODE_SECTION
              withObject: modes];

    static NSString* const configNames[] = {
        @"showsZBarControls", @"enableCache",
        @"showsHelpOnFail", @"takesPicture",
        nil
    };
    NSMutableArray *configs = [NSMutableArray array];
    for(int i = 0; configNames[i]; i++)
        [configs addObject:
            [self cellWithTitle: configNames[i]
                  tag: i
                  checked: [[reader valueForKey: configNames[i]] boolValue]]];
    [sections replaceObjectAtIndex: CONFIG_SECTION
              withObject: configs];

    static const int symbolValues[] = {
        ZBAR_QRCODE, ZBAR_CODE128, ZBAR_CODE39, ZBAR_I25, ZBAR_EAN13, ZBAR_EAN8,
        ZBAR_UPCA, ZBAR_UPCE, ZBAR_ISBN13, ZBAR_ISBN10,
        0
    };
    static NSString* const symbolNames[] = {
        @"QRCODE", @"CODE128", @"CODE39", @"I25", @"EAN13", @"EAN8",
        @"UPCA", @"UPCE", @"ISBN13", @"ISBN10",
        nil
    };
    NSMutableArray *symbols = [NSMutableArray array];
    [symbolEnables release];
    symbolEnables = [[NSMutableArray alloc] init];
    BOOL en = YES;
    for(int i = 0; symbolValues[i]; i++) {
        en = en && (symbolValues[i] != ZBAR_UPCA);
        [symbols addObject:
            [self cellWithTitle: symbolNames[i]
                  tag: symbolValues[i]
                  checked: en]];
        [symbolEnables addObject: [NSNumber numberWithBool: en]];
    }
    [sections replaceObjectAtIndex: SYMBOL_SECTION
              withObject: symbols];

    [self.tableView reloadData];
}

- (void) viewDidLoad
{
    [super viewDidLoad];

    UITableView *view = self.tableView;
    view.delegate = self;
    view.dataSource = self;

    [self initOverlay];

    sections = [[NSMutableArray alloc]
                   initWithCapacity: NUM_SECTIONS];
    for(int i = 0; i < NUM_SECTIONS; i++)
        [sections addObject: [NSNull null]];

    NSArray *classes =
        [NSArray arrayWithObjects:
            [self cellWithTitle: @"ZBarReaderViewController"
                  tag: 0
                  checked: YES],
            [self cellWithTitle: @"ZBarReaderController"
                  tag: 1
                  checked: NO],
            nil];
    [sections replaceObjectAtIndex: CLASS_SECTION
              withObject: classes];

    UITableViewCell *typeCell = [UITableViewCell new];
    typeLabel = typeCell.textLabel;
    UITableViewCell *dataCell = [UITableViewCell new];
    dataLabel = dataCell.textLabel;
    dataLabel.numberOfLines = 0;
    dataLabel.lineBreakMode = UILineBreakModeCharacterWrap;
    UITableViewCell *imageCell = [UITableViewCell new];
    imageView = [UIImageView new];
    imageView.contentMode = UIViewContentModeScaleAspectFit;
    imageView.autoresizingMask = (UIViewAutoresizingFlexibleWidth |
                                  UIViewAutoresizingFlexibleHeight);
    UIView *content = imageCell.contentView;
    imageView.frame = content.bounds;
    [content addSubview: imageView];
    [imageView release];
    NSArray *results =
        [NSArray arrayWithObjects: typeCell, dataCell, imageCell, nil];
    [sections replaceObjectAtIndex: RESULT_SECTION
              withObject: results];

    [self initControlCells];
}

- (void) viewDidUnload
{
    [sections release];
    sections = nil;
    [symbolEnables release];
    symbolEnables = nil;
    [typeLabel release];
    typeLabel = nil;
    [dataLabel release];
    dataLabel = nil;
    [imageView release];
    imageView = nil;
    [overlay release];
    overlay = nil;
    [masks release];
    masks = nil;
    [super viewDidUnload];
}

- (void) dealloc
{
    [reader release];
    reader = nil;
    [nav release];
    nav = nil;
    [window release];
    window = nil;
    [super dealloc];
}

- (void) scan
{
    found = paused = NO;
    imageView.image = nil;
    typeLabel.text = nil;
    dataLabel.text = nil;
    [self.tableView reloadData];
    if([reader respondsToSelector: @selector(readerView)])
        reader.readerView.showsFPS = YES;
    [self presentModalViewController: reader
          animated: YES];
}

- (void) pause
{
    if(![reader respondsToSelector: @selector(readerView)])
        return;
    paused = !paused;
    if(paused)
        [reader.readerView stop];
    else
        [reader.readerView start];
}

// UINavigationControllerDelegate

- (void) navigationController: (UINavigationController*) _nav
       willShowViewController: (UIViewController*) vc
                     animated: (BOOL) animated
{
    self.navigationItem.rightBarButtonItem =
        [[[UIBarButtonItem alloc]
             initWithTitle: @"Scan!"
             style: UIBarButtonItemStyleDone
             target: self
             action: @selector(scan)]
            autorelease];
}

// UITableViewDataSource

- (NSInteger) numberOfSectionsInTableView: (UITableView*) view
{
    return(sections.count - !found);
}

- (NSInteger) tableView: (UITableView*) view
  numberOfRowsInSection: (NSInteger) idx
{
    NSArray *section = [sections objectAtIndex: idx];
    return(section.count);
}

- (UITableViewCell*) tableView: (UITableView*) view
         cellForRowAtIndexPath: (NSIndexPath*) path
{
    return([[sections objectAtIndex: path.section]
               objectAtIndex: path.row]);
}

- (NSString*)  tableView: (UITableView*) view
 titleForHeaderInSection: (NSInteger) idx
{
    assert(idx < NUM_SECTIONS);
    return(section_titles[idx]);
}

// UITableViewDelegate

- (NSIndexPath*) tableView: (UITableView*) view
  willSelectRowAtIndexPath: (NSIndexPath*) path
{
    if(path.section == RESULT_SECTION)
        return(nil);
    return(path);
}

- (void) alertUnsupported
{
    UIAlertView *alert =
        [[UIAlertView alloc]
            initWithTitle: @"Unsupported"
            message: @"Setting not available for this reader"
            @" (or with this OS on this device)"
            delegate: nil
            cancelButtonTitle: @"Cancel"
            otherButtonTitles: nil];
    [alert show];
    [alert release];
}

- (void)       tableView: (UITableView*) view
 didSelectRowAtIndexPath: (NSIndexPath*) path
{
    [view deselectRowAtIndexPath: path
          animated: YES];

    UITableViewCell *cell = [view cellForRowAtIndexPath: path];

    switch(path.section)
    {
    case CLASS_SECTION: {
        NSString *name = cell.textLabel.text;
        [self initReader: name];
        [self initControlCells];
        [self setCheckForName: name
              inSection: CLASS_SECTION];
        break;
    }
    case SOURCE_SECTION:
        [self setCheckForTag: reader.sourceType = cell.tag
              inSection: SOURCE_SECTION];
        break;
    case CAMODE_SECTION:
        @try {
            reader.cameraMode = cell.tag;
        }
        @catch (...) {
            [self alertUnsupported];
        }
        [self setCheckForTag: reader.cameraMode
              inSection: CAMODE_SECTION];
        break;
    case CONFIG_SECTION: {
        BOOL state;
        NSString *key = cell.textLabel.text;
        state = ![[reader valueForKey: key] boolValue];
        @try {
            [reader setValue: [NSNumber numberWithBool: state]
                    forKey: key];
        }
        @catch (...) {
            [self alertUnsupported];
        }

        // read back and update current state
        state = [[reader valueForKey: key] boolValue];
        [self setCheck: state
              forCell: cell];

        if([key isEqualToString: @"showsZBarControls"] &&
           reader.sourceType == UIImagePickerControllerSourceTypeCamera)
            reader.cameraOverlayView = (state) ? nil : overlay;
        break;
    }
    case SYMBOL_SECTION: {
        BOOL state = ![[symbolEnables objectAtIndex: path.row] boolValue];
        [symbolEnables replaceObjectAtIndex: path.row
                       withObject: [NSNumber numberWithBool: state]];
        [reader.scanner setSymbology: cell.tag
               config: ZBAR_CFG_ENABLE
               to: state];
        [self setCheck: state
              forCell: cell];
        break;
    }
    case RESULT_SECTION:
        break;
    default:
        assert(0);
    }
}

- (CGFloat)    tableView: (UITableView*) view
 heightForRowAtIndexPath: (NSIndexPath*) path
{
    if(path.section < RESULT_SECTION)
        return(44);

    switch(path.row) {
    case 0: return(44);
    case 1: return(dataHeight);
    case 2: return(300);
    default: assert(0);
    }
    return(44);
}

// ZBarReaderDelegate

- (void)  imagePickerController: (UIImagePickerController*) picker
  didFinishPickingMediaWithInfo: (NSDictionary*) info
{
    id <NSFastEnumeration> results =
        [info objectForKey: ZBarReaderControllerResults];
    assert(results);

    UIImage *image = [info objectForKey: UIImagePickerControllerOriginalImage];
    assert(image);
    if(image)
        imageView.image = image;

    int quality = 0;
    ZBarSymbol *bestResult = nil;
    for(ZBarSymbol *sym in results)
        if(sym.quality > quality)
            bestResult = sym;
    assert(!!bestResult);

    [self performSelector: @selector(presentResult:)
          withObject: bestResult
          afterDelay: .001];
    [picker dismissModalViewControllerAnimated: YES];
}

- (void) presentResult: (ZBarSymbol*) sym
{
    found = !!sym;
    typeLabel.text = sym.typeName;
    NSString *data = sym.data;
    dataLabel.text = data;

    NSLog(@"imagePickerController:didFinishPickingMediaWithInfo:\n");
    NSLog(@"    type=%@ data=%@\n", sym.typeName, data);

    CGSize size = [data sizeWithFont: [UIFont systemFontOfSize: 17]
                        constrainedToSize: CGSizeMake(288, 2000)
                        lineBreakMode: UILineBreakModeCharacterWrap];
    dataHeight = size.height + 26;
    if(dataHeight > 2000)
        dataHeight = 2000;

    [self.tableView reloadData];
    [self.tableView scrollToRowAtIndexPath:
             [NSIndexPath indexPathForRow: 0
                          inSection: RESULT_SECTION]
         atScrollPosition:UITableViewScrollPositionTop
         animated: NO];
}

- (void) imagePickerControllerDidCancel: (UIImagePickerController*) picker
{
    NSLog(@"imagePickerControllerDidCancel:\n");
    [reader dismissModalViewControllerAnimated: YES];
}

- (void) readerControllerDidFailToRead: (ZBarReaderController*) _reader
                             withRetry: (BOOL) retry
{
    NSLog(@"readerControllerDidFailToRead: retry=%s\n",
          (retry) ? "YES" : "NO");
    if(!retry)
        [_reader dismissModalViewControllerAnimated: YES];
}

@end


int main (int argc, char *argv[])
{
#ifdef VALGRIND
    if(argc < 2 || (argc >= 2 && strcmp(argv[1], "-valgrind")))
        execl(VALGRIND, VALGRIND,
              "--log-file=/tmp/memcheck.log", "--leak-check=full",
              argv[0], "-valgrind",
              NULL);
#endif

    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    int rc = UIApplicationMain(argc, argv, nil, @"AppDelegate");
    [pool release];
    return(rc);
}
