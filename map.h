#ifndef MAP_H
#define MAP_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int map_key_type;
typedef bool map_value_type;

struct map_struct_;
typedef struct map_struct_ map_struct;

map_struct *map_construct();
void map_destruct(map_struct *map);
void map_put_uniq(map_struct *map, const map_key_type key, const map_value_type value);
bool map_exists(const map_struct *map, const map_key_type key);
map_value_type map_get(const map_struct *map, const map_key_type key);

#ifdef __cplusplus
}
#endif

#endif/*MAP_H*/
