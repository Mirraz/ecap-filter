#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "filter.h"
#include "cdebug.h"
#include "uri_parser.h"
#include "map.h"

struct filter_struct_ {
	sqlite3 *db;
	sqlite3_stmt *select_categories_stmt;
	map_struct *map;
};

static void print_err(const char *msg) {
	cdebug_printf(CDEBUG_IL_CRITICAL, "%s", msg);
}

static void print_sqlite3_err(const char *func, int errcode) {
	cdebug_printf(CDEBUG_IL_CRITICAL, "sqlite3_%s: %s\n", func, sqlite3_errstr(errcode));
}

static void print_sqlite3_sql_err(const char *func, const char *sql, int errcode) {
	cdebug_printf(CDEBUG_IL_CRITICAL, "sqlite3_%s sql '%s': %s\n", func, sql, sqlite3_errstr(errcode));
}

static void print_select_categories_stmt_err(const char *func, int errcode) {
	cdebug_printf(CDEBUG_IL_CRITICAL, "sqlite3_%s select_categories_stmt: %s\n", func, sqlite3_errstr(errcode));
}

filter_struct *filter_construct(const char *db_uri) {
	filter_struct *filter = malloc(sizeof(filter_struct));
	if (filter == NULL) {print_err("malloc"); goto err_return;}

	filter->map = map_construct();
	if (filter->map == NULL) {print_err("map_construct"); goto err_filter_free;}

	int res;
	res = sqlite3_open_v2(
		db_uri,
		&filter->db,
		SQLITE_OPEN_URI | SQLITE_OPEN_READONLY,
		NULL
	);
	if (res != SQLITE_OK) {print_sqlite3_err("open_v2", res); goto err_map_destruct;}

	// select rules
	{
		const char *sql = "SELECT category_id, allowed FROM rules";
		sqlite3_stmt *stmt;
		res = sqlite3_prepare_v2(
			filter->db,
			sql, strlen(sql),
			&stmt,
			NULL
		);
		if (res != SQLITE_OK) {print_sqlite3_sql_err("prepare_v2", sql, res); goto err_sqlite3_close;}

		while ((res = sqlite3_step(stmt)) == SQLITE_ROW) {
			int category_id = sqlite3_column_int(stmt, 0);
			int allowed     = sqlite3_column_int(stmt, 1);
			map_put_uniq(filter->map, (map_key_type)category_id, (map_value_type)allowed);
		}
		if (res != SQLITE_DONE) {print_sqlite3_sql_err("step", sql, res); goto err_sqlite3_close;}
		res = sqlite3_finalize(stmt);
		if (res!= SQLITE_OK) {print_sqlite3_sql_err("finalize", sql, res); goto err_sqlite3_close;}
	}

	// prepare select_categories statement
	{
		const char *sql = "SELECT categories FROM sites WHERE domain = ?";
		res = sqlite3_prepare_v2(
			filter->db,
			sql, strlen(sql),
			&filter->select_categories_stmt,
			NULL
		);
		if (res != SQLITE_OK) {print_sqlite3_sql_err("prepare_v2", sql, res); goto err_sqlite3_close;}
	}

	return filter;

err_sqlite3_close:
	res = sqlite3_close(filter->db);
	if (res != SQLITE_OK) print_sqlite3_err("close", res);
err_map_destruct:
	map_destruct(filter->map);
err_filter_free:
	free(filter);
err_return:
	return NULL;

}

void filter_destruct(filter_struct *filter) {
	int res;
	res = sqlite3_finalize(filter->select_categories_stmt);
	if (res != SQLITE_OK) print_select_categories_stmt_err("finalize", res);
	res = sqlite3_close(filter->db);
	if (res != SQLITE_OK) print_sqlite3_err("close", res);
	map_destruct(filter->map);
	free(filter);
}

typedef map_key_type number_type;
#define NUMBER_TYPE_MAX MAP_KEY_TYPE_MAX

typedef enum {
	SPNR_SUCCESS,
	SPNR_INVALID,
	SPNR_OVERFLOW
} str_parse_number_result_enum;

static str_parse_number_result_enum str_parse_number(
		const char *str, const char **end_out, number_type *number_out
) {
	number_type number = 0;
	const char *cur = str;
	while (*cur >= '0' && *cur <= '9') {
		number_type digit = *cur - '0';
		if (!(number < NUMBER_TYPE_MAX / 10 && number * 10 < NUMBER_TYPE_MAX - digit)) {
			*end_out = cur;
			return SPNR_OVERFLOW;
		}
		number = number * 10 + digit;
		++cur;
	}
	if (cur == str) {*end_out = cur; return SPNR_INVALID;}
	*end_out = cur;
	*number_out = number;
	return SPNR_SUCCESS;
}

static filter_uri_result_enum filter_domain_is_allowed(
		const filter_struct *filter, const char *domain, size_t domain_size
) {
	int res;

	res = sqlite3_bind_text(
		filter->select_categories_stmt,
		1,
		domain, domain_size*sizeof(domain[0]),
		SQLITE_STATIC
	);
	if (res != SQLITE_OK) {
		print_select_categories_stmt_err("bind_text", res);
		return FILTER_URI_ERROR;
	}

	res = sqlite3_step(filter->select_categories_stmt);
	if (res != SQLITE_ROW && res != SQLITE_DONE) {
		print_select_categories_stmt_err("step(1)", res);
		return FILTER_URI_ERROR;
	}
	if (res == SQLITE_DONE) {
		return FILTER_URI_DOESNT_EXIST;
	}
	assert(res == SQLITE_ROW);

	const char *category_list = (const char *)sqlite3_column_text(filter->select_categories_stmt, 0);
	assert(category_list != NULL);
	const char *cur = category_list;
	filter_uri_result_enum filter_result = FILTER_URI_ALLOW;
	while (1) {
		map_key_type category;
		str_parse_number_result_enum spn_res = str_parse_number(cur, &cur, &category);
		if (spn_res != SPNR_SUCCESS || !(*cur == '\0' || *cur == ',')) {
			cdebug_printf(
				CDEBUG_IL_CRITICAL,
				"invalid category list '%s' for domain '%.*s'",
				category_list, domain_size, domain
			);
			filter_result = FILTER_URI_ERROR;
			break;
		}
		if (! map_exists(filter->map, category)) {
			cdebug_printf(
				CDEBUG_IL_CRITICAL,
				"unknown category '%u' in category list '%s' for domain '%.*s'",
				category, category_list, domain_size, domain
			);
			filter_result = FILTER_URI_ERROR;
			break;
		}
		map_value_type is_allowed = map_get(filter->map, category);
		if (! is_allowed) {filter_result = FILTER_URI_DENY; break;}
		if (*cur == '\0') break;
		assert(*cur == ',');
		++cur;
	}

	res = sqlite3_step(filter->select_categories_stmt);
	if (res != SQLITE_DONE) {
		print_select_categories_stmt_err("step(2)", res);
		return FILTER_URI_ERROR;
	}

	return filter_result;
}

filter_uri_result_enum filter_uri_is_allowed(
		const filter_struct *filter,
		const char *uri, int uri_is_authority
) {
	assert(filter != NULL);
	if (filter == NULL) return FILTER_URI_ERROR;
	const char *domain;
	size_t domain_size = (
		!uri_is_authority ?
		uri_extract_domain(uri, &domain) :
		authority_extract_domain(uri, &domain)
	);
	if (domain_size == 0) {
		cdebug_printf(
			CDEBUG_IL_CRITICAL,
			"extract_domain from uri '%s', uri_is_authority = %d",
			uri, uri_is_authority
		);
		return FILTER_URI_ERROR;
	}

	filter_uri_result_enum filter_result = filter_domain_is_allowed(filter, domain, domain_size);

	int res = sqlite3_reset(filter->select_categories_stmt);
	if (res != SQLITE_OK) {
		print_select_categories_stmt_err("reset", res);
		filter_result = FILTER_URI_ERROR;
	}
	return filter_result;
}
