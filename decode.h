#ifndef DECODE_H
#define DECODE_H

#include <stdio.h>
#include "types.h"

#define MAX_SECRET_BUF_SIZE 1
#define MAX_FILE_SUFFIX 20

typedef struct _DecodeInfo
{
    char *stego_image_fname;
    FILE *fptr_stego_image;

    char secret_fname[100];
    FILE *fptr_secret;

    int output_fname_given;

    char extn_secret_file[MAX_FILE_SUFFIX];

    long size_secret_file;

} DecodeInfo;

OperationType check_operation_type(char *argv[]);

Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo);

Status do_decoding(DecodeInfo *decInfo);

Status open_decode_files(DecodeInfo *decInfo);

Status decode_byte_from_lsb(char *image_buffer, char *data);

Status decode_data_from_image(char *data, int size,
                              FILE *fptr_stego_image);

Status decode_magic_string(const char *magic_string,
                           DecodeInfo *decInfo);

Status decode_secret_file_extn(DecodeInfo *decInfo);

Status decode_secret_file_size(DecodeInfo *decInfo);

Status decode_secret_file_data(DecodeInfo *decInfo);

#endif