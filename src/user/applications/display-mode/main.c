#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pedigree_config.h>

void usage()
{
    printf("display-mode: List and select the Pedigree display modes.\n");
    printf("usage:\tdisplay-mode list\n");
    printf("\tdisplay-mode select <width>x<height>x<depth>\n");
}

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        usage();
        exit(1);
    }

    char buf[256];
    int result, i;

    if(argc >= 2)
    {
        if(!strcmp(argv[1], "list"))
        {
            result = pedigree_config_query("select * from 'display-modes/0'");
            if (result == -1)
            {
                fprintf(stderr, "Unable to query.\n");
            }

            if (pedigree_config_was_successful(result) != 0)
            {
                pedigree_config_get_error_message(result, buf, 256);
                fprintf(stderr, "error: %s\n", buf);
                return 1;
            }

            if (pedigree_config_numrows(result) == 0)
            {
                // Empty set.
                printf("No modes\n");
                return 0;
            }

            while (pedigree_config_nextrow(result) == 0)
            {
                printf("Mode ");
                memset(buf, 0, 256);
                pedigree_config_getstr_s(result, "width", buf, 256);
                printf("%sx", buf);
                memset(buf, 0, 256);
                pedigree_config_getstr_s(result, "height", buf, 256);
                printf("%sx", buf);
                memset(buf, 0, 256);
                pedigree_config_getstr_s(result, "depth", buf, 256);
                printf("%s", buf);
                printf("\n");
            }
            pedigree_config_freeresult(result);

        }
        else
        {
            usage();
            exit(1);
        }
    }

    exit(0);
}
