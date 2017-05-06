#include "uri_parser.h"

// authority_end -- after last char
static size_t authority_scanned_extract_domain(
		const char *authority, const char *authority_end,
		const char *last_at, const char *last_colon,
		const char **domain_out
	) {
	const char *domain = authority;
	const char *domain_end = authority_end;
	if (domain == domain_end) return 0;
	// cut userinfo
	if (last_at != NULL) domain = last_at + 1;
	if (domain == domain_end) return 0;
	// cut port
	if (last_colon != NULL) domain_end = last_colon;
	if (domain == domain_end) return 0;
	*domain_out = domain;
	return domain_end - domain;
}

size_t authority_extract_domain(const char *authority, const char **domain_out) {
	const char *cur = authority;
	const char *last_at = NULL;
	const char *last_colon = NULL;
	while (*cur != '\0') {
		if (*cur == '@') last_at = cur;
		if (*cur == ':') last_colon = cur;
		++cur;
	}
	return authority_scanned_extract_domain(authority, cur, last_at, last_colon, domain_out);
}

size_t uri_extract_domain(const char *uri, const char **domain_out) {
	const char *cur = uri;

	// scheme://
	while (*cur != '\0' && *cur != ':') ++cur;
	if (*cur == '\0') return 0;
	++cur;
	if (*cur != '/') return 0;
	++cur;
	if (*cur != '/') return 0;
	++cur;

	// authority
	const char *authority = cur;
	const char *last_at = NULL;
	const char *last_colon = NULL;
	while (*cur != '\0' && *cur != '/' && *cur != '?' && *cur != '#') {
		if (*cur == '@') last_at = cur;
		if (*cur == ':') last_colon = cur;
		++cur;
	}
	return authority_scanned_extract_domain(authority, cur, last_at, last_colon, domain_out);
}
