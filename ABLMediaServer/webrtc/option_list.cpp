#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "option_list.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <list>



void file_error(const std::string& filename) {
	std::cerr << "Error opening file: " << filename << std::endl;
	// Handle the file error as per your requirement
}

std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end = str.find(delimiter);
	while (end != std::string::npos) {
		tokens.push_back(str.substr(start, end - start));
		start = end + delimiter.length();
		end = str.find(delimiter, start);
	}
	tokens.push_back(str.substr(start, end));
	return tokens;
}

std::list* read_data_cfg(const std::string& filename) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		file_error(filename);
		return nullptr;
	}

	std::string line;
	int line_count = 0;
	std::list* options = new std::list();

	while (std::getline(file, line)) {
		++line_count;
		line = line.substr(0, line.find_first_of("#;"));  // Remove comments

		if (line.empty())
			continue;

		size_t pos = line.find('=');
		if (pos == std::string::npos) {
			std::cerr << "Config file error line " << line_count << ", could not parse: " << line << std::endl;
			continue;
		}

		std::string key = line.substr(0, pos);
		std::string value = line.substr(pos + 1);
		std::string stripped_key = key;
		std::string stripped_value = value;
		// Strip leading and trailing whitespaces
		stripped_key.erase(0, stripped_key.find_first_not_of(" \t"));
		stripped_key.erase(stripped_key.find_last_not_of(" \t") + 1);
		stripped_value.erase(0, stripped_value.find_first_not_of(" \t"));
		stripped_value.erase(stripped_value.find_last_not_of(" \t") + 1);

		// Add key-value pair to the options list
		// Here, you can define the logic to store the key-value pair in your list structure
	}

	file.close();
	return options;
}



int read_option(char *s, list *options)
{
    size_t i;
    size_t len = strlen(s);
    char *val = 0;
    for(i = 0; i < len; ++i){
        if(s[i] == '='){
            s[i] = '\0';
            val = s+i+1;
            break;
        }
    }
    if(i == len-1) return 0;
    char *key = s;
    option_insert(options, key, val);
    return 1;
}

void option_insert(list *l, char *key, char *val)
{
    kvp* p = (kvp*)xmalloc(sizeof(kvp));
    p->key = key;
    p->val = val;
    p->used = 0;
    list_insert(l, p);
}

void option_unused(list *l)
{
    node *n = l->front;
    while(n){
        kvp *p = (kvp *)n->val;
        if(!p->used){
            fprintf(stderr, "Unused field: '%s = %s'\n", p->key, p->val);
        }
        n = n->next;
    }
}

char *option_find(list *l, char *key)
{
    node *n = l->front;
    while(n){
        kvp *p = (kvp *)n->val;
        if(strcmp(p->key, key) == 0){
            p->used = 1;
            return p->val;
        }
        n = n->next;
    }
    return 0;
}
char *option_find_str(list *l, char *key, char *def)
{
    char *v = option_find(l, key);
    if(v) return v;
    if(def) fprintf(stderr, "%s: Using default '%s'\n", key, def);
    return def;
}

char *option_find_str_quiet(list *l, char *key, char *def)
{
    char *v = option_find(l, key);
    if (v) return v;
    return def;
}

int option_find_int(list *l, char *key, int def)
{
    char *v = option_find(l, key);
    if(v) return atoi(v);
    fprintf(stderr, "%s: Using default '%d'\n", key, def);
    return def;
}

int option_find_int_quiet(list *l, char *key, int def)
{
    char *v = option_find(l, key);
    if(v) return atoi(v);
    return def;
}

float option_find_float_quiet(list *l, char *key, float def)
{
    char *v = option_find(l, key);
    if(v) return atof(v);
    return def;
}

float option_find_float(list *l, char *key, float def)
{
    char *v = option_find(l, key);
    if(v) return atof(v);
    fprintf(stderr, "%s: Using default '%lf'\n", key, def);
    return def;
}
