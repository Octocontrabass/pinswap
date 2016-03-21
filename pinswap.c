#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define A 9+
#define D 1+
#define NONE 0

static const uint_fast8_t jedec[32] = {
    A 19, A 16, A 15, A 12, A 7, A 6,  A 5,  A 4,  A 3,  A 2, A 1, A 0,  D 0,  D 1,  D 2,  NONE,
    D 3,  D 4,  D 5,  D 6,  D 7, NONE, A 10, NONE, A 11, A 9, A 8, A 13, A 14, A 17, A 18, NONE
};

static const char * const name[29] = {
    "",
    "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7",
    "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7",
    "A8", "A9", "A10", "A11", "A12", "A13", "A14", "A15",
    "A16", "A17", "A18", "A19"
};

static void diagram( FILE * stream, const char * const * const source,
                const char * const * const destination, uint_fast8_t limit )
{
    fprintf( stream, "      ___ ___\n"
                     "     |   U   |\n" );
    for( uint_fast8_t i = (limit < (A 16)) ? (limit < (A 12)) ? 4 : 2 : 0; i < 16; i++ )
    {
        fprintf( stream, "%3s--|%-3s %3s|--%s\n",
                destination[(jedec[i] > limit) ? NONE : jedec[i]],
                source[(jedec[i] > limit) ? NONE : jedec[i]],
                source[(jedec[31-i] > limit) ? NONE : jedec[31-i]],
                destination[(jedec[31-i] > limit) ? NONE : jedec[31-i]]);
    }
    fprintf( stream, "     |_______|\n" );
}

int main( int argc, char ** argv )
{
    if( argc == 1 )
    {
        diagram( stdout, name, name, A 9 );
        fprintf( stdout,
                "\n"
                "ROM pin swapper\n"
                "Usage: pinswap [options] file\n"
                "\n"
                "Options:\n"
                "  -o <file>    Save the result to <file>\n"
                "  -a19,18,...  Rearrange address pins, highest first\n"
                "  -d7,6,...    Rearrange data pins, highest first\n"
                "  -r           Reverse the pin-swap operation (useful for dumped ROMs)\n");
        return EXIT_SUCCESS;
    }
    
    FILE * infile = fopen( argv[argc-1], "rb" );
    if( !infile )
    {
        fprintf( stderr, "Couldn't open input file: %s\n", argv[argc-1] );
        return EXIT_FAILURE;
    }
    
    fseek( infile, 0, SEEK_END );
    long length = ftell( infile );
    if( length < 0 )
    {
        fprintf( stderr, "Couldn't determine input file length\n" );
        return EXIT_FAILURE;
    }
    
    uint_fast8_t highest_address_bit = 0;
    
    for( uint_fast8_t i = 9; i <= 19; i++ )
    {
        if( length == 1 << (i + 1) ) highest_address_bit = i;
    }
    if( !highest_address_bit )
    {
        if( length < (1 << 10) )
        {
            fprintf( stderr, "Input file is too small; minimum is 1kiB\n" );
        }
        else if( length > (1 << 20) )
        {
            fprintf( stderr, "Input file is too big; maximum is 1MiB\n" );
        }
        else
        {
            fprintf( stderr, "Input file size is not a power of 2: %ld (0x%lX)\n",
                    length, length );
        }
        return EXIT_FAILURE;
    }
    
    unsigned address_map[20] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
    unsigned data_map[8] = {0,1,2,3,4,5,6,7};
    bool reverse = false;
    FILE * outfile = NULL;
    
    for( int i = 1; i < (argc - 1); i++ )
    {
        if( !strncmp( argv[i], "-a", 2 ) )
        {
            char * string = argv[i] + 1;
            uint_fast32_t address_bits = 0;
            for( uint_fast8_t j = highest_address_bit; j <= highest_address_bit; j-- )
            {
                if( !string )
                {
                    fprintf( stderr, "Invalid parameter: %s\n", argv[i] );
                    return EXIT_FAILURE;
                }
                string++;
                sscanf( string, "%u", &address_map[j] );
                if( address_map[j] < 32 ) address_bits |= 1 << address_map[j];
                string = strchr( string, ',' );
            }
            if( address_bits != (1 << (highest_address_bit + 1)) - 1 )
            {
                fprintf( stderr, "Invalid parameter: %s\n", argv[i] );
                return EXIT_FAILURE;
            }
        }
        else if( !strncmp( argv[i], "-d", 2 ) )
        {
            char * string = argv[i] + 1;
            uint_fast8_t data_bits = 0;
            for( uint_fast8_t j = 7; j <= 7; j-- )
            {
                if( !string )
                {
                    fprintf( stderr, "Invalid parameter: %s\n", argv[i] );
                    return EXIT_FAILURE;
                }
                string++;
                sscanf( string, "%u", &data_map[j] );
                if( data_map[j] < 8 ) data_bits |= 1 << data_map[j];
                string = strchr( string, ',' );
            }
            if( data_bits != (1 << 8) - 1 )
            {
                fprintf( stderr, "Invalid parameter: %s\n", argv[i] );
                return EXIT_FAILURE;
            }
        }
        else if( !strcmp( argv[i], "-r" ) )
        {
            reverse = true;
        }
        else if( i < (argc - 2) && !strcmp( argv[i], "-o" ) )
        {
            if( outfile )
            {
                fprintf( stderr, "Cannot output to multiple files\n" );
                return EXIT_FAILURE;
            }
            i++;
            outfile = fopen( argv[i], "wb" );
            if( !outfile )
            {
                fprintf( stderr, "Couldn't open output file: %s\n", argv[i] );
                return EXIT_FAILURE;
            }
        }
        else
        {
            fprintf( stderr, "Invalid parameter: %s\n", argv[i] );
            return EXIT_FAILURE;
        }
    }
    
    if( !outfile )
    {
        const char * swapname[29];
        swapname[0] = name[0];
        for( uint_fast8_t i = 0; i < 8; i++ )
        {
            swapname[D i] = name[D data_map[i]];
        }
        for( uint_fast8_t i = 0; i < 20; i++ )
        {
            swapname[A i] = name[A address_map[i]];
        }
        if( reverse )
        {
            diagram( stdout, swapname, name, A highest_address_bit );
        }
        else
        {
            diagram( stdout, name, swapname, A highest_address_bit );
        }
        return EXIT_SUCCESS;
    }
    
    uint8_t * bitswap = malloc( 256 );
    uint8_t * input = malloc( length );
    uint8_t * output = malloc( length );
    if( !bitswap || !input || !output )
    {
        fprintf( stderr, "Failed to allocate memory!\n" );
        return EXIT_FAILURE;
    }
    
    rewind( infile );
    if( fread( input, 1, length, infile ) != length )
    {
        fprintf( stderr, "Couldn't read input file\n" );
        return EXIT_FAILURE;
    }
    
    if( reverse )
    {
        for( uint_fast16_t i = 0; i < 256; i++ )
        {
            bitswap[i] = (i >> 7 & 1) << data_map[7] |
                    (i >> 6 & 1) << data_map[6] |
                    (i >> 5 & 1) << data_map[5] |
                    (i >> 4 & 1) << data_map[4] |
                    (i >> 3 & 1) << data_map[3] |
                    (i >> 2 & 1) << data_map[2] |
                    (i >> 1 & 1) << data_map[1] |
                    (i >> 0 & 1) << data_map[0];
        }
        for( long i = 0; i < length; i++ )
        {
            output[(i >> 19 & 1) << address_map[19] |
                    (i >> 18 & 1) << address_map[18] |
                    (i >> 17 & 1) << address_map[17] |
                    (i >> 16 & 1) << address_map[16] |
                    (i >> 15 & 1) << address_map[15] |
                    (i >> 14 & 1) << address_map[14] |
                    (i >> 13 & 1) << address_map[13] |
                    (i >> 12 & 1) << address_map[12] |
                    (i >> 11 & 1) << address_map[11] |
                    (i >> 10 & 1) << address_map[10] |
                    (i >> 9 & 1) << address_map[9] |
                    (i >> 8 & 1) << address_map[8] |
                    (i >> 7 & 1) << address_map[7] |
                    (i >> 6 & 1) << address_map[6] |
                    (i >> 5 & 1) << address_map[5] |
                    (i >> 4 & 1) << address_map[4] |
                    (i >> 3 & 1) << address_map[3] |
                    (i >> 2 & 1) << address_map[2] |
                    (i >> 1 & 1) << address_map[1] |
                    (i >> 0 & 1) << address_map[0]] = bitswap[input[i]];
        }
    }
    else
    {
        for( uint_fast16_t i = 0; i < 256; i++ )
        {
            bitswap[i] = (i >> data_map[7] & 1) << 7 |
                    (i >> data_map[6] & 1) << 6 |
                    (i >> data_map[5] & 1) << 5 |
                    (i >> data_map[4] & 1) << 4 |
                    (i >> data_map[3] & 1) << 3 |
                    (i >> data_map[2] & 1) << 2 |
                    (i >> data_map[1] & 1) << 1 |
                    (i >> data_map[0] & 1) << 0;
        }
        for( long i = 0; i < length; i++ )
        {
            output[(i >> address_map[19] & 1) << 19 |
                    (i >> address_map[18] & 1) << 18 |
                    (i >> address_map[17] & 1) << 17 |
                    (i >> address_map[16] & 1) << 16 |
                    (i >> address_map[15] & 1) << 15 |
                    (i >> address_map[14] & 1) << 14 |
                    (i >> address_map[13] & 1) << 13 |
                    (i >> address_map[12] & 1) << 12 |
                    (i >> address_map[11] & 1) << 11 |
                    (i >> address_map[10] & 1) << 10 |
                    (i >> address_map[9] & 1) << 9 |
                    (i >> address_map[8] & 1) << 8 |
                    (i >> address_map[7] & 1) << 7 |
                    (i >> address_map[6] & 1) << 6 |
                    (i >> address_map[5] & 1) << 5 |
                    (i >> address_map[4] & 1) << 4 |
                    (i >> address_map[3] & 1) << 3 |
                    (i >> address_map[2] & 1) << 2 |
                    (i >> address_map[1] & 1) << 1 |
                    (i >> address_map[0] & 1) << 0] = bitswap[input[i]];
        }
    }
    
    if( fwrite( output, 1, length, outfile ) != length )
    {
        fprintf( stderr, "Couldn't write output file\n" );
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
