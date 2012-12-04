#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include "libsass/sass_interface.h"

#define BUFSIZE 512

int output(int error_status, char* error_message, char* output_string) {
    if (error_status) {
        if (error_message) {
            fprintf(stderr, "%s", error_message);
        } else {
            fprintf(stderr, "An error occured; no error message available.\n");
        }
        return 1;
    } else if (output_string) {
        printf("%s", output_string);
        return 0;
    } else {
        fprintf(stderr, "Unknown internal error.\n");
        return 2;
    }
}

int compile_stdin(struct sass_options options) {
    int ret;
    struct sass_context* ctx;
    char buffer[BUFSIZE];
    size_t size = 1;
    char *source_string = malloc(sizeof(char) * BUFSIZE);

    if(source_string == NULL) {
        perror("Allocation failed");
        exit(1);
    }

    source_string[0] = '\0';

    while(fgets(buffer, BUFSIZE, stdin)) {
        char *old = source_string;
        size += strlen(buffer);
        source_string = realloc(source_string, size);
        if(source_string == NULL) {
            perror("Reallocation failed");
            free(old);
            exit(2);
        }
        strcat(source_string, buffer);
    }

    if(ferror(stdin)) {
        free(source_string);
        perror("Error reading standard input");
        exit(2);
    }

    ctx = sass_new_context();
    ctx->options = options;
    ctx->source_string = source_string;
    sass_compile(ctx);
    ret = output(ctx->error_status, ctx->error_message, ctx->output_string);

    sass_free_context(ctx);
    free(source_string);
    return ret;
}

int compile_file(struct sass_options options, char* input_path) {
    int ret;
    struct sass_file_context* ctx = sass_new_file_context();

    ctx->options = options;
    ctx->input_path = input_path;

    sass_compile_file(ctx);
    ret = output(ctx->error_status, ctx->error_message, ctx->output_string);

    sass_free_file_context(ctx);
    return ret;
}

struct
{
    char* style_string;
    int output_style;
} style_option_strings[] = {
    { "compressed", SASS_STYLE_COMPRESSED },
    { "compact", SASS_STYLE_COMPACT },
    { "expanded", SASS_STYLE_EXPANDED },
    { "nested", SASS_STYLE_NESTED }
};

#define NUM_STYLE_OPTION_STRINGS \
    sizeof(style_option_strings) / sizeof(style_option_strings[0])

int main(int argc, char** argv) {
    int read_from_stdin = 0;
    struct sass_options options;
    options.output_style = SASS_STYLE_NESTED;
    options.source_comments = 0;
    options.image_path = "images";
    options.include_paths = "";

    int c, i;
    while ((c = getopt(argc, argv, "lst:I:")) != -1) {
        switch (c) {
        case 'I':
            options.include_paths = optarg;
            break;
        case 't':
            for(i = 0; i < NUM_STYLE_OPTION_STRINGS; ++i) {
                if(strcmp(optarg, style_option_strings[i].style_string) == 0) {
                    options.output_style = style_option_strings[i].output_style;
                    break;
                }
            }
            if(i == NUM_STYLE_OPTION_STRINGS) {
                fprintf(stderr, "Invalid argument for -t flag: '%s'. Allowed arguments are:", optarg);
                for(i = 0; i < NUM_STYLE_OPTION_STRINGS; ++i) {
                    fprintf(stderr, " %s", style_option_strings[i].style_string);
                }
                fprintf(stderr, "\n");
                return 1;
            }
            break;
        case 'l':
            options.source_comments = 1;
            break;
        case 's':
            read_from_stdin = 1;
            break;
        case '?':
            /* Unrecognized flag or missing an expected value */
            /* getopt should produce it's own error message for this case */
            return 1;
        default:
            fprintf(stderr, "Unknown error while processing arguments\n");
            return 2;
        }
    }

    if (optind < argc && !read_from_stdin) {
        return compile_file(options, argv[optind]);
    } else {
        return compile_stdin(options);
    }
}
