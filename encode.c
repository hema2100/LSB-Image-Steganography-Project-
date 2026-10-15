#include <stdio.h>
#include<string.h>
#include "encode.h"
#include "types.h"
#include "common.h"

/* Function Definitions */

/* Get image size
 * Input: Image file ptr
 * Output: width * height * bytes per pixel (3 in our case)
 * Description: In BMP Image, width is stored in offset 18,
 * and height after that. size is 4 bytes
 */
OperationType check_operation_type(char *argv[])
{
    if(strcmp(argv[1], "-e") == 0)
    {
        return e_encode;
    }
    else if(strcmp(argv[1],"-d") == 0)
    {
        return e_decode;
    }
    else
    {
        return e_unsupported;
    }
}

Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    if(strstr(argv[2], ".bmp") == NULL)
    {
        return e_failure;
    }
    else
    {
        encInfo->src_image_fname = argv[2];
    }

    encInfo->secret_fname = argv[3];

    char *dot = strrchr(argv[3], '.');

if(dot == NULL)
{
    return e_failure;
}

memset(encInfo->extn_secret_file, 0, MAX_FILE_SUFFIX);

if(strlen(dot) >= MAX_FILE_SUFFIX)
{
    return e_failure;
}

strcpy(encInfo->extn_secret_file, dot);

    if(argv[4] != NULL)
    {
        if(strstr(argv[4], ".bmp") != NULL)
        {
            encInfo->stego_image_fname = argv[4];
        }
        else
        {
            return e_failure;
        }
    }
    else
    {
        encInfo->stego_image_fname = "steged_img.bmp";
    }

    return e_success;
}
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;
    // Seek to 18th byte
    fseek(fptr_image, 18, SEEK_SET);

    // Read the width (an int)
    fread(&width, sizeof(int), 1, fptr_image);
    printf("width = %u\n", width);

    // Read the height (an int)
    fread(&height, sizeof(int), 1, fptr_image);
    printf("height = %u\n", height);

    // Return image capacity
    return width * height * 3;
}
uint get_file_size(FILE *fptr)
{
    uint size;
    fseek(fptr,0,SEEK_END);
    size=ftell(fptr);
    rewind(fptr);
    return size;
}


/* 
 * Get File pointers for i/p and o/p files
 * Inputs: Src Image file, Secret file and
 * Stego Image file
 * Output: FILE pointer for above files
 * Return Value: e_success or e_failure, on file errors
 */
Status open_files(EncodeInfo *encInfo)
{
    // Source image
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "rb");

    if(encInfo->fptr_src_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n",
                encInfo->src_image_fname);

        return e_failure;
    }

    // Secret file
    encInfo->fptr_secret = fopen(encInfo->secret_fname, "rb");

    if(encInfo->fptr_secret == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n",
                encInfo->secret_fname);

        fclose(encInfo->fptr_src_image);
        return e_failure;
    }

    // Stego image
    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "wb");

    if(encInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n",
                encInfo->stego_image_fname);

        fclose(encInfo->fptr_src_image);
        fclose(encInfo->fptr_secret);
        return e_failure;
    }

    return e_success;
}
Status check_capacity(EncodeInfo *encInfo)
{
    uint required_size;
    encInfo->image_capacity = get_image_size_for_bmp(encInfo->fptr_src_image);
    encInfo->size_secret_file=get_file_size(encInfo->fptr_secret);
    required_size=(strlen(MAGIC_STRING)*8)+(MAX_FILE_SUFFIX * 8)+(sizeof(int)*8)+(encInfo->size_secret_file *8);
    if(encInfo->image_capacity >= required_size)
    {
        return e_success;
    }
    else
    {
        return e_failure;
    }
}
Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image)
{
    char header[54];
    rewind(fptr_src_image);
    fread(header,54,1,fptr_src_image);
    fwrite(header,54,1,fptr_dest_image);
    return e_success;
}
Status encode_byte_to_lsb(char data, char *image_buffer)
{
    unsigned char byte = (unsigned char)data;
    for(int i=0;i<8;i++)
    {
        unsigned char bit = (byte >> (7 - i)) & 1;
        image_buffer[i] = (image_buffer[i] & 0xFE) | bit;
    }
    return e_success;
}
Status encode_size_to_lsb(int data, char *image_buffer)
{
    for(int i=0;i<32;i++)
    {
        int bit = (data >> (31 - i)) & 1;
        image_buffer[i]=(image_buffer[i] & 0xFE) | bit;
    }
    return e_success;
}
Status encode_data_to_image(char *data, int size, FILE *fptr_src_image, FILE *fptr_stego_image)
{
    char image_buffer[MAX_IMAGE_BUF_SIZE];
    for(int i=0;i<size;i++)
    {
        fread(image_buffer,1,MAX_IMAGE_BUF_SIZE,fptr_src_image);
        encode_byte_to_lsb(data[i],image_buffer);
        fwrite(image_buffer,1,MAX_IMAGE_BUF_SIZE,fptr_stego_image);
    }
    return e_success;
}
Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)
{
    return encode_data_to_image((char *)magic_string, strlen(magic_string), encInfo->fptr_src_image, encInfo->fptr_stego_image);
}
Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)
{
    return encode_data_to_image((char *)file_extn,
                                MAX_FILE_SUFFIX,
                                encInfo->fptr_src_image,
                                encInfo->fptr_stego_image);
}
Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
    int size = (int)file_size;
    char image_buffer[32];
    fread(image_buffer,1,32,encInfo->fptr_src_image);
    encode_size_to_lsb(size, image_buffer);
    fwrite(image_buffer,1,32,encInfo->fptr_stego_image);
    return e_success;
}
Status encode_secret_file_data(EncodeInfo *encInfo)
{
    rewind(encInfo->fptr_secret);
    while(fread(encInfo->secret_data, 1, MAX_SECRET_BUF_SIZE, encInfo->fptr_secret)>0)
    {
        if(encode_data_to_image(encInfo->secret_data, MAX_SECRET_BUF_SIZE, encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure)
        {
            return e_failure;
        }
    }
    return e_success;
}
Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest)
{
    char buffer[1024];
    int n;
    while((n=fread(buffer,1,sizeof(buffer), fptr_src)) > 0)
    {
        fwrite(buffer,1,n,fptr_dest);
    }
    return e_success;
}
Status do_encoding(EncodeInfo *encInfo)
{
    if(open_files(encInfo) == e_failure)
    {
        return e_failure;
    }
    if(check_capacity(encInfo) == e_failure)
{
    printf("ERROR: Insufficient image capacity\n");

    fclose(encInfo->fptr_src_image);
    fclose(encInfo->fptr_secret);
    fclose(encInfo->fptr_stego_image);

    return e_failure;
}
    if(copy_bmp_header(encInfo->fptr_src_image,encInfo->fptr_stego_image)==e_failure)
    {
        return e_failure;
    }
    if(encode_magic_string(MAGIC_STRING,encInfo)==e_failure)
    {
        return e_failure;
    }
    if(encode_secret_file_extn(encInfo->extn_secret_file,encInfo)==e_failure)
    {
        return e_failure;
    }
    if(encode_secret_file_size(encInfo->size_secret_file,encInfo)==e_failure)
    {
        return e_failure;
    }
    if(encode_secret_file_data(encInfo)==e_failure)
    {
        return e_failure;
    }
    if(copy_remaining_img_data(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure)
    {
        return e_failure;
    }
    fclose(encInfo->fptr_src_image);
    fclose(encInfo->fptr_secret);
    fclose(encInfo->fptr_stego_image);

    return e_success;
}