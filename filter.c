#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "filter.h"
#include "map.h"

struct filter_struct_ {
	sqlite3 *db;
	sqlite3_stmt *select_categories_stmt;
	map_struct *map;
};

filter_struct *filter_construct(const char *filename) {
	filter_struct *filter = malloc(sizeof(filter_struct));
	if (filter == NULL) goto err_return;

	filter->map = map_construct();
	if (filter->map == NULL) goto err_filter_free;

	if (
		sqlite3_open_v2(
				filename,
				&filter->db,
				SQLITE_OPEN_URI | SQLITE_OPEN_READONLY,
				NULL
		) != SQLITE_OK
	) goto err_map_destruct;

	const char *select_rules_sql = "SELECT category_id, allowed FROM rules";
	sqlite3_stmt *select_rules_stmt;
	if (
		sqlite3_prepare_v2(
			filter->db, select_rules_sql, strlen(select_rules_sql), &select_rules_stmt, NULL
		) != SQLITE_OK
	) goto err_sqlite3_close;
	int res;
	while ((res = sqlite3_step(select_rules_stmt)) == SQLITE_ROW) {
		int category_id = sqlite3_column_int(select_rules_stmt, 0);
		int allowed     = sqlite3_column_int(select_rules_stmt, 1);
		map_put_uniq(filter->map, (map_key_type)category_id, (map_value_type)allowed);
	}
	if (res != SQLITE_DONE) goto err_sqlite3_close;
	if (sqlite3_finalize(select_rules_stmt) != SQLITE_OK) goto err_sqlite3_close;

	const char *select_categories_sql = "SELECT categories FROM sites WHERE domain = ?";
	if (
		sqlite3_prepare_v2(
			filter->db,
			select_categories_sql, strlen(select_categories_sql),
			&filter->select_categories_stmt,
			NULL
		) != SQLITE_OK
	) goto err_sqlite3_close;

	return filter;

err_sqlite3_close:
	sqlite3_close(filter->db); // TODO: result code
err_map_destruct:
	map_destruct(filter->map);
err_filter_free:
	free(filter);
err_return:
	return NULL;

}

void filter_destruct(filter_struct *filter) {
	sqlite3_finalize(filter->select_categories_stmt); // TODO: result code
	sqlite3_close(filter->db); // TODO: result code
	map_destruct(filter->map);
	free(filter);
}

static size_t extract_domain(const char *uri, const char **domain_out) {
	// scheme:[//[user[:password]@]host[:port]][/path][?query][#fragment]
	const char *cur = uri, *token = cur;
	// 'scheme:'
	while (*cur != '\0' && *cur != ':') ++cur;
	if (*cur == '\0') return 0;
	assert(*cur == ':');
	++cur;
	token = cur;
	// '//'
	if (*cur != '/') return 0;
	++cur;
	if (*cur != '/') return 0;
	++cur;
	token = cur;
	// authority part
	while (*cur != '\0' && *cur != ':' && *cur != '@' && *cur != '/') ++cur;
	if (*cur == '\0' || *cur == '/') {*domain_out = token; return cur-token;}
	if (*cur == ':') {
		++cur;
		const char *prev_token = token;
		token = cur;
		// 'password@' or 'port/' or 'port\0'
		while (*cur != '\0' && *cur != '@' && *cur != '/') ++cur;
		if (*cur == '\0' || *cur == '/') {*domain_out = prev_token; return token-prev_token-1;}
	}
	assert(*cur == '@');
	++cur;
	token = cur;
	// 'host:' or 'host/' or 'host\0'
	while (*cur != '\0' && *cur != ':' && *cur != '/') ++cur;
	*domain_out = token;
	return cur-token;
}

static filter_uri_result_enum categories_are_allowed(const char *categories_list, const map_struct *map) {
	const char *cur = categories_list;
	while (1) {
		if (!(*cur >= '0' && *cur <= '9')) return FILTER_URI_ERROR;
		map_key_type category = 0;
		while (*cur >= '0' && *cur <= '9') {
			if (!(category < MAP_KEY_TYPE_MAX / 10)) return FILTER_URI_ERROR;
			category *= 10;
			map_key_type num = *cur - '0';
			if (!(category < MAP_KEY_TYPE_MAX - num)) return FILTER_URI_ERROR;
			category += num;
			++cur;
		}
		if (!(*cur == '\0' || *cur == ',')) return FILTER_URI_ERROR;
		if (! map_exists(map, category)) return FILTER_URI_ERROR;
		map_value_type is_allowed = map_get(map, category);
		if (! is_allowed) return FILTER_URI_DENY;
		if (*cur == '\0') break;
		assert(*cur == ',');
		++cur;
	}
	return FILTER_URI_ALLOW;
}

filter_uri_result_enum filter_uri_is_allowed(const filter_struct *filter, const char *uri) {
	filter_uri_result_enum result = FILTER_URI_ERROR;
	if (filter == NULL) goto err_return;
	const char *domain;
	size_t domain_size = extract_domain(uri, &domain);
	if (domain_size == 0) goto err_return;

	if (
		sqlite3_bind_text(
			filter->select_categories_stmt, 1, domain, domain_size*sizeof(domain[0]), SQLITE_STATIC
		) != SQLITE_OK
	) goto err_reset;

	int step_res = sqlite3_step(filter->select_categories_stmt);
	if (step_res != SQLITE_ROW && step_res != SQLITE_DONE) goto err_reset;
	if (step_res == SQLITE_DONE) {result = FILTER_URI_DOESNT_EXIST; goto err_reset;}
	assert(step_res == SQLITE_ROW);

	const unsigned char *categories = sqlite3_column_text(filter->select_categories_stmt, 0);
	if (sqlite3_step(filter->select_categories_stmt) != SQLITE_DONE) goto err_reset;

	if (sqlite3_reset(filter->select_categories_stmt) != SQLITE_OK) goto err_return;
	return categories_are_allowed((const char *)categories, filter->map);

err_reset:
	sqlite3_reset(filter->select_categories_stmt); // TODO result code
err_return:
	return result;
}
