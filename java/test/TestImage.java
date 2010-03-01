
import org.junit.Test;
import org.junit.Before;
import org.junit.After;
import static org.junit.Assert.*;

import net.sourceforge.zbar.Image;

public class TestImage
{
    protected Image image;

    @Before public void setUp ()
    {
        image = new Image();
    }

    @After public void tearDown ()
    {
        image.destroy();
        image = null;
    }


    @Test public void creation ()
    {
        Image img0 = new Image(123, 456);
        Image img1 = new Image("BGR3");
        Image img2 = new Image(987, 654, "UYVY");

        assertEquals(123, img0.getWidth());
        assertEquals(456, img0.getHeight());
        assertEquals(null, img0.getFormat());

        assertEquals(0, img1.getWidth());
        assertEquals(0, img1.getHeight());
        assertEquals("BGR3", img1.getFormat());

        assertEquals(987, img2.getWidth());
        assertEquals(654, img2.getHeight());
        assertEquals("UYVY", img2.getFormat());
    }

    @Test public void sequence ()
    {
        assertEquals(0, image.getSequence());
        image.setSequence(42);
        assertEquals(42, image.getSequence());
    }

    @Test public void size ()
    {
        assertEquals(0, image.getWidth());
        assertEquals(0, image.getHeight());
        image.setSize(640, 480);
        assertEquals(640, image.getWidth());
        assertEquals(480, image.getHeight());
    }

    @Test public void format ()
    {
        assertNull(image.getFormat());
        image.setFormat("Y800");
        assertEquals("Y800", image.getFormat());
        boolean gotException = false;
        try {
            image.setFormat("[]");
        }
        catch(IllegalArgumentException e) {
            // expected
            gotException = true;
        }
        assertTrue("Expected exception", gotException);
        assertEquals("Y800", image.getFormat());
    }

    @Test(expected=IllegalArgumentException.class)
        public void setFormatInvalid0 ()
    {
        image.setFormat(null);
    }

    @Test(expected=IllegalArgumentException.class)
        public void setFormatInvalid1 ()
    {
        image.setFormat("");
    }

    @Test(expected=IllegalArgumentException.class)
        public void setFormatInvalid2 ()
    {
        image.setFormat("YOMAMA");
    }

    @Test(expected=IllegalArgumentException.class)
        public void setFormatInvalid3 ()
    {
        image.setFormat("foo");
    }

    @Test public void data ()
    {
        assertNull(image.getData());

        int[] ints = new int[24];
        image.setData(ints);
        assertSame(ints, image.getData());

        byte[] bytes = new byte[280];
        image.setData(bytes);
        assertSame(bytes, image.getData());

        image.setData((byte[])null);
        assertNull(image.getData());
    }

    @Test public void convert ()
    {
        image.setSize(4, 4);
        image.setFormat("RGB4");
        int[] rgb4 = new int[16];
        byte[] exp = new byte[16];
        for(int i = 0; i < 16; i++) {
            int c = i * 15;
            rgb4[i] = c | (c << 8) | (c << 16) | (c << 24);
            exp[i] = (byte)c;
        }
        image.setData(rgb4);

        Image gray = image.convert("Y800");
        assertEquals(4, gray.getWidth());
        assertEquals(4, gray.getHeight());
        assertEquals("Y800", gray.getFormat());

        byte[] y800 = gray.getData();
        assertEquals(16, y800.length);

        assertArrayEquals(exp, y800);
    }
}
