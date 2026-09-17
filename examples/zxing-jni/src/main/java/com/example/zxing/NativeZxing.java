package com.example.zxing;

import java.awt.Graphics2D;
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferByte;
import java.io.IOException;
import java.nio.file.Path;
import javax.imageio.ImageIO;

/** JNI facade for zxing-cpp's C API. */
public final class NativeZxing {
    static { System.loadLibrary("ZXingJni"); }
    private NativeZxing() {}

    /** Receives packed B,G,R pixels (width * height * 3 bytes). */
    private static native Barcode[] readBgr(byte[] bgr, int width, int height);

    /** Reads a JPG/PNG image, converts it to BGR and passes it to JNI. */
    public static Barcode[] readImage(Path imageFile) throws IOException {
        BufferedImage input = ImageIO.read(imageFile.toFile());
        if (input == null) throw new IOException("Unsupported image: " + imageFile);
        BufferedImage bgrImage = new BufferedImage(input.getWidth(), input.getHeight(), BufferedImage.TYPE_3BYTE_BGR);
        Graphics2D graphics = bgrImage.createGraphics();
        try { graphics.drawImage(input, 0, 0, null); } finally { graphics.dispose(); }
        byte[] bgr = ((DataBufferByte) bgrImage.getRaster().getDataBuffer()).getData();
        return readBgr(bgr, bgrImage.getWidth(), bgrImage.getHeight());
    }

    /** Java-8 replacement for a record. bytes are the original payload. */
    public static final class Barcode {
        private final String text;
        private final byte[] bytes;
        private final String format;
        private final int topLeftX, topLeftY, topRightX, topRightY;
        private final int bottomRightX, bottomRightY, bottomLeftX, bottomLeftY;

        public Barcode(String text, byte[] bytes, String format,
                       int topLeftX, int topLeftY, int topRightX, int topRightY,
                       int bottomRightX, int bottomRightY, int bottomLeftX, int bottomLeftY) {
            this.text = text;
            this.bytes = bytes;
            this.format = format;
            this.topLeftX = topLeftX;
            this.topLeftY = topLeftY;
            this.topRightX = topRightX;
            this.topRightY = topRightY;
            this.bottomRightX = bottomRightX;
            this.bottomRightY = bottomRightY;
            this.bottomLeftX = bottomLeftX;
            this.bottomLeftY = bottomLeftY;
        }

        public String getText() { return text; }
        /** A defensive copy protects this immutable value object. */
        public byte[] getBytes() { return bytes.clone(); }
        public String getFormat() { return format; }
        public int getTopLeftX() { return topLeftX; }
        public int getTopLeftY() { return topLeftY; }
        public int getTopRightX() { return topRightX; }
        public int getTopRightY() { return topRightY; }
        public int getBottomRightX() { return bottomRightX; }
        public int getBottomRightY() { return bottomRightY; }
        public int getBottomLeftX() { return bottomLeftX; }
        public int getBottomLeftY() { return bottomLeftY; }
    }
}
