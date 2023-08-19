//
//  Copyright (C) 2010 Alexey Bobkov
//
//  This file is part of Fb2toepub converter.
//
//  Fb2toepub converter is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Fb2toepub converter is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Fb2toepub converter.  If not, see <http://www.gnu.org/licenses/>.
//


#include "hdr.h"

#include "converter.h"
#include <sstream>
#include <set>
#include "base64.h"
#include "streamconv.h"

namespace Fb2ToEpub
{
	//-----------------------------------------------------------------------
	// CONVERTER PASS 1 IMPLEMENTATION
	//-----------------------------------------------------------------------
	class FB2TOEPUB_DECL ConverterPass1 : public Object, Noncopyable
	{
	public:
		ConverterPass1(LexScanner *scanner, UnitArray *units) : s_(scanner), units_(units), sectionCnt_(0), textMode_(false), bodyType_(Unit::BODY_NONE) {}

		void Scan();
        void GetMetaData(Fb2MetaData& md, bool wantCoverImage);
        void binaryFindCoverImg(String coverImgId, std::vector<unsigned char>& bytes);

	private:
		Ptr<LexScanner>         s_;
		UnitArray               *units_;
		int                     sectionCnt_;
		bool                    textMode_;
		Unit::BodyType          bodyType_;
		std::set<String>        xlns_;      // xlink namespaces
		std::set<String>        allRefIds_; // all ref ids

		void SwitchUnitIfSizeAbove(std::size_t size, int parent);
		const String* AddId(const AttrMap &attrmap);
		String Findhref(const AttrMap &attrmap) const;
		void ParseTextAndEndElement(const String &element, String *plainText);

		// Old Lex Scanner FictionBook elements
		void FictionBook();
		void a(String *plainText);
        void annotation(bool startUnit = false);
        String annotationText(bool startUnit = false);
        //void author                 ();
		//void binary                 ();
		void body(Unit::BodyType bodyType);
		//void book_name              ();
		//void book_title             ();
		void cite();
		//void city                   ();
		void code(String *plainText);
		void coverpage();
		//void custom_info            ();
		//void date                   ();
		void description();
		//void document_info          ();
		//void email                  ();
		void emphasis(String *plainText);
		void empty_line();
		void epigraph();
		//void first_name             ();
		//void genre                  ();
		//void history                ();
		//void home_page              ();
		//void id                     ();
		void image(bool in_line, Unit::Type unitType = Unit::UNIT_NONE);
		//void isbn                   ();
		//void keywords               ();
		//void lang                   ();
		//void last_name              ();
		//void middle_name            ();
		//void nickname               ();
		//void output_document_class  ();
		//void output                 ();
		void p(String *plainText = NULL);
		//void part                   ();
		void poem();
		//void program_used           ();
		//void publish_info           ();
		//void publisher              ();
		void section(int parent, const char* tag = NULL);
		//void sequence               ();
		//void src_lang               ();
		//void src_ocr                ();
		//void src_title_info         ();
		//void src_url                ();
		void stanza();
		void strikethrough(String *plainText);
		void strong(String *plainText);
		void style(String *plainText);
		//void stylesheet             ();
		void sub(String *plainText);
		void subtitle(String *plainText = NULL);
		void sup(String *plainText);
		void table();
		void td();
		void text_author(String *plainText = NULL);
		void th();
		void title(String *plainText = NULL, bool startUnit = false);
		void title_info();
		void tr();
		//void translator             ();
		void v(String *plainText = NULL);
		//void version                ();
		//void year                   ();
	};

//-----------------------------------------------------------------------
void ConverterPass1::Scan()
{
    s_->SkipXMLDeclaration();
    FictionBook();
}

void ConverterPass1::GetMetaData(Fb2MetaData& md, bool wantCoverImage) {
    s_->SkipXMLDeclaration();

    // Below repeats FictionBook() code up to description
    AttrMap attrmap;
    s_->BeginNotEmptyElement("FictionBook", &attrmap);

    // namespaces
    AttrMap::const_iterator cit = attrmap.begin(), cit_end = attrmap.end();
    bool has_fb = false, has_emptyfb = false;
    for (; cit != cit_end; ++cit)
    {
        static const String xmlns = "xmlns";
        static const std::size_t xmlns_len = xmlns.length();
        static const String fbID = "http://www.gribuser.ru/xml/fictionbook/2.0", xlID = "http://www.w3.org/1999/xlink";
        static const String fbID21 = "http://www.gribuser.ru/xml/fictionbook/2.1";

        if (!cit->second.compare(fbID))
        {
            if (!cit->first.compare(xmlns))
                has_emptyfb = true;
            else if (cit->first.compare(0, xmlns_len + 1, xmlns + ":"))
                s_->Error("bad FictionBook namespace definition");
            has_fb = true;
        }
        else if (!cit->second.compare(fbID21))
        {
            if (!cit->first.compare(xmlns))
                has_emptyfb = true;
            else if (cit->first.compare(0, xmlns_len + 1, xmlns + ":"))
                s_->Error("bad FictionBook namespace definition");
            has_fb = true;
        }
        else if (!cit->second.compare(xlID))
        {
            if (cit->first.compare(0, xmlns_len + 1, xmlns + ":"))
                s_->Error("bad xlink namespace definition");
            xlns_.insert(cit->first.substr(xmlns_len + 1));
        }
    }
    if (!has_fb)
        s_->Error("missing FictionBook namespace definition");
    if (!has_emptyfb)
        s_->Error("non-empty FictionBook namespace not implemented");

    //<stylesheet>
    s_->SkipAll("stylesheet");
    //</stylesheet>

    s_->BeginNotEmptyElement("description");

    //<title-info>
    s_->BeginNotEmptyElement("title-info");

    String coverImgId;

    for (LexScanner::Token t = s_->LookAhead(); t.type_ == LexScanner::START; t = s_->LookAhead())
    {
        if (!t.s_.compare("genre")) {
            s_->CheckAndSkipElement("genre");
            s_->SkipAll("genre");
        }
        else if (!t.s_.compare("author")) {
            s_->BeginNotEmptyElement("author");

            String author, fn, mn, ln;
            for (int i = 0; i < 4; i++) {
                if (s_->IsNextElement("first-name"))
                    fn = s_->SimpleTextElement("first-name");
                else if (s_->IsNextElement("middle-name"))
                    mn = s_->SimpleTextElement("middle-name");
                else if (s_->IsNextElement("last-name"))
                    ln = s_->SimpleTextElement("last-name");
                else if (s_->IsNextElement("nickname"))
                    author = s_->SimpleTextElement("nickname");
            }
            if (author.empty()) {
                author = ln;
                author = Concat(author, ", ", fn);
                author = Concat(author, " ", mn);
            }

            // authors_.push_back(author);
            if (!md.author.empty())
                md.author += "; ";
            md.author += author;
            s_->SkipRestOfElementContent();
        }
        else if (!t.s_.compare("book-title")) {
            md.title = s_->SimpleTextElement("book-title");
        }
        else if (!t.s_.compare("lang")) {
            md.lang = s_->SimpleTextElement("lang");
        }
        else if (!t.s_.compare("annotation")) {
            md.description = annotationText(true);
        }
        else if (!t.s_.compare("coverpage")) {
            //coverpage();
            s_->BeginNotEmptyElement("coverpage");
            units_->push_back(Unit(bodyType_, Unit::COVERPAGE, 0, -1));
            do {
                //image(false);
                AttrMap imgAttrMap;
                s_->BeginElement("image", &imgAttrMap);
                if (imgAttrMap.size() > 0) {
                    std::map<String, String>::iterator it = imgAttrMap.begin();
                    while (it != imgAttrMap.end())
                    {
                        if (it->first.find("href") >= 0) {
                            coverImgId = it->second; // is like "#cover.jpg"
                            if (coverImgId[0] == '#')
                                coverImgId = coverImgId.substr(1);
                            break;
                        }
                        it++;
                    }
                }
            }  while (s_->IsNextElement("image"));
            s_->EndElement();

        }
        else {
            s_->SkipElement();
        }
    }
    s_->SkipRestOfElementContent(); // skip rest of <title-info>
    s_->SkipRestOfElementContent(); // skip rest of <description>
    // end of description

    md.hasCover = !coverImgId.empty();

    if (!wantCoverImage || coverImgId.empty()) // will be empty if wantCoverImage is false
        return;

    //<binary>
    md.coverImgBytes.clear();
    md.coverImgBytes.reserve(128 * 1024);
    while (s_->IsNextElement("binary")) {
        binaryFindCoverImg(coverImgId, md.coverImgBytes); // search for cover image
        if (md.coverImgBytes.size() > 0)
            return;
    }
    while (!s_->IsNextElement("body")) {
        LexScanner::Token t = s_->GetToken();
        s_->UngetToken(t);
        if (t.type_ == LexScanner::START)
            s_->SkipElement();
        else
            s_->SkipRestOfElementContent();
    }

    //<body> elements - ignore for metatdata extraction
    while (s_->IsNextElement("body"))
        s_->SkipElement();
    //</body>

    //<binary>
    while (s_->IsNextElement("binary")) { // images and other binary resources
        binaryFindCoverImg(coverImgId, md.coverImgBytes); // search for cover image, if not found yet.
        if (md.coverImgBytes.size() > 0)
            return;
    }
    //</binary>

}

void ConverterPass1::binaryFindCoverImg(String coverImgId, std::vector<unsigned char>& bytes) {
    AttrMap attrmap;
    s_->BeginNotEmptyElement("binary", &attrmap);
    if (attrmap["id"] != coverImgId) {
        s_->SkipRestOfElementContent();
        return;
    }
    // store binary data in bytes vector
    {
        SetScannerDataMode setDataMode(s_);
        LexScanner::Token t = s_->GetToken();
        if (t.type_ != LexScanner::DATA)
            s_->Error("<binary> data expected");
        // pout_->BeginFile((String("OPS/") + b.file_).c_str(), false);
        Ptr<OutStm> pout = CreateOutMemStm(bytes);
        if (!DecodeBase64(t.s_.c_str(), pout))
            fprintf(stderr, "base64 error\n");
        //s_->Error("base64 error");
    }

    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::SwitchUnitIfSizeAbove(std::size_t size, int parent)
{
    if(units_->size() && units_->back().size_ > size)
        units_->push_back(Unit(bodyType_, Unit::SECTION, sectionCnt_++, parent));
}

//-----------------------------------------------------------------------
const String* ConverterPass1::AddId(const AttrMap &attrmap)
{
    AttrMap::const_iterator cit = attrmap.find("id");
    if(cit == attrmap.end())
        return NULL;

    if(allRefIds_.find(cit->second) != allRefIds_.end())
        return NULL;    // ignore second instance

    units_->back().refIds_.push_back(cit->second);
    return &cit->second;
}

//-----------------------------------------------------------------------
String ConverterPass1::Findhref(const AttrMap &attrmap) const
{
    std::set<String>::const_iterator cit = xlns_.begin(), cit_end = xlns_.end();
    for(; cit != cit_end; ++cit)
    {
        String href;
        if(cit->empty())
            href = "href";
        else
            href = (*cit)+":href";
        AttrMap::const_iterator ait = attrmap.find(href);
        if(ait != attrmap.end())
            return ait->second;
    }
    return "";
}

//-----------------------------------------------------------------------
void ConverterPass1::ParseTextAndEndElement(const String &element, String *plainText)
{
    SetScannerDataMode setDataMode(s_);
    for(;;)
    {
        LexScanner::Token t = s_->LookAhead();
        switch(t.type_)
        {
        default:
            s_->EndElement();
            return;

        case LexScanner::DATA:
            s_->GetToken();
			if (units_->size())
				units_->back().size_ += t.size_;
            if(plainText)
                *plainText += t.s_;
            continue;

        case LexScanner::START:
            //<strong>, <emphasis>, <stile>, <a>, <strikethrough>, <sub>, <sup>, <code>, <image>
			if (!t.s_.compare("strong"))
				strong(plainText);
			else if (!t.s_.compare("emphasis"))
				emphasis(plainText);
			else if (!t.s_.compare("style"))
				style(plainText);
			else if (!t.s_.compare("a"))
				a(plainText);
			else if (!t.s_.compare("strikethrough"))
				strikethrough(plainText);
			else if (!t.s_.compare("sub"))
				sub(plainText);
			else if (!t.s_.compare("sup"))
				sup(plainText);
			else if (!t.s_.compare("code"))
				code(plainText);
			else if (!t.s_.compare("image"))
				image(true);
			else if (!t.s_.compare("cite")) {
				cite();
			}
			else if (!t.s_.compare("p")) {
				p();
			}
			else
            {
                std::ostringstream ss;
                ss << "<" << t.s_ << "> unexpected in <" << element + ">";
                //s_->Error(ss.str());
				s_->SkipElement();
            }
            continue;
            //</strong>, </emphasis>, </stile>, </a>, </strikethrough>, </sub>, </sup>, </code>, </image>
        }
    }
}


//-----------------------------------------------------------------------
void ConverterPass1::FictionBook()
{
    AttrMap attrmap;
    s_->BeginNotEmptyElement("FictionBook", &attrmap);

    // namespaces
    AttrMap::const_iterator cit = attrmap.begin(), cit_end = attrmap.end();
    bool has_fb = false, has_emptyfb = false;
    for(; cit != cit_end; ++cit)
    {
        static const String xmlns = "xmlns";
        static const std::size_t xmlns_len = xmlns.length();
        static const String fbID = "http://www.gribuser.ru/xml/fictionbook/2.0", xlID = "http://www.w3.org/1999/xlink";
		static const String fbID21 = "http://www.gribuser.ru/xml/fictionbook/2.1";

        if(!cit->second.compare(fbID))
        {
            if(!cit->first.compare(xmlns))
                has_emptyfb = true;
            else if(cit->first.compare(0, xmlns_len+1, xmlns+":"))
                s_->Error("bad FictionBook namespace definition");
            has_fb = true;
        }
		else if (!cit->second.compare(fbID21))
		{
			if (!cit->first.compare(xmlns))
				has_emptyfb = true;
			else if (cit->first.compare(0, xmlns_len + 1, xmlns + ":"))
				s_->Error("bad FictionBook namespace definition");
			has_fb = true;
		}
        else if(!cit->second.compare(xlID))
        {
            if(cit->first.compare(0, xmlns_len+1, xmlns+":"))
                s_->Error("bad xlink namespace definition");
            xlns_.insert(cit->first.substr(xmlns_len+1));
        }
    }
    if(!has_fb)
        s_->Error("missing FictionBook namespace definition");
    if(!has_emptyfb)
        s_->Error("non-empty FictionBook namespace not implemented");

    //<stylesheet>
    s_->SkipAll("stylesheet");
    //</stylesheet>

    //<description>
    description();
    //</description>

    //<body>
	while (!s_->IsNextElement("body"))
		s_->SkipElement();
	body(Unit::MAIN);
    if(s_->IsNextElement("body"))
        body(Unit::NOTES);
    if(s_->IsNextElement("body"))
        body(Unit::COMMENTS);
    //</body>
}

//-----------------------------------------------------------------------
void ConverterPass1::a(String *plainText)
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("a", &attrmap);

    String id = Findhref(attrmap);
    if(!id.empty() && id[0] == '#')
        units_->back().refs_.insert(id.substr(1));  // collect internal references

    if(!notempty)
        return;

    SetScannerDataMode setDataMode(s_);
    for(;;)
    {
        LexScanner::Token t = s_->LookAhead();
        switch(t.type_)
        {
        default:
            s_->EndElement();
            return;

        case LexScanner::DATA:
            s_->GetToken();
			if (units_->size())
	            units_->back().size_ += t.size_;
            if(plainText)
                *plainText += t.s_;
            continue;

        case LexScanner::START:
            //<strong>, <emphasis>, <stile>, <strikethrough>, <sub>, <sup>, <code>, <image>
            if(!t.s_.compare("strong"))
                strong(plainText);
            else if(!t.s_.compare("emphasis"))
                emphasis(plainText);
            else if(!t.s_.compare("style"))
                style(plainText);
            else if(!t.s_.compare("strikethrough"))
                strikethrough(plainText);
            else if(!t.s_.compare("sub"))
                sub(plainText);
            else if(!t.s_.compare("sup"))
                sup(plainText);
            else if(!t.s_.compare("code"))
                code(plainText);
            else if(!t.s_.compare("image"))
                image(true);
            else
            {
                std::ostringstream ss;
                ss << "<" << t.s_ << "> unexpected in <a>";
                //s_->Error(ss.str());
				s_->SkipElement();
            }
            continue;
            //</strong>, </emphasis>, </stile>, </strikethrough>, </sub>, </sup>, </code>, </image>
        }
    }
}

//-----------------------------------------------------------------------
void ConverterPass1::annotation(bool startUnit)
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("annotation", &attrmap);
    if(startUnit)
        units_->push_back(Unit(bodyType_, Unit::ANNOTATION, 0, -1));
    AddId(attrmap);
    if(!notempty)
        return;

    for(LexScanner::Token t = s_->LookAhead(); t.type_ == LexScanner::START; t = s_->LookAhead())
    {
        //<p>, <poem>, <cite>, <subtitle>, <empty-line>, <table>
        if(!t.s_.compare("p"))
            p();
        else if(!t.s_.compare("poem"))
            poem();
        else if(!t.s_.compare("cite"))
            cite();
        else if(!t.s_.compare("subtitle"))
            subtitle();
        else if(!t.s_.compare("empty-line"))
            empty_line();
        else if(!t.s_.compare("table"))
            table();
        else
        {
            std::ostringstream ss;
            ss << "<" << t.s_ << "> unexpected in <annotation>";
            //s_->Error(ss.str());
			s_->SkipElement();
        }
        //</p>, </poem>, </cite>, </subtitle>, </empty-line>, </table>
    }

    s_->EndElement();
}

//-----------------------------------------------------------------------
String ConverterPass1::annotationText(bool startUnit)
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("annotation", &attrmap);
    if (startUnit)
        units_->push_back(Unit(bodyType_, Unit::ANNOTATION, 0, -1));
    AddId(attrmap);
    String ann;
    if (!notempty)
        return ann;
    for (LexScanner::Token t = s_->LookAhead(); t.type_ == LexScanner::START; t = s_->LookAhead())
    {
        //<p>, <poem>, <cite>, <subtitle>, <empty-line>, <table>
        if (!t.s_.compare("p")) {
            String text;
            p(&text);
            ann += text + "\n";
        }
        else if (!t.s_.compare("poem"))
            poem();
        else if (!t.s_.compare("cite"))
            cite();
        else if (!t.s_.compare("subtitle"))
            subtitle(&ann);
        else if (!t.s_.compare("empty-line")) {
            empty_line();
            ann += "\n";
        }
        else if (!t.s_.compare("table"))
            table();
        else
        {
            std::ostringstream ss;
            ss << "<" << t.s_ << "> unexpected in <annotation>";
            //s_->Error(ss.str());
            s_->SkipElement();
        }
        //</p>, </poem>, </cite>, </subtitle>, </empty-line>, </table>
    }

    s_->EndElement();
    return ann;
}

//-----------------------------------------------------------------------
void ConverterPass1::body(Unit::BodyType bodyType)
{
    s_->BeginNotEmptyElement("body");

    bodyType_ = bodyType;

	for (LexScanner::Token t = s_->LookAhead(); t.type_ == LexScanner::START; t = s_->LookAhead())
	{
		if (!t.s_.compare("image"))
			image(false, Unit::IMAGE);
		else if (!t.s_.compare("title"))
			title(NULL, true);
		else if (!t.s_.compare("epigraph"))
			epigraph();
		else if (!t.s_.compare("section") || !t.s_.compare("p"))
			section(-1, t.s_.c_str());
		else {
			s_->SkipElement();
		}
	}
	s_->EndElement();

    ////<image>
    //if(s_->IsNextElement("image"))
    //    image(false, Unit::IMAGE);
    ////</image>

    ////<title>
    //if(s_->IsNextElement("title"))
    //{
    //    title(NULL, true);
    //}
    ////</title>

    ////<title>
    //while(s_->IsNextElement("epigraph"))
    //    epigraph();
    ////</title>

    //do
    //{
    //    //<section>
    //    section(-1);
    //    //</section>
    //}
    //while(s_->IsNextElement("section"));

    //s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::cite()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("cite", &attrmap);
    AddId(attrmap);
    if(!notempty)
        return;

    for(LexScanner::Token t = s_->LookAhead(); t.type_ == LexScanner::START; t = s_->LookAhead())
    {
        //<p>, <subtitle>, <empty-line>, <poem>, <table>
        if(!t.s_.compare("p"))
            p();
        else if(!t.s_.compare("subtitle"))
            subtitle();
        else if(!t.s_.compare("empty-line"))
            empty_line();
        else if(!t.s_.compare("poem"))
            poem();
        else if(!t.s_.compare("table"))
            table();
        else if(!t.s_.compare("text-author"))
            break;
        else
        {
            std::ostringstream ss;
            ss << "<" << t.s_ << "> unexpected in <cite>";
            //s_->Error(ss.str());
			s_->SkipElement();
        }
        //</p>, </subtitle>, </empty-line>, </poem>, </table>
    }

    //<text-author>
    while(s_->IsNextElement("text-author"))
        text_author();
    //</text-author>

    //s_->EndElement();
	s_->SkipRestOfElementContent();
}

//-----------------------------------------------------------------------
void ConverterPass1::code(String *plainText)
{
    if(s_->BeginElement("code"))
        ParseTextAndEndElement("code", plainText);
}

//-----------------------------------------------------------------------
void ConverterPass1::coverpage()
{
    s_->BeginNotEmptyElement("coverpage");
    units_->push_back(Unit(bodyType_, Unit::COVERPAGE, 0, -1));
    do
        image(true);
    while(s_->IsNextElement("image"));
    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::description()
{
    s_->BeginNotEmptyElement("description");
    
    //<title-info>
    title_info();
    //</title-info>

    s_->SkipRestOfElementContent(); // skip rest of <description>
}

//-----------------------------------------------------------------------
void ConverterPass1::emphasis(String *plainText)
{
    if(s_->BeginElement("emphasis"))
        ParseTextAndEndElement("emphasis", plainText);
}

//-----------------------------------------------------------------------
void ConverterPass1::empty_line()
{
    if(s_->BeginElement("empty-line"))
        s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::epigraph()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("epigraph", &attrmap);
    AddId(attrmap);
    if(!notempty)
        return;

    for(LexScanner::Token t = s_->LookAhead(); t.type_ == LexScanner::START; t = s_->LookAhead())
    {
        //<p>, <poem>, <cite>, <empty-line>
        if(!t.s_.compare("p"))
            p();
        else if(!t.s_.compare("poem"))
            poem();
        else if(!t.s_.compare("cite"))
            cite();
        else if(!t.s_.compare("empty-line"))
            empty_line();
        else if(!t.s_.compare("text-author"))
            break;
        else
        {
            std::ostringstream ss;
            ss << "<" << t.s_ << "> unexpected in <epigraph>";
            //s_->Error(ss.str());
			s_->SkipElement();
        }
        //</p>, </poem>, </cite>, </empty-line>
    }

    //<text-author>
    while(s_->IsNextElement("text-author"))
        text_author();
    //</text-author>

    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::image(bool in_line, Unit::Type unitType)
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("image", in_line ? NULL : &attrmap);

    if(unitType != Unit::UNIT_NONE)
        units_->push_back(Unit(bodyType_, unitType, 0, -1));
    if(!in_line)
        AddId(attrmap);
    if(notempty)
    {
        ClrScannerDataMode clrDataMode(s_);
        s_->EndElement();
    }
}

//-----------------------------------------------------------------------
void ConverterPass1::p(String *plainText)
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("p", &attrmap);
    AddId(attrmap);
    if(notempty)
        ParseTextAndEndElement("p", plainText);
}

//-----------------------------------------------------------------------
void ConverterPass1::poem()
{
    AttrMap attrmap;
    s_->BeginNotEmptyElement("poem", &attrmap);
    AddId(attrmap);

    //<title>
    if(s_->IsNextElement("title"))
        title();
    //</title>

    if (s_->IsNextElement("subtitle")) {
        subtitle();
    }

    //<epigraph>
    while(s_->IsNextElement("epigraph"))
        epigraph();
    //</epigraph>

    //<stanza>
    do
        stanza();
    while(s_->IsNextElement("stanza"));
    //</stanza>

    //<text-author>
    while(s_->IsNextElement("text-author"))
        text_author();
    //</text-author>

    //<date>
    s_->SkipIfElement("date");
    //</date>

    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::section(int parent, const char* tag)
{
    AttrMap attrmap;
	if (tag == NULL)
		tag = "section";
	int idx = units_->size();
	if (strcmp(tag, "section") == 0) {
		bool notempty = s_->BeginElement(tag, &attrmap);

		units_->push_back(Unit(bodyType_, Unit::SECTION, sectionCnt_++, parent));
		const String *id = AddId(attrmap);
		if (!notempty)
			return;

		//<title>
		if (s_->IsNextElement("title"))
		{
			// check if it has anchor
			if ((bodyType_ == Unit::NOTES || bodyType_ == Unit::COMMENTS) && id && !id->empty())
				units_->back().noteRefId_ = *id;

			String plainText;
			title(&plainText);
			units_->back().title_ = plainText;
		}
		//</title>

		//<epigraph>
		while (s_->IsNextElement("epigraph"))
			epigraph();
		//</epigraph>

		//<image>
		if (s_->IsNextElement("image"))
			image(false);
		//</image>

		//<annotation>
		if (s_->IsNextElement("annotation"))
			annotation();
		//</annotation>
	}
	else {
		units_->push_back(Unit(bodyType_, Unit::SECTION, sectionCnt_++, parent));
	}

    //if(s_->IsNextElement("section"))
    //    do
    //    {
    //        //<section>
    //        section(idx);
    //        //</section>
    //    }
    //    while(s_->IsNextElement("section"));
    //else
        for(LexScanner::Token t = s_->LookAhead(); t.type_ == LexScanner::START; t = s_->LookAhead())
        {
            //<p>, <image>, <poem>, <subtitle>, <cite>, <empty-line>, <table>
			if (!t.s_.compare("p"))
				p();
			else if (!t.s_.compare("section"))
				section(idx);
            else if(!t.s_.compare("image"))
            {
                SwitchUnitIfSizeAbove(UNIT_SIZE1, parent);
                image(false);
            }
            else if(!t.s_.compare("poem"))
            {
                SwitchUnitIfSizeAbove(UNIT_SIZE1, parent);
                poem();
            }
			else if (!t.s_.compare("epigraph"))
			{
				SwitchUnitIfSizeAbove(UNIT_SIZE1, parent);
				epigraph();
			}
			else if(!t.s_.compare("subtitle"))
            {
                SwitchUnitIfSizeAbove(UNIT_SIZE0, parent);
                subtitle();
            }
            else if(!t.s_.compare("cite"))
            {
                SwitchUnitIfSizeAbove(UNIT_SIZE2, parent);
                cite();
            }
            else if(!t.s_.compare("empty-line"))
            {
                SwitchUnitIfSizeAbove(UNIT_SIZE2, parent);
                empty_line();
            }
            else if(!t.s_.compare("table"))
            {
                SwitchUnitIfSizeAbove(UNIT_SIZE1, parent);
                table();
            }
            else
            {
                std::ostringstream ss;
                ss << "<" << t.s_ << "> unexpected in <section>";
                // s_->Error(ss.str());
				s_->SkipElement();
            }
            //</p>, </image>, </poem>, </subtitle>, </cite>, </empty-line>, </table>

            SwitchUnitIfSizeAbove(MAX_UNIT_SIZE, parent);
        }

    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::stanza()
{
    s_->BeginNotEmptyElement("stanza");

    //<title>
    if(s_->IsNextElement("title"))
        title();
    //</title>

    //<title>
    if(s_->IsNextElement("subtitle"))
        subtitle();
    //</title>

    do
        v();
    while(s_->IsNextElement("v"));

    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::strikethrough(String *plainText)
{
    if(s_->BeginElement("strikethrough"))
        ParseTextAndEndElement("strikethrough", plainText);
}

//-----------------------------------------------------------------------
void ConverterPass1::strong(String *plainText)
{
    if(s_->BeginElement("strong"))
        ParseTextAndEndElement("strong", plainText);
}

//-----------------------------------------------------------------------
void ConverterPass1::style(String *plainText)
{
    if(s_->BeginElement("style"))
        ParseTextAndEndElement("style", plainText);
}

//-----------------------------------------------------------------------
void ConverterPass1::sub(String *plainText)
{
    if(s_->BeginElement("sub"))
        ParseTextAndEndElement("sub", plainText);
}

//-----------------------------------------------------------------------
void ConverterPass1::subtitle(String *plainText)
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("subtitle", &attrmap);
    AddId(attrmap);
    if(notempty)
        ParseTextAndEndElement("subtitle", plainText);
}

//-----------------------------------------------------------------------
void ConverterPass1::sup(String *plainText)
{
    if(s_->BeginElement("sup"))
        ParseTextAndEndElement("sup", plainText);
}

//-----------------------------------------------------------------------
void ConverterPass1::table()
{
    AttrMap attrmap;
    s_->BeginNotEmptyElement("table", &attrmap);
    AddId(attrmap);
    do
    {
        //<tr>
        tr();
        //</tr>
    }
    while(s_->IsNextElement("tr"));
    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::td()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("td", &attrmap);
    AddId(attrmap);
    if(notempty)
        ParseTextAndEndElement("td", NULL);
}

//-----------------------------------------------------------------------
void ConverterPass1::text_author(String *plainText)
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("text-author", &attrmap);
    AddId(attrmap);
    if(notempty)
        ParseTextAndEndElement("text-author", plainText);
}

//-----------------------------------------------------------------------
void ConverterPass1::th()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("th", &attrmap);
    AddId(attrmap);
    if(notempty)
        ParseTextAndEndElement("th", NULL);
}

//-----------------------------------------------------------------------
void ConverterPass1::title(String *plainText, bool startUnit)
{
    if(!s_->BeginElement("title"))
        return;

    String buf;
    if(startUnit)
    {
        units_->push_back(Unit(bodyType_, Unit::TITLE, 0, -1));
        if(!plainText)
            plainText = &buf;
    }

    SetScannerDataMode setDataMode(s_);
    LexScanner::Token t = s_->LookAhead();
    if (t.type_ != LexScanner::START) {
        if (t.type_ == LexScanner::DATA) {
            const char *pc = s_->GetToken().s_.c_str();
            while (*pc==' ' || *pc=='\t' || *pc=='\n' || *pc=='\r' || *pc=='\v' || *pc=='\f')
                pc++;
            if (*pc)
                *plainText = pc;
            ClrScannerDataMode clrDataMode(s_);
            t = s_->LookAhead();
        }
    }
    ClrScannerDataMode clrDataMode(s_);
    for (; t.type_ == LexScanner::START; t = s_->LookAhead())
    {
        if (!t.s_.compare("p"))
        {
            //<p>
            if (!plainText)
                p();
            else
            {
                String text;
                p(&text);
                *plainText = Concat(*plainText, " ", text);
            }
            //</p>
        }
        else if (!t.s_.compare("empty-line"))
        {
            //<empty-line>
            empty_line();
            if (plainText)
                *plainText += " ";
            //</empty-line>
        }
        else
        {
            std::ostringstream ss;
            ss << "<" << t.s_ << "> unexpected in <title>";
            //s_->Error(ss.str());
            s_->SkipElement();
        }
    }

    if (startUnit)
        units_->back().title_ = *plainText;

    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::title_info()
{
    s_->BeginNotEmptyElement("title-info");

	for (LexScanner::Token t = s_->LookAhead(); t.type_ == LexScanner::START; t = s_->LookAhead())
	{
		if (!t.s_.compare("genre")) {
			s_->CheckAndSkipElement("genre");
			s_->SkipAll("genre");
		}
		else if (!t.s_.compare("author")) {
			s_->CheckAndSkipElement("author");
			s_->SkipAll("author");
		}
		else if (!t.s_.compare("book-title")) {
			s_->CheckAndSkipElement("book-title");
		}
		else if (!t.s_.compare("annotation")) {
			annotation(true);
		}
		else if (!t.s_.compare("coverpage")) {
			coverpage();
		}
		else {
			s_->SkipElement();
		}
	}
	s_->SkipRestOfElementContent(); // skip rest of <title-info>


    ////<genre>
    //s_->CheckAndSkipElement("genre");
    //s_->SkipAll("genre");
    ////</genre>

    ////<author>
    //s_->CheckAndSkipElement("author");
    //s_->SkipAll("author");
    ////<author>
    //
    ////<book-title>
    //s_->CheckAndSkipElement("book-title");
    ////</book-title>

    ////<annotation>
    //if(s_->IsNextElement("annotation"))
    //    annotation(true);
    ////</annotation>

    ////<keywords>
    //s_->SkipIfElement("keywords");
    ////</keywords>

    ////<date>
    //s_->SkipIfElement("date");
    ////<date>

    ////<coverpage>
    //if(s_->IsNextElement("coverpage"))
    //    coverpage();
    ////</coverpage>

    //s_->SkipRestOfElementContent(); // skip rest of <title-info>
}

//-----------------------------------------------------------------------
void ConverterPass1::tr()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("tr", &attrmap);
    if(!notempty)
        return;

    for(;;)
    {
        //<th>, <td>
        if(s_->IsNextElement("th"))
            th();
        else if(s_->IsNextElement("td"))
            td();
        else {
            LexScanner::Token t1 = s_->GetToken();
            s_->UngetToken(t1);
            if (t1.type_ == LexScanner::START)
                s_->SkipElement();
            else
                break;
        }
        //</th>, </td>
    }

    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::v(String *plainText)
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("v", &attrmap);
    AddId(attrmap);
    if(notempty)
        ParseTextAndEndElement("v", plainText);
}


//-----------------------------------------------------------------------
void FB2TOEPUB_DECL DoConvertionPass1(LexScanner *scanner, UnitArray *units)
{
    Ptr<ConverterPass1> conv = new ConverterPass1(scanner, units);
    conv->Scan();
}

void FB2TOEPUB_DECL DoGetMetaData(const char* fnameFb2, bool wantCoverImage, Fb2MetaData& md) {
    UnitArray units;
    // create input stream - this just takes the very first file inside ZIP... If it's not .fb2, won't work.
    Ptr<InStm> pin = CreateInUnicodeStm(CreateUnpackStm(fnameFb2));
    Ptr<ConverterPass1> conv = new ConverterPass1(CreateScanner(pin), &units);
    conv->GetMetaData(md, wantCoverImage);
}

};  //namespace Fb2ToEpub
