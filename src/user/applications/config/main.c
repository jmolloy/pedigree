#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pedigree_config.h>

void usage()
{
    printf("config: Query and update the Pedigree configuration manager.\n");
    printf("usage: config <sql>\n"); 
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        usage();
        exit(1);
    }

    char buf[256];

    const char *sql = argv[1];

    int result = pedigree_config_query(sql);
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
        printf("Ø\n");
        return 0;
    }


    // Print out the result set.
    int cols = pedigree_config_numcols(result);
    
    int *col_lens = malloc(sizeof(int)*cols);

    int i;
    for (i = 0; i < cols; i++)
    {
        pedigree_config_getcolname(result, i, buf, 256);
        printf(" %s |", buf);
        col_lens[i] = strlen(buf);
    }
    printf("\n");
    for (i = 0; i < cols; i++)
    {
        printf("-");
        int j;
        for (j = 0; j < col_lens[i]; j++)
            printf("-");
        printf("-+");
    }
    printf("\n");

    while (pedigree_config_nextrow(result) == 0)
    {
        for (i = 0; i < cols; i++)
        {
            pedigree_config_getstr_n(result, i, buf, 256);
            printf(" %s\t", buf);
            int j;
            for (j = strlen(buf); j < col_lens[i]; j++)
                printf(" ");
            printf(" |");

        }
        printf("\n");
    }

    pedigree_config_freeresult(result);

    exit(0);
}
