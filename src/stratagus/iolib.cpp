//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name iolib.cpp - Compression-IO helper functions. */
//
//      (c) Copyright 2000-2020 by Andreas Arens, Lutz Sammer Jimmy Salmon,
//                                 Pali Rohár and Andrettin
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//

#include "stratagus.h"

#include "iolib.h"

#include "database/database.h"
#include "game.h"
#include "iocompat.h"
#include "map/map.h"
#include "parameters.h"
//Wyrmgus start
#include "script.h"
//Wyrmgus end
#include "util/util.h"

#ifdef USE_ZLIB
#include <zlib.h>
#endif

#ifdef USE_BZ2LIB
#include <bzlib.h>
#endif

#ifdef USE_PHYSFS
#include <physfs.h>
#endif

#ifdef __MORPHOS__
#undef tell
#endif

class CFile::PImpl
{
public:
	PImpl();
	~PImpl();

	int open(const char *name, long flags);
	int close();
	void flush();
	int read(void *buf, size_t len);
	int seek(long offset, int whence);
	long tell();
	int write(const void *buf, size_t len);

private:
	PImpl(const PImpl &rhs); // No implementation
	const PImpl &operator = (const PImpl &rhs); // No implementation

private:
	int   cl_type;   /// type of CFile
	FILE *cl_plain;  /// standard file pointer
#ifdef USE_ZLIB
	gzFile cl_gz;    /// gzip file pointer
#endif // !USE_ZLIB
#ifdef USE_BZ2LIB
	BZFILE *cl_bz;   /// bzip2 file pointer
#endif // !USE_BZ2LIB
#ifdef USE_PHYSFS
	PHYSFS_File *cl_pf;
#endif
};

CFile::CFile() : pimpl(std::make_unique<CFile::PImpl>())
{
}

CFile::~CFile()
{
}


/**
**  CLopen Library file open
**
**  @param name       File name.
**  @param openflags  Open read, or write and compression options
**
**  @return File Pointer
*/
int CFile::open(const char *name, long flags)
{
	return pimpl->open(name, flags);
}

/**
**  CLclose Library file close
*/
int CFile::close()
{
	return pimpl->close();
}

void CFile::flush()
{
	pimpl->flush();
}

/**
**  CLread Library file read
**
**  @param buf  Pointer to read the data to.
**  @param len  number of bytes to read.
*/
int CFile::read(void *buf, size_t len)
{
	return pimpl->read(buf, len);
}

/**
**  CLseek Library file seek
**
**  @param offset  Seek position
**  @param whence  How to seek
*/
int CFile::seek(long offset, int whence)
{
	return pimpl->seek(offset, whence);
}

/**
**  CLtell Library file tell
*/
long CFile::tell()
{
	return pimpl->tell();
}

/**
**  CLprintf Library file write
**
**  @param format  String Format.
**  @param ...     Parameter List.
*/
int CFile::printf(const char *format, ...)
{
	int size = 500;
	auto p = std::make_unique<char[]>(size);
	if (p == nullptr) {
		return -1;
	}
	while (1) {
		// Try to print in the allocated space.
		va_list ap;
		va_start(ap, format);
		const int n = vsnprintf(p.get(), size, format, ap);
		va_end(ap);
		// If that worked, string was processed.
		if (n > -1 && n < size) {
			break;
		}
		// Else try again with more space.
		if (n > -1) { // glibc 2.1
			size = n + 1; // precisely what is needed
		} else {    /* glibc 2.0, vc++ */
			size *= 2;  // twice the old size
		}
		p = std::make_unique<char[]>(size);
		if (p == nullptr) {
			return -1;
		}
	}
	size = strlen(p.get());
	int ret = pimpl->write(p.get(), size);
	return ret;
}

//
//  Implementation.
//

CFile::PImpl::PImpl()
{
	cl_type = CLF_TYPE_INVALID;
}

CFile::PImpl::~PImpl()
{
	if (cl_type != CLF_TYPE_INVALID) {
		DebugPrint("File wasn't closed\n");
		close();
	}
}

#ifdef USE_ZLIB

#ifndef z_off_t // { ZLIB_VERSION<="1.0.4"

/**
**  Seek on compressed input. (Newer libs support it directly)
**
**  @param file    File
**  @param offset  Seek position
**  @param whence  How to seek
*/
static int gzseek(CFile *file, unsigned offset, int whence)
{
	char buf[32];

	while (offset > sizeof(buf)) {
		gzread(file, buf, sizeof(buf));
		offset -= sizeof(buf);
	}
	return gzread(file, buf, offset);
}

#endif // } ZLIB_VERSION<="1.0.4"

#endif // USE_ZLIB

#ifdef USE_BZ2LIB

/**
**  Seek on compressed input. (I hope newer libs support it directly)
**
**  @param file    File handle
**  @param offset  Seek position
**  @param whence  How to seek
*/
static void bzseek(BZFILE *file, unsigned offset, int)
{
	char buf[32];

	while (offset > sizeof(buf)) {
		BZ2_bzread(file, buf, sizeof(buf));
		offset -= sizeof(buf);
	}
	BZ2_bzread(file, buf, offset);
}

#endif // USE_BZ2LIB

int CFile::PImpl::open(const char *name, long openflags)
{
	char buf[512];
	const char *openstring;

	if ((openflags & CL_OPEN_READ) && (openflags & CL_OPEN_WRITE)) {
		openstring = "rwb";
	} else if (openflags & CL_OPEN_READ) {
		openstring = "rb";
	} else if (openflags & CL_OPEN_WRITE) {
		openstring = "wb";
	} else {
		DebugPrint("Bad CLopen flags");
		Assert(0);
		return -1;
	}

	cl_type = CLF_TYPE_INVALID;

	if (openflags & CL_OPEN_WRITE) {
#ifdef USE_BZ2LIB
		if ((openflags & CL_WRITE_BZ2)
			&& (cl_bz = BZ2_bzopen(strcat(strcpy(buf, name), ".bz2"), openstring))) {
			cl_type = CLF_TYPE_BZIP2;
		} else
#endif
#ifdef USE_ZLIB
			if ((openflags & CL_WRITE_GZ)
				&& (cl_gz = gzopen(strcat(strcpy(buf, name), ".gz"), openstring))) {
				cl_type = CLF_TYPE_GZIP;
			} else
#endif
				if ((cl_plain = fopen(name, openstring))) {
					cl_type = CLF_TYPE_PLAIN;
				}
	} else {
#ifdef USE_PHYSFS
		if (PHYSFS_isInit()) {
			cl_pf = PHYSFS_openRead(name);
			if (cl_pf) {
				cl_type = CLF_TYPE_PHYSFS;
				return 0;
			}
		}
#endif

		if (!(cl_plain = fopen(name, openstring))) { // try plain first
#ifdef USE_ZLIB
			if ((cl_gz = gzopen(strcat(strcpy(buf, name), ".gz"), "rb"))) {
				cl_type = CLF_TYPE_GZIP;
			} else
#endif
#ifdef USE_BZ2LIB
				if ((cl_bz = BZ2_bzopen(strcat(strcpy(buf, name), ".bz2"), "rb"))) {
					cl_type = CLF_TYPE_BZIP2;
				} else
#endif
				{ }

		} else {
			cl_type = CLF_TYPE_PLAIN;
			// Hmm, plain worked, but nevertheless the file may be compressed!
			if (fread(buf, 2, 1, cl_plain) == 1) {
#ifdef USE_BZ2LIB
				if (buf[0] == 'B' && buf[1] == 'Z') {
					fclose(cl_plain);
					if ((cl_bz = BZ2_bzopen(name, "rb"))) {
						cl_type = CLF_TYPE_BZIP2;
					} else {
						if (!(cl_plain = fopen(name, "rb"))) {
							cl_type = CLF_TYPE_INVALID;
						}
					}
				}
#endif // USE_BZ2LIB
#ifdef USE_ZLIB
				if (buf[0] == 0x1f) { // don't check for buf[1] == 0x8b, so that old compress also works!
					fclose(cl_plain);
					if ((cl_gz = gzopen(name, "rb"))) {
						cl_type = CLF_TYPE_GZIP;
					} else {
						if (!(cl_plain = fopen(name, "rb"))) {
							cl_type = CLF_TYPE_INVALID;
						}
					}
				}
#endif // USE_ZLIB
			}
			if (cl_type == CLF_TYPE_PLAIN) { // ok, it is not compressed
				rewind(cl_plain);
			}
		}
	}

	if (cl_type == CLF_TYPE_INVALID) {
		//fprintf(stderr, "%s in ", buf);
		return -1;
	}
	return 0;
}

int CFile::PImpl::close()
{
	int ret = EOF;
	int tp = cl_type;

	if (tp != CLF_TYPE_INVALID) {
		if (tp == CLF_TYPE_PLAIN) {
			ret = fclose(cl_plain);
		}
#ifdef USE_ZLIB
		if (tp == CLF_TYPE_GZIP) {
			ret = gzclose(cl_gz);
		}
#endif // USE_ZLIB
#ifdef USE_BZ2LIB
		if (tp == CLF_TYPE_BZIP2) {
			BZ2_bzclose(cl_bz);
			ret = 0;
		}
#endif // USE_BZ2LIB
#ifdef USE_PHYSFS
		if (PHYSFS_isInit() && tp == CLF_TYPE_PHYSFS) {
			ret = PHYSFS_close(cl_pf);
		}
#endif // USE_PHYSFS
	} else {
		errno = EBADF;
	}
	cl_type = CLF_TYPE_INVALID;
	return ret;
}

int CFile::PImpl::read(void *buf, size_t len)
{
	int ret = 0;

	if (cl_type != CLF_TYPE_INVALID) {
		if (cl_type == CLF_TYPE_PLAIN) {
			ret = fread(buf, 1, len, cl_plain);
		}
#ifdef USE_ZLIB
		if (cl_type == CLF_TYPE_GZIP) {
			ret = gzread(cl_gz, buf, len);
		}
#endif // USE_ZLIB
#ifdef USE_BZ2LIB
		if (cl_type == CLF_TYPE_BZIP2) {
			ret = BZ2_bzread(cl_bz, buf, len);
		}
#endif // USE_BZ2LIB
#ifdef USE_PHYSFS
		if (PHYSFS_isInit() && cl_type == CLF_TYPE_PHYSFS) {
			ret = PHYSFS_read(cl_pf, buf, 1, len);
		}
#endif
	} else {
		errno = EBADF;
	}
	return ret;
}

void CFile::PImpl::flush()
{
	if (cl_type != CLF_TYPE_INVALID) {
		if (cl_type == CLF_TYPE_PLAIN) {
			fflush(cl_plain);
		}
#ifdef USE_ZLIB
		if (cl_type == CLF_TYPE_GZIP) {
			gzflush(cl_gz, Z_SYNC_FLUSH);
		}
#endif // USE_ZLIB
#ifdef USE_BZ2LIB
		if (cl_type == CLF_TYPE_BZIP2) {
			BZ2_bzflush(cl_bz);
		}
#endif // USE_BZ2LIB
	} else {
		errno = EBADF;
	}
}

int CFile::PImpl::write(const void *buf, size_t size)
{
	int tp = cl_type;
	int ret = -1;

	if (tp != CLF_TYPE_INVALID) {
		if (tp == CLF_TYPE_PLAIN) {
			ret = fwrite(buf, size, 1, cl_plain);
		}
#ifdef USE_ZLIB
		if (tp == CLF_TYPE_GZIP) {
			ret = gzwrite(cl_gz, buf, size);
		}
#endif // USE_ZLIB
#ifdef USE_BZ2LIB
		if (tp == CLF_TYPE_BZIP2) {
			ret = BZ2_bzwrite(cl_bz, const_cast<void *>(buf), size);
		}
#endif // USE_BZ2LIB
	} else {
		errno = EBADF;
	}
	return ret;
}

int CFile::PImpl::seek(long offset, int whence)
{
	int ret = -1;
	int tp = cl_type;

	if (tp != CLF_TYPE_INVALID) {
		if (tp == CLF_TYPE_PLAIN) {
			ret = fseek(cl_plain, offset, whence);
		}
#ifdef USE_ZLIB
		if (tp == CLF_TYPE_GZIP) {
			ret = gzseek(cl_gz, offset, whence);
		}
#endif // USE_ZLIB
#ifdef USE_BZ2LIB
		if (tp == CLF_TYPE_BZIP2) {
			bzseek(cl_bz, offset, whence);
			ret = 0;
		}
#endif // USE_BZ2LIB
#ifdef USE_PHYSFS
		if (PHYSFS_isInit() && tp == CLF_TYPE_PHYSFS) {
			ret = PHYSFS_seek(cl_pf, whence == SEEK_CUR ? PHYSFS_tell(cl_pf)+offset : offset);
		}
#endif
	} else {
		errno = EBADF;
	}
	return ret;
}

long CFile::PImpl::tell()
{
	int ret = -1;
	int tp = cl_type;

	if (tp != CLF_TYPE_INVALID) {
		if (tp == CLF_TYPE_PLAIN) {
			ret = ftell(cl_plain);
		}
#ifdef USE_ZLIB
		if (tp == CLF_TYPE_GZIP) {
			ret = gztell(cl_gz);
		}
#endif // USE_ZLIB
#ifdef USE_BZ2LIB
		if (tp == CLF_TYPE_BZIP2) {
			// FIXME: need to implement this
			ret = -1;
		}
#endif // USE_BZ2LIB
#ifdef USE_PHYSFS
		if (PHYSFS_isInit() && tp == CLF_TYPE_PHYSFS) {
			ret = PHYSFS_tell(cl_pf);
		}
#endif
	} else {
		errno = EBADF;
	}
	return ret;
}


/**
**  Find a file with its correct extension ("", ".gz" or ".bz2")
**
**  @param file      The string with the file path. Upon success, the string
**                   is replaced by the full filename with the correct extension.
**  @param filesize  Size of the file buffer
**
**  @return true if the file has been found.
*/
static bool FindFileWithExtension(char(&file)[PATH_MAX])
{
	const std::filesystem::path filepath = file;

	if (std::filesystem::exists(filepath)) {
		return true;
	}

#ifdef USE_ZLIB // gzip or bzip2 in global shared directory
	std::filesystem::path filepath_gz = filepath;
	filepath_gz += ".gz";

	if (std::filesystem::exists(filepath_gz)) {
		strcpy_s(file, PATH_MAX, filepath_gz.string().c_str());
		return true;
	}
#endif

#ifdef USE_BZ2LIB
	std::filesystem::path filepath_bz2 = filepath;
	filepath_bz2 += ".bz2";

	if (std::filesystem::exists(filepath_bz2)) {
		strcpy_s(file, PATH_MAX, filepath_bz2.string().c_str());
		return true;
	}
#endif

#ifdef USE_PHYSFS
	if (PHYSFS_isInit() && PHYSFS_exists(file)) {
		return true;
	}
#endif

	return false;
}

/**
**  Generate a filename into library.
**
**  Try current directory, user home directory, global directory.
**  This supports .gz, .bz2 and .zip.
**
**  @param file        Filename to open.
**  @param buffer      Allocated buffer for generated filename.
*/
static void LibraryFileName(const char *file, char(&buffer)[PATH_MAX])
{
	// Absolute path or in current directory.
	strcpy_s(buffer, PATH_MAX, file);
	if (*buffer == '/') {
		return;
	}
	if (FindFileWithExtension(buffer)) {
		return;
	}

	const std::string root_path_str = wyrmgus::database::get()->get_root_path().string();

	// Try in map directory
	if (*CurrentMapPath) {
		if (*CurrentMapPath == '.' || *CurrentMapPath == '/') {
			strcpy_s(buffer, PATH_MAX, CurrentMapPath);
			char *s = strrchr(buffer, '/');
			if (s) {
				s[1] = '\0';
			}
			strcat_s(buffer, PATH_MAX, file);
		} else {
			strcpy_s(buffer, PATH_MAX, root_path_str.c_str());
			if (*buffer) {
				strcat_s(buffer, PATH_MAX, "/");
			}
			strcat_s(buffer, PATH_MAX, CurrentMapPath);
			char *s = strrchr(buffer, '/');
			if (s) {
				s[1] = '\0';
			}
			strcat_s(buffer, PATH_MAX, file);
		}
		if (FindFileWithExtension(buffer)) {
			return;
		}
	}

	// In user home directory
	if (!GameName.empty()) {
		snprintf(buffer, PATH_MAX, "%s/%s/%s", Parameters::Instance.GetUserDirectory().c_str(), GameName.c_str(), file);
		if (FindFileWithExtension(buffer)) {
			return;
		}
	}

	snprintf(buffer, PATH_MAX, "%s/%s", Parameters::Instance.GetUserDirectory().c_str(), file);
	if (FindFileWithExtension(buffer)) {
		return;
	}
		
	// In global shared directory
	#ifndef __MORPHOS__
	snprintf(buffer, PATH_MAX, "%s/%s", root_path_str.c_str(), file);
	if (FindFileWithExtension(buffer)) {
		return;
	}
	#endif
	// Support for graphics in default graphics dir.
	// They could be anywhere now, but check if they haven't
	// got full paths.
	snprintf(buffer, PATH_MAX, "graphics/%s", file);
	if (FindFileWithExtension(buffer)) {
		return;
	}
	#ifndef __MORPHOS__	
	snprintf(buffer, PATH_MAX, "%s/graphics/%s", root_path_str.c_str(), file);
	if (FindFileWithExtension(buffer)) {
		return;
	}
	#endif

	// Support for sounds in default sounds dir.
	// They could be anywhere now, but check if they haven't
	// got full paths.
	snprintf(buffer, PATH_MAX, "sounds/%s", file);
	if (FindFileWithExtension(buffer)) {
		return;
	}
	#ifndef __MORPHOS__	
	snprintf(buffer, PATH_MAX, "%s/sounds/%s", root_path_str.c_str(), file);
	if (FindFileWithExtension(buffer)) {
		return;
	}
	#endif

	// Support for music in the default music dir.
	snprintf(buffer, PATH_MAX, "music/%s", file);
	if (FindFileWithExtension(buffer)) {
		return;
	}
	#ifndef __MORPHOS__	
	snprintf(buffer, PATH_MAX, "%s/music/%s", root_path_str.c_str(), file);
	if (FindFileWithExtension(buffer)) {
		return;
	}
	#endif

	// Support for scripts in default scripts dir.
	sprintf(buffer, "scripts/%s", file);
	if (FindFileWithExtension(buffer)) {
		return;
	}
	#ifndef __MORPHOS__	
	sprintf(buffer, "%s/scripts/%s", root_path_str.c_str(), file);
	if (FindFileWithExtension(buffer)) {
		return;
	}
	#endif
	//Wyrmgus start
	if (DLCFileEquivalency.find(std::string(file)) != DLCFileEquivalency.end()) { //if the file hasn't been found and it has an equivalent file, try to get that instead
		LibraryFileName(DLCFileEquivalency[std::string(file)].c_str(), buffer);
		return;
	}
	//Wyrmgus end

	DebugPrint("File '%s' not found\n" _C_ file);
	strcpy_s(buffer, PATH_MAX, file);
}

extern std::string LibraryFileName(const char *file)
{
	char buffer[PATH_MAX];
	LibraryFileName(file, buffer);
	return buffer;
}

bool CanAccessFile(const char *filename)
{
	if (filename && filename[0] != '\0') {
		char name[PATH_MAX];
		name[0] = '\0';
		LibraryFileName(filename, name);
		if (name[0] == '\0') {
			return false;
		}

		const std::filesystem::path filepath = name;
		if (std::filesystem::exists(filepath)) {
			return true;
		}

#ifdef USE_PHYSFS
		if (PHYSFS_isInit() && PHYSFS_exists(name)) {
			return true;
		}
#endif
	}

	return false;
}

/**
**  Generate a list of files within a specified directory
**
**  @param dirname  Directory to read.
**  @param fl       Filelist pointer.
**
**  @return the number of entries added to FileList.
*/
std::vector<FileList> ReadDataDirectory(const std::filesystem::path &dir_path, const int sortmode)
{
	std::vector<FileList> file_list;

	if (!std::filesystem::exists(dir_path)) {
		return file_list;
	}

	std::filesystem::directory_iterator dir_iterator(dir_path);

	for (const std::filesystem::directory_entry &dir_entry : dir_iterator) {
		if (!dir_entry.is_directory() && !dir_entry.is_regular_file()) {
			continue;
		}

		FileList nfl;

		nfl.name = dir_entry.path().filename().string();
		if (!dir_entry.is_directory()) {
			nfl.type = 1;
		}
		nfl.mtime = std::filesystem::last_write_time(dir_entry.path());
		nfl.sortmode = sortmode;
		// sorted insertion
		file_list.insert(std::lower_bound(file_list.begin(), file_list.end(), nfl), nfl);
	}

	return file_list;
}

void FileWriter::printf(const char *format, ...)
{
	// FIXME: hardcoded size
	char buf[1024];

	va_list ap;
	va_start(ap, format);
	buf[sizeof(buf) - 1] = '\0';
	vsnprintf(buf, sizeof(buf) - 1, format, ap);
	va_end(ap);
	write(buf, strlen(buf));
}


class RawFileWriter : public FileWriter
{
	FILE *file;

public:
	RawFileWriter(const std::string &filename)
	{
		file = fopen(filename.c_str(), "wb");
		if (!file) {
			fprintf(stderr, "Can't open file '%s' for writing\n", filename.c_str());
			throw FileException();
		}
	}

	virtual ~RawFileWriter()
	{
		if (file) { fclose(file); }
	}

	virtual int write(const char *data, unsigned int size)
	{
		return fwrite(data, size, 1, file);
	}
};

class GzFileWriter : public FileWriter
{
	gzFile file;

public:
	GzFileWriter(const std::string &filename)
	{
		file = gzopen(filename.c_str(), "wb9");
		if (!file) {
			fprintf(stderr, "Can't open file '%s' for writing\n", filename.c_str());
			throw FileException();
		}
	}

	virtual ~GzFileWriter()
	{
		if (file) { gzclose(file); }
	}

	virtual int write(const char *data, unsigned int size)
	{
		return gzwrite(file, data, size);
	}
};

/**
**  Create FileWriter
*/
std::unique_ptr<FileWriter> CreateFileWriter(const std::string &filename)
{
	if (strcasestr(filename.c_str(), ".gz")) {
		return std::make_unique<GzFileWriter>(filename);
	} else {
		return std::make_unique<RawFileWriter>(filename);
	}
}
