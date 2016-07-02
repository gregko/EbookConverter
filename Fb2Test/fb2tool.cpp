#include "fb2toepub/hdr.h"
#include "fb2toepub/streamzip.h"
#include "fb2toepub/streamconv.h"
#include "fb2toepub/scanner.h"
#include "fb2toepub/fb2toepubconv.h"

#define LOGD printf

using namespace Fb2ToEpub;

int fb2ToEpub(const char* fnameFb2, const char* cssDir, const char* fnameEpub)
{
	int ret;
	try
	{
		// create input stream
		Ptr<InStm> pin = CreateInUnicodeStm(CreateUnpackStm(fnameFb2));

		// create output stream
		Ptr<OutPackStm> pout = CreatePackStm(fnameEpub);
		bool fOutputFileCreated = true;

		// create translite converter
		Ptr<XlitConv> xlitConv; // empty transliterator, we don't need it.

		strvector css, fonts, mfonts;
		// TODO: need the fb2styles.css file somehow.
		if (cssDir != NULL)
			css.push_back(std::string(cssDir));

		// TODO:
		// Test JimBoton.fb2! Still empty on Android, though worked with a strange error on PC.
		// The "strange error" is </head> text visible at start of each chapter or section.
		// This is caused by XML tag <title/>, which my sentence splitter does not process correctly...
		ret = Convert(pin, css, fonts, mfonts, xlitConv, pout);
	}
	catch (InternalException& ei)
	{
		LOGD("InternalException in Convert(): %s, file: %s, line: %d", ei.what(), ei.File(), ei.Line());
		ret = -3;
	}
	catch (ParserException& ep)
	{
		LOGD("ParserException in Convert(): %s, file: %s, 1st line %d, last line %d", ep.what(), ep.File(), ep.Location().fstLn_, ep.Location().lstLn_);
		ret = -4;
	}
	catch (FontException& ef)
	{
		LOGD("FontException in Convert(): %s, file: %s", ef.what(), ef.File());
		ret = -5;
	}
	catch (IOException& e)
	{
		LOGD("IOException in Convert(): %s, file: %s", e.what(), e.File());
		ret = -1;
	}
	catch (ExternalException& ee)
	{
		LOGD("Exception in Convert(): %s", ee.what());
		ret = -2;
	}
	catch (...)
	{
		LOGD("Unknown exception in Convert.");
		ret = -9;
	}

	return ret;
}

int main(int argc, char* argv[])
{
	if (argc < 2) {
		printf("Usage: Fb2Test filename.fb2\n");
		printf("   or: Fb2Test filename.fb2.zip\n");
		printf("Will output filename.fb2.epub.");
		return 1;
	}
	std::string outName = argv[1];
	int n = outName.find_last_of('.');
	if (outName.substr(n) == ".zip")
		outName = outName.substr(0, n);
	outName += +".epub";
	return fb2ToEpub(argv[1], NULL, outName.c_str());
}
