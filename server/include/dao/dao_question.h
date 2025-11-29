// server/include/dao/dao_question.h
#ifndef DAO_QUESTION_H
#define DAO_QUESTION_H

#include <stdint.h>

typedef struct {
    int64_t question_id;
    char    difficulty[16];
    char    content[1024];
    char    op_a[512];
    char    op_b[512];
    char    op_c[512];
    char    op_d[512];
    char    correct_op[2];
} Question;

int dao_question_get_random(const char *difficulty, Question *out_q);

#endif
