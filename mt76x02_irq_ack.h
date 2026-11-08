/* SPDX-License-Identifier: BSD-3-Clause-Clear */
#ifndef __MT76X02_IRQ_ACK_H
#define __MT76X02_IRQ_ACK_H
/* Keep masked NAPI completion latches; clear other unhandled INTx sources. */
static inline u32 mt76x02_irq_ack_sources(u32 pending, u32 enabled)
{
	u32 deferred = MT_INT_RX_DONE_ALL | MT_INT_TX_DONE_ALL;
	return pending & (enabled | ~deferred);
}
#endif
