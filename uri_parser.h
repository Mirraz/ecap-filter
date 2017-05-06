#ifndef URI_PARSER_H
#define URI_PARSER_H

#include <stddef.h>

size_t authority_extract_domain(const char *authority, const char **domain_out);
size_t uri_extract_domain(const char *uri, const char **domain_out);

#endif/*URI_PARSER_H*/
