#include <stdio.h>
#include "encode.h"
#include "decode.h"
#include "types.h"

int main(int argc, char *argv[])
{
    EncodeInfo encInfo;
    DecodeInfo decInfo;

    if (argc < 3)
    {
        printf("ERROR: Insufficient arguments\n");
        return 1;
    }

    OperationType op_type = check_operation_type(argv);

    if (op_type == e_encode)
    {
        if (argc < 4)
        {
            printf("ERROR: Insufficient arguments for encoding\n");
            return 1;
        }

        if (read_and_validate_encode_args(argv, &encInfo) == e_success)
        {
            if (do_encoding(&encInfo) == e_success)
            {
                printf("Encoding successful\n");
            }
            else
            {
                printf("Encoding failed\n");
                return 1;
            }
        }
        else
        {
            printf("Read and validation failed\n");
            return 1;
        }
    }
    else if (op_type == e_decode)
    {
        if (read_and_validate_decode_args(argv, &decInfo) == e_success)
        {
            if (do_decoding(&decInfo) == e_failure)
            {
                printf("Decoding failed\n");
                return 1;
            }
        }
        else
        {
            printf("Read and validation failed\n");
            return 1;
        }
    }
    else
    {
        printf("Unsupported operation\n");
        return 1;
    }

    return 0;
}
