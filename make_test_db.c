#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <math.h>
#include <sqlite3.h>

#define MAX_CATEGS_NUMBER 512
#define MAX_CATEG_DEC_SIZE 3
#define MAX_CATEGS_OF_DOMAIN 32
#define MAX_CATEGS_STR_SIZE ((MAX_CATEG_DEC_SIZE+1)*MAX_CATEGS_OF_DOMAIN-1)

#define MIN_DOMAIN_LENGTH 4
#define MAX_DOMAIN_LENGTH 253
static const char domain_symbols[] = "abcdefghijklmnopqrstuvwxyz0123456789.";
#define DOMAIN_SYMBOLS_NUMBER (sizeof(domain_symbols)/sizeof(domain_symbols[0])-1)
#define DOMAIN_SYMBOLS_NUMBER_NO_DOT (DOMAIN_SYMBOLS_NUMBER-1)
#define DOMAIN_LENGTH_GAMMA_THETA 2.0
#define DOMAIN_LENGTH_GAMMA_K 11

typedef unsigned int categ_type;
#define CATEG_TYPE_MAX UINT_MAX
typedef unsigned long long int domains_number_type;
#define DOMAINS_NUMBER_TYPE_MAX ULLONG_MAX

void print_err_and_exit(const char *msg) {
	fprintf(stderr, "error: %s\n", msg);
	exit(EXIT_FAILURE);
}

void print_sqlite3_err(const char *func, int errcode) {
	fprintf(stderr, "error while '%s': %s\n", func, sqlite3_errstr(errcode));
}

void print_sqlite3_sql_err(const char *func, const char *sql, int errcode) {
	fprintf(stderr, "error while '%s' sql [%s]: %s\n", func, sql, sqlite3_errstr(errcode));
}

int sqlite3_do(sqlite3 *db, const char *sql) {
	int res = sqlite3_exec(db, sql, NULL, NULL, NULL);
	if (res == SQLITE_OK) return 0;
	print_sqlite3_sql_err("exec", sql, res);
	return 1;
}

int create_tables(sqlite3 *db) {
	if (sqlite3_do(db, "BEGIN TRANSACTION")) return 1;
	if (sqlite3_do(db, "DROP TABLE IF EXISTS sites")) return 1;
	if (sqlite3_do(
		db,
		"CREATE TABLE sites (\n"
		"	domain TEXT PRIMARY KEY NOT NULL,\n"
		"	categories TEXT NOT NULL\n"
		")"
	)) return 1;
	if (sqlite3_do(db, "DROP TABLE IF EXISTS rules")) return 1;
	if (sqlite3_do(
		db,
		"CREATE TABLE rules (\n"
		"	category_id INTEGER PRIMARY KEY NOT NULL,\n"
		"	allowed INTEGER NOT NULL\n"
		")"
	)) return 1;
	if (sqlite3_do(db, "COMMIT")) return 1;
	return 0;
}

int fill_rules(sqlite3 *db, categ_type categs_number) {
	assert(categs_number > 0 && categs_number < CATEG_TYPE_MAX);
	int res;
	if (sqlite3_do(db, "BEGIN TRANSACTION")) return 1;

	sqlite3_stmt *stmt;
	const char *sql = "INSERT INTO rules(category_id, allowed) VALUES (?, ?)";
	res = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	if (res != SQLITE_OK) {print_sqlite3_sql_err("prepare_v2", sql, res); return 1;}

	for (categ_type i=0; i<categs_number; ++i) {
		categ_type categ = i + 1;
		int allowed = rand() & 1;

		res = sqlite3_bind_int(stmt, 1, categ);
		if (res != SQLITE_OK) {print_sqlite3_sql_err("bind_int(1)", sql, res); return 1;}
		res = sqlite3_bind_int(stmt, 2, allowed);
		if (res != SQLITE_OK) {print_sqlite3_sql_err("bind_int(2)", sql, res); return 1;}
		res = sqlite3_step(stmt);
		if (res != SQLITE_DONE) {print_sqlite3_sql_err("step", sql, res); return 1;}
		res = sqlite3_reset(stmt);
		if (res != SQLITE_OK) {print_sqlite3_sql_err("reset", sql, res); return 1;}
	}

	res = sqlite3_finalize(stmt);
	if (res != SQLITE_OK) {print_sqlite3_sql_err("finalize", sql, res); return 1;}

	if (sqlite3_do(db, "COMMIT")) return 1;
	return 0;
}

unsigned int rand_gamma_distributed(double theta, unsigned int k) {
	assert(theta > 0);
	assert(k > 0);
	double value = 0;
	for (unsigned int i=0; i<k; ++i) value += log((double)RAND_MAX+1.0) - log((double)rand()+1.0);
	return lrint(value * theta);
}

size_t generate_random_domain(char domain[MAX_DOMAIN_LENGTH]) {
	size_t domain_length;
	do {
		domain_length = rand_gamma_distributed(DOMAIN_LENGTH_GAMMA_THETA, DOMAIN_LENGTH_GAMMA_K);
	} while (!(domain_length >= MIN_DOMAIN_LENGTH && domain_length <= MAX_DOMAIN_LENGTH));

	unsigned int dots_count = 0;
	for (size_t i=0; i<domain_length; ++i) {
		size_t symbol_idx = rand() % (
			i != 0 && i != domain_length-1 ?
			DOMAIN_SYMBOLS_NUMBER :
			DOMAIN_SYMBOLS_NUMBER_NO_DOT
		);
		domain[i] = domain_symbols[symbol_idx];
		if (domain[i] == '.') ++dots_count;
	}

	if (dots_count == 0) {
		size_t dot_idx;
		do {
			dot_idx = rand() % (domain_length-2) + 1;
		} while (domain[dot_idx-1] == '.' || domain[dot_idx+1] == '.');
		domain[dot_idx] = '.';
	}

	return domain_length;
}

size_t generate_random_categs(char categs_str[MAX_CATEGS_STR_SIZE+1], size_t all_categs_number) {
	static int categs[MAX_CATEGS_NUMBER];
	for (categ_type i=0; i<all_categs_number; ++i) categs[i] = 0;
	categ_type cur_categs_number = (rand() % MAX_CATEGS_OF_DOMAIN) + 1;
	for (categ_type i=0; i<cur_categs_number; ++i) {
		categ_type idx;
		do {
			idx = rand() % all_categs_number;
		} while (categs[idx]);
		categs[idx] = 1;
	}
	char *str = categs_str;
	categ_type i, j;
	for (i=0, j=0; i<all_categs_number; ++i) {
		if (categs[i]) {
			str += sprintf(str, "%u", i+1);
			++j;
			if (j < cur_categs_number) str += sprintf(str, ",");
		}
	}
	return str - categs_str;
}

int fill_sites(sqlite3 *db, categ_type categs_number, domains_number_type domains_number) {
	assert(domains_number > 0 && domains_number < DOMAINS_NUMBER_TYPE_MAX);
	assert(categs_number > 0 && categs_number <= MAX_CATEGS_NUMBER);
	int res;
	static char domain[MAX_DOMAIN_LENGTH];
	static char categs_list[MAX_CATEGS_STR_SIZE+1];

	sqlite3_stmt *stmt;
	const char *sql = "INSERT INTO sites(domain, categories) VALUES (?, ?)";
	res = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	if (res != SQLITE_OK) {print_sqlite3_sql_err("prepare_v2", sql, res); return 1;}

	if (sqlite3_do(db, "BEGIN TRANSACTION")) return 1;
	for (domains_number_type counter=0; counter<domains_number; ++counter) {
		int step_res;
		do {
			size_t domain_length = generate_random_domain(domain);
			size_t categs_list_length = generate_random_categs(categs_list, categs_number);

			res = sqlite3_bind_text(stmt, 1, domain, domain_length*sizeof(domain[0]), NULL);
			if (res != SQLITE_OK) {print_sqlite3_sql_err("bind_bind_text(1)", sql, res); return 1;}
			res = sqlite3_bind_text(stmt, 2, categs_list, categs_list_length*sizeof(categs_list[0]), NULL);
			if (res != SQLITE_OK) {print_sqlite3_sql_err("bind_bind_text(2)", sql, res); return 1;}
			step_res = sqlite3_step(stmt);
			if (!(
				step_res == SQLITE_DONE ||
				(
					step_res == SQLITE_CONSTRAINT &&
					sqlite3_extended_errcode(db) == SQLITE_CONSTRAINT_PRIMARYKEY
				)
			)) {
				return 1;
			}
			res = sqlite3_reset(stmt);
			if (!(res == SQLITE_OK || (step_res == SQLITE_CONSTRAINT && res == SQLITE_CONSTRAINT))) {
				print_sqlite3_sql_err("reset", sql, res);
				return 1;
			}
		} while(step_res == SQLITE_CONSTRAINT);
	}
	if (sqlite3_do(db, "COMMIT")) return 1;

	res = sqlite3_finalize(stmt);
	if (res != SQLITE_OK) {print_sqlite3_sql_err("finalize", sql, res); return 1;}

	return 0;
}

int fill_tables(sqlite3 *db, categ_type categs_number, domains_number_type domains_number) {
	if (fill_rules(db, categs_number)) return 1;
	if (fill_sites(db, categs_number, domains_number)) return 1;
	return 0;
}

void make_test_db(const char *db_uri, categ_type categs_number, domains_number_type domains_number) {
	sqlite3 *db;
	int res;

	res = sqlite3_open_v2(
		db_uri,
		&db,
		SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
		NULL
	);
	if (res != SQLITE_OK) {print_sqlite3_err("open_v2", res); goto err_exit;}

	if (create_tables(db)) goto err_sqlite3_close;
	if (fill_tables(db, categs_number, domains_number)) goto err_sqlite3_close;

err_sqlite3_close:
	res = sqlite3_close(db);
	if (res != SQLITE_OK) print_sqlite3_err("close", res);
err_exit:
	exit(EXIT_FAILURE);
}

int main (int argc, char *argv[]) {
	if (!(argc >= 2 && argc <= 3)) print_err_and_exit("wrong amount of arguments");
	unsigned int seed;
	if (argc == 2) {
		seed = time(NULL);
	} else {
		seed = atoi(argv[2]);
	}
	printf("seed = %u\n", seed);
	srand(seed);
	make_test_db(argv[1], 128, 1048576);
	return EXIT_SUCCESS;
}
