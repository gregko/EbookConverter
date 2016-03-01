# EbookConverter for Android

MOBI/AZW/PRC and FB2 ebook format converter to EPUB. Using open source libraries libmobi by Bartek Fabiszewski
(https://github.com/bfabiszewski/libmobi) and fb2-to-epub-converter by Alexey Bobkov 
(https://code.google.com/archive/p/fb2-to-epub-converter/)

The shared library libEbookConv.so is used in my @Voice Aloud Reader app for Android under GNU LGPL license,
therefore publishing this source code here also serves to fulfill the terms of LGPL.

The EbookConv.sln and .vcxproj are Visual Studio 2015 solution and project which I use to develop and debug
the native Android code, with the help of VisualGDB tools, but the native .so libraries can be also built
stand alone by running Android ndk-build command in jni directory.

The MobiTest and Fb2Test are Visual Studio C++ windows command line programs to test the converter code.

The EbookConvTest is a small Android application project (APK) for testing under Android and for the
end users, should they wish to use this tool for ebook format conversions.

Greg Kochaniak, Hyperionics
