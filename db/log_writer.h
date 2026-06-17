#include <cstdint>
#include "status.h"
#include "slice.h"
#include <string>
#include "../util/coding.h"
#include "../include/env.h"
#include "../util/crc32c.h"
#include "../db/logformat.h"

#ifndef LEVELDB_CLONE_DB_LOGWRITER_H
#define LEVELDB_CLONE_DB_LOGWRITER_H

namespace leveldb_clone {

constexpr size_t kMaxBufferSize = 32768;


class LogWriter {
public:
	const std::string MakeHeader(log::RecordType t, const char* payload, size_t psize) {
		uint8_t type = static_cast<uint8_t>(t);
		uint32_t crc32 = crc32c::Crc32(type, payload, psize);
		char header[7];
		header[0] = static_cast<char>(crc32);
		header[1] = static_cast<char>(crc32 >> 8);
		header[2] = static_cast<char>(crc32 >> 16);
		header[3] = static_cast<char>(crc32 >> 24);
		header[4] = static_cast<char>(psize);
		header[5] = static_cast<char>(psize >> 8);
		header[6] = static_cast<char>(type);
		return std::string{header, 7 };
	};

	// fd, filename is used for constructing a writablefile 
	// LogWriter only needs writablefile for its API
	LogWriter(WritableFile* wfile) : wfile_(wfile), block_offset_(0) {}

	Status Append(const Slice& s) {
		// Header: [crc:4bytes(uint32_t)| len:2btyes(uint16_t) | type: 1bytes | payload]
		// three kinds of situations:
		// 1. header + payload fits: Append(header) + Append(payload)
		// 2. hedaer fits, payload does not: Append(header) + clear block + Append(payload)
		// 3. even header cannot fit: clear block + Append(header) + Appoend(payload)
		// Note: payload may seperated in several blcoks
		size_t space = kMaxBufferSize - block_offset_; // space left for header + payload
		const char* payload = s.data();
		size_t psize = s.size();
		if (space >= psize + 7) {
			std::string s = MakeHeader(log::kFullType, payload, psize);
			Slice header{s};
			block_offset_ += psize + 7;

			// cannot pass in only char*, since Slice will use strlen() to get len
			// which could get the unexpected result
			wfile_->Append(header);
			wfile_->Append(Slice(payload, psize));
			return Status::Ok();
			// TODO: better version of buliding header
		}
	
		// put header first
		// data append in chunck
		uint16_t chunk_size = psize;
		uint16_t header_size = 7;
		while (chunk_size > 0) {
			// No space for header
			if (kMaxBufferSize < block_offset_ + header_size) {
				size_t leftover = kMaxBufferSize - block_offset_;
				wfile_->Append(Slice("\x00\x00\x00\x00\x00\x00", leftover));
				block_offset_ = 0;
			}

			// Determine log type
			log::RecordType type = log::kMiddleType;
			// Always consider the size of header
			uint16_t left = kMaxBufferSize - block_offset_ - header_size;
			if (chunk_size == psize)
				type = log::RecordType::kFirstType;
			// this is the last chunk
			else if (chunk_size <= left)
				type = log::RecordType::kLastType;
			
			// calculate how many bytes can be sent
			uint16_t bytes = std::min(left, chunk_size);
			chunk_size -= bytes;

			std::string s = MakeHeader(type, payload, bytes);
			const Slice header{s};
			wfile_->Append(header);

			const Slice chunk{payload, bytes};
			wfile_->Append(chunk);

			payload += bytes;
			block_offset_ += bytes + 7;
		}
		return Status::Ok();
	};
	
private:
	WritableFile* wfile_;
	size_t block_offset_;
};
} // namesapce leveldb_clone

#endif
