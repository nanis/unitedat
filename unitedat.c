const char* UNITEDAT_VERSION = "0.0.2";

#ifdef WINDOWS
#define WIN32_LEAN_AND_MEAN /* https://stackoverflow.com/a/11040290/100754 */
#define NOMINMAX /* https://stackoverflow.com/a/11544154/100754 */
#include <windows.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WINDOWS
#define FILE_OPEN_FUNC(path, mode) _wfopen(path, mode)
#define MODE_READ L"r"
#else
#define FILE_OPEN_FUNC(path, mode) fopen(path, mode)
#define MODE_READ "r"
#endif

#define INPUT_BUFFER_SIZE (32 * 1024)

struct data_file_info {
    FILE *fh;
    const char *name;
};

struct param
{
    int num_lines;
    int first_filename_idx;
};

/*
 * Homebrew CLI argument processing because I am hard to please. I like
 * [CLI11](https://github.com/CLIUtils/CLI11), so maybe I'll throw some C++ on
 * this later.
 *
 * Takes only one argument -n/--header-lines which is the number of lines to
 * skip from the start of the files after the first one.
 */

static struct param
get_params(const int argc, const char* const* argv)
{
    struct param p;
    p.num_lines = 1;
    p.first_filename_idx = 1;

    if (argc >= 2) {
        if ((strcmp(argv[1], "-n") == 0) ||
            (strcmp(argv[1], "--header-lines") == 0)) {
            char* endp = NULL;

            if (argc < 3) {
                fputs("Missing value for --header-lines argument\n", stderr);
                exit(EXIT_FAILURE);
            }

            p.num_lines = strtol(argv[2], &endp, 10);
            if (*endp) {
                fprintf(
                  stderr, "Failed to parse '%s' as an integer\n", argv[2]);
                exit(EXIT_FAILURE);
            }

            if (p.num_lines < 0) {
                fputs("Number of lines to skip must be nonnegative\n", stderr);
                exit(EXIT_FAILURE);
            }

            p.first_filename_idx = 3;
        }
    }

    if (p.first_filename_idx >= argc) {
        fputs("No files provided\n", stderr);
        exit(EXIT_FAILURE);
    }

    return p;
}

static size_t
fill_buffer(struct data_file_info* source, uint8_t* buffer, size_t buffer_size)
{
    size_t nread = fread(buffer, 1, buffer_size, source->fh);
    if ((nread == 0) && ferror(source->fh)) {
        if (ferror(source->fh)) {
            perror(source->name);
            fprintf(stderr,
                    "Failed to read %zu bytes from '%s'\n",
                    buffer_size,
                    source->name);
            exit(EXIT_FAILURE);
        }
    }
    return nread;
}

/* It would have been easier to use C++ as it has a standard "getline", but
 * sticking with C, given the choice between implementing another "getline" or
 * trying to find an implementation I liked, or just doing block input, I went
 * with the latter. Note that if you are dealing with data files containing Mac
 * OS 9 style line-endings (i.e., CR only), this will not work. Apparently,
 * Excel on OS X does this. It's nuts. I don't want to cater to it.
 */

static uint64_t
find_data_start(struct data_file_info* source, int num_lines)
{
    uint8_t buffer[INPUT_BUFFER_SIZE];
    uint64_t offset;
    uint64_t buffer_size = 0;
    int skipped_lines = 0;

    // Easier to bail out early
    if (num_lines == 0) {
        return 0;
    }

    while (1) {
        // This loop will be skipped the first time around because we haven't
        // read anything into the buffer yet.
        for (offset = 0; offset < buffer_size; ++offset) {
            if (buffer[offset] == '\n') {
                ++skipped_lines;
                if (skipped_lines == num_lines) {
                    ++offset;
                    // We are done, so output the header. Frankly, I don't like
                    // the fact that find_data_start actually outputs the header
                    // but 1) naming is hard and 2) the alternative is to
                    // arrange for the header to be output somewhere else or to
                    // treat the first file differently (which is what I did
                    // originally). I like both of those alternatives even less.
                    size_t nwritten = fwrite(buffer, 1, offset, stdout);
                    if (nwritten < offset) {
                        perror("Failed to write");
                        fprintf(stderr,
                                "Failed to write out %zu bytes of header "
                                "(wrote %zu)\n",
                                offset - nwritten,
                                nwritten);
                        exit(EXIT_FAILURE);
                    }
                    return offset;
                }
            }
        }

        buffer_size = fill_buffer(source, buffer, INPUT_BUFFER_SIZE);
        if (buffer_size == 0) {
            fprintf(stderr,
                    "Ran out of input while attempting the skip %d lines of "
                    "header in '%s'\n",
                    num_lines,
                    source->name);
            exit(EXIT_FAILURE);
        }
    }
}

static void
cat(struct data_file_info* source, uint64_t data_start)
{
    size_t nread;
    uint8_t buffer[INPUT_BUFFER_SIZE];
    size_t blocks_written = 0;

    // Apparently, attempting to seek past EOF does not return an error
    if (fseek(source->fh, data_start, SEEK_SET) < 0) {
        perror("Failed to seek");
        fprintf(stderr,
                "Failed to skip %zu bytes from the start of '%s'",
                data_start,
                source->name);
        exit(EXIT_FAILURE);
    }

    while ((nread = fill_buffer(source, buffer, INPUT_BUFFER_SIZE))) {
        size_t nwritten = fwrite(buffer, 1, nread, stdout);
        if (nwritten < nread) {
            perror("Failed to write");
            fprintf(stderr,
                    "Failed to write out %zu bytes (wrote %zu)\n",
                    nread - nwritten,
                    nwritten);
            exit(EXIT_FAILURE);
        }
        ++blocks_written;
    }

    if (ferror(source->fh)) {
        perror(source->name);
        exit(EXIT_FAILURE);
    }

    if (blocks_written == 0) {
        fprintf(stderr, "Produced no output from %s\n", source->name);
        exit(EXIT_FAILURE);
    }

    fclose(source->fh);
    fflush(stdout);
}

static void
unite(struct data_file_info* sources, int num_files, int num_lines)
{
    uint64_t data_start = find_data_start(sources, num_lines);

    for (int i = 0; i < num_files; ++i) {
        cat(sources + i, data_start);
    }
}

#ifdef WINDOWS
#include <fcntl.h>
#include <io.h>

static void
binmode_stdout()
{
    if (_setmode(fileno(stdout), _O_BINARY), 0) {
        perror("Failed to binmode stdout");
        exit(EXIT_FAILURE);
    }
}

static const char*
utf8_encode(const wchar_t* source)
{
    char* target = NULL;

    // https://docs.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte
    int required = WideCharToMultiByte(CP_UTF8, // target code page
                                       WC_ERR_INVALID_CHARS,
                                       source,
                                       -1, // argv entries are NUL terminated
                                       target,
                                       0,    // return the required space
                                       NULL, // must be NULL for UTF-8 target
                                       NULL  // must be NULL for UTF-8 target
    );

    if (required == 0) {
        fputs("Failed to obtain required buffer size to encode wide string as "
              "UTF-8\n",
              stderr);
        exit(EXIT_FAILURE);
    }

    target = malloc(required);
    if (target == NULL) {
        perror("Malloc failed");
        fprintf(stderr,
                "Failed to allocate memory for %d bytes required to hold UTF-8 "
                "encoded string\n",
                required);
        exit(EXIT_FAILURE);
    }

    int result = WideCharToMultiByte(CP_UTF8, // target code page
                                     WC_ERR_INVALID_CHARS,
                                     source,
                                     -1, // argv entries are NUL terminated
                                     target,
                                     required,
                                     NULL, // must be NULL for UTF-8 target
                                     NULL  // must be NULL for UTF-8 target
    );

    if (result == 0) {
        perror("Failed to encode wide string as UTF-8");
        exit(EXIT_FAILURE);
    }

    return target;
}

const char* const*
utf8_encode_array(const int n, const wchar_t* sources[])
{
    const char** targets = malloc(n * sizeof(*targets));
    if (!targets) {
        perror("Malloc failed");
        fprintf(stderr, "Failed to allocate memory for %d pointers\n", n);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n; ++i) {
        targets[i] = utf8_encode(sources[i]);
    }
    return targets;
}
#endif

#ifdef WINDOWS
int
wmain(int argc, wchar_t* argv[])
{
    const char* const* narrow_argv = utf8_encode_array(argc, argv);
#else
int
main(int argc, char* argv[])
{
    const char* const* narrow_argv = (const char* const*)argv;
#endif
    struct param p = get_params(argc, narrow_argv);
    int num_files = argc - p.first_filename_idx;
    struct data_file_info* sources = malloc(num_files * sizeof(*sources));

#ifdef WINDOWS
    binmode_stdout();
#endif

    if (!sources) {
        perror("Malloc failed");
        fprintf(stderr,
                "Failed to allocated memory for %d file info structs\n",
                num_files);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_files; ++i) {
        int idx = i + p.first_filename_idx;
        sources[i].name = narrow_argv[idx];
        sources[i].fh = FILE_OPEN_FUNC(argv[idx], MODE_READ);
        if (!sources[i].fh) {
            perror("Failed to open file");
            fprintf(
              stderr, "Failed to open '%s' for reading\n", narrow_argv[idx]);
            exit(EXIT_FAILURE);
        }
    }

    unite(sources, num_files, p.num_lines);

    return 0;
}
