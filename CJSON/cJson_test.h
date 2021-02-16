#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "includes.h"

/* Used by some code below as an example datatype. */
struct record
{
    const char *precision;
    double lat;
    double lon;
    const char *address;
    const char *city;
    const char *state;
    const char *zip;
    const char *country;
};

int print_preallocated(cJSON *root);
void create_objects(void);

