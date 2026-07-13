#ifndef MOCK_SEQ_H_
#define MOCK_SEQ_H_

/* Shared monotonic event counter so GPIO and SPI mock logs can be merged
 * and ordering asserted across the two (e.g. "DC high + CS low happen
 * before tx_dma"). */
int mock_next_seq(void);
void mock_seq_reset(void);

#endif
