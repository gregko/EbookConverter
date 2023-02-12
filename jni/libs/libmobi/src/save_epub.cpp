/*
* Added by Grzegorz Kochaniak, greg@hyperionics.com, in Feb. 2016 -
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
/* include libmobi header */
#include "save_epub.h"
// unzip101e headers
#include <zip.h>

#include <regex>
#include <string>

// #define WANT_TIDY_CLEANUP // Nov. 18, 2016 - disconnecting TidyLib
#ifdef WANT_TIDY_CLEANUP
	typedef unsigned long ulong;
#   include "tidy.h"
#   include "tidybuffio.h"

	static void TIDY_CALL emptyPutByteFunc(void* sinkData, byte bt)
	{
		// printf("In emptyPutByteFunc()\n");
	}
#endif

/* return codes */
#define ERROR 1
#define ENCRYPTED 10
#define SUCCESS 0


static bool startFileInZip(zipFile zf, const char *name, bool compress)
{
	zip_fileinfo zi;
	time_t ltime;
	time(&ltime);
	struct tm *filedate = localtime(&ltime);

	zi.tmz_date.tm_sec = filedate->tm_sec;
	zi.tmz_date.tm_min = filedate->tm_min;
	zi.tmz_date.tm_hour = filedate->tm_hour;
	zi.tmz_date.tm_mday = filedate->tm_mday;
	zi.tmz_date.tm_mon = filedate->tm_mon;
	zi.tmz_date.tm_year = filedate->tm_year;
	zi.dosDate = 0;
	zi.internal_fa = 0;
	zi.external_fa = 0;

	if (ZIP_OK != zipOpenNewFileInZip(zf, name, &zi, NULL, 0, NULL, 0, NULL,
		compress ? Z_DEFLATED : 0,
		compress ? Z_BEST_COMPRESSION : Z_NO_COMPRESSION))
	{
		printf("zipOpenNewFileInZip error");
		return false;
	}
	return true;
}

// like strstr() if block is not 0-terminated
static char* search_block(const char* block, size_t block_len, const char *target) {
	size_t len = strlen(target);
	if (!len || block_len < len)
		return NULL; // for 0-length target strstr returns block pointer, I don't care.
	char *pc;
	block_len -= len - 1; // no point searching for the first character near end of block
	while (pc = (char*)memchr(block, *target, block_len)) {
		if (memcmp(pc, target, len) == 0)
			return pc;
		pc++;
		block_len -= (pc - block);
		block = pc;
	}
	return NULL;
}

/**
@brief Dump parsed markup files and resources into created folder
@param[in] rawml MOBIRawml structure holding parsed records
@param[in] epub_fn File to the epub file to be created.
E
xample structure:
--ZIP Container--
mimetype
META-INF/
  container.xml
OEBPS/
  content.opf
  chapter1.xhtml
  ch1-pic.png
  css/
    style.css
    myfont.otf
  toc.ncx
*/
int epub_rawml_parts(const MOBIRawml *rawml, const char *epub_fn) {
	if (rawml == NULL) {
		printf("Rawml structure not initialized\n");
		return ERROR;
	}
	printf("Saving EPUB %s\n", epub_fn);

	zipFile zf = zipOpen(epub_fn, APPEND_STATUS_CREATE);

	if (zf == NULL) {
		printf("Creating EPUB/zip file failed, file name: %s\n", epub_fn);
		return ERROR;
	}
	// Create regular EPUB structure in zf here...
	bool noError;

	// mimetype :
	static const char contents[] = "application/epub+zip";
	noError = startFileInZip(zf, "mimetype", false);
	noError &= zipWriteInFileInZip(zf, contents, sizeof(contents)-1) == ZIP_OK; // Must strip ending 0 byte, hence -1
	noError &= zipCloseFileInZip(zf) == ZIP_OK;
	if (!noError)
	{
		printf("Could not open file inside EPUB for writing: mimetype\n");
		zipClose(zf, NULL);
		return ERROR;
	}

	// META-INF/container.xml :
	static const char cont_xml[] =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<container version=\"1.0\" xmlns=\"urn:oasis:names:tc:opendocument:xmlns:container\">\n"
		"  <rootfiles>\n"
		"    <rootfile full-path=\"OEBPS/content.opf\" media-type=\"application/oebps-package+xml\"/>\n"
		"  </rootfiles>\n"
		"</container>";
	noError = startFileInZip(zf, "META-INF/container.xml", true);
	noError &= zipWriteInFileInZip(zf, cont_xml, sizeof(cont_xml)-1) == ZIP_OK; // again -1 to strip ending 0
	noError &= zipCloseFileInZip(zf) == ZIP_OK;
	if (!noError)
	{
		printf("Could not open file inside EPUB for writing: META-INF/container.xml\n");
		zipClose(zf, NULL);
		return ERROR;
	}


    // Now save all the ebook parts to zf...
    char partname[FILENAME_MAX];
    if (rawml->markup != NULL) {
        /* Linked list of MOBIPart structures in rawml->markup holds main text files */
        MOBIPart *curr = rawml->markup;
        while (curr != NULL) {
            MOBIFileMeta file_meta = mobi_get_filemeta_by_type(curr->type);
            sprintf(partname, "OEBPS/part%05zu.%s", curr->uid, file_meta.extension);
            noError = startFileInZip(zf, partname, true);

            // GKochaniak, added - removes erroneous HTML <...<...>...> inside angled brackets
            size_t size = curr->size;
            char* pData = (char*)curr->data;

			if (curr->type == T_HTML) {
				std::regex pattern("(<.[^>]*?)(<.[^<]*?>\\s*</.*?>)(.*?>)");
				char lastChar = pData[size - 1];
				pData[size - 1] = '\0';
				std::match_results<const char*> m;
				const char* data = (const char*)curr->data;
				while (std::regex_search(data, m, pattern) && m.size() == 4) {
					std::string ss[4];
					for (int i = 0; i < m.size(); i++) {
						ss[i] = std::string(m[i].first, m[i].second - m[i].first);
					}
					std::string fixed = ss[1] + ss[3] + ss[2];
					if (fixed.length() == m[0].second - m[0].first) // double check for safety
						memcpy((void*)m[0].first, (void*)fixed.c_str(), fixed.length());
					data = m[0].second;
				}
				pData[size - 1] = lastChar;
			}

			// Alternative:
			//if (curr->type == T_HTML) {
   //             std::regex pattern("(<.[^>]*?)(<.[^<]*?>\\s*</.*?>)(.*?>)");
   //             char lastChar = pData[size - 1];
   //             pData[size - 1] = '\0';
   //             std::match_results<const char*> m;
   //             const char* data = (const char*)curr->data;
   //             while (std::regex_search(data, m, pattern) && m.size() == 4) {
   //                 memmove((void*) m[2].first, (void*)m[2].second, strlen(m[2].second) + 1);
   //                 data = m[2].first;
   //             }
   //             size = strlen(pData);
   //             pData[size] = lastChar;
   //         }

			noError &= zipWriteInFileInZip(zf, pData, size) == ZIP_OK;
            // GKochaniak, end change

            noError &= zipCloseFileInZip(zf) == ZIP_OK;
            if (!noError)
            {
                printf("Could not open file inside EPUB for writing: %s\n", partname);
                zipClose(zf, NULL);
                return ERROR;
            }
            curr = curr->next;
        }
    }
    if (rawml->flow != NULL) {
		/* Linked list of MOBIPart structures in rawml->flow holds supplementary text files, 
		   e.g. .css, .svg
		*/
		MOBIPart *curr = rawml->flow;
		/* skip raw html file */
		curr = curr->next;
		while (curr != NULL) {
			MOBIFileMeta file_meta = mobi_get_filemeta_by_type(curr->type);
			sprintf(partname, "OEBPS/flow%05zu.%s", curr->uid, file_meta.extension);
			// optional, get rid of negative text-indent
			if (file_meta.type == T_CSS) {
				char *pc = (char*)curr->data;
				int data_len = curr->size;
				// Previously was using here strstr(), but curr->data is not 0-terminated
				while (pc = search_block(pc, data_len, "text-indent:")) {
					pc += 12; // strlen("text-indent:");
					while (isspace(*pc) && (pc - (char*)curr->data) < curr->size)
						pc++;
					if (*pc == '-') {
						*pc++ = ' ';
						while (isdigit(*pc)) {
							*pc++ = ' ';
						}
						// now *pc is maybe %, p for px etc.
						*(--pc) = '0';
					}
					data_len = curr->size - (pc - (char*)curr->data);
				}
			}
			noError = startFileInZip(zf, partname, true);
			noError &= zipWriteInFileInZip(zf, curr->data, curr->size) == ZIP_OK;
			noError &= zipCloseFileInZip(zf) == ZIP_OK;
			if (!noError)
			{
				printf("Could not open file inside EPUB for writing: %s\n", partname);
				zipClose(zf, NULL);
				return ERROR;
			}
			curr = curr->next;
		}
	}
	if (rawml->resources != NULL) {
		/* Linked list of MOBIPart structures in rawml->resources holds binary files */
		MOBIPart *curr = rawml->resources;
		/* jpg, gif, png, bmp, font, audio, video */
		while (curr != NULL) {
			MOBIFileMeta file_meta = mobi_get_filemeta_by_type(curr->type);
			if (curr->size > 0) {
				MOBIFiletype typ = file_meta.type;
				if (typ == T_NCX)
					sprintf(partname, "OEBPS/toc.%s", file_meta.extension);
				else if (typ == T_OPF)
					sprintf(partname, "OEBPS/content.%s", file_meta.extension);
				else
					sprintf(partname, "OEBPS/resource%05zu.%s", curr->uid, file_meta.extension);
				bool compress = !(typ==T_JPG || typ==T_GIF || typ==T_PNG || typ==T_MP3 || typ==T_MPG);
				noError = startFileInZip(zf, partname, compress);
				noError &= zipWriteInFileInZip(zf, curr->data, curr->size) == ZIP_OK;
				noError &= zipCloseFileInZip(zf) == ZIP_OK;
				if (!noError)
				{
					printf("Could not open file inside EPUB for writing: %s\n", partname);
					zipClose(zf, NULL);
					return ERROR;
				}
			}
			curr = curr->next;
		}
	}
	zipClose(zf, NULL);
	return SUCCESS;
}

/**
@brief Loads Rawml data of Mobi
@param[in] mobiFn Mobi file name
@param[in] m MOBIData initialized with *m = mobi_init();
MOBIData* m = mobi_init();
if (m == NULL) {
printf("Memory allocation failed\n");
return NULL;
}
...
mobi_free(m);
mobi_free_rawml(rawml);
@param[in] pid Device ID for decription, default NULL
@param[in] parse_kf7_opt - true if KF7 part of hybrid KF7/KF8 file should be parsed, default false
*/
// TODO: Compare with Bartek's latest souces... Error codes?
MOBIRawml* loadMobiRawml(MOBIData *m, const char *mobiFn, const char* pid, bool parse_kf7_opt) {
	MOBI_RET mobi_ret;
	/* Initialize main MOBIData structure */

	/* By default loader will parse KF8 part of hybrid KF7/KF8 file */
	if (parse_kf7_opt) {
		/* Force it to parse KF7 part */
		mobi_parse_kf7(m);
	}
	errno = 0;
	FILE *file = fopen(mobiFn, "rb");
	if (file == NULL) {
		int errsv = errno;
		printf("Error opening file: %s (%s)\n", mobiFn, strerror(errsv));
		return NULL;
	}
	/* MOBIData structure will be filled with loaded document data and metadata */
	mobi_ret = mobi_load_file(m, file);
	fclose(file); file = NULL;
	/* Try to print basic metadata, even if further loading failed */
	/* In case of some unsupported formats it may still print some useful info */
	// print_meta(m);
	if (mobi_ret != MOBI_SUCCESS) {
		printf("Error while loading document (%i)\n", mobi_ret);
		return NULL;
	}
	/* Try to print EXTH metadata */
	// print_exth(m);
#ifdef USE_ENCRYPTION
	if (pid != NULL) {
		/* Try to set key for decompression */
		if (m->rh && m->rh->encryption_type == 0) {
			printf("\nDocument is not encrypted, ignoring PID\n");
		}
		else if (m->rh && m->rh->encryption_type == 1) {
			printf("\nEncryption type 1, ignoring PID\n");
		}
		else {
			printf("\nVerifying PID... ");
			mobi_ret = mobi_drm_setkey(m, pid);
			if (mobi_ret != MOBI_SUCCESS) {
				printf("failed (%i)\n", mobi_ret);
				return NULL;
			}
			//printf("ok\n");
		}
	}
#endif

	// printf("\nReconstructing source resources...\n");
	/* Initialize MOBIRawml structure */
	/* This structure will be filled with parsed records data */
	MOBIRawml *rawml = mobi_init_rawml(m);
	if (rawml == NULL) {
		printf("Memory allocation failed\n");
		return NULL;
	}

	/* Parse rawml text and other data held in MOBIData structure into MOBIRawml structure */
	mobi_ret = mobi_parse_rawml(rawml, m);
	if (mobi_ret != MOBI_SUCCESS) {
		printf("Parsing rawml failed (%i)\n", mobi_ret);
		mobi_free_rawml(rawml);
		return NULL;
	}

	return rawml;
}


// TODO: Compare with Bartek's latest souces... Return better error codes.
extern "C"
int convertMobiToEpub(const char* mobiFn, const char* epubFn, const char* pid, bool parse_kf7_opt)
{
	MOBIData* m = mobi_init();
	if (m == NULL) {
		printf("Memory allocation failed\n");
		return false;
	}
	MOBIRawml *rawml = loadMobiRawml(m, mobiFn, pid, parse_kf7_opt);
	if (m->rh != NULL && m->rh->encryption_type != 0) {
		mobi_free(m);
		mobi_free_rawml(rawml);
		return ENCRYPTED;
	}
	if (rawml == NULL) {
		mobi_free(m);
		return ERROR;
	}
	int ret = SUCCESS;
	if (rawml->flow->type == T_PDF) {
		printf("This is Replica Print ebook (azw4), got PDF resource.\n");
#if defined(_DEBUG) && defined(WIN32)
		int len = strlen(epubFn) + 16;
		char *pdfFn = (char*) malloc(len);
		strncpy(pdfFn, epubFn, len);
		for (char *pc = pdfFn + strlen(pdfFn); pc > pdfFn; pc--) {
			if (*pc == '.') {
				*pc = 0;
				break;
			}
		}
		strncat(pdfFn, ".pdf", 5);
		FILE* fp = fopen(pdfFn, "wb");
		free(pdfFn);
		if (fp != NULL) {
			char *pc = (char*)rawml->flow->data;
			fwrite(rawml->flow->data, 1, rawml->flow->size, fp);
			char *s = "\n%HyperionicsAvarOrg: c:/tmp/TestFileName.pdf\n";
			fwrite(s, 1, strlen(s), fp);
			fclose(fp);
			ret = 101;
		}
#endif
		if (ret != 101)
			ret = ERROR;
	}
	else {
		/* Save parts to files */
		ret = epub_rawml_parts(rawml, epubFn);
		if (ret != SUCCESS) {
			printf("Dumping parts to EPUB failed\n");
		}
	}
	mobi_free(m);
	mobi_free_rawml(rawml);
	return ret;
}
