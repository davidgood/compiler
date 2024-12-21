//
// Created by dgood on 12/19/24.
//

#ifndef COMPILER_UTILS_H
#define COMPILER_UTILS_H

void *_strdup(void *s);
void *_copy_object(void *obj);
void *_copy_symbol(void *obj);
char *get_err_msg(const char *s, ...);
int compare_object_hash_keys(const void *v1, const void *v2) ;
#endif //COMPILER_UTILS_H
