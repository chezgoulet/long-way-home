// ibize - native Linux replacement for Raven's IBIze.exe
//
// Compiles an ICARUS mission script (.txt) into the block-instruction stream the
// game loads at runtime (.IBI). Built from the offline compiler that ships inside
// Raven's released single-player source: CTokenizer (lexer) + CInterpreter (parser)
// + CBlockStream (writer). Raven's own command-line driver was never released;
// this is that driver, nothing more.
//
// Usage: ibize <input.txt> [output.IBI]

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>

#include "tokenizer.h"
#include "blockstream.h"   // must precede interpreter.h: it defines CBlock/CBlockStream
#include "interpreter.h"

// Raven's driver reported diagnostics through this callback; without it the tokenizer's
// error path is silent, and a silent failure is indistinguishable from success.
static void ibize_error(LPCTSTR errString)
{
    fprintf(stderr, "%s\n", errString ? errString : "(null error)");
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: ibize <input.txt> [output.IBI]\n");
        return 2;
    }

    // Default output name mirrors the game's expectation: same path, .IBI extension.
    char out[MAX_FILENAME_LENGTH];
    if (argc >= 3) {
        snprintf(out, sizeof(out), "%s", argv[2]);
    } else {
        snprintf(out, sizeof(out), "%s", argv[1]);
        char *dot = strrchr(out, '.');
        if (dot) {
            *dot = '\0';
        }
        strncat(out, IBI_EXT, sizeof(out) - strlen(out) - 1);
    }

    CBlockStream stream;
    if (stream.Create(out) == 0) {
        fprintf(stderr, "ibize: cannot create output '%s'\n", out);
        return 3;
    }

    CInterpreter interp;
    CTokenizer *tokenizer = CTokenizer::Create(0);
    if (!tokenizer) {
        fprintf(stderr, "ibize: cannot create tokenizer\n");
        return 4;
    }

    tokenizer->SetErrorProc(ibize_error);
    tokenizer->SetSymbols(interp.GetSymbols());

    // The tokenizer scans a keyword table until it sees TK_EOF. The interpreter's ID table
    // terminates with its own ID_EOF sentinel instead, so handing it over directly makes the
    // scan run off the end of the array -- silently on 32-bit, a heap/global overread at 64-bit
    // (caught by AddressSanitizer). Copy the table and terminate it the way the tokenizer expects.
    // Both the ID table and the type table are terminated with their own sentinels
    // (ID_EOF / TYPE_EOF), so neither can go to the tokenizer as-is. Merge both into one
    // TK_EOF-terminated table: the interpreter matches on the numeric values either way, and
    // the scripts' BehavEd annotations (/*@AFFECT_TYPE*/ FLUSH, /*@CAMERA_COMMANDS*/ MOVE)
    // are type keywords that must be lexable.
    std::vector<keywordArray_t> keywords;
    // Measured, not assumed: the interpreter resolves script names against its own tables, and
    // handing them to the tokenizer makes it emit keyword tokens the interpreter then rejects.
    // Across the 2,408-script shipped corpus: no table = mean 1,143 B output, 52 minimal streams;
    // ID table = mean 232 B, 742 minimal streams. Override only for experiments.
    const char *which = getenv("IBIZE_KEYWORDS") ? getenv("IBIZE_KEYWORDS") : "none";
    keywordArray_t *tables[2];
    int n = 0;
    if (strcmp(which, "ids") == 0 || strcmp(which, "both") == 0)   tables[n++] = interp.GetIDs();
    if (strcmp(which, "types") == 0 || strcmp(which, "both") == 0) tables[n++] = interp.GetTypes();
    for (int ti = 0; ti < n; ++ti) {
        for (keywordArray_t *p = tables[ti]; p->m_keyword && p->m_keyword[0] != '\0'; ++p) {
            keywords.push_back(*p);
        }
    }
    keywordArray_t terminator;
    terminator.m_keyword = (char *)"";
    terminator.m_tokenvalue = TK_EOF;
    keywords.push_back(terminator);
    tokenizer->SetKeywords(keywords.data());

    if (!tokenizer->AddParseFile(argv[1])) {
        fprintf(stderr, "ibize: cannot open input '%s'\n", argv[1]);
        tokenizer->Delete();
        return 5;
    }

    if (getenv("IBIZE_TRACE")) fprintf(stderr, "stage: interpret\n");
    const int rc = interp.Interpret(tokenizer, &stream, argv[1]);
    if (getenv("IBIZE_TRACE")) fprintf(stderr, "stage: interpret done rc=%d\n", rc);

    if (getenv("IBIZE_TRACE")) fprintf(stderr, "stage: tokenizer delete\n");
    tokenizer->Delete();
    if (getenv("IBIZE_TRACE")) fprintf(stderr, "stage: stream free\n");
    stream.Free();
    if (getenv("IBIZE_TRACE")) fprintf(stderr, "stage: exit\n");
    return rc;
}
