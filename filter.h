#ifndef FILTER_H
#define FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct {
} filter_struct;

void filter_construct(filter_struct *filter);
void filter_destruct(filter_struct *filter);
bool filter_uri_is_allowed(const filter_struct *filter, const char *uri);

#ifdef __cplusplus
}
#endif

#endif/*FILTER_H*/
