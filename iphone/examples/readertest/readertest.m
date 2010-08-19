#if 0
# define VALGRIND "/usr/local/bin/valgrind"
#endif

enum {
    CLASS_SECTION = 0,
    SOURCE_SECTION,
    CAMODE_SECTION,
    CONFIG_SECTION,
    CUSTOM_SECTION,
    SYMBOL_SECTION,
    RESULT_SECTION,
    NUM_SECTIONS
};

static NSString* const section_titles[] = {
    @"Classes",
    @"SourceType",
    @"CameraMode",
    @"Reader Configuration",
    nil,
    @"Enabled Symbologies",
    @"Decode Results",
};

static const CGRect const crop_choices[] = {
    { { 0, 0 }, { 1, 1 } },
    { { .25, 0 }, { .5, 1 } },
    { { 0, .3 }, { 1, .4 } },
    { { 0, 0 }, { 0, 0 } }
};

static const NSInteger const density_choices[] = {
    3, 2, 1, 0, 4, -1
};

@interface AppDelegate
    : UITableViewController
    < UIApplicationDelegate,
      UINavigationControllerDelegate,
      UITableViewDelegate,
      UITableViewDataSource,
      UIActionSheetDelegate,
      ZBarReaderDelegate >
{
    UIWindow *window;
    UINavigationController *nav;

    NSMutableArray *sections, *symbolEnables;

    BOOL found, paused, continuous;
    NSInteger dataHeight;
    UILabel *typeLabel, *dataLabel;
    UIImageView *imageView;

    ZBarReaderViewController *reader;
    UIView *overlay;
    UIBarButtonItem *manualBtn;
    UILabel *typeOvl, *dataOvl;
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
                  initWithFrame: CGRectMake(0, 426, 320, 54)];
    overlay.backgroundColor = [UIColor clearColor];

    masks = [[NSArray alloc]
                initWithObjects:
                    [[[UIView alloc]
                         initWithFrame: CGRectMake(0, -426, 320, 0)]
                        autorelease],
                    [[[UIView alloc]
                         initWithFrame: CGRectMake(0, -426, 0, 426)]
                        autorelease],
                    [[[UIView alloc]
                         initWithFrame: CGRectMake(0, 0, 320, 0)]
                        autorelease],
                    [[[UIView alloc]
                         initWithFrame: CGRectMake(320, -426, 0, 426)]
                        autorelease],
                nil];
    for(UIView *mask in masks) {
        mask.backgroundColor = [UIColor colorWithWhite: 0
                                        alpha: .5];
        [overlay addSubview: mask];
    }

    UILabel *label =
        [[UILabel alloc]
            initWithFrame: CGRectMake(0, -426, 320, 48)];
    label.backgroundColor = [UIColor clearColor];
    label.textColor = [UIColor whiteColor];
    label.font = [UIFont boldSystemFontOfSize: 24];
    label.text = @"Custom Overlay";
    [overlay addSubview: label];
    [label release];

    typeOvl = [[UILabel alloc]
                  initWithFrame: CGRectMake(0, -378, 80, 24)];
    typeOvl.backgroundColor = [UIColor clearColor];
    typeOvl.textColor = [UIColor whiteColor];
    typeOvl.font = [UIFont systemFontOfSize: 16];
    typeOvl.textAlignment = UITextAlignmentCenter;
    [overlay addSubview: typeOvl];

    dataOvl = [[UILabel alloc]
                  initWithFrame: CGRectMake(96, -378, 224, 24)];
    dataOvl.backgroundColor = [UIColor clearColor];
    dataOvl.textColor = [UIColor whiteColor];
    dataOvl.font = [UIFont systemFontOfSize: 16];
    [overlay addSubview: dataOvl];

    UIToolbar *toolbar =
        [[UIToolbar alloc]
            initWithFrame: CGRectMake(0, 0, 320, 54)];
    toolbar.tintColor = [UIColor colorWithRed: .5
                                     green: 0
                                     blue: 0
                                     alpha: 1];
    [manualBtn release];
    manualBtn = [[UIBarButtonItem alloc]
                    initWithBarButtonSystemItem: UIBarButtonSystemItemCamera
                    target: self
                    action: @selector(manualCapture)];

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
            manualBtn,
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
            [[[UIBarButtonItem alloc]
                 initWithBarButtonSystemItem: UIBarButtonSystemItemFlexibleSpace
                 target: nil
                 action: nil]
                autorelease],
            nil];
    [overlay addSubview: toolbar];
    [toolbar release];


    UIButton *info =
        [UIButton buttonWithType: UIButtonTypeInfoLight];
    info.frame = CGRectMake(266, 0, 54, 54);
    [info addTarget: self
             action: @selector(info)
             forControlEvents: UIControlEventTouchUpInside];
    [overlay addSubview: info];
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
        @"showsCameraControls", @"showsZBarControls", @"tracksSymbols",
        @"enableCache", @"showsHelpOnFail", @"takesPicture",
        nil
    };
    NSMutableArray *configs = [NSMutableArray array];
    for(int i = 0; configNames[i]; i++)
        @try {
            BOOL checked = [[reader valueForKey: configNames[i]] boolValue];
            [configs addObject:
                [self cellWithTitle: configNames[i]
                      tag: i
                      checked: checked]];
        }
        @catch(...) { }
    [sections replaceObjectAtIndex: CONFIG_SECTION
              withObject: configs];

    UITableViewCell *cropCell =
        [[[UITableViewCell alloc]
             initWithStyle: UITableViewCellStyleValue1
             reuseIdentifier: nil]
            autorelease];
    cropCell.textLabel.text = @"scanCrop";
    cropCell.detailTextLabel.text = NSStringFromCGRect(crop_choices[0]);

    UITableViewCell *xDensityCell =
        [[[UITableViewCell alloc]
             initWithStyle: UITableViewCellStyleValue1
             reuseIdentifier: nil]
            autorelease];
    xDensityCell.textLabel.text = @"CFG_X_DENSITY";
    xDensityCell.detailTextLabel.tag = ZBAR_CFG_X_DENSITY;
    xDensityCell.detailTextLabel.text =
        [NSString stringWithFormat: @"%d", density_choices[0]];

    UITableViewCell *yDensityCell =
        [[[UITableViewCell alloc]
             initWithStyle: UITableViewCellStyleValue1
             reuseIdentifier: nil]
            autorelease];
    yDensityCell.textLabel.text = @"CFG_Y_DENSITY";
    yDensityCell.detailTextLabel.tag = ZBAR_CFG_Y_DENSITY;
    yDensityCell.detailTextLabel.text =
        [NSString stringWithFormat: @"%d", density_choices[0]];

    [sections replaceObjectAtIndex: CUSTOM_SECTION
              withObject: [NSArray arrayWithObjects:
                              xDensityCell,
                              yDensityCell,
                              cropCell,
                              [self cellWithTitle: @"continuous"
                                    tag: 1
                                    checked: continuous],
                              nil]];

    static const int symbolValues[] = {
        ZBAR_QRCODE, ZBAR_CODE128, ZBAR_CODE39, ZBAR_I25,
        ZBAR_DATABAR, ZBAR_DATABAR_EXP,
        ZBAR_EAN13, ZBAR_EAN8, ZBAR_UPCA, ZBAR_UPCE, ZBAR_ISBN13, ZBAR_ISBN10,
        0
    };
    NSMutableArray *symbols = [NSMutableArray array];
    [symbolEnables release];
    symbolEnables = [[NSMutableArray alloc] init];
    BOOL en = YES;
    for(int i = 0; symbolValues[i]; i++) {
        en = en && (symbolValues[i] != ZBAR_UPCA);
        [symbols addObject:
            [self cellWithTitle: [ZBarSymbol nameForType: symbolValues[i]]
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
    typeLabel = [typeCell.textLabel retain];
    UITableViewCell *dataCell = [UITableViewCell new];
    dataLabel = [dataCell.textLabel retain];
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
    [manualBtn release];
    manualBtn = nil;
    [typeLabel release];
    typeLabel = nil;
    [dataLabel release];
    dataLabel = nil;
    [imageView release];
    imageView = nil;
    [typeOvl release];
    typeOvl = nil;
    [dataOvl release];
    dataOvl = nil;
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
    typeOvl.text = nil;
    dataOvl.text = nil;
    [self.tableView reloadData];
    if([reader respondsToSelector: @selector(readerView)])
        reader.readerView.showsFPS = YES;
    if(reader.sourceType == UIImagePickerControllerSourceTypeCamera)
        reader.cameraOverlayView = (reader.showsZBarControls) ? nil : overlay;
    manualBtn.enabled = TARGET_IPHONE_SIMULATOR ||
        (reader.cameraMode == ZBarReaderControllerCameraModeDefault);
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

- (void) manualCapture
{
    [(UIImagePickerController*)reader takePicture];
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
    if(path.section == RESULT_SECTION && path.row != 2)
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

- (void) advanceCrop: (UILabel*) label
{
    CGRect r = CGRectFromString(label.text);
    int i;
    for(i = 0; crop_choices[i].size.width;)
        if(CGRectEqualToRect(r, crop_choices[i++]))
            break;
    if(!crop_choices[i].size.width)
        i = 0;
    r = crop_choices[i];
    reader.scanCrop = r;
    label.text = NSStringFromCGRect(r);

    r.origin.x *= 426;
    r.origin.y *= 320;
    r.size.width *= 426;
    r.size.height *= 320;
    UIView *mask = [masks objectAtIndex: 0];
    mask.frame = CGRectMake(0, -426, 320, r.origin.x);
    mask = [masks objectAtIndex: 1];
    mask.frame = CGRectMake(0, r.origin.x - 426, r.origin.y, r.size.width);

    r.origin.y += r.size.height;
    mask = [masks objectAtIndex: 2];
    mask.frame = CGRectMake(r.origin.y, r.origin.x - 426,
                            320 - r.origin.y, r.size.width);

    r.origin.x += r.size.width;
    mask = [masks objectAtIndex: 3];
    mask.frame = CGRectMake(0, r.origin.x - 426, 320, 426 - r.origin.x);
}

- (void) advanceDensity: (UILabel*) label
{
    NSInteger d = [label.text integerValue];
    int i;
    for(i = 0; density_choices[i] >= 0;)
        if(d == density_choices[i++])
            break;
    if(density_choices[i] < 0)
        i = 0;
    d = density_choices[i];
    assert(d >= 0);
    [reader.scanner setSymbology: 0
           config: label.tag
           to: d];
    label.text = [NSString stringWithFormat: @"%d", d];
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
        break;
    }

    case CUSTOM_SECTION:
        switch(path.row)
        {
        case 0:
        case 1:
            [self advanceDensity: cell.detailTextLabel];
            break;
        case 2:
            [self advanceCrop: cell.detailTextLabel];
            break;
        case 3:
            [self setCheck: continuous = !continuous
                  forCell: cell];
            break;
        default:
            assert(0);
        }
        break;

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
        if(path.row == 2)
            [[[[UIActionSheet alloc]
                  initWithTitle: nil
                  delegate: self
                  cancelButtonTitle: @"Cancel"
                  destructiveButtonTitle: nil
                  otherButtonTitles: @"Save Image", nil]
                 autorelease]
                showInView: self.view];
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

// UIActionSheetDelegate

- (void)  actionSheet: (UIActionSheet*) sheet
 clickedButtonAtIndex: (NSInteger) idx
{
    if(idx == sheet.cancelButtonIndex)
        return;
    idx -= sheet.firstOtherButtonIndex;
    if(!idx)
        UIImageWriteToSavedPhotosAlbum(imageView.image, nil, NULL, NULL);
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
    if(!continuous)
        [picker dismissModalViewControllerAnimated: YES];
}

- (void) presentResult: (ZBarSymbol*) sym
{
    found = !!sym;
    NSString *typeName = sym.typeName;
    typeLabel.text = typeName;
    NSString *data = sym.data;
    dataLabel.text = data;

    if(continuous) {
        typeOvl.text = typeName;
        dataOvl.text = data;
    }

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

- (void) info
{
    [reader showHelpWithReason: @"INFO"];
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
