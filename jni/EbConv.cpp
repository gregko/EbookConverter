#include <stdlib.h>
#include "libs/libmobi/src/save_epub.h"
#include "libs/fb2toepub/hdr.h"
#include "libs/fb2toepub/streamzip.h"
#include "libs/fb2toepub/streamconv.h"
#include "libs/fb2toepub/scanner.h"
#include "libs/fb2toepub/fb2toepubconv.h"

using namespace Fb2ToEpub;

int MyTestFunction()
{
	// a fake call just to pull into the .so object all the functions we need
	return convertMobiToEpub("test.mobi", "test.epub", NULL, false);
}

int MyTestFb2Function()
{
	int ret;
	try
	{
		// create input stream
		Ptr<InStm> pin = CreateInUnicodeStm(CreateUnpackStm("test.fb2"));

		// create output stream
		Ptr<OutPackStm> pout = CreatePackStm("test.epub");
		bool fOutputFileCreated = true;

		// create translite converter
		Ptr<XlitConv> xlitConv; // empty transliterator, we don't need it.

		strvector css, fonts, mfonts;
		// css.push_back(std::string(globalExternalFilesPath) + "/assets/css");
	
		ret = Convert(pin, css, fonts, mfonts, xlitConv, pout);
	}
	catch (InternalException& ei)
	{
		//LOGD("InternalException in Convert(): %s, file: %s, line: %d", ei.what(), ei.File(), ei.Line());
		ret = -3;
	}
	catch (ParserException& ep)
	{
		//LOGD("ParserException in Convert(): %s, file: %s, 1st line %d, last line %d", ep.what(), ep.File(), ep.Location().fstLn_, ep.Location().lstLn_);
		ret = -4;
	}
	catch (FontException& ef)
	{
		//LOGD("FontException in Convert(): %s, file: %s", ef.what(), ef.File());
		ret = -5;
	}
	catch (IOException& e)
	{
		//LOGD("IOException in Convert(): %s, file: %s", e.what(), e.File());
		ret = -1;
	}
	catch (ExternalException& ee)
	{
		//LOGD("Exception in Convert(): %s", ee.what());
		ret = -2;
	}
	catch (...)
	{
		//LOGD("Unknown exception in Convert.");
		ret = -9;
	}

	return ret;
}