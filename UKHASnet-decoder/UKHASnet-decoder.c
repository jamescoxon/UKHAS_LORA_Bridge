#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>
#include <unistd.h> // getopt
#include <curl/curl.h>

#ifdef _WIN32	// We could also use __MINGW32__
    #include <fcntl.h>
#endif

char gateway_id[6] = "CHANGE";
#define BUFSIZE 5
bool new_id = false;

// Size of the character buffer to use for curl requests
#define CURLBUF_SIZE 250

// Ukhas protocol layer 1: https://www.ukhas.net/wiki/protocol_details
#define BIT_RATE 2000

// CRC computation start value
// Warning: there are many falvours of CRC-CCITT: XModem, 0xFFFF, 0x1D0F, Kermit
// 0x1D0F seems to be the one uses by the RFM69 radios
#define CRC_START 0x1D0F

// maximum packet size, not including length byte nor CRC checksum
#define MAX_PACKET_SIZE 255

// preamble is not used now (only for debugging)
#define PREAMBLE 0xAAAA

// 2 bytes sync words: https://www.ukhas.net/wiki/protocol_details
// TODO: add sync tolerance (maybe with preamble to be more robust)
#define SYNC_WORD 0x2DAA
uint16_t syncBuffer;

// Output more details (for debug)
bool verbose = false;

// Output only decoded packets?
bool quiet = false;

// Sink data to the net?
bool api = false;

// Default value assumes a 64kHz sampling rate
uint16_t sampleRate = 64000;

// byte buffers used when sync word has been recognized
uint8_t byte;
uint8_t buffer[MAX_PACKET_SIZE];

// frequency shift threshold automatically computed with a sample moving averge over a 8 bit window
int16_t threshold = 0;

// indicates that we are currently synchronized with bit stream for packet decoding
bool packetSync = false;

/////////////// PACKET parsing //////////////
// TODO refactor in another class

// packet length (without length byte)
int len = -1;

// packet offset
int offset = 0;

// packet crc16 computed on the fly
uint16_t computedCrc = 0x1D0F;

// packet crc read
uint16_t readCrc = 0;

// Use libcurl to submit data to server
CURL *curl;
CURLcode res;
// write http responses to /dev/null for now
FILE *devnull;

// see http://www.atmel.com/webdoc/AVRLibcReferenceManual/group__util__crc_1gaca726c22a1900f9bad52594c8846115f.html
uint16_t crc_xmodem_update (uint16_t crc, uint8_t data) {
    crc = crc ^ ((uint16_t) data << 8);
    for (int i=0; i<8; i++) {
        if (crc & 0x8000) {
            crc = (crc << 1) ^ 0x1021;
        } else {
            crc <<= 1;
        }
    }
    return crc;
}

// print time
void printTime() {
    char buff[100];
    time_t now = time(0);
    strftime(buff, 100, "%Y-%m-%d %H:%M:%S", localtime(&now));
    printf("%s ", buff);
}

// process one byte
bool processByte(uint8_t byte) {
    // Buffer for the post request
    char curlbuf[CURLBUF_SIZE];
    if (len == -1) {
        // read length (does not account for length byte)
        len = (int) byte;
        computedCrc = crc_xmodem_update(computedCrc, byte);
        if (len <= MAX_PACKET_SIZE - 1) {
            if (verbose) {
                printTime();
                printf("Parsing %d bytes\n", len);
            }
            // continue reading packet
            return true;
        } else {
            if (verbose) {
                printTime();
                printf("Length: %d > %d, skip\n", len, MAX_PACKET_SIZE - 1);
            }
            // stop reading packet
            return false;
        }
    } else if (offset < len) {
        // read data
        buffer[offset++] = byte;
        computedCrc = crc_xmodem_update(computedCrc, byte);
        // continue reading packet
        return true;
    } else if (offset == len) {
        // null terminate the packet buffer
        buffer[len] = '\0';
        // read crc first byte
        readCrc |= ((uint16_t) byte << 8);
        offset++;
        return true;
    } else if (offset == len + 1) {
        // read crc 2nd byte
        readCrc |= byte;
        // TODO
        // I don't know why I need to invert this CRC to work
        // bits could be all inverted but not just CRC...
        // This seems to be the RFM which has a strange CRC implementation
        readCrc = 0xffff - readCrc;
        if (computedCrc == readCrc) {
            if (!quiet) {
                printTime();
                printf("PACKET ");
            }
            for (int i=0; i<len; ++i) {
                putchar(buffer[i]);
            }
            printf("\n");
            fflush(stdout);
            // Curl
            if(curl && api)
            {
                snprintf(curlbuf, CURLBUF_SIZE, "origin=%s&data=%s", 
                        gateway_id, buffer);
                curl_easy_setopt(curl, CURLOPT_URL, 
                        "http://www.ukhas.net/api/upload");
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
                       curlbuf);
                if (api) {
	                res = curl_easy_perform(curl);
 	               if(res != CURLE_OK)
        	            fprintf(stderr, "CURL request failed (%s)\n", curl_easy_strerror(res));
                	    //printf("CURL request failed, aborting...\n");
			}

                FILE *fp;
                fp = fopen("output.txt", "a+");
                char buff[100];
                time_t now = time(0);
                strftime(buff, 100, "%Y-%m-%d %H:%M:%S", localtime(&now));

                fprintf(fp, "%s,%s,%s\n", buff, buffer, gateway_id);
                fclose(fp);

                FILE *fp1;
                fp1 = fopen("latest.txt", "w");
                fprintf(fp1, "%s\n", buffer);
                fclose(fp1);


            }
        } else if (verbose) {
            printTime();
            printf("CRC mismatch: read(%04X), computed(%04X)\n", readCrc, computedCrc);
        }
    }
    return false;
}

/////////////// END of PACKET parsing //////////////

int skipBit = 8;

// process one bit
void processBit(bool bit) {
    if (packetSync) {
        // process bits by 8 (byte)
        byte = (byte << 1) | bit;
        if (--skipBit < 1) {
            // reset
            skipBit = 8;
            // process new byte
            packetSync = processByte(byte);
        }
    } else {
        syncBuffer = (syncBuffer << 1) | bit;
        packetSync = (syncBuffer == SYNC_WORD || syncBuffer == 0xffff - SYNC_WORD);
        if (packetSync) {
            // TODO
            // if sync with inverted SYNC_WORD, should invert all bits
            if (verbose) {
                printTime();
                printf("Sync: %04X\n", syncBuffer);
            }
            // start reading bits 8 by 8 (as bytes)
            skipBit = 8;
            // initialize packet
            len = -1;
            offset = 0;
            computedCrc = 0x1D0F;
            readCrc = 0;
        }
    }
}

int main (int argc, char**argv){

    // parse options
    int opt;
    while ((opt = getopt(argc, argv, "g:hvqws:")) != -1) {
        switch (opt) {
            case 's':
                sampleRate = atoi(optarg);
                if (sampleRate < 2 * BIT_RATE || sampleRate % BIT_RATE != 0) {
                    fprintf(stderr, "Illegal sampling rate - %d.\n", sampleRate);
                    fprintf(stderr, "Must be over %d Hz and a multiple of %d Hz.\n", 2*BIT_RATE, BIT_RATE);
                    return 1;
                }
                break;
            case 'v':
                quiet = false;
                verbose = true;
                break;

            case 'g':

                new_id = true;
                snprintf( gateway_id, BUFSIZE, "%s", optarg );
                printf("Gateway ID: %s\n", gateway_id);
                break;

            case 'q':
                verbose = false;
                quiet = true;
                break;
            case 'w':
                api = true;
                break;
            case 'h':
            default:
                printf("Usage: UKHASnet-decoder [-s sample_rate][-v][-h] \n"
                        "Expects rtl_fm output:\n"
                        "\trtl_fm -f 433961890 -s 64k -g 0 -p 162 | ./UKHASnet-decoder -v -s 64000\n"
                        "\trtl_fm -f 433961890 -s 64k -g 0 -p 162 -r 8000 | ./UKHASnet-decoder -v -s 8000\n"
                        "\t[-s sample_rate in Hz. Above 4kHz and a multiple of 2kHz.]\n"
                        "\t[-w submit data to ukhas.net api]\n"
                        "\t[-v verbose mode] (negates prev. quiet)]\n"
                        "\t[-q quiet mode] (negates prev. verbose)]\n"
                        "\t[-g set your gateway ID, max 6 characters]\n"
                        "\t[-h this help. Check the code if more needed !]\n\n");
                return 0;
                break;
        }
    }
    if (!quiet) {
        printf("UKHAS decoder using rtl_fm\n");
        printf("Sample rate: %d Hz\n", sampleRate);
        if (verbose) {
            printf("Verbose mode\n");
        }
        printf("\n");
    }

    if (!new_id) {
        printf("\n********\nPlease change your gateway ID with -g\n********\n\n");
        return 0;
    }
    // process samples
    int32_t samples = 0;
    time_t start_time = time(NULL);
    // sample rate / BIT_RATE
    int downSamples = sampleRate / BIT_RATE;
    int skipSamples = downSamples;

    // Configure curl if required
    if(api)
    {
        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();
        // send all output to /dev/null
#       ifdef __linux__
            devnull = fopen("/dev/null", "w+");
#       elif _WIN32
            devnull = fopen("nul", "w+");
#       endif
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, devnull);
    }

#ifdef _WIN32	// We could also use __MINGW32__
    // Set stdin to Binary Mode
    setmode(fileno(stdin), O_BINARY);
#endif

    // Read incoming data from dongle
    while(!feof(stdin) ) {
        // process 1 sample
        int16_t sample = (int16_t) (fgetc(stdin) | fgetc(stdin) << 8);
        // threshold is a moving average over 8 bits
        // this suppose that packet has sufficient bit transitions
        // TODO find better formula to account for not well balanced packets
        // use all samples and not only subsampled bits
        threshold = (sample + (8 * downSamples - 1) * threshold) / (8 * downSamples);
        // only process 1 bit over g_srate
        if (--skipSamples < 1) {
            // reset
            skipSamples = downSamples;
            // process bit
            bool bit = sample > threshold;
            processBit(bit);
            // to debug more deeply uncomment bellow and comment all other outputs,
            // pipe to a file and plot !
            //printf("%d,%d,%d,%d,%d\n", samples++, sample, threshold, bit ? 6000 : -6000, packetSync ? 6000 : 0);
            //fflush(stdout);
        }
    }
    if (!quiet) {
        printf("%d samples in %d sec\n", samples, (int) (time(NULL)-start_time));
    }

    // Clean up curl
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return 0;
}
