
import org.junit.Test;
import org.junit.Before;
import org.junit.After;
import static org.junit.Assert.*;

import net.sourceforge.zbar.*;

import java.text.CharacterIterator;
import java.text.StringCharacterIterator;
import java.util.Iterator;

public class TestScanImage
{
    protected ImageScanner scanner;
    protected Image image;

    @Before public void setUp ()
    {
        scanner = new ImageScanner();
        image = new Image();
    }

    @After public void tearDown ()
    {
        image = null;
        scanner = null;
        System.gc();
    }

    public static final String encoded_widths =
        "9 111 212241113121211311141132 11111 311213121312121332111132 111 9";

    protected void generateY800 ()
    {
        int width = 114, height = 85;
        image.setSize(width, height);
        image.setFormat("Y800");
        int datalen = width * height;
        byte[] data = new byte[datalen];

        int y = 0;
        int p = 0;
        for(; y < 10 && y < height; y++)
            for(int x = 0; x < width; x++)
                data[p++] = -1;

        for(; y < height - 10; y++) {
            int x = 0;
            byte color = -1;
            CharacterIterator it = new StringCharacterIterator(encoded_widths);
            for(char c = it.first();
                c != CharacterIterator.DONE;
                c = it.next())
            {
                if(c == ' ')
                    continue;
                for(int dx = (int)c - 0x30; dx > 0; dx--) {
                    data[p++] = color;
                    x++;
                }
                color = (byte)~color;
            }
            for(; x < width; x++)
                data[p++] = (byte)~color;
        }

        for(; y < height; y++)
            for(int x = 0; x < width; x++)
                data[p++] = -1;
        assert(p == datalen);

        image.setData(data);
    }

    protected void checkResults (SymbolSet syms)
    {
        assertNotNull(syms);
        assert(syms.size() == 1);
        Iterator<Symbol> it = syms.iterator();
        assertTrue(it.hasNext());
        Symbol sym = it.next();
        assertNotNull(sym);
        assertFalse(it.hasNext());

        assertEquals(Symbol.EAN13, sym.getType());
        assertEquals(sym.EAN13, sym.getType()); // cached

        assertTrue(sym.getQuality() > 1);
        assertEquals(0, sym.getCount());

        SymbolSet comps = sym.getComponents();
        assertNotNull(comps);
        assertEquals(0, comps.size());
        it = comps.iterator();
        assertNotNull(it);
        assertFalse(it.hasNext());

        String data = sym.getData();
        assertEquals("6268964977804", data);

        byte[] bytes = sym.getDataBytes();
        byte[] exp = { '6','2','6','8','9','6','4','9','7','7','8','0','4' };
        assertArrayEquals(exp, bytes);

        int[] r = sym.getBounds();
        assertTrue(r[0] > 6);
        assertTrue(r[1] > 6);
        assertTrue(r[2] < 102);
        assertTrue(r[3] < 73);

        assertEquals(Orientation.UP, sym.getOrientation());
    }

    @Test public void generated ()
    {
        generateY800();
        int n = scanner.scanImage(image);
        assertEquals(1, n);

        checkResults(image.getSymbols());
        checkResults(scanner.getResults());
    }

    @Test public void config ()
    {
        generateY800();
        scanner.setConfig(Symbol.EAN13, Config.ENABLE, 0);
        int n = scanner.scanImage(image);
        assertEquals(0, n);
    }

    @Test public void cache ()
    {
        generateY800();
        scanner.enableCache(true);

        int n = 0;
        for(int i = 0; i < 10; i++) {
            n = scanner.scanImage(image);
            if(n > 0) {
                assertTrue(i > 1);
                break;
            }
        }

        assertEquals(1, n);
        checkResults(scanner.getResults());
    }

    @Test public void orientation()
    {
        generateY800();

        // flip the image
        int width = image.getWidth();
        int height = image.getHeight();
        byte[] data = image.getData();
        int p = 0;
        for(int y = 0; y < height; y++) {
            for(int x0 = 0; x0 < width / 2; x0++) {
                int x1 = width - x0 - 1;
                assert(x0 < x1);
                byte b = data[p + x0];
                data[p + x0] = data[p + x1];
                data[p + x1] = b;
            }
            p += width;
        }
        image.setData(data);

        int n = scanner.scanImage(image);
        assertEquals(1, n);

        SymbolSet syms = scanner.getResults();
        assert(syms.size() == 1);
        for(Symbol sym : syms) {
            assertEquals(Symbol.EAN13, sym.getType());
            assertEquals("6268964977804", sym.getData());
            assertEquals(Orientation.DOWN, sym.getOrientation());
        }
    }
}
