#include "Gfx/RHI/Png.hpp"

#include "Platform/Log.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace cinder::Gfx::RHI {
namespace {

std::array<std::uint32_t, 256> makeCrcTable() {
    std::array<std::uint32_t, 256> table{};
    for (std::uint32_t n = 0; n < 256; ++n) {
        std::uint32_t c = n;
        for (int k = 0; k < 8; ++k) c = (c & 1u) != 0 ? (0xedb88320u ^ (c >> 1)) : (c >> 1);
        table[n] = c;
    }
    return table;
}

std::uint32_t crc32(const unsigned char* data, std::size_t len) {
    static const auto table = makeCrcTable();
    std::uint32_t c = 0xffffffffu;
    for (std::size_t i = 0; i < len; ++i) c = table[(c ^ data[i]) & 0xffu] ^ (c >> 8);
    return c ^ 0xffffffffu;
}

std::uint32_t adler32(const unsigned char* data, std::size_t len) {
    std::uint32_t a = 1;
    std::uint32_t b = 0;
    for (std::size_t i = 0; i < len; ++i) {
        a = (a + data[i]) % 65521u;
        b = (b + a) % 65521u;
    }
    return (b << 16) | a;
}

void put32be(std::vector<unsigned char>& out, std::uint32_t v) {
    out.push_back(static_cast<unsigned char>((v >> 24) & 0xff));
    out.push_back(static_cast<unsigned char>((v >> 16) & 0xff));
    out.push_back(static_cast<unsigned char>((v >> 8) & 0xff));
    out.push_back(static_cast<unsigned char>(v & 0xff));
}

void putChunk(std::vector<unsigned char>& out, const char type[4], const unsigned char* data, std::size_t len) {
    put32be(out, static_cast<std::uint32_t>(len));
    const std::size_t typePos = out.size();
    out.insert(out.end(), reinterpret_cast<const unsigned char*>(type),
               reinterpret_cast<const unsigned char*>(type) + 4);
    out.insert(out.end(), data, data + len);
    put32be(out, crc32(out.data() + typePos, 4 + len));
}

void deflateStored(const unsigned char* data, std::size_t len, std::vector<unsigned char>& out) {
    std::size_t pos = 0;
    while (true) {
        const std::size_t remaining = len - pos;
        const std::size_t block = std::min<std::size_t>(remaining, 65535);
        const bool last = pos + block == len;
        out.push_back(static_cast<unsigned char>(last ? 0x01 : 0x00));
        out.push_back(static_cast<unsigned char>(block & 0xff));
        out.push_back(static_cast<unsigned char>((block >> 8) & 0xff));
        const std::uint16_t nlen = static_cast<std::uint16_t>(~static_cast<std::uint16_t>(block));
        out.push_back(static_cast<unsigned char>(nlen & 0xff));
        out.push_back(static_cast<unsigned char>((nlen >> 8) & 0xff));
        out.insert(out.end(), data + pos, data + pos + block);
        pos += block;
        if (last) break;
    }
}

struct BitReader {
    const unsigned char* data;
    std::size_t size;
    std::size_t bytePos = 0;
    std::uint32_t buf = 0;
    int bits = 0;

    std::uint32_t readBits(int n) {
        while (bits < n) {
            if (bytePos >= size) throw std::runtime_error("png: unexpected end of deflate stream");
            buf |= static_cast<std::uint32_t>(data[bytePos++]) << bits;
            bits += 8;
        }
        const std::uint32_t v = buf & ((1u << n) - 1u);
        buf >>= n;
        bits -= n;
        return v;
    }

    void alignToByte() {
        buf = 0;
        bits = 0;
    }

    const unsigned char* consumeBytes(std::size_t n) {
        if (bytePos + n > size) throw std::runtime_error("png: unexpected end of deflate stream");
        const unsigned char* p = data + bytePos;
        bytePos += n;
        return p;
    }
};

struct HuffmanTable {
    std::vector<int> counts;
    std::vector<int> symbols;
    int maxLen = 0;

    void build(const int* codeLengths, int n) {
        int ml = 0;
        for (int i = 0; i < n; ++i) if (codeLengths[i] > ml) ml = codeLengths[i];
        maxLen = ml;
        counts.assign(ml + 1, 0);
        for (int i = 0; i < n; ++i) counts[codeLengths[i]]++;
        counts[0] = 0;
        std::vector<int> offsets(ml + 2, 0);
        for (int l = 1; l <= ml; ++l) offsets[l + 1] = offsets[l] + counts[l];
        symbols.assign(offsets[ml + 1], 0);
        std::vector<int> cursor = offsets;
        for (int i = 0; i < n; ++i) {
            const int len = codeLengths[i];
            if (len == 0) continue;
            symbols[cursor[len]++] = i;
        }
    }

    int decode(BitReader& br) const {
        int code = 0;
        int first = 0;
        int index = 0;
        for (int len = 1; len <= maxLen; ++len) {
            code |= static_cast<int>(br.readBits(1));
            const int count = counts[len];
            if (code - count < first) {
                return symbols[index + (code - first)];
            }
            index += count;
            first += count;
            first <<= 1;
            code <<= 1;
        }
        throw std::runtime_error("png: invalid Huffman code");
    }
};

void doInflate(const unsigned char* src, std::size_t srcLen, std::vector<unsigned char>& out) {
    static const int lengthBase[29] = {
        3, 4, 5, 6, 7, 8, 9, 10,
        11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115,
        131, 163, 195, 227, 258};
    static const int lengthExtra[29] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4,
        5, 5, 5, 5, 0};
    static const int distBase[30] = {
        1, 2, 3, 4, 5, 7, 9, 13,
        17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073,
        4097, 6145, 8193, 12289, 16385, 24577};
    static const int distExtra[30] = {
        0, 0, 0, 0, 1, 1, 2, 2,
        3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10,
        11, 11, 12, 12, 13, 13};
    static const int codeOrder[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

    BitReader br{src, srcLen};
    bool last = false;
    while (!last) {
        last = br.readBits(1) != 0;
        const int type = static_cast<int>(br.readBits(2));
        if (type == 0) {
            br.alignToByte();
            const unsigned char* hdr = br.consumeBytes(4);
            const std::uint16_t len = static_cast<std::uint16_t>(hdr[0] | (hdr[1] << 8));
            const std::uint16_t nlen = static_cast<std::uint16_t>(hdr[2] | (hdr[3] << 8));
            if (static_cast<std::uint16_t>(~len) != nlen) throw std::runtime_error("png: stored block length mismatch");
            const unsigned char* bytes = br.consumeBytes(len);
            out.insert(out.end(), bytes, bytes + len);
        } else if (type == 1 || type == 2) {
            HuffmanTable lit;
            HuffmanTable dist;
            if (type == 1) {
                int lengths[288];
                for (int i = 0; i < 144; ++i) lengths[i] = 8;
                for (int i = 144; i < 256; ++i) lengths[i] = 9;
                for (int i = 256; i < 280; ++i) lengths[i] = 7;
                for (int i = 280; i < 288; ++i) lengths[i] = 8;
                lit.build(lengths, 288);
                int dlens[30];
                for (int i = 0; i < 30; ++i) dlens[i] = 5;
                dist.build(dlens, 30);
            } else {
                const int hlit = static_cast<int>(br.readBits(5)) + 257;
                const int hdist = static_cast<int>(br.readBits(5)) + 1;
                const int hclen = static_cast<int>(br.readBits(4)) + 4;
                int clLens[19] = {0};
                for (int i = 0; i < hclen; ++i) clLens[codeOrder[i]] = static_cast<int>(br.readBits(3));
                HuffmanTable cl;
                cl.build(clLens, 19);

                std::vector<int> lens(static_cast<std::size_t>(hlit + hdist), 0);
                int idx = 0;
                while (idx < hlit + hdist) {
                    const int sym = cl.decode(br);
                    if (sym < 0 || sym > 18) throw std::runtime_error("png: invalid code-length symbol");
                    if (sym < 16) {
                        lens[idx++] = sym;
                    } else if (sym == 16) {
                        if (idx == 0) throw std::runtime_error("png: code-length repeat with no previous");
                        const int prev = lens[idx - 1];
                        int count = static_cast<int>(br.readBits(2)) + 3;
                        while (count-- > 0) {
                            if (idx >= hlit + hdist) throw std::runtime_error("png: code-length overflow");
                            lens[idx++] = prev;
                        }
                    } else if (sym == 17) {
                        int count = static_cast<int>(br.readBits(3)) + 3;
                        while (count-- > 0) {
                            if (idx >= hlit + hdist) throw std::runtime_error("png: code-length overflow");
                            lens[idx++] = 0;
                        }
                    } else {
                        int count = static_cast<int>(br.readBits(7)) + 11;
                        while (count-- > 0) {
                            if (idx >= hlit + hdist) throw std::runtime_error("png: code-length overflow");
                            lens[idx++] = 0;
                        }
                    }
                }

                lit.build(lens.data(), hlit);
                dist.build(lens.data() + hlit, hdist);
            }

            while (true) {
                const int sym = lit.decode(br);
                if (sym < 256) {
                    out.push_back(static_cast<unsigned char>(sym));
                } else if (sym == 256) {
                    break;
                } else {
                    const int lenCode = sym - 257;
                    if (lenCode < 0 || lenCode >= 29) throw std::runtime_error("png: invalid length code");
                    const int length = lengthBase[lenCode] + static_cast<int>(br.readBits(lengthExtra[lenCode]));
                    const int dSym = dist.decode(br);
                    if (dSym < 0 || dSym >= 30) throw std::runtime_error("png: invalid distance code");
                    const int distance = distBase[dSym] + static_cast<int>(br.readBits(distExtra[dSym]));
                    if (distance <= 0 || static_cast<std::size_t>(distance) > out.size())
                        throw std::runtime_error("png: bad back-reference distance");
                    const std::size_t srcPos = out.size() - static_cast<std::size_t>(distance);
                    for (int i = 0; i < length; ++i) {
                        const unsigned char byte = out[srcPos + static_cast<std::size_t>(i)];
                        out.push_back(byte);
                    }
                }
            }
        } else {
            throw std::runtime_error("png: invalid deflate block type");
        }
    }
}

void writePngToBuffer(std::uint32_t width, std::uint32_t height, const unsigned char* rgba,
                      std::vector<unsigned char>& out) {
    static const unsigned char signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    out.insert(out.end(), signature, signature + 8);

    unsigned char ihdr[13];
    ihdr[0] = static_cast<unsigned char>((width >> 24) & 0xff);
    ihdr[1] = static_cast<unsigned char>((width >> 16) & 0xff);
    ihdr[2] = static_cast<unsigned char>((width >> 8) & 0xff);
    ihdr[3] = static_cast<unsigned char>(width & 0xff);
    ihdr[4] = static_cast<unsigned char>((height >> 24) & 0xff);
    ihdr[5] = static_cast<unsigned char>((height >> 16) & 0xff);
    ihdr[6] = static_cast<unsigned char>((height >> 8) & 0xff);
    ihdr[7] = static_cast<unsigned char>(height & 0xff);
    ihdr[8] = 8;
    ihdr[9] = 6;
    ihdr[10] = 0;
    ihdr[11] = 0;
    ihdr[12] = 0;
    putChunk(out, "IHDR", ihdr, 13);

    const std::size_t rowBytes = static_cast<std::size_t>(width) * 4;
    std::vector<unsigned char> filtered;
    filtered.reserve((rowBytes + 1) * height);
    for (std::uint32_t y = 0; y < height; ++y) {
        filtered.push_back(0);
        filtered.insert(filtered.end(), rgba + y * rowBytes, rgba + (y + 1) * rowBytes);
    }

    std::vector<unsigned char> zlib;
    zlib.reserve(filtered.size() + 64);
    zlib.push_back(0x78);
    zlib.push_back(0x01);
    deflateStored(filtered.data(), filtered.size(), zlib);
    const std::uint32_t adler = adler32(filtered.data(), filtered.size());
    zlib.push_back(static_cast<unsigned char>((adler >> 24) & 0xff));
    zlib.push_back(static_cast<unsigned char>((adler >> 16) & 0xff));
    zlib.push_back(static_cast<unsigned char>((adler >> 8) & 0xff));
    zlib.push_back(static_cast<unsigned char>(adler & 0xff));

    putChunk(out, "IDAT", zlib.data(), zlib.size());
    putChunk(out, "IEND", nullptr, 0);
}

Image readPngImpl(const std::string& path, const unsigned char* data, std::size_t size) {
    static const unsigned char signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    if (size < 8 || std::memcmp(data, signature, 8) != 0)
        throw std::runtime_error("Failed to load " + path + ": not a PNG");

    std::size_t pos = 8;
    std::vector<unsigned char> idat;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    int bitDepth = 0;
    int colorType = 0;
    int interlace = 0;
    std::vector<unsigned char> palette;
    std::vector<unsigned char> paletteAlpha;
    bool sawIhdr = false;
    bool sawIend = false;

    while (pos + 8 <= size) {
        const std::uint32_t chunkLen =
                (static_cast<std::uint32_t>(data[pos]) << 24) |
                (static_cast<std::uint32_t>(data[pos + 1]) << 16) |
                (static_cast<std::uint32_t>(data[pos + 2]) << 8) |
                static_cast<std::uint32_t>(data[pos + 3]);
        if (pos + 8 + static_cast<std::size_t>(chunkLen) + 4 > size)
            throw std::runtime_error("Failed to load " + path + ": truncated chunk");
        const char* type = reinterpret_cast<const char*>(data + pos + 4);
        const unsigned char* body = data + pos + 8;
        const std::uint32_t crcExpected =
                (static_cast<std::uint32_t>(data[pos + 8 + chunkLen]) << 24) |
                (static_cast<std::uint32_t>(data[pos + 8 + chunkLen + 1]) << 16) |
                (static_cast<std::uint32_t>(data[pos + 8 + chunkLen + 2]) << 8) |
                static_cast<std::uint32_t>(data[pos + 8 + chunkLen + 3]);
        const std::uint32_t crcActual = crc32(data + pos + 4, 4 + chunkLen);
        if (crcExpected != crcActual)
            throw std::runtime_error("Failed to load " + path + ": chunk CRC mismatch");

        if (std::memcmp(type, "IHDR", 4) == 0) {
            if (chunkLen != 13) throw std::runtime_error("Failed to load " + path + ": bad IHDR");
            width = (static_cast<std::uint32_t>(body[0]) << 24) | (static_cast<std::uint32_t>(body[1]) << 16) |
                    (static_cast<std::uint32_t>(body[2]) << 8) | static_cast<std::uint32_t>(body[3]);
            height = (static_cast<std::uint32_t>(body[4]) << 24) | (static_cast<std::uint32_t>(body[5]) << 16) |
                     (static_cast<std::uint32_t>(body[6]) << 8) | static_cast<std::uint32_t>(body[7]);
            bitDepth = body[8];
            colorType = body[9];
            if (body[10] != 0 || body[11] != 0)
                throw std::runtime_error("Failed to load " + path + ": unsupported compression/filter method");
            interlace = body[12];
            if (bitDepth != 8)
                throw std::runtime_error("Failed to load " + path + ": only 8-bit PNGs are supported");
            if (interlace != 0)
                throw std::runtime_error("Failed to load " + path + ": interlaced PNGs are not supported");
            if (colorType != 0 && colorType != 2 && colorType != 3 && colorType != 4 && colorType != 6)
                throw std::runtime_error("Failed to load " + path + ": unsupported colour type");
            if (width == 0 || height == 0)
                throw std::runtime_error("Failed to load " + path + ": zero-size image");
            sawIhdr = true;
        } else if (std::memcmp(type, "IDAT", 4) == 0) {
            if (!sawIhdr) throw std::runtime_error("Failed to load " + path + ": IDAT before IHDR");
            idat.insert(idat.end(), body, body + chunkLen);
        } else if (std::memcmp(type, "PLTE", 4) == 0) {
            if (chunkLen == 0 || chunkLen % 3 != 0 || chunkLen > 768)
                throw std::runtime_error("Failed to load " + path + ": bad PLTE");
            palette.assign(body, body + chunkLen);
        } else if (std::memcmp(type, "tRNS", 4) == 0) {
            paletteAlpha.assign(body, body + chunkLen);
        } else if (std::memcmp(type, "IEND", 4) == 0) {
            sawIend = true;
        }

        pos += 8 + static_cast<std::size_t>(chunkLen) + 4;
        if (sawIend) break;
    }

    if (!sawIhdr) throw std::runtime_error("Failed to load " + path + ": no IHDR");
    if (!sawIend) throw std::runtime_error("Failed to load " + path + ": no IEND");
    if (idat.empty()) throw std::runtime_error("Failed to load " + path + ": no IDAT");
    if (idat.size() < 6) throw std::runtime_error("Failed to load " + path + ": truncated zlib stream");

    const unsigned char cmf = idat[0];
    const unsigned char flg = idat[1];
    if ((cmf & 0x0f) != 8)
        throw std::runtime_error("Failed to load " + path + ": unsupported zlib compression method");
    if (((static_cast<unsigned int>(cmf) << 8) + static_cast<unsigned int>(flg)) % 31u != 0)
        throw std::runtime_error("Failed to load " + path + ": bad zlib header checksum");
    if ((flg & 0x20) != 0)
        throw std::runtime_error("Failed to load " + path + ": zlib preset dictionary not supported");

    std::vector<unsigned char> inflated;
    inflated.reserve(idat.size() * 4 + 64);
    doInflate(idat.data() + 2, idat.size() - 2 - 4, inflated);

    const std::size_t adlerPos = idat.size() - 4;
    const std::uint32_t adlerExpected =
            (static_cast<std::uint32_t>(idat[adlerPos]) << 24) |
            (static_cast<std::uint32_t>(idat[adlerPos + 1]) << 16) |
            (static_cast<std::uint32_t>(idat[adlerPos + 2]) << 8) |
            static_cast<std::uint32_t>(idat[adlerPos + 3]);
    const std::uint32_t adlerActual = adler32(inflated.data(), inflated.size());
    if (adlerExpected != adlerActual)
        throw std::runtime_error("Failed to load " + path + ": zlib Adler-32 mismatch");

    static const int samplesPerPixel[7] = {1, 0, 3, 1, 2, 0, 4};
    const int samples = samplesPerPixel[colorType];
    const std::size_t bpp = static_cast<std::size_t>(samples);
    const std::size_t rowBytes = static_cast<std::size_t>(width) * bpp;
    const std::size_t expected = (rowBytes + 1) * height;
    if (inflated.size() < expected)
        throw std::runtime_error("Failed to load " + path + ": inflated data too short");

    std::vector<unsigned char> raw(rowBytes * height);
    for (std::uint32_t y = 0; y < height; ++y) {
        const unsigned char filter = inflated[y * (rowBytes + 1)];
        const unsigned char* row = inflated.data() + y * (rowBytes + 1) + 1;
        unsigned char* dst = raw.data() + y * rowBytes;
        const unsigned char* prevRow = y == 0 ? nullptr : raw.data() + (y - 1) * rowBytes;
        for (std::size_t x = 0; x < rowBytes; ++x) {
            const unsigned char a = x >= bpp ? dst[x - bpp] : 0;
            const unsigned char b = prevRow != nullptr ? prevRow[x] : 0;
            const unsigned char c = prevRow != nullptr && x >= bpp ? prevRow[x - bpp] : 0;
            unsigned char v = row[x];
            switch (filter) {
                case 0:
                    break;
                case 1:
                    v = static_cast<unsigned char>(v + a);
                    break;
                case 2:
                    v = static_cast<unsigned char>(v + b);
                    break;
                case 3:
                    v = static_cast<unsigned char>(v + static_cast<unsigned char>((static_cast<int>(a) + static_cast<int>(b)) / 2));
                    break;
                case 4: {
                    const int p = static_cast<int>(a) + static_cast<int>(b) - static_cast<int>(c);
                    const int pa = std::abs(p - static_cast<int>(a));
                    const int pb = std::abs(p - static_cast<int>(b));
                    const int pc = std::abs(p - static_cast<int>(c));
                    unsigned char paeth = 0;
                    if (pa <= pb && pa <= pc) paeth = a;
                    else if (pb <= pc) paeth = b;
                    else paeth = c;
                    v = static_cast<unsigned char>(v + paeth);
                    break;
                }
                default:
                    throw std::runtime_error("Failed to load " + path + ": invalid PNG filter");
            }
            dst[x] = v;
        }
    }

    std::vector<unsigned char> out(static_cast<std::size_t>(width) * height * 4);
    for (std::uint32_t y = 0; y < height; ++y) {
        const unsigned char* src = raw.data() + y * rowBytes;
        unsigned char* dst = out.data() + y * static_cast<std::size_t>(width) * 4;
        for (std::uint32_t x = 0; x < width; ++x) {
            switch (colorType) {
                case 0: {
                    const unsigned char g = src[x];
                    dst[4 * x + 0] = g;
                    dst[4 * x + 1] = g;
                    dst[4 * x + 2] = g;
                    dst[4 * x + 3] = 255;
                    break;
                }
                case 2:
                    dst[4 * x + 0] = src[3 * x + 0];
                    dst[4 * x + 1] = src[3 * x + 1];
                    dst[4 * x + 2] = src[3 * x + 2];
                    dst[4 * x + 3] = 255;
                    break;
                case 3: {
                    const unsigned char idx = src[x];
                    if (static_cast<std::size_t>(idx) * 3 + 2 >= palette.size())
                        throw std::runtime_error("Failed to load " + path + ": palette index out of range");
                    dst[4 * x + 0] = palette[3 * idx + 0];
                    dst[4 * x + 1] = palette[3 * idx + 1];
                    dst[4 * x + 2] = palette[3 * idx + 2];
                    dst[4 * x + 3] = idx < paletteAlpha.size() ? paletteAlpha[idx] : 255;
                    break;
                }
                case 4: {
                    const unsigned char g = src[2 * x + 0];
                    dst[4 * x + 0] = g;
                    dst[4 * x + 1] = g;
                    dst[4 * x + 2] = g;
                    dst[4 * x + 3] = src[2 * x + 1];
                    break;
                }
                case 6:
                    dst[4 * x + 0] = src[4 * x + 0];
                    dst[4 * x + 1] = src[4 * x + 1];
                    dst[4 * x + 2] = src[4 * x + 2];
                    dst[4 * x + 3] = src[4 * x + 3];
                    break;
            }
        }
    }

    return Image{width, height, std::move(out)};
}

}

Image readPng(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("Failed to load " + path + ": cannot open");
    const std::streampos size = file.tellg();
    if (size < 0) throw std::runtime_error("Failed to load " + path + ": cannot determine size");
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> data(static_cast<std::size_t>(size));
    if (size > 0 && !file.read(reinterpret_cast<char*>(data.data()), size))
        throw std::runtime_error("Failed to load " + path + ": read failed");
    return readPngImpl(path, data.data(), data.size());
}

bool writePng(const std::string& path, std::uint32_t width, std::uint32_t height, const unsigned char* rgba) {
    if (width == 0 || height == 0) return false;
    std::vector<unsigned char> buffer;
    writePngToBuffer(width, height, rgba, buffer);
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    file.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
    return static_cast<bool>(file);
}

}
