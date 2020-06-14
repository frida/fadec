
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>

#include <fadec.h>


static
uint8_t
parse_nibble(const char nibble)
{
    if (nibble >= '0' && nibble <= '9')
        return nibble - '0';
    else if (nibble >= 'a' && nibble <= 'f')
        return nibble - 'a' + 10;
    else if (nibble >= 'A' && nibble <= 'F')
        return nibble - 'A' + 10;
    printf("Invalid hexadecimal number: %x\n", nibble);
    exit(1);
}

int
main(int argc, char** argv)
{
    if (argc != 3 && argc != 4)
    {
        printf("usage: %s [mode] [instruction bytes] ([repetitions])\n", argv[0]);
        return -1;
    }

    size_t mode = strtoul(argv[1], NULL, 0);

    // Avoid allocation by transforming hex to binary in-place.
    uint8_t* code = (uint8_t*) argv[2];
    uint8_t* code_end = code;
    char* hex = argv[2];
    for (; *hex; hex += 2, code_end++)
        *code_end = (parse_nibble(hex[0]) << 4) | parse_nibble(hex[1]);

    size_t length = (size_t) (code_end - code);

    size_t repetitions = 1;
    if (argc >= 4)
        repetitions = strtoul(argv[3], NULL, 0);

    struct timespec time_start;
    struct timespec time_end;

    FdInstr instr;
    int retval = 0;

    __asm__ volatile("" : : : "memory");
    clock_gettime(CLOCK_MONOTONIC, &time_start);
    for (size_t i = 0; i < repetitions; i++)
    {
        size_t current_off = 0;
        while (current_off != length)
        {
            size_t remaining = length - current_off;
            retval = fd_decode(code + current_off, remaining, mode, 0, &instr);
            if (retval < 0)
                break;
            current_off += retval;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &time_end);
    __asm__ volatile("" : : : "memory");

    if (retval >= 0)
    {
        char format_buffer[128];
        fd_format(&instr, format_buffer, sizeof(format_buffer));
        printf("%s\n", format_buffer);
    }
    else if (retval == FD_ERR_UD)
    {
        printf("UD\n");
    }
    else if (retval == FD_ERR_PARTIAL)
    {
        printf("PARTIAL\n");
    }

    if (repetitions > 1)
    {
        uint64_t nsecs = 1000000000ull * (time_end.tv_sec - time_start.tv_sec) +
                                        (time_end.tv_nsec - time_start.tv_nsec);

        printf("%" PRIu64 " ns\n", nsecs);
    }

    return 0;
}
