#include <stdio.h>
#include <string.h>
#include "decode.h"
#include "common.h"

Status decode_byte_from_lsb(char *image_buffer, char *data)
{
    unsigned char value = 0;

    for (int i = 0; i < 8; i++)
    {
        value = (value << 1) | (image_buffer[i] & 1);
    }

    *data = value;

    return e_success;
}

Status decode_data_from_image(char *data, int size,
                              FILE *fptr_stego_image)
{
    char image_buffer[8];

    for (int i = 0; i < size; i++)
    {
        if (fread(image_buffer, 1, 8, fptr_stego_image) != 8)
        {
            return e_failure;
        }

        if (decode_byte_from_lsb(image_buffer, &data[i]) == e_failure)
        {
            return e_failure;
        }
    }

    return e_success;
}

Status open_decode_files(DecodeInfo *decInfo)
{
    decInfo->fptr_stego_image =
        fopen(decInfo->stego_image_fname, "rb");

    if (decInfo->fptr_stego_image == NULL)
    {
        perror("fopen");

        printf("ERROR: Unable to open file %s\n",
               decInfo->stego_image_fname);

        return e_failure;
    }

    return e_success;
}

Status read_and_validate_decode_args(char *argv[],
                                     DecodeInfo *decInfo)
{
    if (strstr(argv[2], ".bmp") == NULL)
    {
        return e_failure;
    }

    decInfo->stego_image_fname = argv[2];

    if (argv[3] != NULL)
    {
        strcpy(decInfo->secret_fname, argv[3]);
        decInfo->output_fname_given = 1;
    }
    else
    {
        strcpy(decInfo->secret_fname, "decoded");
        decInfo->output_fname_given = 0;
    }

    return e_success;
}

Status decode_magic_string(const char *magic_string,
                           DecodeInfo *decInfo)
{
    char decoded_magic[100];
    int length = strlen(magic_string);

    if (decode_data_from_image(decoded_magic,
                               length,
                               decInfo->fptr_stego_image) == e_failure)
    {
        return e_failure;
    }

    decoded_magic[length] = '\0';

    if (strcmp(decoded_magic, magic_string) == 0)
    {
        return e_success;
    }

    return e_failure;
}

Status decode_secret_file_extn(DecodeInfo *decInfo)
{
    if (decode_data_from_image(decInfo->extn_secret_file,
                               MAX_FILE_SUFFIX,
                               decInfo->fptr_stego_image) == e_failure)
    {
        return e_failure;
    }

    decInfo->extn_secret_file[MAX_FILE_SUFFIX - 1] = '\0';

    return e_success;
}

Status decode_secret_file_size(DecodeInfo *decInfo)
{
    char image_buffer[32];
    unsigned int size = 0;

    if (fread(image_buffer, 1, 32,
              decInfo->fptr_stego_image) != 32)
    {
        return e_failure;
    }

    for (int i = 0; i < 32; i++)
    {
        size = (size << 1) | (image_buffer[i] & 1);
    }

    decInfo->size_secret_file = size;

    return e_success;
}

Status decode_secret_file_data(DecodeInfo *decInfo)
{
    char image_buffer[8];
    char data;

    for (long i = 0; i < decInfo->size_secret_file; i++)
    {
        if (fread(image_buffer, 1, 8,
                  decInfo->fptr_stego_image) != 8)
        {
            return e_failure;
        }

        if (decode_byte_from_lsb(image_buffer, &data) == e_failure)
        {
            return e_failure;
        }

        if (fwrite(&data, 1, 1, decInfo->fptr_secret) != 1)
        {
            return e_failure;
        }
    }

    return e_success;
}

Status do_decoding(DecodeInfo *decInfo)
{
    if (open_decode_files(decInfo) == e_failure)
    {
        return e_failure;
    }

    printf("Stego image opened successfully\n");

    if (fseek(decInfo->fptr_stego_image, 54, SEEK_SET) != 0)
    {
        printf("ERROR: Unable to skip BMP header\n");

        fclose(decInfo->fptr_stego_image);

        return e_failure;
    }

    printf("BMP 54 bytes header skipped successfully\n");

    if (decode_magic_string(MAGIC_STRING, decInfo) == e_failure)
    {
        printf("ERROR: Magic string validation failed\n");

        fclose(decInfo->fptr_stego_image);

        return e_failure;
    }

    printf("Magic string decoded successfully\n");

    if (decode_secret_file_extn(decInfo) == e_failure)
    {
        printf("ERROR: Secret file extension decoding failed\n");

        fclose(decInfo->fptr_stego_image);

        return e_failure;
    }

    printf("Secret file extension decoded successfully\n");
    printf("Secret file extension: %s\n",
           decInfo->extn_secret_file);

    char output_fname[120];

    if (decInfo->output_fname_given == 1)
    {
        strcpy(output_fname, decInfo->secret_fname);
    }
    else
    {
        strcpy(output_fname, decInfo->secret_fname);
        strcat(output_fname, decInfo->extn_secret_file);
    }

    decInfo->fptr_secret = fopen(output_fname, "wb");

    if (decInfo->fptr_secret == NULL)
    {
        perror("fopen");

        fclose(decInfo->fptr_stego_image);

        return e_failure;
    }

    if (decode_secret_file_size(decInfo) == e_failure)
    {
        printf("ERROR: Secret file size decoding failed\n");

        fclose(decInfo->fptr_stego_image);
        fclose(decInfo->fptr_secret);

        return e_failure;
    }

    printf("Secret file size decoded successfully\n");
    printf("Secret file size: %ld bytes\n",
           decInfo->size_secret_file);

    if (decode_secret_file_data(decInfo) == e_failure)
    {
        printf("ERROR: Secret file data decoding failed\n");

        fclose(decInfo->fptr_stego_image);
        fclose(decInfo->fptr_secret);

        return e_failure;
    }

    printf("Secret file data decoded successfully\n");

    fclose(decInfo->fptr_stego_image);
    fclose(decInfo->fptr_secret);

    printf("Decoding completed successfully\n");

    return e_success;
}