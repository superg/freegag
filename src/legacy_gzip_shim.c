/* Memory-I/O adapter for the unmodified GNU gzip 1.3.5 codec sources.
 * Upstream copyright and GPLv2-or-later notices are retained in the fetched
 * source revision and its COPYING file. */

#include "legacy_gzip_shim.h"
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include "gzip.h"
#include "lzw.h"
#include "tailor.h"

uch inbuf[INBUFSIZ + INBUF_EXTRA];
uch outbuf[OUTBUFSIZ + OUTBUF_EXTRA];
ush d_buf[DIST_BUFSIZE];
uch window[2L * WSIZE];
#ifndef MAXSEG_64K
ush tab_prefix[1L << BITS];
#else
ush tab_prefix0[1L << (BITS - 1)];
ush tab_prefix1[1L << (BITS - 1)];
#endif

unsigned insize;
unsigned inptr;
unsigned outcnt;
long bytes_in;
long bytes_out;
long header_bytes;
int ifd;
int ofd;
char ifname[1024];
char ofname[1024];
char *progname = "freegag";
long time_stamp;
long ifile_size;
int method = DEFLATED;
int decrypt;
int exit_code = OK;
int verbose;
int quiet = 1;
int level = 9;
int test;
int to_stdout;
int save_orig_name;

static const uch *memory_input;
static uint32_t memory_input_size;
static uint32_t memory_input_position;
static uint32_t memory_input_padding;
static uch *memory_output;
static uint32_t memory_output_capacity;
static uint32_t memory_output_position;
static int memory_failed;
static jmp_buf failure_target;

static void fail_operation(void)
{
    memory_failed = 1;
    longjmp(failure_target, 1);
}

static int memory_read(char *destination, unsigned size)
{
    uint32_t remaining = memory_input_size - memory_input_position;
    uint32_t count = remaining < size ? remaining : size;
    if(count != 0)
    {
        memcpy(destination, memory_input + memory_input_position, count);
        memory_input_position += count;
        bytes_in += count;
    }
    return (int)count;
}

int file_read(char *destination, unsigned size)
{
    return memory_read(destination, size);
}

static void memory_write(const void *source, uint32_t size)
{
    if(size > memory_output_capacity - memory_output_position)
        fail_operation();
    memcpy(memory_output + memory_output_position, source, size);
    memory_output_position += size;
}

void clear_bufs(void)
{
    outcnt = 0;
    insize = inptr = 0;
    bytes_in = bytes_out = 0;
}

int fill_inbuf(int eof_ok)
{
    uint32_t remaining = memory_input_size - memory_input_position;
    uint32_t count = remaining < INBUFSIZ ? remaining : INBUFSIZ;
    if(count == 0)
    {
        if(memory_input_padding != 0)
        {
            count = memory_input_padding < INBUFSIZ ? memory_input_padding : INBUFSIZ;
            memset(inbuf, 0, count);
            memory_input_padding -= count;
            insize = count;
            inptr = 1;
            return inbuf[0];
        }
        if(eof_ok)
            return EOF;
        fail_operation();
    }
    memcpy(inbuf, memory_input + memory_input_position, count);
    memory_input_position += count;
    insize = count;
    inptr = 1;
    bytes_in += count;
    return inbuf[0];
}

void flush_outbuf(void)
{
    if(outcnt == 0)
        return;
    memory_write(outbuf, outcnt);
    bytes_out += outcnt;
    outcnt = 0;
}

void flush_window(void)
{
    if(outcnt == 0)
        return;
    memory_write(window, outcnt);
    bytes_out += outcnt;
    outcnt = 0;
}

void write_buf(int fd, voidp buffer, unsigned count)
{
    (void)fd;
    memory_write(buffer, count);
}

ulg updcrc(uch *source, unsigned count)
{
    (void)source;
    (void)count;
    return 0;
}

void error(char *message)
{
    (void)message;
    fail_operation();
}

void warn(char *left, char *right)
{
    (void)left;
    (void)right;
}

void warning(char *message)
{
    (void)message;
}

void read_error(void)
{
    fail_operation();
}

void write_error(void)
{
    fail_operation();
}

void freegag_legacy_gzip_begin_compression(void)
{
    memset(inbuf, 0, sizeof(inbuf));
    memset(outbuf, 0, sizeof(outbuf));
    memset(d_buf, 0, sizeof(d_buf));
    memset(window, 0, sizeof(window));
    memset(tab_prefix, 0, sizeof(tab_prefix));
    clear_bufs();
}

uint32_t freegag_legacy_gzip_compress(const void *source, uint32_t source_size, void *destination, uint32_t destination_capacity)
{
    ush attributes = 0;
    ush flags = 0;

    if((source == NULL && source_size != 0) || destination == NULL)
        return 0;
    memory_input = (const uch *)source;
    memory_input_size = source_size;
    memory_input_position = 0;
    memory_input_padding = 0;
    memory_output = (uch *)destination;
    memory_output_capacity = destination_capacity;
    memory_output_position = 0;
    memory_failed = 0;
    clear_bufs();
    if(setjmp(failure_target) != 0)
        return 0;
    read_buf = memory_read;
    method = DEFLATED;
    bi_init(NO_FILE);
    ct_init(&attributes, &method);
    lm_init(9, &flags);
    (void)deflate();
    flush_outbuf();
    return memory_failed ? 0 : memory_output_position;
}

int freegag_legacy_gzip_decompress(const void *source, uint32_t source_size, void *destination, uint32_t destination_capacity, uint32_t *decoded_size, int accept_full_output)
{
    int result;

    if(source == NULL || destination == NULL || decoded_size == NULL)
        return 0;
    memory_input = (const uch *)source;
    memory_input_size = source_size;
    memory_input_position = 0;
    memory_input_padding = INBUF_EXTRA;
    memory_output = (uch *)destination;
    memory_output_capacity = destination_capacity;
    memory_output_position = 0;
    memory_failed = 0;
    clear_bufs();
    if(setjmp(failure_target) != 0)
    {
        if(accept_full_output && outcnt == memory_output_capacity - memory_output_position)
        {
            memcpy(memory_output + memory_output_position, window, outcnt);
            memory_output_position += outcnt;
            outcnt = 0;
        }
        *decoded_size = memory_output_position;
        return accept_full_output && memory_output_position == destination_capacity;
    }
    result = inflate();
    *decoded_size = memory_output_position;
    return !memory_failed && result == 0;
}
