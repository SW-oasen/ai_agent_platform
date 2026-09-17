// Windows command-line barcode reader.
// Usage: ZXingJpegCli.exe image.jpg
// stdout: one Base64-encoded barcode payload per line; diagnostics: stderr.

// Prevent Windows' min/max macros from corrupting C++ headers included by ZXing.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincodec.h>

#include <cstdint>
#include <cwchar>
#include <iostream>
#include <string>
#include <vector>

#include <ZXing/ZXingC.h>

namespace {

template <typename T>
void release(T*& pointer)
{
    if (pointer != nullptr) {
        pointer->Release();
        pointer = nullptr;
    }
}

std::string base64(const uint8_t* data, int size)
{
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve(((size + 2) / 3) * 4);
    for (int i = 0; i < size; i += 3) {
        const unsigned value = static_cast<unsigned>(data[i]) << 16 |
            (i + 1 < size ? static_cast<unsigned>(data[i + 1]) << 8 : 0) |
            (i + 2 < size ? static_cast<unsigned>(data[i + 2]) : 0);
        result += alphabet[(value >> 18) & 63];
        result += alphabet[(value >> 12) & 63];
        result += i + 1 < size ? alphabet[(value >> 6) & 63] : '=';
        result += i + 2 < size ? alphabet[value & 63] : '=';
    }
    return result;
}

struct Arguments {
    const wchar_t* imageFile = nullptr;
    bool tryHarder = false;
    bool tryRotate = false;
    bool tryDownscale = false;
    std::string formats;
};

bool parseArguments(int argc, wchar_t* argv[], Arguments& arguments)
{
    for (int index = 1; index < argc; ++index) {
        const wchar_t* value = argv[index];
        if (std::wcscmp(value, L"--hard") == 0) {
            arguments.tryHarder = true;
        } else if (std::wcscmp(value, L"--rotate") == 0) {
            arguments.tryRotate = true;
        } else if (std::wcscmp(value, L"--downscale") == 0) {
            arguments.tryDownscale = true;
        } else if (std::wcsncmp(value, L"--formats=", 10) == 0) {
            // Barcode format identifiers are ASCII (for example: AZTEC,QRCode).
            for (const wchar_t* character = value + 10; *character != L'\0'; ++character) {
                if (*character > 0x7f) {
                    std::cerr << "--formats accepts ASCII format identifiers only\n";
                    return false;
                }
                arguments.formats += static_cast<char>(*character);
            }
            if (arguments.formats.empty()) {
                std::cerr << "--formats requires a comma-separated value\n";
                return false;
            }
        } else if (value[0] == L'-') {
            std::cerr << "Unknown option\n";
            return false;
        } else if (arguments.imageFile == nullptr) {
            arguments.imageFile = value;
        } else {
            std::cerr << "Only one image file is allowed\n";
            return false;
        }
    }
    if (arguments.imageFile == nullptr) {
        std::cerr << "An image file is required\n";
        return false;
    }
    return true;
}

bool loadBgr(const wchar_t* filename, std::vector<uint8_t>& pixels, UINT& width, UINT& height)
{
    IWICImagingFactory* factory = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* converter = nullptr;
    bool success = false;

    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&factory));
    if (SUCCEEDED(hr))
        hr = factory->CreateDecoderFromFilename(filename, nullptr, GENERIC_READ,
                                                WICDecodeMetadataCacheOnLoad, &decoder);
    if (SUCCEEDED(hr))
        hr = decoder->GetFrame(0, &frame);
    if (SUCCEEDED(hr))
        hr = factory->CreateFormatConverter(&converter);
    if (SUCCEEDED(hr))
        hr = converter->Initialize(frame, GUID_WICPixelFormat24bppBGR,
                                   WICBitmapDitherTypeNone, nullptr, 0.0,
                                   WICBitmapPaletteTypeCustom);
    if (SUCCEEDED(hr))
        hr = converter->GetSize(&width, &height);

    const UINT stride = width * 3;
    if (SUCCEEDED(hr) && (width == 0 || height == 0 || height > UINT_MAX / stride))
        hr = E_INVALIDARG;
    if (SUCCEEDED(hr)) {
        pixels.resize(static_cast<size_t>(stride) * height);
        hr = converter->CopyPixels(nullptr, stride, static_cast<UINT>(pixels.size()), pixels.data());
    }
    success = SUCCEEDED(hr);
    if (!success)
        std::cerr << "Cannot decode image (Windows error 0x" << std::hex << static_cast<unsigned long>(hr) << ")\n";

    release(converter);
    release(frame);
    release(decoder);
    release(factory);
    return success;
}

} // namespace

int wmain(int argc, wchar_t* argv[])
{
    Arguments arguments;
    if (!parseArguments(argc, argv, arguments)) {
        std::cerr << "Usage: ZXingJpegCli.exe [--hard] [--rotate] [--downscale] "
                     "[--formats=AZTEC,QRCode] <image.jpg>\n";
        return -1;
    }

    const HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(init) && init != RPC_E_CHANGED_MODE) {
        std::cerr << "Cannot initialize Windows COM\n";
        return -2;
    }

    std::vector<uint8_t> pixels;
    UINT width = 0, height = 0;
    if (!loadBgr(arguments.imageFile, pixels, width, height)) {
        if (SUCCEEDED(init)) CoUninitialize();
        return -3;
    }

    ZXing_ImageView* image = ZXing_ImageView_new_checked(pixels.data(), static_cast<int>(pixels.size()),
        static_cast<int>(width), static_cast<int>(height), ZXing_ImageFormat_BGR, static_cast<int>(width * 3), 3);
    ZXing_ReaderOptions* options = ZXing_ReaderOptions_new();
    if (options != nullptr) {
        // Do not call a setter for absent switches: retain zxing-cpp defaults.
        if (arguments.tryHarder) ZXing_ReaderOptions_setTryHarder(options, true);
        if (arguments.tryRotate) ZXing_ReaderOptions_setTryRotate(options, true);
        if (arguments.tryDownscale) ZXing_ReaderOptions_setTryDownscale(options, true);
        if (!arguments.formats.empty()) {
            int formatCount = 0;
            ZXing_BarcodeFormat* formats = ZXing_BarcodeFormatsFromString(arguments.formats.c_str(), &formatCount);
            if (formats == nullptr || formatCount == 0) {
                if (formats != nullptr) ZXing_free(formats);
                ZXing_ReaderOptions_delete(options);
                ZXing_ImageView_delete(image);
                if (SUCCEEDED(init)) CoUninitialize();
                std::cerr << "Invalid --formats value: " << arguments.formats << '\n';
                return -5;
            }
            ZXing_ReaderOptions_setFormats(options, formats, formatCount);
            ZXing_free(formats);
        }
    }
    ZXing_Barcodes* barcodes = image != nullptr ? ZXing_ReadBarcodes(image, options) : nullptr;
    if (options != nullptr) ZXing_ReaderOptions_delete(options);
    if (image != nullptr) ZXing_ImageView_delete(image);
    if (SUCCEEDED(init)) CoUninitialize();

    if (barcodes == nullptr) {
        char* error = ZXing_LastErrorMsg();
        std::cerr << (error != nullptr ? error : "ZXing barcode reader failed") << '\n';
        if (error != nullptr) ZXing_free(error);
        return -6;
    }

    const int count = ZXing_Barcodes_size(barcodes);
    if (count == 0) {
        ZXing_Barcodes_delete(barcodes);
        return 0;
    }

    std::vector<std::string> payloads;
    payloads.reserve(count);
    for (int index = 0; index < count; ++index) {
        int length = 0;
        uint8_t* bytes = ZXing_Barcode_bytes(ZXing_Barcodes_at(barcodes, index), &length);
        if (bytes == nullptr && length != 0) {
            ZXing_Barcodes_delete(barcodes);
            std::cerr << "Cannot obtain barcode payload\n";
            return -7;
        }
        payloads.push_back(base64(bytes, length));
        if (bytes != nullptr) ZXing_free(bytes);
    }
    ZXing_Barcodes_delete(barcodes);

    for (const auto& payload : payloads)
        std::cout << payload << '\n';
    return count;
}
