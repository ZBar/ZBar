#import <zbar/ZBarReaderController.h>

#if 0
# define VALGRIND "/usr/local/bin/valgrind"
#endif

enum {
    SOURCE_SECTION = 0,
    CONFIG_SECTION,
    RESULT_SECTION,
    NUM_SECTIONS
};

@interface AppDelegate : UITableViewController
                       < UIApplicationDelegate,
                         UINavigationControllerDelegate,
                         UITableViewDelegate,
                         UITableViewDataSource,
                         ZBarReaderDelegate >
{
    UIWindow *window;
    UINavigationController *nav;

    NSArray *sections;

    BOOL found;
    NSInteger dataHeight;
    UILabel *typeLabel, *dataLabel;
    UIImageView *imageView;

    ZBarReaderController *reader;
}

@end


@implementation AppDelegate

- (id) init
{
    return([super initWithStyle: UITableViewStyleGrouped]);
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

    reader = [ZBarReaderController new];
    reader.readerDelegate = self;
}

- (void) viewDidLoad
{
    [super viewDidLoad];

    UITableView *view = self.tableView;
    view.delegate = self;
    view.dataSource = self;

    // NB don't need SourceTypeSavedPhotosAlbum
    static NSString* const sourceNames[] = {
        @"Library", @"Camera", @"Album"
    };
    NSMutableArray *sources = [NSMutableArray arrayWithCapacity: 3];
    for(int i = 0; i < 3; i++) {
        UITableViewCell *cell = [UITableViewCell new];
        if([ZBarReaderController isSourceTypeAvailable: i]) {
            cell.textLabel.text = sourceNames[i];
            cell.tag = i;
            if(reader.sourceType == i)
                cell.accessoryType = UITableViewCellAccessoryCheckmark;
            [sources addObject: cell];
            [cell release];
        }
    }

    static NSString* const configNames[] = {
        @"showsHelpOnFail", @"showsZBarControls"
    };
    NSMutableArray *configs = [NSMutableArray arrayWithCapacity: 2];
    for(int i = 0; i < 2; i++) {
        UITableViewCell *cell = [UITableViewCell new];
        cell.textLabel.text = configNames[i];
        if([reader performSelector: NSSelectorFromString(configNames[i])])
            cell.accessoryType = UITableViewCellAccessoryCheckmark;
        [configs addObject: cell];
        [cell release];
    }

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

    sections =
        [[NSArray arrayWithObjects: sources, configs, results, nil] retain];
}

- (void) viewDidUnload
{
    [sections release];
    sections = nil;
    typeLabel = nil;
    dataLabel = nil;
    imageView = nil;

    [super viewDidUnload];
}

- (void) dealloc
{
    [reader release];
    [nav release];
    [window release];
    [super dealloc];
}

- (void) scan
{
    found = NO;
    imageView.image = nil;
    typeLabel.text = nil;
    dataLabel.text = nil;
    [self.tableView reloadData];
    [self presentModalViewController: reader
          animated: YES];
}

- (void) setCheck: (BOOL) state
          forCell: (UITableViewCell*) cell
{
    cell.accessoryType =
        ((state)
         ? UITableViewCellAccessoryCheckmark
         : UITableViewCellAccessoryNone);
}

- (void) setSource: (int) state
{
    NSArray *sources = [sections objectAtIndex: 0];
    for(UITableViewCell *cell in sources)
        [self setCheck: (cell.tag == state)
              forCell: cell];
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
    static NSString * const titles[] = {
        @"SourceType",
        @"Reader Configuration",
        @"Decode Results",
    };
    assert(idx < NUM_SECTIONS);
    return(titles[idx]);
}

// UITableViewDelegate

- (NSIndexPath*) tableView: (UITableView*) view
  willSelectRowAtIndexPath: (NSIndexPath*) path
{
    if(path.section == RESULT_SECTION)
        return(nil);
    return(path);
}

- (void)       tableView: (UITableView*) view
 didSelectRowAtIndexPath: (NSIndexPath*) path
{
    [view deselectRowAtIndexPath: path
          animated: YES];

    UITableViewCell *cell = [view cellForRowAtIndexPath: path];

    switch(path.section) {
    case SOURCE_SECTION:
        [self setSource: reader.sourceType = cell.tag];
        break;
    case CONFIG_SECTION: {
        BOOL state;
        switch(path.row) {
        case 0:
            reader.showsHelpOnFail = !reader.showsHelpOnFail;
            state = reader.showsHelpOnFail;
            break;
        case 1:
            @try {
                reader.showsZBarControls = !reader.showsZBarControls;
            }
            @catch (...) {
                UIAlertView *alert =
                    [[UIAlertView alloc]
                        initWithTitle: @"Unsupported"
                        message: @"Not available for iPhone OS < 3.1"
                        delegate: nil
                        cancelButtonTitle: @"Cancel"
                        otherButtonTitles: nil];
                [alert show];
                [alert release];
            }
            state = reader.showsZBarControls;
            break;
        default:
            assert(0);
        }
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
}

// ZBarReaderDelegate

- (void)  imagePickerController: (UIImagePickerController*) picker
  didFinishPickingMediaWithInfo: (NSDictionary*) info
{
    found = YES;
    id <NSFastEnumeration> results =
        [info objectForKey: ZBarReaderControllerResults];
    assert(results);

    NSLog(@"imagePickerController:didFinishPickingMediaWithInfo:\n");
    UIImage *image = [info objectForKey: UIImagePickerControllerOriginalImage];
    assert(image);
    imageView.image = image;

    int quality = 0, n = 0;
    for(ZBarSymbol *sym in results) {
        NSLog(@"    [%d] type=%@ data=%@\n", n, sym.typeName, sym.data);

        if(sym.quality > quality) {
            typeLabel.text = sym.typeName;
            NSString *data = sym.data;
            dataLabel.text = data;

            CGSize size = [data sizeWithFont: [UIFont systemFontOfSize: 17]
                                constrainedToSize: CGSizeMake(288, 2000)
                                lineBreakMode: UILineBreakModeCharacterWrap];
            dataHeight = size.height + 26;
            if(dataHeight > 2000)
                dataHeight = 2000;
        }
        n++;
    }
    assert(n);

    [picker dismissModalViewControllerAnimated: YES];
    [self.tableView reloadData];
}

- (void) imagePickerControllerDidCancel: (UIImagePickerController*) picker
{
    NSLog(@"imagePickerControllerDidCancel:\n");
    [picker dismissModalViewControllerAnimated: YES];
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
