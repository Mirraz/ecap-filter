# ecap-filter
ecap adapter for URL filtering

## Requirements
* [libsqlite](https://sqlite.org/index.html)
* [libecap](http://www.e-cap.org/Home)

## Building
Install `libsqlite` and `libecap` headers and libraries.

For example, on Ubuntu install packages:
```
libsqlite3
libsqlite3-dev
libecap3
libecap3-dev
```

Then compile using command `make`.

## Install
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

## Test database
To generate random test database use `make_test_db`

### Compilation
Use command `make make_test_db`

### Usage
```
make_test_db <db_uri> [seed]
```
* `db_uri` -- sqlite database uri
* `seed` -- random seed
