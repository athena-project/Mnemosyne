#include "Xz.h"


namespace Athena{
    namespace Mnemosyne{

        Xz::Xz(){
            init();
        }

        Xz::~Xz(){
            lzma_end( ptr );
        }

        void Xz::init(){
            stream = LZMA_STREAM_INIT;
            ptr = &stream;
        }

        bool Xz::init_encoder(uint32_t preset){
            lzma_ret ret = lzma_easy_encoder(ptr, preset, LZMA_CHECK_SHA256);

            // Return successfully if the initialization went fine.
            if (ret == LZMA_OK)
                return true;

            const char *msg;
            switch (ret){
                case LZMA_MEM_ERROR:
                    msg = "Memory allocation failed";
                    break;

                case LZMA_OPTIONS_ERROR:
                    msg = "Specified preset is not supported";
                    break;

                case LZMA_UNSUPPORTED_CHECK:
                    msg = "Specified integrity check is not supported";
                    break;

                default:
                    msg = "Unknown error, possibly a bug";
                    break;
            }
            throw msg;
            return false;
        }

        bool Xz::compress(char* inpath, char* outpath){
            FILE* infile=fopen( inpath, "r");
            FILE* outfile=fopen( outpath, "w");

            lzma_action action = LZMA_RUN;

            uint8_t inbuf[ BUFSIZ ];
            uint8_t outbuf[ BUFSIZ ];

            ptr->next_in = NULL;
            ptr->avail_in = 0;
            ptr->next_out = outbuf;
            ptr->avail_out = sizeof(outbuf);

            // Loop until the file has been successfully compressed or until
            // an error occurs.
            while (true) {
                // Fill the input buffer if it is empty.
                if (ptr->avail_in == 0 && !feof(infile)) {
                    ptr->next_in = inbuf;
                    ptr->avail_in = fread(inbuf, 1, sizeof(inbuf), infile);

                    if (ferror(infile)) {
                        //fprintf(stderr, "Read error: %s\n", strerror(errno));
                        throw "";
                        return false;
                    }

                    if (feof(infile))
                        action = LZMA_FINISH;
                }

                lzma_ret ret = lzma_code(ptr, action);

                if (ptr->avail_out == 0 || ret == LZMA_STREAM_END) {
                    size_t write_size = sizeof(outbuf) - ptr->avail_out;

                    if( fwrite(outbuf, 1, write_size, outfile) != write_size ){
                        //fprintf(stderr, "Write error: %s\n", strerror(errno));
                        throw "";
                        return false;
                    }

                    // Reset next_out and avail_out.
                    ptr->next_out = outbuf;
                    ptr->avail_out = sizeof(outbuf);
                }


                if (ret != LZMA_OK) {
                    if (ret == LZMA_STREAM_END){
                        fclose(infile);
                        fclose(outfile);
                        return true;
                    }

                    const char *msg;
                    switch (ret){
                        case LZMA_MEM_ERROR:
                            msg = "Memory allocation failed";
                            break;

                        case LZMA_DATA_ERROR:
                            msg = "File size limits exceeded";
                            break;

                        default:
                            msg = "Unknown error, possibly a bug";
                            break;
                    }

                    fprintf(stderr, "Encoder error: %s (error code %u)\n", msg, ret);
                    throw"";
                    return false;
                }

                fclose(infile);
                fclose(outfile);
                return true;
            }
        }


        bool Xz::init_decoder(){
            lzma_ret ret = lzma_stream_decoder(ptr, UINT64_MAX, LZMA_CONCATENATED);

            // Return successfully if the initialization went fine.
            if (ret == LZMA_OK)
                return true;

            const char *msg;
            switch (ret){
                case LZMA_MEM_ERROR:
                    msg = "Memory allocation failed";
                    break;

                case LZMA_OPTIONS_ERROR:
                    msg = "Unsupported decompressor flags";
                    break;

                default:
                    msg = "Unknown error, possibly a bug";
                    break;
            }

            fprintf(stderr, "Error initializing the decoder: %s (error code %u)\n", msg, ret);
            throw "";
            return false;
        }

        bool Xz::decompress(const char *inname, FILE *infile, FILE *outfile){
            lzma_action action = LZMA_RUN;

            uint8_t inbuf[ BUFSIZ ];
            uint8_t outbuf[ BUFSIZ ];

            ptr->next_in = NULL;
            ptr->avail_in = 0;
            ptr->next_out = outbuf;
            ptr->avail_out = sizeof(outbuf);

            while (true) {
                if (ptr->avail_in == 0 && !feof(infile)) {
                    ptr->next_in = inbuf;
                    ptr->avail_in = fread(inbuf, 1, sizeof(inbuf),
                            infile);

                    if (ferror(infile)) {
//                        fprintf(stderr, "%s: Read error: %s\n", inname, strerror(errno));
                        throw"";
                        return false;
                    }

                    if (feof(infile))
                        action = LZMA_FINISH;
                }

                lzma_ret ret = lzma_code(ptr, action);

                if (ptr->avail_out == 0 || ret == LZMA_STREAM_END) {
                    size_t write_size = sizeof(outbuf) - ptr->avail_out;

                    if (fwrite(outbuf, 1, write_size, outfile)
                            != write_size) {
//                        fprintf(stderr, "Write error: %s\n", strerror(errno));
                        throw"";
                        return false;
                    }

                    ptr->next_out = outbuf;
                    ptr->avail_out = sizeof(outbuf);
                }

                if (ret != LZMA_OK) {
                    if (ret == LZMA_STREAM_END)
                        return true;

                    const char *msg;
                    switch (ret){
                        case LZMA_MEM_ERROR:
                            msg = "Memory allocation failed";
                            break;

                        case LZMA_FORMAT_ERROR:
                            msg = "The input is not in the .xz format";
                            break;

                        case LZMA_OPTIONS_ERROR:
                            msg = "Unsupported compression options";
                            break;

                        case LZMA_DATA_ERROR:
                            msg = "Compressed file is corrupt";
                            break;

                        case LZMA_BUF_ERROR:
                            msg = "Compressed file is truncated or otherwise corrupt";
                            break;

                        default:
                            msg = "Unknown error, possibly a bug";
                            break;
                    }

                    fprintf(stderr, "%s: Decoder error: %s (error code %u)\n", inname, msg, ret);
                    throw"";
                    return false;
                }
            }
        }


    }
}
