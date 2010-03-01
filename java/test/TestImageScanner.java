
import org.junit.Test;
import org.junit.Before;
import org.junit.After;
import org.junit.Assert.*;

import net.sourceforge.zbar.ImageScanner;
import net.sourceforge.zbar.Config;

public class TestImageScanner
{
    protected ImageScanner scanner;

    @Before public void setUp ()
    {
        scanner = new ImageScanner();
    }

    @After public void tearDown ()
    {
        scanner.destroy();
        scanner = null;
    }


    @Test public void creation ()
    {
        // create/destroy
    }

    @Test public void callSetConfig ()
    {
        scanner.setConfig(0, Config.X_DENSITY, 2);
        scanner.setConfig(0, Config.Y_DENSITY, 4);
    }

    @Test public void callParseConfig ()
    {
        scanner.parseConfig("disable");
    }

    @Test(expected=IllegalArgumentException.class)
        public void callParseConfigInvalid ()
    {
        scanner.parseConfig("yomama");
    }

    @Test public void callEnableCache ()
    {
        scanner.enableCache(true);
        scanner.enableCache(false);
    }
}
