#include <jni.h>
#include <ZXing/ZXingC.h>

namespace {

void throwException(JNIEnv* env, const char* type, const char* message)
{
    jclass exceptionClass = env->FindClass(type);
    if (exceptionClass != nullptr)
        env->ThrowNew(exceptionClass, message);
}

jobject toJavaBarcode(JNIEnv* env, const ZXing_Barcode* barcode)
{
    char* text = ZXing_Barcode_text(barcode);
    char* format = ZXing_BarcodeFormatToString(ZXing_Barcode_format(barcode));
    int byteCount = 0;
    uint8_t* bytes = ZXing_Barcode_bytes(barcode, &byteCount);
    const ZXing_Position position = ZXing_Barcode_position(barcode);

    jstring javaText = env->NewStringUTF(text != nullptr ? text : "");
    jstring javaFormat = env->NewStringUTF(format != nullptr ? format : "Unknown");
    jbyteArray javaBytes = env->NewByteArray(byteCount);
    if (javaBytes != nullptr && byteCount > 0)
        env->SetByteArrayRegion(javaBytes, 0, byteCount, reinterpret_cast<const jbyte*>(bytes));
    if (text != nullptr) ZXing_free(text);
    if (format != nullptr) ZXing_free(format);
    if (bytes != nullptr) ZXing_free(bytes);

    jclass barcodeClass = env->FindClass("com/example/zxing/NativeZxing$Barcode");
    jmethodID constructor = env->GetMethodID(
        barcodeClass, "<init>", "(Ljava/lang/String;[BLjava/lang/String;IIIIIIII)V");

    return env->NewObject(barcodeClass, constructor, javaText, javaBytes, javaFormat,
        position.topLeft.x, position.topLeft.y,
        position.topRight.x, position.topRight.y,
        position.bottomRight.x, position.bottomRight.y,
        position.bottomLeft.x, position.bottomLeft.y);
}

} // namespace

extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_example_zxing_NativeZxing_readBgr(
    JNIEnv* env, jclass /* caller */, jbyteArray bgr, jint width, jint height)
{
    if (bgr == nullptr || width <= 0 || height <= 0) {
        throwException(env, "java/lang/IllegalArgumentException", "Invalid BGR image arguments");
        return nullptr;
    }

    const jlong neededBytes = static_cast<jlong>(width) * height * 3;
    if (env->GetArrayLength(bgr) < neededBytes) {
        throwException(env, "java/lang/IllegalArgumentException", "BGR byte array is too small");
        return nullptr;
    }

    jbyte* pixels = env->GetByteArrayElements(bgr, nullptr);
    if (pixels == nullptr)
        return nullptr; // Die JVM hat OutOfMemoryError gesetzt.

    auto* image = ZXing_ImageView_new_checked(
        reinterpret_cast<const uint8_t*>(pixels), static_cast<int>(neededBytes),
        width, height, ZXing_ImageFormat_BGR, width * 3, 3);
    auto* options = ZXing_ReaderOptions_new();
    if (options != nullptr) {
        ZXing_ReaderOptions_setTryRotate(options, true);
        ZXing_ReaderOptions_setTryInvert(options, true);
    }
    ZXing_Barcodes* results = image != nullptr ? ZXing_ReadBarcodes(image, options) : nullptr;

    // zxing-cpp greift ab hier nicht mehr auf den Java-Puffer zu.
    env->ReleaseByteArrayElements(bgr, pixels, JNI_ABORT);
    if (options != nullptr) ZXing_ReaderOptions_delete(options);
    if (image != nullptr) ZXing_ImageView_delete(image);

    if (results == nullptr) {
        char* error = ZXing_LastErrorMsg();
        throwException(env, "java/lang/RuntimeException", error != nullptr ? error : "ZXing error");
        ZXing_free(error);
        return nullptr;
    }

    const int count = ZXing_Barcodes_size(results);
    jclass barcodeClass = env->FindClass("com/example/zxing/NativeZxing$Barcode");
    jobjectArray output = env->NewObjectArray(count, barcodeClass, nullptr);
    for (int index = 0; output != nullptr && index < count; ++index) {
        jobject item = toJavaBarcode(env, ZXing_Barcodes_at(results, index));
        if (item == nullptr || env->ExceptionCheck())
            break;
        env->SetObjectArrayElement(output, index, item);
        env->DeleteLocalRef(item);
    }
    ZXing_Barcodes_delete(results);
    return output;
}
