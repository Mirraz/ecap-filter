#include <map>
#include "map.h"

typedef std::map<map_key_type, map_value_type> map_type;

struct map_struct_ {
	map_type map;
};

map_struct *map_construct() {
	return new(std::nothrow) map_struct;
}

void map_destruct(map_struct *map) {
	delete[] map;

}

void map_put_uniq(map_struct *map, const map_key_type key, const map_value_type value) {
	map->map[key] = value;
}

bool map_exists(const map_struct *map, const map_key_type key) {
	return map->map.count(key) > 0;
}

map_value_type map_get(const map_struct *map, const map_key_type key) {
	return map->map.find(key)->second;
}
