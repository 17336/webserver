//
// Created by 17336 on 2022/2/26.
//

#include "deal_error.h"
void errExit(const char *s) {
    perror(s);
    exit(1);
}