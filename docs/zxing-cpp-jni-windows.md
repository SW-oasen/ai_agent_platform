# ZXing-C++ aus Java per JNI verwenden (Windows x64)

Diese Anleitung bindet die native **zxing-cpp**-Bibliothek in eine Java-Anwendung ein:

```text
Java-Anwendung
    │  JNI
    ▼
ZXingJni.dll       ← eigene, kleine JNI-Bridge
    │  C-API (ZXingC.h)
    ▼
ZXing.dll          ← zxing-cpp, als x64 Shared Library gebaut
```

Die Datei aus `pip install zxing-cpp` ist dafür nicht geeignet: Sie ist eine Python-Erweiterung (`.pyd`) und exportiert die Python-C-API, nicht JNI-Funktionen.

## Voraussetzungen

- 64-Bit-JDK mit `JAVA_HOME` (z. B. JDK 21)
- Visual Studio 2019 16.10 oder neuer, Workload **Desktop development with C++**
- CMake
- eine als DLL gebaute zxing-cpp-Installation, z. B. in `D:\Projects\API\ZXing-cpp\zxing-install`

Die Architektur der JVM, `ZXingJni.dll` und `ZXing.dll` muss jeweils **x64** sein.

## Die relevante zxing-cpp-Schnittstelle

Für eine JNI-Bridge sollte ausschließlich die stabile C-API aus `ZXing/ZXingC.h` verwendet werden. Die wichtigsten Aufrufe zum Lesen eines Bildpuffers sind:

| Aufgabe | C-API |
| --- | --- |
| Bildpuffer beschreiben | `ZXing_ImageView_new_checked(...)` |
| Leseoptionen anlegen | `ZXing_ReaderOptions_new()` |
| Formate / Robustheit setzen | `ZXing_ReaderOptions_setFormats`, `setTryHarder`, `setTryRotate`, `setTryInvert` |
| Codes lesen | `ZXing_ReadBarcodes(...)` |
| Anzahl / einzelnes Resultat | `ZXing_Barcodes_size`, `ZXing_Barcodes_at` |
| Text / Format / Position | `ZXing_Barcode_text`, `ZXing_Barcode_format`, `ZXing_Barcode_position` |
| Speicher freigeben | `ZXing_Barcodes_delete`, `ZXing_ImageView_delete`, `ZXing_free` |

**Speicherregel:** Zeichenketten wie das Resultat von `ZXing_Barcode_text()` gehören dem Aufrufer und werden mit `ZXing_free()` freigegeben. Ein Element von `ZXing_Barcodes_at()` gehört dagegen dem Ergebniscontainer und darf nicht einzeln gelöscht werden.

`ZXing_ImageView` kopiert die Pixel nicht. Die Java-Bytes müssen deshalb während `ZXing_ReadBarcodes()` gültig bleiben. Im folgenden Beispiel garantiert `GetByteArrayElements` genau das.

## Schlanke Java-API

`src/main/java/com/example/zxing/NativeZxing.java`:

```java
package com.example.zxing;

import java.nio.file.Path;

public final class NativeZxing {
    static {
        // Variante A: DLLs liegen in java.library.path.
        System.loadLibrary("ZXingJni");

        // Variante B: bei einer mitgelieferten Anwendung mit absolutem Pfad laden:
        // System.load(Path.of("native", "ZXingJni.dll").toAbsolutePath().toString());
    }

    private NativeZxing() {}

    /**
     * Liest alle Barcodes aus einem unveränderten Pixelpuffer.
     * @param pixels Pixel, zeilenweise ohne Lücken bei RGB: width * height * 3 Bytes
     */
    public static native Barcode[] readRgb(byte[] pixels, int width, int height);

    public record Barcode(String text, String format,
                          int topLeftX, int topLeftY, int topRightX, int topRightY,
                          int bottomRightX, int bottomRightY, int bottomLeftX, int bottomLeftY) {}
}
```

Aufruf (beispielhaft mit bereits als RGB vorliegenden Bytes):

```java
NativeZxing.Barcode[] codes = NativeZxing.readRgb(rgbBytes, width, height);
for (var code : codes) {
    System.out.printf("%s: %s%n", code.format(), code.text());
}
```

Für `BufferedImage` sollten die Daten nicht blind mit `getData()` verwendet werden, weil Farbmodell, Kanalreihenfolge und Zeilenabstand variieren können. Konvertiere vorher explizit nach `BufferedImage.TYPE_3BYTE_BGR` und verwende in der Bridge `ZXing_ImageFormat_BGR`; oder konvertiere nach RGB und verwende das untenstehende Interface.

## JNI-Implementierung

`src/main/cpp/ZXingJni.cpp`:

```cpp
#include <jni.h>
#include <string>
#include <vector>
#include <ZXing/ZXingC.h>

namespace {
void throwJava(JNIEnv* env, const char* className, const char* message)
{
    jclass clazz = env->FindClass(className);
    if (clazz != nullptr)
        env->ThrowNew(clazz, message);
}

jobject makeBarcode(JNIEnv* env, const ZXing_Barcode* barcode)
{
    char* nativeText = ZXing_Barcode_text(barcode);
    char* nativeFormat = ZXing_BarcodeFormatToString(ZXing_Barcode_format(barcode));
    ZXing_Position p = ZXing_Barcode_position(barcode);

    jstring text = env->NewStringUTF(nativeText ? nativeText : "");
    jstring format = env->NewStringUTF(nativeFormat ? nativeFormat : "Unknown");
    if (nativeText) ZXing_free(nativeText);
    if (nativeFormat) ZXing_free(nativeFormat);
    if (text == nullptr || format == nullptr) return nullptr;

    jclass clazz = env->FindClass("com/example/zxing/NativeZxing$Barcode");
    jmethodID ctor = env->GetMethodID(clazz, "<init>",
        "(Ljava/lang/String;Ljava/lang/String;IIIIIIII)V");
    return env->NewObject(clazz, ctor, text, format,
        p.topLeft.x, p.topLeft.y, p.topRight.x, p.topRight.y,
        p.bottomRight.x, p.bottomRight.y, p.bottomLeft.x, p.bottomLeft.y);
}
}

extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_example_zxing_NativeZxing_readRgb(
    JNIEnv* env, jclass, jbyteArray pixels, jint width, jint height)
{
    if (pixels == nullptr || width <= 0 || height <= 0) {
        throwJava(env, "java/lang/IllegalArgumentException", "pixels, width and height must be valid");
        return nullptr;
    }

    const jlong expected = static_cast<jlong>(width) * height * 3;
    if (env->GetArrayLength(pixels) < expected) {
        throwJava(env, "java/lang/IllegalArgumentException", "RGB buffer is too small");
        return nullptr;
    }

    jboolean copied = JNI_FALSE;
    jbyte* data = env->GetByteArrayElements(pixels, &copied);
    if (data == nullptr) return nullptr; // JVM hat bereits OutOfMemoryError gesetzt

    auto* image = ZXing_ImageView_new_checked(
        reinterpret_cast<const uint8_t*>(data), static_cast<int>(expected),
        width, height, ZXing_ImageFormat_RGB, width * 3, 3);
    auto* options = ZXing_ReaderOptions_new();
    if (options) {
        ZXing_ReaderOptions_setTryRotate(options, true);
        ZXing_ReaderOptions_setTryInvert(options, true);
    }
    ZXing_Barcodes* results = image ? ZXing_ReadBarcodes(image, options) : nullptr;

    // Der native Code liest nicht mehr aus data: JNI-Puffer ohne Rückkopie freigeben.
    env->ReleaseByteArrayElements(pixels, data, JNI_ABORT);
    if (options) ZXing_ReaderOptions_delete(options);
    if (image) ZXing_ImageView_delete(image);

    if (results == nullptr) {
        char* error = ZXing_LastErrorMsg();
        throwJava(env, "java/lang/RuntimeException", error ? error : "ZXing native error");
        if (error) ZXing_free(error);
        return nullptr;
    }

    const int count = ZXing_Barcodes_size(results);
    jclass barcodeClass = env->FindClass("com/example/zxing/NativeZxing$Barcode");
    jobjectArray output = env->NewObjectArray(count, barcodeClass, nullptr);
    for (int i = 0; output != nullptr && i < count; ++i) {
        jobject item = makeBarcode(env, ZXing_Barcodes_at(results, i));
        if (item == nullptr || env->ExceptionCheck()) break;
        env->SetObjectArrayElement(output, i, item);
        env->DeleteLocalRef(item);
    }
    ZXing_Barcodes_delete(results);
    return output;
}
```

Die Funktionssignatur in C++ muss exakt dem Java-Paket, Klassennamen und der `native`-Methode entsprechen. Zum Verifizieren kann `javac -h src/main/cpp ...` einen Header erzeugen.

## CMake für die JNI-Bridge

`src/main/cpp/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.21)
project(ZXingJni LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(JNI REQUIRED)
find_package(ZXing CONFIG REQUIRED)

add_library(ZXingJni SHARED ZXingJni.cpp)
target_link_libraries(ZXingJni PRIVATE JNI::JNI ZXing::ZXing)
```

Falls `find_package(ZXing ...)` die Installation nicht findet, beim Konfigurieren den Installationspfad angeben:

```powershell
cmake -S src/main/cpp -B build/jni -A x64 `
  "-DCMAKE_PREFIX_PATH=D:\Projects\API\ZXing-cpp\zxing-install"
cmake --build build/jni --config Release
```

Falls `find_package(JNI REQUIRED)` scheitert, ist meist `JAVA_HOME` nicht gesetzt oder zeigt auf eine JRE statt auf ein JDK:

```powershell
$env:JAVA_HOME
java -version
javac -version
```

## DLLs zur Laufzeit bereitstellen

Beim Start muss Windows sowohl `ZXingJni.dll` als auch deren Abhängigkeit `ZXing.dll` finden. Lege beide DLLs in denselben Ordner, etwa:

```text
mein-programm/
  app.jar
  native/
    ZXingJni.dll
    ZXing.dll
```

Dann kann `System.load(...)` `ZXingJni.dll` über den absoluten Pfad laden. Windows sucht für dessen Abhängigkeit zuerst im gleichen Ordner. Eine Alternative ist der JVM-Startparameter:

```powershell
java "-Djava.library.path=$PWD\native" -jar app.jar
```

`ZXing.dll` darf nicht nur beim Linken vorhanden sein; sie muss auch mit ausgeliefert werden. Bei einem dynamisch gelinkten MSVC-Build ist außerdem ggf. der aktuelle Microsoft Visual C++ Redistributable erforderlich. Mit `-DZXING_LINK_CPP_STATICALLY=ON` beim Bau von zxing-cpp kann diese zusätzliche C++-Runtime-Abhängigkeit reduziert werden.

## Alternative: eigener Prozess statt JNI

Eine `ZXingReader.exe` kann von Java über `ProcessBuilder` aufgerufen werden. Das ist sinnvoll für seltene Datei-Scans und eine bewusst isolierte Prozessgrenze. Für Kamera-Frames oder viele Scans ist JNI geeigneter: kein Prozessstart je Bild und kein Dateiaustausch. Die Beispiel-EXE ist jedoch keine stabile JSON-API; dafür müsste ein eigenes CLI mit definiertem Ein-/Ausgabeformat gebaut werden.

## Quellen

- [zxing-cpp: Build-Hinweise und Paketquellen](https://github.com/zxing-cpp/zxing-cpp)
- [Offizielle C-API: `ZXingC.h`](https://github.com/zxing-cpp/zxing-cpp/blob/master/core/src/ZXingC.h)
- [Offizieller Android-JNI-Wrapper als weiteres Referenzbeispiel](https://github.com/zxing-cpp/zxing-cpp/blob/master/wrappers/android/zxingcpp/src/main/cpp/ZXingCpp.cpp)
