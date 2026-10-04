// ibi-dump - read a compiled ICARUS block stream back and report its structure.
//
// The mirror of tools/ibize: it opens a .IBI with CBlockStream::Open (which validates the
// "IBI" magic and the 1.57 version field), then walks the block stream with
// BlockAvailable/ReadBlock. Used to verify that what ibize writes is what the game's own
// reader would accept and understand, without needing the game or the original compiler.
//
// Usage: ibi-dump <file.IBI> [more.IBI ...]
// Output, one line per file:
//   OK<TAB><path><TAB>blocks=<n><TAB>members=<n><TAB>ids=<distinct block ids>
//   FAIL<TAB><path><TAB><reason>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <set>

#include "blockstream.h"

static long slurp(const char *path, std::vector<char> &into)
{
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    into.resize(n > 0 ? n : 0);
    if (n > 0 && fread(into.data(), 1, n, f) != (size_t)n) { fclose(f); return -1; }
    fclose(f);
    return n;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: ibi-dump <file.IBI> [more.IBI ...]\n");
        return 2;
    }

    int failures = 0;
    for (int i = 1; i < argc; ++i) {
        std::vector<char> buf;
        long n = slurp(argv[i], buf);
        if (n < 0) { printf("FAIL\t%s\tunreadable\n", argv[i]); failures++; continue; }

        CBlockStream stream;
        if (!stream.Open(buf.data(), n)) {
            printf("FAIL\t%s\tbad header or version\n", argv[i]);
            failures++;
            continue;
        }

        int blocks = 0, members = 0, malformed = 0;
        std::set<int> ids;
        while (stream.BlockAvailable()) {
            CBlock block;
            if (!stream.ReadBlock(&block)) { malformed++; break; }
            blocks++;
            members += block.GetNumMembers();
            ids.insert(block.GetBlockID());
        }

        if (malformed) {
            printf("FAIL\t%s\ttruncated block at block %d\n", argv[i], blocks);
            failures++;
        } else {
            printf("OK\t%s\tblocks=%d\tmembers=%d\tids=%zu\n", argv[i], blocks, members, ids.size());
        }
    }
    return failures ? 1 : 0;
}
