package com.example.zxing;

import java.nio.file.Paths;

/** Usage: Demo barcode.jpg */
public final class Demo {
    public static void main(String[] args) throws Exception {
        if (args.length != 1) {
            System.err.println("Usage: Demo barcode.jpg");
            System.exit(2);
        }
        for (NativeZxing.Barcode barcode : NativeZxing.readImage(Paths.get(args[0]))) {
            System.out.printf("%s: %s (%d payload bytes)%n",
                    barcode.getFormat(), barcode.getText(), barcode.getBytes().length);
        }
    }
}
