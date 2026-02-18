#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mo_parser.h"

int main(int argc, char* argv[])
{
    int exit = 0;
    
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <mo_file>\n", argv[0]);
        exit = 1;
    }
    else
    {
        /* 测试查找 */
        static const char* test_strings[] = {
            "Frequency",
            "Duty-cycle",
            "Title",
            "New screen",
            "Button",
            "Help",
            "Save",
            "Open",
            "Exit",
            "Frequency1",
            "1Frequency",
            "1Frequency1",
            "Welcome",
            "File",
            "Edit",
            "View",
            "Close",
            "About",
        };
    
        int arg_idx = 1;
        while(arg_idx < argc)
        {
            puts("\n================================================");
            puts(argv[arg_idx]);
            puts("================================================\n");
            
            mo_context_t* ctx = NULL;
            mo_error_t err = mo_context_create(argv[arg_idx], &ctx);
            if (err != MO_SUCCESS)
            {
                fprintf(stderr, "Failed to load MO file: %s\n", mo_error_string(err));
                exit = 1;
                break;
            }
            else
            {
                printf("Loaded MO file with %u strings\n", mo_get_string_count(ctx));
                printf("Search method is %s\n", mo_get_search_method(ctx));
                for (int i = 0; i < sizeof(test_strings)/sizeof(test_strings[0]); i++)
                {
                    const char* translated = mo_translate(ctx, test_strings[i]);
                    printf("'%s' -> '%s'\n", test_strings[i], translated);
                }
                /* 复数形式测试 */
                const char* plural = mo_translate_cp(ctx, NULL, "%d file", "%d files", 5);
                printf("Plural: 5 files -> '%s'\n", plural);

#ifdef MO_ENABLE_STATS
                mo_stats_t stats;
                if (mo_get_stats(ctx, &stats))
                {
                    printf("\nPerformance Statistics:\n");
                    printf("  Total lookups: %u\n", stats.total_lookups);
                    printf("  Cache hits: %u (%.1f%%)\n", 
                           stats.cache_hits, 
                           stats.total_lookups > 0 ? 
                           (float)stats.cache_hits / stats.total_lookups * 100.0f : 0.0f);
                    printf("  Cache misses: %u\n", stats.cache_misses);
                    #ifdef MO_SEARCH_METHOD_HASH
                    printf("  Hash collisions: %u\n", stats.hash_collisions);
                    #endif
                    #if defined(MO_SEARCH_METHOD_LINEAR) || defined(MO_SEARCH_METHOD_BINARY)
                    printf("  Comparisons: %u\n", stats.comparisons);
                    #endif
                }
#endif
                mo_context_free(ctx);
                
                ++arg_idx;
            }
        }
    }
    return 0;
}
