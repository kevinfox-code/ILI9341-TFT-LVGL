#include "mock_seq.h"

static int s_seq = 0;

int mock_next_seq(void)
{
    return s_seq++;
}

void mock_seq_reset(void)
{
    s_seq = 0;
}
