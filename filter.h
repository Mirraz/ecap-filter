#ifndef FILTER_H
#define FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

struct filter_struct_;
typedef struct filter_struct_ filter_struct;

typedef enum {
	FILTER_URI_ALLOW,
	FILTER_URI_DENY,
	FILTER_URI_DOESNT_EXIST,
	FILTER_URI_ERROR
} filter_uri_result_enum;

filter_struct *filter_construct(const char *db_uri);
void filter_destruct(filter_struct *filter);
filter_uri_result_enum filter_uri_is_allowed(const filter_struct *filter, const char *uri, int uri_is_authority);

#ifdef __cplusplus
}
#endif

#endif/*FILTER_H*/
