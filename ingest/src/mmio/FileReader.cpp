#include "ingest/mmio.hpp"

extern "C" {
#include "mmio.h"
}

#include <fcntl.h>
#include <unistd.h>

auto
ingest::mmio::Reader::_open_mmio_file(std::string_view path)
	-> FILE*
{
	int const fd = open(path.data(), O_RDONLY);
	if (fd < 0) {
		throw _error("open failed on {}, {}: {}\n", path, errno, strerror(errno));
	}

	FILE* f = fdopen(fd, "r");
	if (f == nullptr) {
		close(fd);
		throw _error("fdopen failed on {}, {}: {}\n", path, errno, strerror(errno));
	}

	return f;
}

auto
ingest::mmio::Reader::_process_mmio_header(FILE* f, std::string_view path)
	-> _mmio_header_data
{
	_mmio_header_data out{};
	
	MM_typecode type;
	switch (mm_read_banner(f, &type)) {
	  case MM_PREMATURE_EOF:    // if all items are not present on first line of file.
	  case MM_NO_HEADER:        // if the file does not begin with "%%MatrixMarket".
	  case MM_UNSUPPORTED_TYPE: // if not recongizable description.
		fclose(f);
		throw _error("could not parse {} as an mmio file", path);
	}

	if (!mm_is_coordinate(type)) {
		fclose(f);
		throw _error("mmio file reader only supports coordinate format");
	}
	
	switch (mm_read_mtx_crd_size(f, &out.n, &out.m, &out.nnz)) {
	  case MM_PREMATURE_EOF:    // if an end-of-file is encountered before processing these three values.
		fclose(f);
		throw _error("mmio file {} missing data", path);
	}

	auto const i = ftell(f);
	fseek(f, 0L, SEEK_END);
	auto const e = ftell(f);
	fseek(f, i, SEEK_SET);
	out.bytes = e - i;

	return out;
}
