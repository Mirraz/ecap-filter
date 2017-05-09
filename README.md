# ecap-filter
Squid ecap adapter for URL filtering.  
It filters URLs domains according to sqlite database of domains categories.

## Building

### Requirements
* [libsqlite](https://sqlite.org/index.html)
* [libecap](http://www.e-cap.org/Home)

### Compilation
Install `libsqlite` and `libecap` headers and libraries.

For example, on Ubuntu install packages:
```
libsqlite3
libsqlite3-dev
libecap3
libecap3-dev
```

Then compile using command `make`.

## Installation
Put `ecap_adapter_filter.so` into `/usr/local/lib/`

## Squid configuration
```
loadable_modules /usr/local/lib/ecap_adapter_filter.so
ecap_enable on
ecap_service ecapFilter reqmod_precache \
    uri=ecap://e-cap.org/ecap/services/sample/minimal \
    db_uri=/tmp/db.sqlite \
    default_policy=allow
adaptation_access ecapFilter allow all
```
Parameters:
* `db_uri` -- sqlite database uri
* `default_policy` -- what to do if domain is not in db (possible values: `allow` or `deny`)

## Database
Sqlite database schema:
```
CREATE TABLE "sites" (
	"domain" TEXT PRIMARY KEY NOT NULL,
	"categories" TEXT NOT NULL
);
CREATE TABLE "rules" (
	"category_id" INTEGER PRIMARY KEY NOT NULL,
	"allowed" INTEGER NOT NULL
);
```
`categories` -- text list of categories separated by commas  
If any category of domain is not allowed then domain is not allowed.

Example:
```
TABLE "sites"
+-------------+------------+
| domain      | categories |
+-------------+------------+
| example.com | 1,2        |
+-------------+------------+

TABLE "rules"
+-------------+---------+
| category_id | allowed |
+-------------+---------+
| 1           | 1       |
+-------------+---------+
| 2           | 0       |
+-------------+---------+
```

## Test database
To generate random test database use `make_test_db`.  
It generates sqlite database with
128 categories (1..128) with random rules and
1048576 random domains with random assigned categories (about 125MiB).

### Compilation
Use command `make make_test_db`

### Usage
```
make_test_db <db_uri> [seed]
```
* `db_uri` -- sqlite database uri
* `seed` -- random seed
