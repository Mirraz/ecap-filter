# ecap-filter
ecap module for URL filtering

## Compile
Use command `make`

## Install
Put `ecap_adapter_filter.so` into `/usr/local/lib/`

## Squid configuration
```
loadable_modules /usr/local/lib/ecap_adapter_filter.so
ecap_enable on
ecap_service ecapModifier respmod_precache \
    uri=ecap://e-cap.org/ecap/services/sample/minimal \
    db_uri=/tmp/db.sqlite \
    default_policy=allow
adaptation_access ecapModifier allow all
```

Parameters:
* `db_uri` -- sqlite database uri
* `default_policy` -- what to do if domain is not in db (possible values: `allow` or `deny`)
