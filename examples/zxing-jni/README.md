# ZXing-C++ JNI example (Windows x64)

The example builds `ZXingJni.dll` and reads a normal JPG or PNG barcode image from Java. Java decodes the image with `ImageIO`, converts it to packed BGR pixels, and JNI calls the zxing-cpp C API.

`image.rgb` is not used. It would be an uncompressed raw B,G,R pixel stream without a JPEG/PNG header; that format is useful only for camera buffers and tests.

## Prerequisites

Install zxing-cpp as a shared library first. The prefix must contain, for example:

```text
D:\Projects\API\ZXing-cpp\zxing-install\
  bin\ZXing.dll
  lib\ZXing.lib
  lib\cmake\ZXing\...
  include\ZXing\ZXingC.h
```

## Build Java and the JNI DLL

Run these commands from this directory:

```powershell
javac -source 8 -target 8 -d build\classes src\main\java\com\example\zxing\NativeZxing.java src\main\java\com\example\zxing\Demo.java

$env:ZXING_PREFIX = 'D:\Projects\API\ZXing-cpp\zxing-cpp\zxing-install'
cmake -S src\main\cpp -B build\native -A x64 "-DCMAKE_PREFIX_PATH=$env:ZXING_PREFIX"
cmake --build build\native --config Release
```

This second CMake build compiles `ZXingJni.cpp` and creates:

```text
build\native\Release\ZXingJni.dll
```

## Run with a JPG

Both native DLLs must be accessible at runtime:

```powershell
New-Item -ItemType Directory -Force build\native-runtime
Copy-Item build\native\Release\ZXingJni.dll build\native-runtime
Copy-Item "$env:ZXING_PREFIX\bin\ZXing.dll" build\native-runtime

java "-Djava.library.path=$PWD\build\native-runtime" -cp build\classes com.example.zxing.Demo barcode.jpg
```

Example output:

```text
QRCode: https://example.org (19 payload bytes)
```

`Barcode.getText()` is zxing-cpp's text interpretation. `Barcode.getBytes()` always contains the original barcode payload, so use it for binary content.

The JVM, `ZXingJni.dll`, and `ZXing.dll` must all be x64. If the zxing-cpp DLL was built dynamically with MSVC, the Microsoft Visual C++ Redistributable can also be needed on the target computer.

## Standalone EXE: JPG to Base64

The same CMake project also builds `ZXingJpegCli.exe`. It uses Windows Image Component (WIC), so it accepts JPG and PNG files without Java or Python. Build just this target:

```powershell
cmake --build .\build\native --config Release --target ZXingJpegCli
```

Copy `ZXing.dll` next to it, then call it. The executable returns the original barcode payloads as Base64. Errors are written to `stderr`.

```powershell
Copy-Item "$env:ZXING_PREFIX\bin\ZXing.dll" .\build\native\Release -Force
.\build\native\Release\ZXingJpegCli.exe barcode.jpg
```

Optional reader switches:

```powershell
.\build\native\Release\ZXingJpegCli.exe --hard --rotate --downscale --formats=AZTEC,QRCode barcode.jpg
```

- `--hard` enables `ZXing_ReaderOptions_setTryHarder(true)`.
- `--rotate` enables `ZXing_ReaderOptions_setTryRotate(true)`.
- `--downscale` enables `ZXing_ReaderOptions_setTryDownscale(true)`.
- `--formats=...` limits recognition to the comma-separated format list, e.g. `--formats=AZTEC,QRCode`.

Exit-code and stdout contract:

| Result | Exit code | stdout |
| --- | ---: | --- |
| no barcode | `0` | empty |
| one barcode | `1` | one Base64 line |
| multiple barcodes | number of results, e.g. `2` | one Base64 line per barcode |
| technical error | negative | empty; diagnostic on stderr |
