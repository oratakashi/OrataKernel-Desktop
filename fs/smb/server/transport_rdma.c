// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *   Copyright (C) 2017, Microsoft Corporation.
 *   Copyright (C) 2018, LG Electronics.
 *
 *   Author(s): Long Li <longli@microsoft.com>,
 *		Hyunchul Lee <hyc.lee@gmail.com>
 */

#define SUBMOD_NAME	"smb_direct"

#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/string_choices.h>

#include "glob.h"
#include "connection.h"
#include "smb_common.h"
#include "../common/smb2status.h"
#include "transport_rdma.h"
#include "../smbdirect/public.h"


#define SMB_DIRECT_PORT_IWARP		5445
#define SMB_DIRECT_PORT_INFINIBAND	445

/* SMB_DIRECT negotiation timeout (for the server) in seconds */
#define SMB_DIRECT_NEGOTIATE_TIMEOUT		5

/* The timeout to wait for a keepalive message from peer in seconds */
#define SMB_DIRECT_KEEPALIVE_SEND_INTERVAL	120

/* The timeout to wait for a keepalive message from peer in seconds */
#define SMB_DIRECT_KEEPALIVE_RECV_TIMEOUT	5

/*
 * Default maximum number of RDMA read/write outstanding on this connection
 * This value is possibly decreased during QP creation on hardware limit
 */
#define SMB_DIRECT_CM_INITIATOR_DEPTH		8

/*
 * User configurable initial values per SMB_DIRECT transport connection
 * as defined in [MS-SMBD] 3.1.1.1
 * Those may change after a SMB_DIRECT negotiation
 */

/* The local peer's maximum number of credits to grant to the peer */
static int smb_direct_receive_credit_max = 255;

/* The remote peer's credit request of local peer */
static int smb_direct_send_credit_target = 255;

/* The maximum single message size can be sent to remote peer */
static int smb_direct_max_send_size = 1364;

/*
 * The maximum fragmented upper-layer payload receive size supported
 *
 * Assume max_payload_per_credit is
 * smb_direct_receive_credit_max - 24 = 1340
 *
 * The maximum number would be
 * smb_direct_receive_credit_max * max_payload_per_credit
 *
 *                       1340 * 255 = 341700 (0x536C4)
 *
 * The minimum value from the spec is 131072 (0x20000)
 *
 * For now we use the logic we used before:
 *                 (1364 * 255) / 2 = 173910 (0x2A756)
 */
static int smb_direct_max_fragmented_recv_size = (1364 * 255) / 2;

/*  The maximum single-message size which can be received */
static int smb_direct_max_receive_size = 1364;

static int smb_direct_max_read_write_size = SMBD_DEFAULT_IOSIZE;

static struct smb_direct_listener {
	int			port;

	struct task_struct	*thread;

	struct smbdirect_socket *socket;
} smb_direct_ib_listener, smb_direct_iw_listener;

struct smb_direct_transport {
	struct ksmbd_transport	transport;

	struct smbdirect_socket *socket;
};

static bool smb_direct_logging_needed(struct smbdirect_socket *sc,
				      void *private_ptr,
				      unsigned int lvl,
				      unsigned int cls)
{
	if (lvl <= SMBDIRECT_LOG_ERR)
		return true;

	if (lvl > SMBDIRECT_LOG_INFO)
		return false;

	switch (cls) {
	/*
	 * These were more or less also logged before
	 * the move to common code.
	 *
	 * SMBDIRECT_LOG_RDMA_MR was not used, but
	 * that's client only code and we should
	 * notice if it's used on the server...
	 */
	case SMBDIRECT_LOG_RDMA_EVENT:
	case SMBDIRECT_LOG_RDMA_SEND:
	case SMBDIRECT_LOG_RDMA_RECV:
	case SMBDIRECT_LOG_WRITE:
	case SMBDIRECT_LOG_READ:
	case SMBDIRECT_LOG_NEGOTIATE:
	case SMBDIRECT_LOG_OUTGOING:
	case SMBDIRECT_LOG_RDMA_RW:
	case SMBDIRECT_LOG_RDMA_MR:
		return true;
	/*
	 * These were not logged before the move
	 * to common code.
	 */
	case SMBDIRECT_LOG_KEEP_ALIVE:
	case SMBDIRECT_LOG_INCOMING:
		return false;
	}

	/*
	 * Log all unknown messages
	 */
	return true;
}

static void smb_direct_logging_vaprintf(struct smbdirect_socket *sc,
					const char *func,
					unsigned int line,
					void *private_ptr,
					unsigned int lvl,
					unsigned int cls,
					struct va_format *vaf)
{
	if (lvl <= SMBDIRECT_LOG_ERR)
		pr_err("%pV", vaf);
	else
		ksmbd_debug(RDMA, "%pV", vaf);
}

#define KSMBD_TRANS(t) (&(t)->transport)
#define SMBD_TRANS(t)	(container_of(t, \
				struct smb_direct_transport, transport))

static const struct ksmbd_transport_ops ksmbd_smb_direct_transport_ops;

void init_smbd_max_io_size(unsigned int sz)
{
	sz = clamp_val(sz, SMBD_MIN_IOSIZE, SMBD_MAX_IOSIZE);
	smb_direct_max_read_write_size = sz;
}

unsigned int get_smbd_max_read_write_size(struct ksmbd_transport *kt)
{
	struct smb_direct_transport *t;
	const struct smbdirect_socket_parameters *sp;

	if (kt->ops != &ksmbd_smb_direct_transport_ops)
		return 0;

	t = SMBD_TRANS(kt);
	sp = smbdirect_socket_get_current_parameters(t->socket);

	return sp->max_read_write_size;
}

<<<<<<< HEAD
static struct smb_direct_transport *alloc_transport(struct smbdirect_socket *sc)
||||||| 05f7e89ab9731
static inline int get_buf_page_count(void *buf, int size)
{
	return DIV_ROUND_UP((uintptr_t)buf + size, PAGE_SIZE) -
		(uintptr_t)buf / PAGE_SIZE;
}

static void smb_direct_destroy_pools(struct smbdirect_socket *sc);
static void smb_direct_post_recv_credits(struct work_struct *work);
static int smb_direct_post_send_data(struct smbdirect_socket *sc,
				     struct smbdirect_send_batch *send_ctx,
				     struct kvec *iov, int niov,
				     int remaining_data_length);

static inline void
*smbdirect_recv_io_payload(struct smbdirect_recv_io *recvmsg)
{
	return (void *)recvmsg->packet;
}

static struct
smbdirect_recv_io *get_free_recvmsg(struct smbdirect_socket *sc)
{
	struct smbdirect_recv_io *recvmsg = NULL;
	unsigned long flags;

	spin_lock_irqsave(&sc->recv_io.free.lock, flags);
	if (!list_empty(&sc->recv_io.free.list)) {
		recvmsg = list_first_entry(&sc->recv_io.free.list,
					   struct smbdirect_recv_io,
					   list);
		list_del(&recvmsg->list);
	}
	spin_unlock_irqrestore(&sc->recv_io.free.lock, flags);
	return recvmsg;
}

static void put_recvmsg(struct smbdirect_socket *sc,
			struct smbdirect_recv_io *recvmsg)
{
	unsigned long flags;

	if (likely(recvmsg->sge.length != 0)) {
		ib_dma_unmap_single(sc->ib.dev,
				    recvmsg->sge.addr,
				    recvmsg->sge.length,
				    DMA_FROM_DEVICE);
		recvmsg->sge.length = 0;
	}

	spin_lock_irqsave(&sc->recv_io.free.lock, flags);
	list_add(&recvmsg->list, &sc->recv_io.free.list);
	spin_unlock_irqrestore(&sc->recv_io.free.lock, flags);

	queue_work(sc->workqueue, &sc->recv_io.posted.refill_work);
}

static void enqueue_reassembly(struct smbdirect_socket *sc,
			       struct smbdirect_recv_io *recvmsg,
			       int data_length)
{
	unsigned long flags;

	spin_lock_irqsave(&sc->recv_io.reassembly.lock, flags);
	list_add_tail(&recvmsg->list, &sc->recv_io.reassembly.list);
	sc->recv_io.reassembly.queue_length++;
	/*
	 * Make sure reassembly_data_length is updated after list and
	 * reassembly_queue_length are updated. On the dequeue side
	 * reassembly_data_length is checked without a lock to determine
	 * if reassembly_queue_length and list is up to date
	 */
	virt_wmb();
	sc->recv_io.reassembly.data_length += data_length;
	spin_unlock_irqrestore(&sc->recv_io.reassembly.lock, flags);
}

static struct smbdirect_recv_io *get_first_reassembly(struct smbdirect_socket *sc)
{
	if (!list_empty(&sc->recv_io.reassembly.list))
		return list_first_entry(&sc->recv_io.reassembly.list,
				struct smbdirect_recv_io, list);
	else
		return NULL;
}

static void smb_direct_disconnect_wake_up_all(struct smbdirect_socket *sc)
{
	/*
	 * Wake up all waiters in all wait queues
	 * in order to notice the broken connection.
	 */
	wake_up_all(&sc->status_wait);
	wake_up_all(&sc->send_io.lcredits.wait_queue);
	wake_up_all(&sc->send_io.credits.wait_queue);
	wake_up_all(&sc->send_io.pending.zero_wait_queue);
	wake_up_all(&sc->recv_io.reassembly.wait_queue);
	wake_up_all(&sc->rw_io.credits.wait_queue);
}

static void smb_direct_disconnect_rdma_work(struct work_struct *work)
{
	struct smbdirect_socket *sc =
		container_of(work, struct smbdirect_socket, disconnect_work);

	if (sc->first_error == 0)
		sc->first_error = -ECONNABORTED;

	/*
	 * make sure this and other work is not queued again
	 * but here we don't block and avoid
	 * disable[_delayed]_work_sync()
	 */
	disable_work(&sc->disconnect_work);
	disable_work(&sc->connect.work);
	disable_work(&sc->recv_io.posted.refill_work);
	disable_delayed_work(&sc->idle.timer_work);
	disable_work(&sc->idle.immediate_work);

	switch (sc->status) {
	case SMBDIRECT_SOCKET_NEGOTIATE_NEEDED:
	case SMBDIRECT_SOCKET_NEGOTIATE_RUNNING:
	case SMBDIRECT_SOCKET_NEGOTIATE_FAILED:
	case SMBDIRECT_SOCKET_CONNECTED:
	case SMBDIRECT_SOCKET_ERROR:
		sc->status = SMBDIRECT_SOCKET_DISCONNECTING;
		rdma_disconnect(sc->rdma.cm_id);
		break;

	case SMBDIRECT_SOCKET_CREATED:
	case SMBDIRECT_SOCKET_RESOLVE_ADDR_NEEDED:
	case SMBDIRECT_SOCKET_RESOLVE_ADDR_RUNNING:
	case SMBDIRECT_SOCKET_RESOLVE_ADDR_FAILED:
	case SMBDIRECT_SOCKET_RESOLVE_ROUTE_NEEDED:
	case SMBDIRECT_SOCKET_RESOLVE_ROUTE_RUNNING:
	case SMBDIRECT_SOCKET_RESOLVE_ROUTE_FAILED:
	case SMBDIRECT_SOCKET_RDMA_CONNECT_NEEDED:
	case SMBDIRECT_SOCKET_RDMA_CONNECT_RUNNING:
	case SMBDIRECT_SOCKET_RDMA_CONNECT_FAILED:
		/*
		 * rdma_accept() never reached
		 * RDMA_CM_EVENT_ESTABLISHED
		 */
		sc->status = SMBDIRECT_SOCKET_DISCONNECTED;
		break;

	case SMBDIRECT_SOCKET_DISCONNECTING:
	case SMBDIRECT_SOCKET_DISCONNECTED:
	case SMBDIRECT_SOCKET_DESTROYED:
		break;
	}

	/*
	 * Wake up all waiters in all wait queues
	 * in order to notice the broken connection.
	 */
	smb_direct_disconnect_wake_up_all(sc);
}

static void
smb_direct_disconnect_rdma_connection(struct smbdirect_socket *sc)
{
	if (sc->first_error == 0)
		sc->first_error = -ECONNABORTED;

	/*
	 * make sure other work (than disconnect_work) is
	 * not queued again but here we don't block and avoid
	 * disable[_delayed]_work_sync()
	 */
	disable_work(&sc->connect.work);
	disable_work(&sc->recv_io.posted.refill_work);
	disable_work(&sc->idle.immediate_work);
	disable_delayed_work(&sc->idle.timer_work);

	switch (sc->status) {
	case SMBDIRECT_SOCKET_RESOLVE_ADDR_FAILED:
	case SMBDIRECT_SOCKET_RESOLVE_ROUTE_FAILED:
	case SMBDIRECT_SOCKET_RDMA_CONNECT_FAILED:
	case SMBDIRECT_SOCKET_NEGOTIATE_FAILED:
	case SMBDIRECT_SOCKET_ERROR:
	case SMBDIRECT_SOCKET_DISCONNECTING:
	case SMBDIRECT_SOCKET_DISCONNECTED:
	case SMBDIRECT_SOCKET_DESTROYED:
		/*
		 * Keep the current error status
		 */
		break;

	case SMBDIRECT_SOCKET_RESOLVE_ADDR_NEEDED:
	case SMBDIRECT_SOCKET_RESOLVE_ADDR_RUNNING:
		sc->status = SMBDIRECT_SOCKET_RESOLVE_ADDR_FAILED;
		break;

	case SMBDIRECT_SOCKET_RESOLVE_ROUTE_NEEDED:
	case SMBDIRECT_SOCKET_RESOLVE_ROUTE_RUNNING:
		sc->status = SMBDIRECT_SOCKET_RESOLVE_ROUTE_FAILED;
		break;

	case SMBDIRECT_SOCKET_RDMA_CONNECT_NEEDED:
	case SMBDIRECT_SOCKET_RDMA_CONNECT_RUNNING:
		sc->status = SMBDIRECT_SOCKET_RDMA_CONNECT_FAILED;
		break;

	case SMBDIRECT_SOCKET_NEGOTIATE_NEEDED:
	case SMBDIRECT_SOCKET_NEGOTIATE_RUNNING:
		sc->status = SMBDIRECT_SOCKET_NEGOTIATE_FAILED;
		break;

	case SMBDIRECT_SOCKET_CREATED:
		sc->status = SMBDIRECT_SOCKET_DISCONNECTED;
		break;

	case SMBDIRECT_SOCKET_CONNECTED:
		sc->status = SMBDIRECT_SOCKET_ERROR;
		break;
	}

	/*
	 * Wake up all waiters in all wait queues
	 * in order to notice the broken connection.
	 */
	smb_direct_disconnect_wake_up_all(sc);

	queue_work(sc->workqueue, &sc->disconnect_work);
}

static void smb_direct_send_immediate_work(struct work_struct *work)
{
	struct smbdirect_socket *sc =
		container_of(work, struct smbdirect_socket, idle.immediate_work);

	if (sc->status != SMBDIRECT_SOCKET_CONNECTED)
		return;

	smb_direct_post_send_data(sc, NULL, NULL, 0, 0);
}

static void smb_direct_idle_connection_timer(struct work_struct *work)
{
	struct smbdirect_socket *sc =
		container_of(work, struct smbdirect_socket, idle.timer_work.work);
	struct smbdirect_socket_parameters *sp = &sc->parameters;

	if (sc->idle.keepalive != SMBDIRECT_KEEPALIVE_NONE) {
		smb_direct_disconnect_rdma_connection(sc);
		return;
	}

	if (sc->status != SMBDIRECT_SOCKET_CONNECTED)
		return;

	/*
	 * Now use the keepalive timeout (instead of keepalive interval)
	 * in order to wait for a response
	 */
	sc->idle.keepalive = SMBDIRECT_KEEPALIVE_PENDING;
	mod_delayed_work(sc->workqueue, &sc->idle.timer_work,
			 msecs_to_jiffies(sp->keepalive_timeout_msec));
	queue_work(sc->workqueue, &sc->idle.immediate_work);
}

static struct smb_direct_transport *alloc_transport(struct rdma_cm_id *cm_id)
=======
static inline int get_buf_page_count(void *buf, int size)
{
	return DIV_ROUND_UP((uintptr_t)buf + size, PAGE_SIZE) -
		(uintptr_t)buf / PAGE_SIZE;
}

static void smb_direct_destroy_pools(struct smbdirect_socket *sc);
static void smb_direct_post_recv_credits(struct work_struct *work);
static int smb_direct_post_send_data(struct smbdirect_socket *sc,
				     struct smbdirect_send_batch *send_ctx,
				     struct kvec *iov, int niov,
				     int remaining_data_length);

static inline void
*smbdirect_recv_io_payload(struct smbdirect_recv_io *recvmsg)
{
	return (void *)recvmsg->packet;
}

static struct
smbdirect_recv_io *get_free_recvmsg(struct smbdirect_socket *sc)
{
	struct smbdirect_recv_io *recvmsg = NULL;
	unsigned long flags;

	spin_lock_irqsave(&sc->recv_io.free.lock, flags);
	if (!list_empty(&sc->recv_io.free.list)) {
		recvmsg = list_first_entry(&sc->recv_io.free.list,
					   struct smbdirect_recv_io,
					   list);
		list_del(&recvmsg->list);
	}
	spin_unlock_irqrestore(&sc->recv_io.free.lock, flags);
	return recvmsg;
}

static void put_recvmsg(struct smbdirect_socket *sc,
			struct smbdirect_recv_io *recvmsg)
{
	unsigned long flags;

	if (likely(recvmsg->sge.length != 0)) {
		ib_dma_unmap_single(sc->ib.dev,
				    recvmsg->sge.addr,
				    recvmsg->sge.length,
				    DMA_FROM_DEVICE);
		recvmsg->sge.length = 0;
	}

	spin_lock_irqsave(&sc->recv_io.free.lock, flags);
	list_add(&recvmsg->list, &sc->recv_io.free.list);
	spin_unlock_irqrestore(&sc->recv_io.free.lock, flags);

	queue_work(sc->workqueue, &sc->recv_io.posted.refill_work);
}

static void enqueue_reassembly(struct smbdirect_socket *sc,
			       struct smbdirect_recv_io *recvmsg,
			       int data_length)
{
	unsigned long flags;

	spin_lock_irqsave(&sc->recv_io.reassembly.lock, flags);
	list_add_tail(&recvmsg->list, &sc->recv_io.reassembly.list);
	sc->recv_io.reassembly.queue_length++;
	/*
	 * Make sure reassembly_data_length is updated after list and
	 * reassembly_queue_length are updated. On the dequeue side
	 * reassembly_data_length is checked without a lock to determine
	 * if reassembly_queue_length and list is up to date
	 */
	virt_wmb();
	sc->recv_io.reassembly.data_length += data_length;
	spin_unlock_irqrestore(&sc->recv_io.reassembly.lock, flags);
}

static struct smbdirect_recv_io *get_first_reassembly(struct smbdirect_socket *sc)
{
	if (!list_empty(&sc->recv_io.reassembly.list))
		return list_first_entry(&sc->recv_io.reassembly.list,
				struct smbdirect_recv_io, list);
	else
		return NULL;
}

static void smb_direct_disconnect_wake_up_all(struct smbdirect_socket *sc)
{
	/*
	 * Wake up all waiters in all wait queues
	 * in order to notice the broken connection.
	 */
	wake_up_all(&sc->status_wait);
	wake_up_all(&sc->send_io.bcredits.wait_queue);
	wake_up_all(&sc->send_io.lcredits.wait_queue);
	wake_up_all(&sc->send_io.credits.wait_queue);
	wake_up_all(&sc->send_io.pending.zero_wait_queue);
	wake_up_all(&sc->recv_io.reassembly.wait_queue);
	wake_up_all(&sc->rw_io.credits.wait_queue);
}

static void smb_direct_disconnect_rdma_work(struct work_struct *work)
{
	struct smbdirect_socket *sc =
		container_of(work, struct smbdirect_socket, disconnect_work);

	if (sc->first_error == 0)
		sc->first_error = -ECONNABORTED;

	/*
	 * make sure this and other work is not queued again
	 * but here we don't block and avoid
	 * disable[_delayed]_work_sync()
	 */
	disable_work(&sc->disconnect_work);
	disable_work(&sc->connect.work);
	disable_work(&sc->recv_io.posted.refill_work);
	disable_delayed_work(&sc->idle.timer_work);
	disable_work(&sc->idle.immediate_work);

	switch (sc->status) {
	case SMBDIRECT_SOCKET_NEGOTIATE_NEEDED:
	case SMBDIRECT_SOCKET_NEGOTIATE_RUNNING:
	case SMBDIRECT_SOCKET_NEGOTIATE_FAILED:
	case SMBDIRECT_SOCKET_CONNECTED:
	case SMBDIRECT_SOCKET_ERROR:
		sc->status = SMBDIRECT_SOCKET_DISCONNECTING;
		rdma_disconnect(sc->rdma.cm_id);
		break;

	case SMBDIRECT_SOCKET_CREATED:
	case SMBDIRECT_SOCKET_RESOLVE_ADDR_NEEDED:
	case SMBDIRECT_SOCKET_RESOLVE_ADDR_RUNNING:
	case SMBDIRECT_SOCKET_RESOLVE_ADDR_FAILED:
	case SMBDIRECT_SOCKET_RESOLVE_ROUTE_NEEDED:
	case SMBDIRECT_SOCKET_RESOLVE_ROUTE_RUNNING:
	case SMBDIRECT_SOCKET_RESOLVE_ROUTE_FAILED:
	case SMBDIRECT_SOCKET_RDMA_CONNECT_NEEDED:
	case SMBDIRECT_SOCKET_RDMA_CONNECT_RUNNING:
	case SMBDIRECT_SOCKET_RDMA_CONNECT_FAILED:
		/*
		 * rdma_accept() never reached
		 * RDMA_CM_EVENT_ESTABLISHED
		 */
		sc->status = SMBDIRECT_SOCKET_DISCONNECTED;
		break;

	case SMBDIRECT_SOCKET_DISCONNECTING:
	case SMBDIRECT_SOCKET_DISCONNECTED:
	case SMBDIRECT_SOCKET_DESTROYED:
		break;
	}

	/*
	 * Wake up all waiters in all wait queues
	 * in order to notice the broken connection.
	 */
	smb_direct_disconnect_wake_up_all(sc);
}

static void
smb_direct_disconnect_rdma_connection(struct smbdirect_socket *sc)
{
	if (sc->first_error == 0)
		sc->first_error = -ECONNABORTED;

	/*
	 * make sure other work (than disconnect_work) is
	 * not queued again but here we don't block and avoid
	 * disable[_delayed]_work_sync()
	 */
	disable_work(&sc->connect.work);
	disable_work(&sc->recv_io.posted.refill_work);
	disable_work(&sc->idle.immediate_work);
	disable_delayed_work(&sc->idle.timer_work);

	switch (sc->status) {
	case SMBDIRECT_SOCKET_RESOLVE_ADDR_FAILED:
	case SMBDIRECT_SOCKET_RESOLVE_ROUTE_FAILED:
	case SMBDIRECT_SOCKET_RDMA_CONNECT_FAILED:
	case SMBDIRECT_SOCKET_NEGOTIATE_FAILED:
	case SMBDIRECT_SOCKET_ERROR:
	case SMBDIRECT_SOCKET_DISCONNECTING:
	case SMBDIRECT_SOCKET_DISCONNECTED:
	case SMBDIRECT_SOCKET_DESTROYED:
		/*
		 * Keep the current error status
		 */
		break;

	case SMBDIRECT_SOCKET_RESOLVE_ADDR_NEEDED:
	case SMBDIRECT_SOCKET_RESOLVE_ADDR_RUNNING:
		sc->status = SMBDIRECT_SOCKET_RESOLVE_ADDR_FAILED;
		break;

	case SMBDIRECT_SOCKET_RESOLVE_ROUTE_NEEDED:
	case SMBDIRECT_SOCKET_RESOLVE_ROUTE_RUNNING:
		sc->status = SMBDIRECT_SOCKET_RESOLVE_ROUTE_FAILED;
		break;

	case SMBDIRECT_SOCKET_RDMA_CONNECT_NEEDED:
	case SMBDIRECT_SOCKET_RDMA_CONNECT_RUNNING:
		sc->status = SMBDIRECT_SOCKET_RDMA_CONNECT_FAILED;
		break;

	case SMBDIRECT_SOCKET_NEGOTIATE_NEEDED:
	case SMBDIRECT_SOCKET_NEGOTIATE_RUNNING:
		sc->status = SMBDIRECT_SOCKET_NEGOTIATE_FAILED;
		break;

	case SMBDIRECT_SOCKET_CREATED:
		sc->status = SMBDIRECT_SOCKET_DISCONNECTED;
		break;

	case SMBDIRECT_SOCKET_CONNECTED:
		sc->status = SMBDIRECT_SOCKET_ERROR;
		break;
	}

	/*
	 * Wake up all waiters in all wait queues
	 * in order to notice the broken connection.
	 */
	smb_direct_disconnect_wake_up_all(sc);

	queue_work(sc->workqueue, &sc->disconnect_work);
}

static void smb_direct_send_immediate_work(struct work_struct *work)
{
	struct smbdirect_socket *sc =
		container_of(work, struct smbdirect_socket, idle.immediate_work);

	if (sc->status != SMBDIRECT_SOCKET_CONNECTED)
		return;

	smb_direct_post_send_data(sc, NULL, NULL, 0, 0);
}

static void smb_direct_idle_connection_timer(struct work_struct *work)
{
	struct smbdirect_socket *sc =
		container_of(work, struct smbdirect_socket, idle.timer_work.work);
	struct smbdirect_socket_parameters *sp = &sc->parameters;

	if (sc->idle.keepalive != SMBDIRECT_KEEPALIVE_NONE) {
		smb_direct_disconnect_rdma_connection(sc);
		return;
	}

	if (sc->status != SMBDIRECT_SOCKET_CONNECTED)
		return;

	/*
	 * Now use the keepalive timeout (instead of keepalive interval)
	 * in order to wait for a response
	 */
	sc->idle.keepalive = SMBDIRECT_KEEPALIVE_PENDING;
	mod_delayed_work(sc->workqueue, &sc->idle.timer_work,
			 msecs_to_jiffies(sp->keepalive_timeout_msec));
	queue_work(sc->workqueue, &sc->idle.immediate_work);
}

static struct smb_direct_transport *alloc_transport(struct rdma_cm_id *cm_id)
>>>>>>> hardened/6.19
{
	struct smb_direct_transport *t;
	struct ksmbd_conn *conn;

	t = kzalloc_obj(*t, KSMBD_DEFAULT_GFP);
	if (!t)
		return NULL;
	t->socket = sc;

	conn = ksmbd_conn_alloc();
	if (!conn)
		goto conn_alloc_failed;

	down_write(&conn_list_lock);
	hash_add(conn_list, &conn->hlist, 0);
	up_write(&conn_list_lock);

	conn->transport = KSMBD_TRANS(t);
	KSMBD_TRANS(t)->conn = conn;
	KSMBD_TRANS(t)->ops = &ksmbd_smb_direct_transport_ops;

	return t;

conn_alloc_failed:
	kfree(t);
	return NULL;
}

static void smb_direct_free_transport(struct ksmbd_transport *kt)
{
	struct smb_direct_transport *t = SMBD_TRANS(kt);

	smbdirect_socket_release(t->socket);
	kfree(t);
}

static void free_transport(struct smb_direct_transport *t)
{
	smbdirect_socket_shutdown(t->socket);
	ksmbd_conn_free(KSMBD_TRANS(t)->conn);
}

<<<<<<< HEAD
||||||| 05f7e89ab9731
static struct smbdirect_send_io
*smb_direct_alloc_sendmsg(struct smbdirect_socket *sc)
{
	struct smbdirect_send_io *msg;

	msg = mempool_alloc(sc->send_io.mem.pool, KSMBD_DEFAULT_GFP);
	if (!msg)
		return ERR_PTR(-ENOMEM);
	msg->socket = sc;
	INIT_LIST_HEAD(&msg->sibling_list);
	msg->num_sge = 0;
	return msg;
}

static void smb_direct_free_sendmsg(struct smbdirect_socket *sc,
				    struct smbdirect_send_io *msg)
{
	int i;

	/*
	 * The list needs to be empty!
	 * The caller should take care of it.
	 */
	WARN_ON_ONCE(!list_empty(&msg->sibling_list));

	if (msg->num_sge > 0) {
		ib_dma_unmap_single(sc->ib.dev,
				    msg->sge[0].addr, msg->sge[0].length,
				    DMA_TO_DEVICE);
		for (i = 1; i < msg->num_sge; i++)
			ib_dma_unmap_page(sc->ib.dev,
					  msg->sge[i].addr, msg->sge[i].length,
					  DMA_TO_DEVICE);
	}
	mempool_free(msg, sc->send_io.mem.pool);
}

static int smb_direct_check_recvmsg(struct smbdirect_recv_io *recvmsg)
{
	struct smbdirect_socket *sc = recvmsg->socket;

	switch (sc->recv_io.expected) {
	case SMBDIRECT_EXPECT_DATA_TRANSFER: {
		struct smbdirect_data_transfer *req =
			(struct smbdirect_data_transfer *)recvmsg->packet;
		struct smb2_hdr *hdr = (struct smb2_hdr *)(recvmsg->packet
				+ le32_to_cpu(req->data_offset));
		ksmbd_debug(RDMA,
			    "CreditGranted: %u, CreditRequested: %u, DataLength: %u, RemainingDataLength: %u, SMB: %x, Command: %u\n",
			    le16_to_cpu(req->credits_granted),
			    le16_to_cpu(req->credits_requested),
			    req->data_length, req->remaining_data_length,
			    hdr->ProtocolId, hdr->Command);
		return 0;
	}
	case SMBDIRECT_EXPECT_NEGOTIATE_REQ: {
		struct smbdirect_negotiate_req *req =
			(struct smbdirect_negotiate_req *)recvmsg->packet;
		ksmbd_debug(RDMA,
			    "MinVersion: %u, MaxVersion: %u, CreditRequested: %u, MaxSendSize: %u, MaxRecvSize: %u, MaxFragmentedSize: %u\n",
			    le16_to_cpu(req->min_version),
			    le16_to_cpu(req->max_version),
			    le16_to_cpu(req->credits_requested),
			    le32_to_cpu(req->preferred_send_size),
			    le32_to_cpu(req->max_receive_size),
			    le32_to_cpu(req->max_fragmented_size));
		if (le16_to_cpu(req->min_version) > 0x0100 ||
		    le16_to_cpu(req->max_version) < 0x0100)
			return -EOPNOTSUPP;
		if (le16_to_cpu(req->credits_requested) <= 0 ||
		    le32_to_cpu(req->max_receive_size) <= 128 ||
		    le32_to_cpu(req->max_fragmented_size) <=
					128 * 1024)
			return -ECONNABORTED;

		return 0;
	}
	case SMBDIRECT_EXPECT_NEGOTIATE_REP:
		/* client only */
		break;
	}

	/* This is an internal error */
	return -EINVAL;
}

static void recv_done(struct ib_cq *cq, struct ib_wc *wc)
{
	struct smbdirect_recv_io *recvmsg;
	struct smbdirect_socket *sc;
	struct smbdirect_socket_parameters *sp;

	recvmsg = container_of(wc->wr_cqe, struct smbdirect_recv_io, cqe);
	sc = recvmsg->socket;
	sp = &sc->parameters;

	if (wc->status != IB_WC_SUCCESS || wc->opcode != IB_WC_RECV) {
		put_recvmsg(sc, recvmsg);
		if (wc->status != IB_WC_WR_FLUSH_ERR) {
			pr_err("Recv error. status='%s (%d)' opcode=%d\n",
			       ib_wc_status_msg(wc->status), wc->status,
			       wc->opcode);
			smb_direct_disconnect_rdma_connection(sc);
		}
		return;
	}

	ksmbd_debug(RDMA, "Recv completed. status='%s (%d)', opcode=%d\n",
		    ib_wc_status_msg(wc->status), wc->status,
		    wc->opcode);

	ib_dma_sync_single_for_cpu(wc->qp->device, recvmsg->sge.addr,
				   recvmsg->sge.length, DMA_FROM_DEVICE);

	/*
	 * Reset timer to the keepalive interval in
	 * order to trigger our next keepalive message.
	 */
	sc->idle.keepalive = SMBDIRECT_KEEPALIVE_NONE;
	mod_delayed_work(sc->workqueue, &sc->idle.timer_work,
			 msecs_to_jiffies(sp->keepalive_interval_msec));

	switch (sc->recv_io.expected) {
	case SMBDIRECT_EXPECT_NEGOTIATE_REQ:
		/* see smb_direct_negotiate_recv_done */
		break;
	case SMBDIRECT_EXPECT_DATA_TRANSFER: {
		struct smbdirect_data_transfer *data_transfer =
			(struct smbdirect_data_transfer *)recvmsg->packet;
		u32 remaining_data_length, data_offset, data_length;
		u16 old_recv_credit_target;

		if (wc->byte_len <
		    offsetof(struct smbdirect_data_transfer, padding)) {
			put_recvmsg(sc, recvmsg);
			smb_direct_disconnect_rdma_connection(sc);
			return;
		}

		remaining_data_length = le32_to_cpu(data_transfer->remaining_data_length);
		data_length = le32_to_cpu(data_transfer->data_length);
		data_offset = le32_to_cpu(data_transfer->data_offset);
		if (wc->byte_len < data_offset ||
		    wc->byte_len < (u64)data_offset + data_length) {
			put_recvmsg(sc, recvmsg);
			smb_direct_disconnect_rdma_connection(sc);
			return;
		}
		if (remaining_data_length > sp->max_fragmented_recv_size ||
		    data_length > sp->max_fragmented_recv_size ||
		    (u64)remaining_data_length + (u64)data_length >
		    (u64)sp->max_fragmented_recv_size) {
			put_recvmsg(sc, recvmsg);
			smb_direct_disconnect_rdma_connection(sc);
			return;
		}

		if (data_length) {
			if (sc->recv_io.reassembly.full_packet_received)
				recvmsg->first_segment = true;

			if (le32_to_cpu(data_transfer->remaining_data_length))
				sc->recv_io.reassembly.full_packet_received = false;
			else
				sc->recv_io.reassembly.full_packet_received = true;
		}

		atomic_dec(&sc->recv_io.posted.count);
		atomic_dec(&sc->recv_io.credits.count);

		old_recv_credit_target = sc->recv_io.credits.target;
		sc->recv_io.credits.target =
				le16_to_cpu(data_transfer->credits_requested);
		sc->recv_io.credits.target =
			min_t(u16, sc->recv_io.credits.target, sp->recv_credit_max);
		sc->recv_io.credits.target =
			max_t(u16, sc->recv_io.credits.target, 1);
		atomic_add(le16_to_cpu(data_transfer->credits_granted),
			   &sc->send_io.credits.count);

		if (le16_to_cpu(data_transfer->flags) &
		    SMBDIRECT_FLAG_RESPONSE_REQUESTED)
			queue_work(sc->workqueue, &sc->idle.immediate_work);

		if (atomic_read(&sc->send_io.credits.count) > 0)
			wake_up(&sc->send_io.credits.wait_queue);

		if (data_length) {
			if (sc->recv_io.credits.target > old_recv_credit_target)
				queue_work(sc->workqueue, &sc->recv_io.posted.refill_work);

			enqueue_reassembly(sc, recvmsg, (int)data_length);
			wake_up(&sc->recv_io.reassembly.wait_queue);
		} else
			put_recvmsg(sc, recvmsg);

		return;
	}
	case SMBDIRECT_EXPECT_NEGOTIATE_REP:
		/* client only */
		break;
	}

	/*
	 * This is an internal error!
	 */
	WARN_ON_ONCE(sc->recv_io.expected != SMBDIRECT_EXPECT_DATA_TRANSFER);
	put_recvmsg(sc, recvmsg);
	smb_direct_disconnect_rdma_connection(sc);
}

static void smb_direct_negotiate_recv_work(struct work_struct *work);

static void smb_direct_negotiate_recv_done(struct ib_cq *cq, struct ib_wc *wc)
{
	struct smbdirect_recv_io *recv_io =
		container_of(wc->wr_cqe, struct smbdirect_recv_io, cqe);
	struct smbdirect_socket *sc = recv_io->socket;
	unsigned long flags;

	/*
	 * reset the common recv_done for later reuse.
	 */
	recv_io->cqe.done = recv_done;

	if (wc->status != IB_WC_SUCCESS || wc->opcode != IB_WC_RECV) {
		put_recvmsg(sc, recv_io);
		if (wc->status != IB_WC_WR_FLUSH_ERR) {
			pr_err("Negotiate Recv error. status='%s (%d)' opcode=%d\n",
			       ib_wc_status_msg(wc->status), wc->status,
			       wc->opcode);
			smb_direct_disconnect_rdma_connection(sc);
		}
		return;
	}

	ksmbd_debug(RDMA, "Negotiate Recv completed. status='%s (%d)', opcode=%d\n",
		    ib_wc_status_msg(wc->status), wc->status,
		    wc->opcode);

	ib_dma_sync_single_for_cpu(sc->ib.dev,
				   recv_io->sge.addr,
				   recv_io->sge.length,
				   DMA_FROM_DEVICE);

	/*
	 * This is an internal error!
	 */
	if (WARN_ON_ONCE(sc->recv_io.expected != SMBDIRECT_EXPECT_NEGOTIATE_REQ)) {
		put_recvmsg(sc, recv_io);
		smb_direct_disconnect_rdma_connection(sc);
		return;
	}

	/*
	 * Don't reset timer to the keepalive interval in
	 * this will be done in smb_direct_negotiate_recv_work.
	 */

	/*
	 * Only remember the recv_io if it has enough bytes,
	 * this gives smb_direct_negotiate_recv_work enough
	 * information in order to disconnect if it was not
	 * valid.
	 */
	sc->recv_io.reassembly.full_packet_received = true;
	if (wc->byte_len >= sizeof(struct smbdirect_negotiate_req))
		enqueue_reassembly(sc, recv_io, 0);
	else
		put_recvmsg(sc, recv_io);

	/*
	 * Some drivers (at least mlx5_ib and irdma in roce mode)
	 * might post a recv completion before RDMA_CM_EVENT_ESTABLISHED,
	 * we need to adjust our expectation in that case.
	 *
	 * So we defer further processing of the negotiation
	 * to smb_direct_negotiate_recv_work().
	 *
	 * If we are already in SMBDIRECT_SOCKET_NEGOTIATE_NEEDED
	 * we queue the work directly otherwise
	 * smb_direct_cm_handler() will do it, when
	 * RDMA_CM_EVENT_ESTABLISHED arrived.
	 */
	spin_lock_irqsave(&sc->connect.lock, flags);
	if (!sc->first_error) {
		INIT_WORK(&sc->connect.work, smb_direct_negotiate_recv_work);
		if (sc->status == SMBDIRECT_SOCKET_NEGOTIATE_NEEDED)
			queue_work(sc->workqueue, &sc->connect.work);
	}
	spin_unlock_irqrestore(&sc->connect.lock, flags);
}

static void smb_direct_negotiate_recv_work(struct work_struct *work)
{
	struct smbdirect_socket *sc =
		container_of(work, struct smbdirect_socket, connect.work);
	const struct smbdirect_socket_parameters *sp = &sc->parameters;
	struct smbdirect_recv_io *recv_io;

	if (sc->first_error)
		return;

	ksmbd_debug(RDMA, "Negotiate Recv Work running\n");

	/*
	 * Reset timer to the keepalive interval in
	 * order to trigger our next keepalive message.
	 */
	sc->idle.keepalive = SMBDIRECT_KEEPALIVE_NONE;
	mod_delayed_work(sc->workqueue, &sc->idle.timer_work,
			 msecs_to_jiffies(sp->keepalive_interval_msec));

	/*
	 * If smb_direct_negotiate_recv_done() detected an
	 * invalid request we want to disconnect.
	 */
	recv_io = get_first_reassembly(sc);
	if (!recv_io) {
		smb_direct_disconnect_rdma_connection(sc);
		return;
	}

	if (SMBDIRECT_CHECK_STATUS_WARN(sc, SMBDIRECT_SOCKET_NEGOTIATE_NEEDED)) {
		smb_direct_disconnect_rdma_connection(sc);
		return;
	}
	sc->status = SMBDIRECT_SOCKET_NEGOTIATE_RUNNING;
	wake_up(&sc->status_wait);
}

static int smb_direct_post_recv(struct smbdirect_socket *sc,
				struct smbdirect_recv_io *recvmsg)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	struct ib_recv_wr wr;
	int ret;

	recvmsg->sge.addr = ib_dma_map_single(sc->ib.dev,
					      recvmsg->packet,
					      sp->max_recv_size,
					      DMA_FROM_DEVICE);
	ret = ib_dma_mapping_error(sc->ib.dev, recvmsg->sge.addr);
	if (ret)
		return ret;
	recvmsg->sge.length = sp->max_recv_size;
	recvmsg->sge.lkey = sc->ib.pd->local_dma_lkey;

	wr.wr_cqe = &recvmsg->cqe;
	wr.next = NULL;
	wr.sg_list = &recvmsg->sge;
	wr.num_sge = 1;

	ret = ib_post_recv(sc->ib.qp, &wr, NULL);
	if (ret) {
		pr_err("Can't post recv: %d\n", ret);
		ib_dma_unmap_single(sc->ib.dev,
				    recvmsg->sge.addr, recvmsg->sge.length,
				    DMA_FROM_DEVICE);
		recvmsg->sge.length = 0;
		smb_direct_disconnect_rdma_connection(sc);
		return ret;
	}
	return ret;
}

=======
static struct smbdirect_send_io
*smb_direct_alloc_sendmsg(struct smbdirect_socket *sc)
{
	struct smbdirect_send_io *msg;

	msg = mempool_alloc(sc->send_io.mem.pool, KSMBD_DEFAULT_GFP);
	if (!msg)
		return ERR_PTR(-ENOMEM);
	msg->socket = sc;
	INIT_LIST_HEAD(&msg->sibling_list);
	msg->num_sge = 0;
	return msg;
}

static void smb_direct_free_sendmsg(struct smbdirect_socket *sc,
				    struct smbdirect_send_io *msg)
{
	int i;

	/*
	 * The list needs to be empty!
	 * The caller should take care of it.
	 */
	WARN_ON_ONCE(!list_empty(&msg->sibling_list));

	if (msg->num_sge > 0) {
		ib_dma_unmap_single(sc->ib.dev,
				    msg->sge[0].addr, msg->sge[0].length,
				    DMA_TO_DEVICE);
		for (i = 1; i < msg->num_sge; i++)
			ib_dma_unmap_page(sc->ib.dev,
					  msg->sge[i].addr, msg->sge[i].length,
					  DMA_TO_DEVICE);
	}
	mempool_free(msg, sc->send_io.mem.pool);
}

static int smb_direct_check_recvmsg(struct smbdirect_recv_io *recvmsg)
{
	struct smbdirect_socket *sc = recvmsg->socket;

	switch (sc->recv_io.expected) {
	case SMBDIRECT_EXPECT_DATA_TRANSFER: {
		struct smbdirect_data_transfer *req =
			(struct smbdirect_data_transfer *)recvmsg->packet;
		struct smb2_hdr *hdr = (struct smb2_hdr *)(recvmsg->packet
				+ le32_to_cpu(req->data_offset));
		ksmbd_debug(RDMA,
			    "CreditGranted: %u, CreditRequested: %u, DataLength: %u, RemainingDataLength: %u, SMB: %x, Command: %u\n",
			    le16_to_cpu(req->credits_granted),
			    le16_to_cpu(req->credits_requested),
			    req->data_length, req->remaining_data_length,
			    hdr->ProtocolId, hdr->Command);
		return 0;
	}
	case SMBDIRECT_EXPECT_NEGOTIATE_REQ: {
		struct smbdirect_negotiate_req *req =
			(struct smbdirect_negotiate_req *)recvmsg->packet;
		ksmbd_debug(RDMA,
			    "MinVersion: %u, MaxVersion: %u, CreditRequested: %u, MaxSendSize: %u, MaxRecvSize: %u, MaxFragmentedSize: %u\n",
			    le16_to_cpu(req->min_version),
			    le16_to_cpu(req->max_version),
			    le16_to_cpu(req->credits_requested),
			    le32_to_cpu(req->preferred_send_size),
			    le32_to_cpu(req->max_receive_size),
			    le32_to_cpu(req->max_fragmented_size));
		if (le16_to_cpu(req->min_version) > 0x0100 ||
		    le16_to_cpu(req->max_version) < 0x0100)
			return -EOPNOTSUPP;
		if (le16_to_cpu(req->credits_requested) <= 0 ||
		    le32_to_cpu(req->max_receive_size) <= 128 ||
		    le32_to_cpu(req->max_fragmented_size) <=
					128 * 1024)
			return -ECONNABORTED;

		return 0;
	}
	case SMBDIRECT_EXPECT_NEGOTIATE_REP:
		/* client only */
		break;
	}

	/* This is an internal error */
	return -EINVAL;
}

static void recv_done(struct ib_cq *cq, struct ib_wc *wc)
{
	struct smbdirect_recv_io *recvmsg;
	struct smbdirect_socket *sc;
	struct smbdirect_socket_parameters *sp;

	recvmsg = container_of(wc->wr_cqe, struct smbdirect_recv_io, cqe);
	sc = recvmsg->socket;
	sp = &sc->parameters;

	if (wc->status != IB_WC_SUCCESS || wc->opcode != IB_WC_RECV) {
		put_recvmsg(sc, recvmsg);
		if (wc->status != IB_WC_WR_FLUSH_ERR) {
			pr_err("Recv error. status='%s (%d)' opcode=%d\n",
			       ib_wc_status_msg(wc->status), wc->status,
			       wc->opcode);
			smb_direct_disconnect_rdma_connection(sc);
		}
		return;
	}

	ksmbd_debug(RDMA, "Recv completed. status='%s (%d)', opcode=%d\n",
		    ib_wc_status_msg(wc->status), wc->status,
		    wc->opcode);

	ib_dma_sync_single_for_cpu(wc->qp->device, recvmsg->sge.addr,
				   recvmsg->sge.length, DMA_FROM_DEVICE);

	/*
	 * Reset timer to the keepalive interval in
	 * order to trigger our next keepalive message.
	 */
	sc->idle.keepalive = SMBDIRECT_KEEPALIVE_NONE;
	mod_delayed_work(sc->workqueue, &sc->idle.timer_work,
			 msecs_to_jiffies(sp->keepalive_interval_msec));

	switch (sc->recv_io.expected) {
	case SMBDIRECT_EXPECT_NEGOTIATE_REQ:
		/* see smb_direct_negotiate_recv_done */
		break;
	case SMBDIRECT_EXPECT_DATA_TRANSFER: {
		struct smbdirect_data_transfer *data_transfer =
			(struct smbdirect_data_transfer *)recvmsg->packet;
		u32 remaining_data_length, data_offset, data_length;
		int current_recv_credits;
		u16 old_recv_credit_target;

		if (wc->byte_len <
		    offsetof(struct smbdirect_data_transfer, padding)) {
			put_recvmsg(sc, recvmsg);
			smb_direct_disconnect_rdma_connection(sc);
			return;
		}

		remaining_data_length = le32_to_cpu(data_transfer->remaining_data_length);
		data_length = le32_to_cpu(data_transfer->data_length);
		data_offset = le32_to_cpu(data_transfer->data_offset);
		if (wc->byte_len < data_offset ||
		    wc->byte_len < (u64)data_offset + data_length) {
			put_recvmsg(sc, recvmsg);
			smb_direct_disconnect_rdma_connection(sc);
			return;
		}
		if (remaining_data_length > sp->max_fragmented_recv_size ||
		    data_length > sp->max_fragmented_recv_size ||
		    (u64)remaining_data_length + (u64)data_length >
		    (u64)sp->max_fragmented_recv_size) {
			put_recvmsg(sc, recvmsg);
			smb_direct_disconnect_rdma_connection(sc);
			return;
		}

		if (data_length) {
			if (sc->recv_io.reassembly.full_packet_received)
				recvmsg->first_segment = true;

			if (le32_to_cpu(data_transfer->remaining_data_length))
				sc->recv_io.reassembly.full_packet_received = false;
			else
				sc->recv_io.reassembly.full_packet_received = true;
		}

		atomic_dec(&sc->recv_io.posted.count);
		current_recv_credits = atomic_dec_return(&sc->recv_io.credits.count);

		old_recv_credit_target = sc->recv_io.credits.target;
		sc->recv_io.credits.target =
				le16_to_cpu(data_transfer->credits_requested);
		sc->recv_io.credits.target =
			min_t(u16, sc->recv_io.credits.target, sp->recv_credit_max);
		sc->recv_io.credits.target =
			max_t(u16, sc->recv_io.credits.target, 1);
		atomic_add(le16_to_cpu(data_transfer->credits_granted),
			   &sc->send_io.credits.count);

		if (le16_to_cpu(data_transfer->flags) &
		    SMBDIRECT_FLAG_RESPONSE_REQUESTED)
			queue_work(sc->workqueue, &sc->idle.immediate_work);

		if (atomic_read(&sc->send_io.credits.count) > 0)
			wake_up(&sc->send_io.credits.wait_queue);

		if (data_length) {
			if (current_recv_credits <= (sc->recv_io.credits.target / 4) ||
			    sc->recv_io.credits.target > old_recv_credit_target)
				queue_work(sc->workqueue, &sc->recv_io.posted.refill_work);

			enqueue_reassembly(sc, recvmsg, (int)data_length);
			wake_up(&sc->recv_io.reassembly.wait_queue);
		} else
			put_recvmsg(sc, recvmsg);

		return;
	}
	case SMBDIRECT_EXPECT_NEGOTIATE_REP:
		/* client only */
		break;
	}

	/*
	 * This is an internal error!
	 */
	WARN_ON_ONCE(sc->recv_io.expected != SMBDIRECT_EXPECT_DATA_TRANSFER);
	put_recvmsg(sc, recvmsg);
	smb_direct_disconnect_rdma_connection(sc);
}

static void smb_direct_negotiate_recv_work(struct work_struct *work);

static void smb_direct_negotiate_recv_done(struct ib_cq *cq, struct ib_wc *wc)
{
	struct smbdirect_recv_io *recv_io =
		container_of(wc->wr_cqe, struct smbdirect_recv_io, cqe);
	struct smbdirect_socket *sc = recv_io->socket;
	unsigned long flags;

	/*
	 * reset the common recv_done for later reuse.
	 */
	recv_io->cqe.done = recv_done;

	if (wc->status != IB_WC_SUCCESS || wc->opcode != IB_WC_RECV) {
		put_recvmsg(sc, recv_io);
		if (wc->status != IB_WC_WR_FLUSH_ERR) {
			pr_err("Negotiate Recv error. status='%s (%d)' opcode=%d\n",
			       ib_wc_status_msg(wc->status), wc->status,
			       wc->opcode);
			smb_direct_disconnect_rdma_connection(sc);
		}
		return;
	}

	ksmbd_debug(RDMA, "Negotiate Recv completed. status='%s (%d)', opcode=%d\n",
		    ib_wc_status_msg(wc->status), wc->status,
		    wc->opcode);

	ib_dma_sync_single_for_cpu(sc->ib.dev,
				   recv_io->sge.addr,
				   recv_io->sge.length,
				   DMA_FROM_DEVICE);

	/*
	 * This is an internal error!
	 */
	if (WARN_ON_ONCE(sc->recv_io.expected != SMBDIRECT_EXPECT_NEGOTIATE_REQ)) {
		put_recvmsg(sc, recv_io);
		smb_direct_disconnect_rdma_connection(sc);
		return;
	}

	/*
	 * Don't reset timer to the keepalive interval in
	 * this will be done in smb_direct_negotiate_recv_work.
	 */

	/*
	 * Only remember the recv_io if it has enough bytes,
	 * this gives smb_direct_negotiate_recv_work enough
	 * information in order to disconnect if it was not
	 * valid.
	 */
	sc->recv_io.reassembly.full_packet_received = true;
	if (wc->byte_len >= sizeof(struct smbdirect_negotiate_req))
		enqueue_reassembly(sc, recv_io, 0);
	else
		put_recvmsg(sc, recv_io);

	/*
	 * Some drivers (at least mlx5_ib and irdma in roce mode)
	 * might post a recv completion before RDMA_CM_EVENT_ESTABLISHED,
	 * we need to adjust our expectation in that case.
	 *
	 * So we defer further processing of the negotiation
	 * to smb_direct_negotiate_recv_work().
	 *
	 * If we are already in SMBDIRECT_SOCKET_NEGOTIATE_NEEDED
	 * we queue the work directly otherwise
	 * smb_direct_cm_handler() will do it, when
	 * RDMA_CM_EVENT_ESTABLISHED arrived.
	 */
	spin_lock_irqsave(&sc->connect.lock, flags);
	if (!sc->first_error) {
		INIT_WORK(&sc->connect.work, smb_direct_negotiate_recv_work);
		if (sc->status == SMBDIRECT_SOCKET_NEGOTIATE_NEEDED)
			queue_work(sc->workqueue, &sc->connect.work);
	}
	spin_unlock_irqrestore(&sc->connect.lock, flags);
}

static void smb_direct_negotiate_recv_work(struct work_struct *work)
{
	struct smbdirect_socket *sc =
		container_of(work, struct smbdirect_socket, connect.work);
	const struct smbdirect_socket_parameters *sp = &sc->parameters;
	struct smbdirect_recv_io *recv_io;

	if (sc->first_error)
		return;

	ksmbd_debug(RDMA, "Negotiate Recv Work running\n");

	/*
	 * Reset timer to the keepalive interval in
	 * order to trigger our next keepalive message.
	 */
	sc->idle.keepalive = SMBDIRECT_KEEPALIVE_NONE;
	mod_delayed_work(sc->workqueue, &sc->idle.timer_work,
			 msecs_to_jiffies(sp->keepalive_interval_msec));

	/*
	 * If smb_direct_negotiate_recv_done() detected an
	 * invalid request we want to disconnect.
	 */
	recv_io = get_first_reassembly(sc);
	if (!recv_io) {
		smb_direct_disconnect_rdma_connection(sc);
		return;
	}

	if (SMBDIRECT_CHECK_STATUS_WARN(sc, SMBDIRECT_SOCKET_NEGOTIATE_NEEDED)) {
		smb_direct_disconnect_rdma_connection(sc);
		return;
	}
	sc->status = SMBDIRECT_SOCKET_NEGOTIATE_RUNNING;
	wake_up(&sc->status_wait);
}

static int smb_direct_post_recv(struct smbdirect_socket *sc,
				struct smbdirect_recv_io *recvmsg)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	struct ib_recv_wr wr;
	int ret;

	recvmsg->sge.addr = ib_dma_map_single(sc->ib.dev,
					      recvmsg->packet,
					      sp->max_recv_size,
					      DMA_FROM_DEVICE);
	ret = ib_dma_mapping_error(sc->ib.dev, recvmsg->sge.addr);
	if (ret)
		return ret;
	recvmsg->sge.length = sp->max_recv_size;
	recvmsg->sge.lkey = sc->ib.pd->local_dma_lkey;

	wr.wr_cqe = &recvmsg->cqe;
	wr.next = NULL;
	wr.sg_list = &recvmsg->sge;
	wr.num_sge = 1;

	ret = ib_post_recv(sc->ib.qp, &wr, NULL);
	if (ret) {
		pr_err("Can't post recv: %d\n", ret);
		ib_dma_unmap_single(sc->ib.dev,
				    recvmsg->sge.addr, recvmsg->sge.length,
				    DMA_FROM_DEVICE);
		recvmsg->sge.length = 0;
		smb_direct_disconnect_rdma_connection(sc);
		return ret;
	}
	return ret;
}

>>>>>>> hardened/6.19
static int smb_direct_read(struct ksmbd_transport *t, char *buf,
			   unsigned int size, int unused)
{
	struct smb_direct_transport *st = SMBD_TRANS(t);
	struct smbdirect_socket *sc = st->socket;
	struct msghdr msg = { .msg_flags = 0, };
	struct kvec iov = {
		.iov_base = buf,
		.iov_len = size,
	};
	int ret;

	iov_iter_kvec(&msg.msg_iter, ITER_DEST, &iov, 1, size);

<<<<<<< HEAD
	ret = smbdirect_connection_recvmsg(sc, &msg, 0);
	if (ret == -ERESTARTSYS)
		ret = -EINTR;
||||||| 05f7e89ab9731
			recvmsg->first_segment = false;

			ret = smb_direct_post_recv(sc, recvmsg);
			if (ret) {
				pr_err("Can't post recv: %d\n", ret);
				put_recvmsg(sc, recvmsg);
				break;
			}
			credits++;

			atomic_inc(&sc->recv_io.posted.count);
		}
	}

	if (credits)
		queue_work(sc->workqueue, &sc->idle.immediate_work);
}

static void send_done(struct ib_cq *cq, struct ib_wc *wc)
{
	struct smbdirect_send_io *sendmsg, *sibling, *next;
	struct smbdirect_socket *sc;
	int lcredits = 0;

	sendmsg = container_of(wc->wr_cqe, struct smbdirect_send_io, cqe);
	sc = sendmsg->socket;

	ksmbd_debug(RDMA, "Send completed. status='%s (%d)', opcode=%d\n",
		    ib_wc_status_msg(wc->status), wc->status,
		    wc->opcode);

	/*
	 * Free possible siblings and then the main send_io
	 */
	list_for_each_entry_safe(sibling, next, &sendmsg->sibling_list, sibling_list) {
		list_del_init(&sibling->sibling_list);
		smb_direct_free_sendmsg(sc, sibling);
		lcredits += 1;
	}
	/* Note this frees wc->wr_cqe, but not wc */
	smb_direct_free_sendmsg(sc, sendmsg);
	lcredits += 1;

	if (wc->status != IB_WC_SUCCESS || wc->opcode != IB_WC_SEND) {
		pr_err("Send error. status='%s (%d)', opcode=%d\n",
		       ib_wc_status_msg(wc->status), wc->status,
		       wc->opcode);
		smb_direct_disconnect_rdma_connection(sc);
		return;
	}

	atomic_add(lcredits, &sc->send_io.lcredits.count);
	wake_up(&sc->send_io.lcredits.wait_queue);

	if (atomic_dec_and_test(&sc->send_io.pending.count))
		wake_up(&sc->send_io.pending.zero_wait_queue);
}

static int manage_credits_prior_sending(struct smbdirect_socket *sc)
{
	int new_credits;

	if (atomic_read(&sc->recv_io.credits.count) >= sc->recv_io.credits.target)
		return 0;

	new_credits = atomic_read(&sc->recv_io.posted.count);
	if (new_credits == 0)
		return 0;

	new_credits -= atomic_read(&sc->recv_io.credits.count);
	if (new_credits <= 0)
		return 0;

	atomic_add(new_credits, &sc->recv_io.credits.count);
	return new_credits;
}

static int manage_keep_alive_before_sending(struct smbdirect_socket *sc)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;

	if (sc->idle.keepalive == SMBDIRECT_KEEPALIVE_PENDING) {
		sc->idle.keepalive = SMBDIRECT_KEEPALIVE_SENT;
		/*
		 * Now use the keepalive timeout (instead of keepalive interval)
		 * in order to wait for a response
		 */
		mod_delayed_work(sc->workqueue, &sc->idle.timer_work,
				 msecs_to_jiffies(sp->keepalive_timeout_msec));
		return 1;
	}
	return 0;
}

static int smb_direct_post_send(struct smbdirect_socket *sc,
				struct ib_send_wr *wr)
{
	int ret;

	atomic_inc(&sc->send_io.pending.count);
	ret = ib_post_send(sc->ib.qp, wr, NULL);
	if (ret) {
		pr_err("failed to post send: %d\n", ret);
		smb_direct_disconnect_rdma_connection(sc);
	}
	return ret;
}

static void smb_direct_send_ctx_init(struct smbdirect_send_batch *send_ctx,
				     bool need_invalidate_rkey,
				     unsigned int remote_key)
{
	INIT_LIST_HEAD(&send_ctx->msg_list);
	send_ctx->wr_cnt = 0;
	send_ctx->need_invalidate_rkey = need_invalidate_rkey;
	send_ctx->remote_key = remote_key;
}

static int smb_direct_flush_send_list(struct smbdirect_socket *sc,
				      struct smbdirect_send_batch *send_ctx,
				      bool is_last)
{
	struct smbdirect_send_io *first, *last;
	int ret;

	if (list_empty(&send_ctx->msg_list))
		return 0;

	first = list_first_entry(&send_ctx->msg_list,
				 struct smbdirect_send_io,
				 sibling_list);
	last = list_last_entry(&send_ctx->msg_list,
			       struct smbdirect_send_io,
			       sibling_list);

	if (send_ctx->need_invalidate_rkey) {
		first->wr.opcode = IB_WR_SEND_WITH_INV;
		first->wr.ex.invalidate_rkey = send_ctx->remote_key;
		send_ctx->need_invalidate_rkey = false;
		send_ctx->remote_key = 0;
	}

	last->wr.send_flags = IB_SEND_SIGNALED;
	last->wr.wr_cqe = &last->cqe;

	/*
	 * Remove last from send_ctx->msg_list
	 * and splice the rest of send_ctx->msg_list
	 * to last->sibling_list.
	 *
	 * send_ctx->msg_list is a valid empty list
	 * at the end.
	 */
	list_del_init(&last->sibling_list);
	list_splice_tail_init(&send_ctx->msg_list, &last->sibling_list);
	send_ctx->wr_cnt = 0;

	ret = smb_direct_post_send(sc, &first->wr);
	if (ret) {
		struct smbdirect_send_io *sibling, *next;

		list_for_each_entry_safe(sibling, next, &last->sibling_list, sibling_list) {
			list_del_init(&sibling->sibling_list);
			smb_direct_free_sendmsg(sc, sibling);
		}
		smb_direct_free_sendmsg(sc, last);
	}

	return ret;
}

static int wait_for_credits(struct smbdirect_socket *sc,
			    wait_queue_head_t *waitq, atomic_t *total_credits,
			    int needed)
{
	int ret;

	do {
		if (atomic_sub_return(needed, total_credits) >= 0)
			return 0;

		atomic_add(needed, total_credits);
		ret = wait_event_interruptible(*waitq,
					       atomic_read(total_credits) >= needed ||
					       sc->status != SMBDIRECT_SOCKET_CONNECTED);

		if (sc->status != SMBDIRECT_SOCKET_CONNECTED)
			return -ENOTCONN;
		else if (ret < 0)
			return ret;
	} while (true);
}

static int wait_for_send_lcredit(struct smbdirect_socket *sc,
				 struct smbdirect_send_batch *send_ctx)
{
	if (send_ctx && (atomic_read(&sc->send_io.lcredits.count) <= 1)) {
		int ret;

		ret = smb_direct_flush_send_list(sc, send_ctx, false);
		if (ret)
			return ret;
	}

	return wait_for_credits(sc,
				&sc->send_io.lcredits.wait_queue,
				&sc->send_io.lcredits.count,
				1);
}

static int wait_for_send_credits(struct smbdirect_socket *sc,
				 struct smbdirect_send_batch *send_ctx)
{
	int ret;

	if (send_ctx &&
	    (send_ctx->wr_cnt >= 16 || atomic_read(&sc->send_io.credits.count) <= 1)) {
		ret = smb_direct_flush_send_list(sc, send_ctx, false);
		if (ret)
			return ret;
	}

	return wait_for_credits(sc, &sc->send_io.credits.wait_queue, &sc->send_io.credits.count, 1);
}

static int wait_for_rw_credits(struct smbdirect_socket *sc, int credits)
{
	return wait_for_credits(sc,
				&sc->rw_io.credits.wait_queue,
				&sc->rw_io.credits.count,
				credits);
}

static int calc_rw_credits(struct smbdirect_socket *sc,
			   char *buf, unsigned int len)
{
	return DIV_ROUND_UP(get_buf_page_count(buf, len),
			    sc->rw_io.credits.num_pages);
}

static int smb_direct_create_header(struct smbdirect_socket *sc,
				    int size, int remaining_data_length,
				    struct smbdirect_send_io **sendmsg_out)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	struct smbdirect_send_io *sendmsg;
	struct smbdirect_data_transfer *packet;
	int header_length;
	int ret;

	sendmsg = smb_direct_alloc_sendmsg(sc);
	if (IS_ERR(sendmsg))
		return PTR_ERR(sendmsg);

	/* Fill in the packet header */
	packet = (struct smbdirect_data_transfer *)sendmsg->packet;
	packet->credits_requested = cpu_to_le16(sp->send_credit_target);
	packet->credits_granted = cpu_to_le16(manage_credits_prior_sending(sc));

	packet->flags = 0;
	if (manage_keep_alive_before_sending(sc))
		packet->flags |= cpu_to_le16(SMBDIRECT_FLAG_RESPONSE_REQUESTED);

	packet->reserved = 0;
	if (!size)
		packet->data_offset = 0;
	else
		packet->data_offset = cpu_to_le32(24);
	packet->data_length = cpu_to_le32(size);
	packet->remaining_data_length = cpu_to_le32(remaining_data_length);
	packet->padding = 0;

	ksmbd_debug(RDMA,
		    "credits_requested=%d credits_granted=%d data_offset=%d data_length=%d remaining_data_length=%d\n",
		    le16_to_cpu(packet->credits_requested),
		    le16_to_cpu(packet->credits_granted),
		    le32_to_cpu(packet->data_offset),
		    le32_to_cpu(packet->data_length),
		    le32_to_cpu(packet->remaining_data_length));

	/* Map the packet to DMA */
	header_length = sizeof(struct smbdirect_data_transfer);
	/* If this is a packet without payload, don't send padding */
	if (!size)
		header_length =
			offsetof(struct smbdirect_data_transfer, padding);

	sendmsg->sge[0].addr = ib_dma_map_single(sc->ib.dev,
						 (void *)packet,
						 header_length,
						 DMA_TO_DEVICE);
	ret = ib_dma_mapping_error(sc->ib.dev, sendmsg->sge[0].addr);
	if (ret) {
		smb_direct_free_sendmsg(sc, sendmsg);
		return ret;
	}

	sendmsg->num_sge = 1;
	sendmsg->sge[0].length = header_length;
	sendmsg->sge[0].lkey = sc->ib.pd->local_dma_lkey;

	*sendmsg_out = sendmsg;
	return 0;
}

static int get_sg_list(void *buf, int size, struct scatterlist *sg_list, int nentries)
{
	bool high = is_vmalloc_addr(buf);
	struct page *page;
	int offset, len;
	int i = 0;

	if (size <= 0 || nentries < get_buf_page_count(buf, size))
		return -EINVAL;

	offset = offset_in_page(buf);
	buf -= offset;
	while (size > 0) {
		len = min_t(int, PAGE_SIZE - offset, size);
		if (high)
			page = vmalloc_to_page(buf);
		else
			page = kmap_to_page(buf);

		if (!sg_list)
			return -EINVAL;
		sg_set_page(sg_list, page, len, offset);
		sg_list = sg_next(sg_list);

		buf += PAGE_SIZE;
		size -= len;
		offset = 0;
		i++;
	}
	return i;
}

static int get_mapped_sg_list(struct ib_device *device, void *buf, int size,
			      struct scatterlist *sg_list, int nentries,
			      enum dma_data_direction dir, int *npages)
{
	*npages = get_sg_list(buf, size, sg_list, nentries);
	if (*npages < 0)
		return -EINVAL;
	return ib_dma_map_sg(device, sg_list, *npages, dir);
}

static int post_sendmsg(struct smbdirect_socket *sc,
			struct smbdirect_send_batch *send_ctx,
			struct smbdirect_send_io *msg)
{
	int i;

	for (i = 0; i < msg->num_sge; i++)
		ib_dma_sync_single_for_device(sc->ib.dev,
					      msg->sge[i].addr, msg->sge[i].length,
					      DMA_TO_DEVICE);

	msg->cqe.done = send_done;
	msg->wr.opcode = IB_WR_SEND;
	msg->wr.sg_list = &msg->sge[0];
	msg->wr.num_sge = msg->num_sge;
	msg->wr.next = NULL;

	if (send_ctx) {
		msg->wr.wr_cqe = NULL;
		msg->wr.send_flags = 0;
		if (!list_empty(&send_ctx->msg_list)) {
			struct smbdirect_send_io *last;

			last = list_last_entry(&send_ctx->msg_list,
					       struct smbdirect_send_io,
					       sibling_list);
			last->wr.next = &msg->wr;
		}
		list_add_tail(&msg->sibling_list, &send_ctx->msg_list);
		send_ctx->wr_cnt++;
		return 0;
	}

	msg->wr.wr_cqe = &msg->cqe;
	msg->wr.send_flags = IB_SEND_SIGNALED;
	return smb_direct_post_send(sc, &msg->wr);
}

static int smb_direct_post_send_data(struct smbdirect_socket *sc,
				     struct smbdirect_send_batch *send_ctx,
				     struct kvec *iov, int niov,
				     int remaining_data_length)
{
	int i, j, ret;
	struct smbdirect_send_io *msg;
	int data_length;
	struct scatterlist sg[SMBDIRECT_SEND_IO_MAX_SGE - 1];

	ret = wait_for_send_lcredit(sc, send_ctx);
	if (ret)
		goto lcredit_failed;

	ret = wait_for_send_credits(sc, send_ctx);
	if (ret)
		goto credit_failed;

	data_length = 0;
	for (i = 0; i < niov; i++)
		data_length += iov[i].iov_len;

	ret = smb_direct_create_header(sc, data_length, remaining_data_length,
				       &msg);
	if (ret)
		goto header_failed;

	for (i = 0; i < niov; i++) {
		struct ib_sge *sge;
		int sg_cnt;
		int npages;

		sg_init_table(sg, SMBDIRECT_SEND_IO_MAX_SGE - 1);
		sg_cnt = get_mapped_sg_list(sc->ib.dev,
					    iov[i].iov_base, iov[i].iov_len,
					    sg, SMBDIRECT_SEND_IO_MAX_SGE - 1,
					    DMA_TO_DEVICE, &npages);
		if (sg_cnt <= 0) {
			pr_err("failed to map buffer\n");
			ret = -ENOMEM;
			goto err;
		} else if (sg_cnt + msg->num_sge > SMBDIRECT_SEND_IO_MAX_SGE) {
			pr_err("buffer not fitted into sges\n");
			ret = -E2BIG;
			ib_dma_unmap_sg(sc->ib.dev, sg, npages,
					DMA_TO_DEVICE);
			goto err;
		}

		for (j = 0; j < sg_cnt; j++) {
			sge = &msg->sge[msg->num_sge];
			sge->addr = sg_dma_address(&sg[j]);
			sge->length = sg_dma_len(&sg[j]);
			sge->lkey  = sc->ib.pd->local_dma_lkey;
			msg->num_sge++;
		}
	}

	ret = post_sendmsg(sc, send_ctx, msg);
	if (ret)
		goto err;
	return 0;
err:
	smb_direct_free_sendmsg(sc, msg);
header_failed:
	atomic_inc(&sc->send_io.credits.count);
credit_failed:
	atomic_inc(&sc->send_io.lcredits.count);
lcredit_failed:
=======
			recvmsg->first_segment = false;

			ret = smb_direct_post_recv(sc, recvmsg);
			if (ret) {
				pr_err("Can't post recv: %d\n", ret);
				put_recvmsg(sc, recvmsg);
				break;
			}
			credits++;

			atomic_inc(&sc->recv_io.posted.count);
		}
	}

	atomic_add(credits, &sc->recv_io.credits.available);

	/*
	 * If the last send credit is waiting for credits
	 * it can grant we need to wake it up
	 */
	if (credits &&
	    atomic_read(&sc->send_io.bcredits.count) == 0 &&
	    atomic_read(&sc->send_io.credits.count) == 0)
		wake_up(&sc->send_io.credits.wait_queue);

	if (credits)
		queue_work(sc->workqueue, &sc->idle.immediate_work);
}

static void send_done(struct ib_cq *cq, struct ib_wc *wc)
{
	struct smbdirect_send_io *sendmsg, *sibling, *next;
	struct smbdirect_socket *sc;
	int lcredits = 0;

	sendmsg = container_of(wc->wr_cqe, struct smbdirect_send_io, cqe);
	sc = sendmsg->socket;

	ksmbd_debug(RDMA, "Send completed. status='%s (%d)', opcode=%d\n",
		    ib_wc_status_msg(wc->status), wc->status,
		    wc->opcode);

	if (unlikely(!(sendmsg->wr.send_flags & IB_SEND_SIGNALED))) {
		/*
		 * This happens when smbdirect_send_io is a sibling
		 * before the final message, it is signaled on
		 * error anyway, so we need to skip
		 * smbdirect_connection_free_send_io here,
		 * otherwise is will destroy the memory
		 * of the siblings too, which will cause
		 * use after free problems for the others
		 * triggered from ib_drain_qp().
		 */
		if (wc->status != IB_WC_SUCCESS)
			goto skip_free;

		/*
		 * This should not happen!
		 * But we better just close the
		 * connection...
		 */
		pr_err("unexpected send completion wc->status=%s (%d) wc->opcode=%d\n",
		       ib_wc_status_msg(wc->status), wc->status, wc->opcode);
		smb_direct_disconnect_rdma_connection(sc);
		return;
	}

	/*
	 * Free possible siblings and then the main send_io
	 */
	list_for_each_entry_safe(sibling, next, &sendmsg->sibling_list, sibling_list) {
		list_del_init(&sibling->sibling_list);
		smb_direct_free_sendmsg(sc, sibling);
		lcredits += 1;
	}
	/* Note this frees wc->wr_cqe, but not wc */
	smb_direct_free_sendmsg(sc, sendmsg);
	lcredits += 1;

	if (wc->status != IB_WC_SUCCESS || wc->opcode != IB_WC_SEND) {
skip_free:
		pr_err("Send error. status='%s (%d)', opcode=%d\n",
		       ib_wc_status_msg(wc->status), wc->status,
		       wc->opcode);
		smb_direct_disconnect_rdma_connection(sc);
		return;
	}

	atomic_add(lcredits, &sc->send_io.lcredits.count);
	wake_up(&sc->send_io.lcredits.wait_queue);

	if (atomic_dec_and_test(&sc->send_io.pending.count))
		wake_up(&sc->send_io.pending.zero_wait_queue);
}

static int manage_credits_prior_sending(struct smbdirect_socket *sc)
{
	int missing;
	int available;
	int new_credits;

	if (atomic_read(&sc->recv_io.credits.count) >= sc->recv_io.credits.target)
		return 0;

	missing = (int)sc->recv_io.credits.target - atomic_read(&sc->recv_io.credits.count);
	available = atomic_xchg(&sc->recv_io.credits.available, 0);
	new_credits = (u16)min3(U16_MAX, missing, available);
	if (new_credits <= 0) {
		/*
		 * If credits are available, but not granted
		 * we need to re-add them again.
		 */
		if (available)
			atomic_add(available, &sc->recv_io.credits.available);
		return 0;
	}

	if (new_credits < available) {
		/*
		 * Readd the remaining available again.
		 */
		available -= new_credits;
		atomic_add(available, &sc->recv_io.credits.available);
	}

	/*
	 * Remember we granted the credits
	 */
	atomic_add(new_credits, &sc->recv_io.credits.count);
	return new_credits;
}

static int manage_keep_alive_before_sending(struct smbdirect_socket *sc)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;

	if (sc->idle.keepalive == SMBDIRECT_KEEPALIVE_PENDING) {
		sc->idle.keepalive = SMBDIRECT_KEEPALIVE_SENT;
		/*
		 * Now use the keepalive timeout (instead of keepalive interval)
		 * in order to wait for a response
		 */
		mod_delayed_work(sc->workqueue, &sc->idle.timer_work,
				 msecs_to_jiffies(sp->keepalive_timeout_msec));
		return 1;
	}
	return 0;
}

static int smb_direct_post_send(struct smbdirect_socket *sc,
				struct ib_send_wr *wr)
{
	int ret;

	atomic_inc(&sc->send_io.pending.count);
	ret = ib_post_send(sc->ib.qp, wr, NULL);
	if (ret) {
		pr_err("failed to post send: %d\n", ret);
		smb_direct_disconnect_rdma_connection(sc);
	}
	return ret;
}

static void smb_direct_send_ctx_init(struct smbdirect_send_batch *send_ctx,
				     bool need_invalidate_rkey,
				     unsigned int remote_key)
{
	INIT_LIST_HEAD(&send_ctx->msg_list);
	send_ctx->wr_cnt = 0;
	send_ctx->need_invalidate_rkey = need_invalidate_rkey;
	send_ctx->remote_key = remote_key;
	send_ctx->credit = 0;
}

static int smb_direct_flush_send_list(struct smbdirect_socket *sc,
				      struct smbdirect_send_batch *send_ctx,
				      bool is_last)
{
	struct smbdirect_send_io *first, *last;
	int ret = 0;

	if (list_empty(&send_ctx->msg_list))
		goto release_credit;

	first = list_first_entry(&send_ctx->msg_list,
				 struct smbdirect_send_io,
				 sibling_list);
	last = list_last_entry(&send_ctx->msg_list,
			       struct smbdirect_send_io,
			       sibling_list);

	if (send_ctx->need_invalidate_rkey) {
		first->wr.opcode = IB_WR_SEND_WITH_INV;
		first->wr.ex.invalidate_rkey = send_ctx->remote_key;
		send_ctx->need_invalidate_rkey = false;
		send_ctx->remote_key = 0;
	}

	last->wr.send_flags = IB_SEND_SIGNALED;
	last->wr.wr_cqe = &last->cqe;

	/*
	 * Remove last from send_ctx->msg_list
	 * and splice the rest of send_ctx->msg_list
	 * to last->sibling_list.
	 *
	 * send_ctx->msg_list is a valid empty list
	 * at the end.
	 */
	list_del_init(&last->sibling_list);
	list_splice_tail_init(&send_ctx->msg_list, &last->sibling_list);
	send_ctx->wr_cnt = 0;

	ret = smb_direct_post_send(sc, &first->wr);
	if (ret) {
		struct smbdirect_send_io *sibling, *next;

		list_for_each_entry_safe(sibling, next, &last->sibling_list, sibling_list) {
			list_del_init(&sibling->sibling_list);
			smb_direct_free_sendmsg(sc, sibling);
		}
		smb_direct_free_sendmsg(sc, last);
	}

release_credit:
	if (is_last && !ret && send_ctx->credit) {
		atomic_add(send_ctx->credit, &sc->send_io.bcredits.count);
		send_ctx->credit = 0;
		wake_up(&sc->send_io.bcredits.wait_queue);
	}

	return ret;
}

static int wait_for_credits(struct smbdirect_socket *sc,
			    wait_queue_head_t *waitq, atomic_t *total_credits,
			    int needed)
{
	int ret;

	do {
		if (atomic_sub_return(needed, total_credits) >= 0)
			return 0;

		atomic_add(needed, total_credits);
		ret = wait_event_interruptible(*waitq,
					       atomic_read(total_credits) >= needed ||
					       sc->status != SMBDIRECT_SOCKET_CONNECTED);

		if (sc->status != SMBDIRECT_SOCKET_CONNECTED)
			return -ENOTCONN;
		else if (ret < 0)
			return ret;
	} while (true);
}

static int wait_for_send_bcredit(struct smbdirect_socket *sc,
				 struct smbdirect_send_batch *send_ctx)
{
	int ret;

	if (send_ctx->credit)
		return 0;

	ret = wait_for_credits(sc,
			       &sc->send_io.bcredits.wait_queue,
			       &sc->send_io.bcredits.count,
			       1);
	if (ret)
		return ret;

	send_ctx->credit = 1;
	return 0;
}

static int wait_for_send_lcredit(struct smbdirect_socket *sc,
				 struct smbdirect_send_batch *send_ctx)
{
	if (send_ctx && (atomic_read(&sc->send_io.lcredits.count) <= 1)) {
		int ret;

		ret = smb_direct_flush_send_list(sc, send_ctx, false);
		if (ret)
			return ret;
	}

	return wait_for_credits(sc,
				&sc->send_io.lcredits.wait_queue,
				&sc->send_io.lcredits.count,
				1);
}

static int wait_for_send_credits(struct smbdirect_socket *sc,
				 struct smbdirect_send_batch *send_ctx)
{
	int ret;

	if (send_ctx &&
	    (send_ctx->wr_cnt >= 16 || atomic_read(&sc->send_io.credits.count) <= 1)) {
		ret = smb_direct_flush_send_list(sc, send_ctx, false);
		if (ret)
			return ret;
	}

	return wait_for_credits(sc, &sc->send_io.credits.wait_queue, &sc->send_io.credits.count, 1);
}

static int wait_for_rw_credits(struct smbdirect_socket *sc, int credits)
{
	return wait_for_credits(sc,
				&sc->rw_io.credits.wait_queue,
				&sc->rw_io.credits.count,
				credits);
}

static int calc_rw_credits(struct smbdirect_socket *sc,
			   char *buf, unsigned int len)
{
	return DIV_ROUND_UP(get_buf_page_count(buf, len),
			    sc->rw_io.credits.num_pages);
}

static int smb_direct_create_header(struct smbdirect_socket *sc,
				    int size, int remaining_data_length,
				    int new_credits,
				    struct smbdirect_send_io **sendmsg_out)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	struct smbdirect_send_io *sendmsg;
	struct smbdirect_data_transfer *packet;
	int header_length;
	int ret;

	sendmsg = smb_direct_alloc_sendmsg(sc);
	if (IS_ERR(sendmsg))
		return PTR_ERR(sendmsg);

	/* Fill in the packet header */
	packet = (struct smbdirect_data_transfer *)sendmsg->packet;
	packet->credits_requested = cpu_to_le16(sp->send_credit_target);
	packet->credits_granted = cpu_to_le16(new_credits);

	packet->flags = 0;
	if (manage_keep_alive_before_sending(sc))
		packet->flags |= cpu_to_le16(SMBDIRECT_FLAG_RESPONSE_REQUESTED);

	packet->reserved = 0;
	if (!size)
		packet->data_offset = 0;
	else
		packet->data_offset = cpu_to_le32(24);
	packet->data_length = cpu_to_le32(size);
	packet->remaining_data_length = cpu_to_le32(remaining_data_length);
	packet->padding = 0;

	ksmbd_debug(RDMA,
		    "credits_requested=%d credits_granted=%d data_offset=%d data_length=%d remaining_data_length=%d\n",
		    le16_to_cpu(packet->credits_requested),
		    le16_to_cpu(packet->credits_granted),
		    le32_to_cpu(packet->data_offset),
		    le32_to_cpu(packet->data_length),
		    le32_to_cpu(packet->remaining_data_length));

	/* Map the packet to DMA */
	header_length = sizeof(struct smbdirect_data_transfer);
	/* If this is a packet without payload, don't send padding */
	if (!size)
		header_length =
			offsetof(struct smbdirect_data_transfer, padding);

	sendmsg->sge[0].addr = ib_dma_map_single(sc->ib.dev,
						 (void *)packet,
						 header_length,
						 DMA_TO_DEVICE);
	ret = ib_dma_mapping_error(sc->ib.dev, sendmsg->sge[0].addr);
	if (ret) {
		smb_direct_free_sendmsg(sc, sendmsg);
		return ret;
	}

	sendmsg->num_sge = 1;
	sendmsg->sge[0].length = header_length;
	sendmsg->sge[0].lkey = sc->ib.pd->local_dma_lkey;

	*sendmsg_out = sendmsg;
	return 0;
}

static int get_sg_list(void *buf, int size, struct scatterlist *sg_list, int nentries)
{
	bool high = is_vmalloc_addr(buf);
	struct page *page;
	int offset, len;
	int i = 0;

	if (size <= 0 || nentries < get_buf_page_count(buf, size))
		return -EINVAL;

	offset = offset_in_page(buf);
	buf -= offset;
	while (size > 0) {
		len = min_t(int, PAGE_SIZE - offset, size);
		if (high)
			page = vmalloc_to_page(buf);
		else
			page = kmap_to_page(buf);

		if (!sg_list)
			return -EINVAL;
		sg_set_page(sg_list, page, len, offset);
		sg_list = sg_next(sg_list);

		buf += PAGE_SIZE;
		size -= len;
		offset = 0;
		i++;
	}
	return i;
}

static int get_mapped_sg_list(struct ib_device *device, void *buf, int size,
			      struct scatterlist *sg_list, int nentries,
			      enum dma_data_direction dir, int *npages)
{
	*npages = get_sg_list(buf, size, sg_list, nentries);
	if (*npages < 0)
		return -EINVAL;
	return ib_dma_map_sg(device, sg_list, *npages, dir);
}

static int post_sendmsg(struct smbdirect_socket *sc,
			struct smbdirect_send_batch *send_ctx,
			struct smbdirect_send_io *msg)
{
	int i;

	for (i = 0; i < msg->num_sge; i++)
		ib_dma_sync_single_for_device(sc->ib.dev,
					      msg->sge[i].addr, msg->sge[i].length,
					      DMA_TO_DEVICE);

	msg->cqe.done = send_done;
	msg->wr.opcode = IB_WR_SEND;
	msg->wr.sg_list = &msg->sge[0];
	msg->wr.num_sge = msg->num_sge;
	msg->wr.next = NULL;

	if (send_ctx) {
		msg->wr.wr_cqe = NULL;
		msg->wr.send_flags = 0;
		if (!list_empty(&send_ctx->msg_list)) {
			struct smbdirect_send_io *last;

			last = list_last_entry(&send_ctx->msg_list,
					       struct smbdirect_send_io,
					       sibling_list);
			last->wr.next = &msg->wr;
		}
		list_add_tail(&msg->sibling_list, &send_ctx->msg_list);
		send_ctx->wr_cnt++;
		return 0;
	}

	msg->wr.wr_cqe = &msg->cqe;
	msg->wr.send_flags = IB_SEND_SIGNALED;
	return smb_direct_post_send(sc, &msg->wr);
}

static int smb_direct_post_send_data(struct smbdirect_socket *sc,
				     struct smbdirect_send_batch *send_ctx,
				     struct kvec *iov, int niov,
				     int remaining_data_length)
{
	int i, j, ret;
	struct smbdirect_send_io *msg;
	int data_length;
	struct scatterlist sg[SMBDIRECT_SEND_IO_MAX_SGE - 1];
	struct smbdirect_send_batch _send_ctx;
	int new_credits;

	if (!send_ctx) {
		smb_direct_send_ctx_init(&_send_ctx, false, 0);
		send_ctx = &_send_ctx;
	}

	ret = wait_for_send_bcredit(sc, send_ctx);
	if (ret)
		goto bcredit_failed;

	ret = wait_for_send_lcredit(sc, send_ctx);
	if (ret)
		goto lcredit_failed;

	ret = wait_for_send_credits(sc, send_ctx);
	if (ret)
		goto credit_failed;

	new_credits = manage_credits_prior_sending(sc);
	if (new_credits == 0 &&
	    atomic_read(&sc->send_io.credits.count) == 0 &&
	    atomic_read(&sc->recv_io.credits.count) == 0) {
		queue_work(sc->workqueue, &sc->recv_io.posted.refill_work);
		ret = wait_event_interruptible(sc->send_io.credits.wait_queue,
					       atomic_read(&sc->send_io.credits.count) >= 1 ||
					       atomic_read(&sc->recv_io.credits.available) >= 1 ||
					       sc->status != SMBDIRECT_SOCKET_CONNECTED);
		if (sc->status != SMBDIRECT_SOCKET_CONNECTED)
			ret = -ENOTCONN;
		if (ret < 0)
			goto credit_failed;

		new_credits = manage_credits_prior_sending(sc);
	}

	data_length = 0;
	for (i = 0; i < niov; i++)
		data_length += iov[i].iov_len;

	ret = smb_direct_create_header(sc, data_length, remaining_data_length,
				       new_credits, &msg);
	if (ret)
		goto header_failed;

	for (i = 0; i < niov; i++) {
		struct ib_sge *sge;
		int sg_cnt;
		int npages;

		sg_init_table(sg, SMBDIRECT_SEND_IO_MAX_SGE - 1);
		sg_cnt = get_mapped_sg_list(sc->ib.dev,
					    iov[i].iov_base, iov[i].iov_len,
					    sg, SMBDIRECT_SEND_IO_MAX_SGE - 1,
					    DMA_TO_DEVICE, &npages);
		if (sg_cnt <= 0) {
			pr_err("failed to map buffer\n");
			ret = -ENOMEM;
			goto err;
		} else if (sg_cnt + msg->num_sge > SMBDIRECT_SEND_IO_MAX_SGE) {
			pr_err("buffer not fitted into sges\n");
			ret = -E2BIG;
			ib_dma_unmap_sg(sc->ib.dev, sg, npages,
					DMA_TO_DEVICE);
			goto err;
		}

		for (j = 0; j < sg_cnt; j++) {
			sge = &msg->sge[msg->num_sge];
			sge->addr = sg_dma_address(&sg[j]);
			sge->length = sg_dma_len(&sg[j]);
			sge->lkey  = sc->ib.pd->local_dma_lkey;
			msg->num_sge++;
		}
	}

	ret = post_sendmsg(sc, send_ctx, msg);
	if (ret)
		goto err;

	/*
	 * From here msg is moved to send_ctx
	 * and we should not free it explicitly.
	 */

	if (send_ctx == &_send_ctx) {
		ret = smb_direct_flush_send_list(sc, send_ctx, true);
		if (ret)
			goto flush_failed;
	}

	return 0;
err:
	smb_direct_free_sendmsg(sc, msg);
flush_failed:
header_failed:
	atomic_inc(&sc->send_io.credits.count);
credit_failed:
	atomic_inc(&sc->send_io.lcredits.count);
lcredit_failed:
	atomic_add(send_ctx->credit, &sc->send_io.bcredits.count);
	send_ctx->credit = 0;
bcredit_failed:
>>>>>>> hardened/6.19
	return ret;
}

static int smb_direct_writev(struct ksmbd_transport *t,
			     struct kvec *iov, int niovs, int buflen,
			     bool need_invalidate, unsigned int remote_key)
{
	struct smb_direct_transport *st = SMBD_TRANS(t);
	struct smbdirect_socket *sc = st->socket;
	struct iov_iter iter;

	iov_iter_kvec(&iter, ITER_SOURCE, iov, niovs, buflen);

	return smbdirect_connection_send_iter(sc, &iter, 0,
					      need_invalidate, remote_key);
}

static int smb_direct_rdma_write(struct ksmbd_transport *t,
				 void *buf, unsigned int buflen,
				 struct smbdirect_buffer_descriptor_v1 *desc,
				 unsigned int desc_len)
{
	struct smb_direct_transport *st = SMBD_TRANS(t);
	struct smbdirect_socket *sc = st->socket;

	return smbdirect_connection_rdma_xmit(sc, buf, buflen,
					      desc, desc_len, false);
}

static int smb_direct_rdma_read(struct ksmbd_transport *t,
				void *buf, unsigned int buflen,
				struct smbdirect_buffer_descriptor_v1 *desc,
				unsigned int desc_len)
{
	struct smb_direct_transport *st = SMBD_TRANS(t);
	struct smbdirect_socket *sc = st->socket;

	return smbdirect_connection_rdma_xmit(sc, buf, buflen,
					      desc, desc_len, true);
}

static void smb_direct_disconnect(struct ksmbd_transport *t)
{
	struct smb_direct_transport *st = SMBD_TRANS(t);
	struct smbdirect_socket *sc = st->socket;

	ksmbd_debug(RDMA, "Disconnecting sc=%p\n", sc);

	free_transport(st);
}

static void smb_direct_shutdown(struct ksmbd_transport *t)
{
	struct smb_direct_transport *st = SMBD_TRANS(t);
	struct smbdirect_socket *sc = st->socket;

	ksmbd_debug(RDMA, "smb-direct shutdown sc=%p\n", sc);

	smbdirect_socket_shutdown(sc);
}

<<<<<<< HEAD
static int smb_direct_new_connection(struct smb_direct_listener *listener,
				     struct smbdirect_socket *client_sc)
||||||| 05f7e89ab9731
static int smb_direct_cm_handler(struct rdma_cm_id *cm_id,
				 struct rdma_cm_event *event)
{
	struct smbdirect_socket *sc = cm_id->context;
	unsigned long flags;

	ksmbd_debug(RDMA, "RDMA CM event. cm_id=%p event=%s (%d)\n",
		    cm_id, rdma_event_msg(event->event), event->event);

	switch (event->event) {
	case RDMA_CM_EVENT_ESTABLISHED: {
		/*
		 * Some drivers (at least mlx5_ib and irdma in roce mode)
		 * might post a recv completion before RDMA_CM_EVENT_ESTABLISHED,
		 * we need to adjust our expectation in that case.
		 *
		 * If smb_direct_negotiate_recv_done was called first
		 * it initialized sc->connect.work only for us to
		 * start, so that we turned into
		 * SMBDIRECT_SOCKET_NEGOTIATE_NEEDED, before
		 * smb_direct_negotiate_recv_work() runs.
		 *
		 * If smb_direct_negotiate_recv_done didn't happen
		 * yet. sc->connect.work is still be disabled and
		 * queue_work() is a no-op.
		 */
		if (SMBDIRECT_CHECK_STATUS_DISCONNECT(sc, SMBDIRECT_SOCKET_RDMA_CONNECT_RUNNING))
			break;
		sc->status = SMBDIRECT_SOCKET_NEGOTIATE_NEEDED;
		spin_lock_irqsave(&sc->connect.lock, flags);
		if (!sc->first_error)
			queue_work(sc->workqueue, &sc->connect.work);
		spin_unlock_irqrestore(&sc->connect.lock, flags);
		wake_up(&sc->status_wait);
		break;
	}
	case RDMA_CM_EVENT_DEVICE_REMOVAL:
	case RDMA_CM_EVENT_DISCONNECTED: {
		sc->status = SMBDIRECT_SOCKET_DISCONNECTED;
		smb_direct_disconnect_rdma_work(&sc->disconnect_work);
		if (sc->ib.qp)
			ib_drain_qp(sc->ib.qp);
		break;
	}
	case RDMA_CM_EVENT_CONNECT_ERROR: {
		sc->status = SMBDIRECT_SOCKET_DISCONNECTED;
		smb_direct_disconnect_rdma_work(&sc->disconnect_work);
		break;
	}
	default:
		pr_err("Unexpected RDMA CM event. cm_id=%p, event=%s (%d)\n",
		       cm_id, rdma_event_msg(event->event),
		       event->event);
		break;
	}
	return 0;
}

static void smb_direct_qpair_handler(struct ib_event *event, void *context)
{
	struct smbdirect_socket *sc = context;

	ksmbd_debug(RDMA, "Received QP event. cm_id=%p, event=%s (%d)\n",
		    sc->rdma.cm_id, ib_event_msg(event->event), event->event);

	switch (event->event) {
	case IB_EVENT_CQ_ERR:
	case IB_EVENT_QP_FATAL:
		smb_direct_disconnect_rdma_connection(sc);
		break;
	default:
		break;
	}
}

static int smb_direct_send_negotiate_response(struct smbdirect_socket *sc,
					      int failed)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	struct smbdirect_send_io *sendmsg;
	struct smbdirect_negotiate_resp *resp;
	int ret;

	sendmsg = smb_direct_alloc_sendmsg(sc);
	if (IS_ERR(sendmsg))
		return -ENOMEM;

	resp = (struct smbdirect_negotiate_resp *)sendmsg->packet;
	if (failed) {
		memset(resp, 0, sizeof(*resp));
		resp->min_version = SMB_DIRECT_VERSION_LE;
		resp->max_version = SMB_DIRECT_VERSION_LE;
		resp->status = STATUS_NOT_SUPPORTED;

		sc->status = SMBDIRECT_SOCKET_NEGOTIATE_FAILED;
	} else {
		resp->status = STATUS_SUCCESS;
		resp->min_version = SMB_DIRECT_VERSION_LE;
		resp->max_version = SMB_DIRECT_VERSION_LE;
		resp->negotiated_version = SMB_DIRECT_VERSION_LE;
		resp->reserved = 0;
		resp->credits_requested =
				cpu_to_le16(sp->send_credit_target);
		resp->credits_granted = cpu_to_le16(manage_credits_prior_sending(sc));
		resp->max_readwrite_size = cpu_to_le32(sp->max_read_write_size);
		resp->preferred_send_size = cpu_to_le32(sp->max_send_size);
		resp->max_receive_size = cpu_to_le32(sp->max_recv_size);
		resp->max_fragmented_size =
				cpu_to_le32(sp->max_fragmented_recv_size);

		sc->recv_io.expected = SMBDIRECT_EXPECT_DATA_TRANSFER;
		sc->status = SMBDIRECT_SOCKET_CONNECTED;
	}

	sendmsg->sge[0].addr = ib_dma_map_single(sc->ib.dev,
						 (void *)resp, sizeof(*resp),
						 DMA_TO_DEVICE);
	ret = ib_dma_mapping_error(sc->ib.dev, sendmsg->sge[0].addr);
	if (ret) {
		smb_direct_free_sendmsg(sc, sendmsg);
		return ret;
	}

	sendmsg->num_sge = 1;
	sendmsg->sge[0].length = sizeof(*resp);
	sendmsg->sge[0].lkey = sc->ib.pd->local_dma_lkey;

	ret = post_sendmsg(sc, NULL, sendmsg);
	if (ret) {
		smb_direct_free_sendmsg(sc, sendmsg);
		return ret;
	}

	wait_event(sc->send_io.pending.zero_wait_queue,
		   atomic_read(&sc->send_io.pending.count) == 0 ||
		   sc->status != SMBDIRECT_SOCKET_CONNECTED);
	if (sc->status != SMBDIRECT_SOCKET_CONNECTED)
		return -ENOTCONN;

	return 0;
}

static int smb_direct_accept_client(struct smbdirect_socket *sc)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	struct rdma_conn_param conn_param;
	__be32 ird_ord_hdr[2];
	int ret;

	/*
	 * smb_direct_handle_connect_request()
	 * already negotiated sp->initiator_depth
	 * and sp->responder_resources
	 */
	memset(&conn_param, 0, sizeof(conn_param));
	conn_param.initiator_depth = sp->initiator_depth;
	conn_param.responder_resources = sp->responder_resources;

	if (sc->rdma.legacy_iwarp) {
		ird_ord_hdr[0] = cpu_to_be32(conn_param.responder_resources);
		ird_ord_hdr[1] = cpu_to_be32(conn_param.initiator_depth);
		conn_param.private_data = ird_ord_hdr;
		conn_param.private_data_len = sizeof(ird_ord_hdr);
	} else {
		conn_param.private_data = NULL;
		conn_param.private_data_len = 0;
	}
	conn_param.retry_count = SMB_DIRECT_CM_RETRY;
	conn_param.rnr_retry_count = SMB_DIRECT_CM_RNR_RETRY;
	conn_param.flow_control = 0;

	/*
	 * start with the negotiate timeout and SMBDIRECT_KEEPALIVE_PENDING
	 * so that the timer will cause a disconnect.
	 */
	sc->idle.keepalive = SMBDIRECT_KEEPALIVE_PENDING;
	mod_delayed_work(sc->workqueue, &sc->idle.timer_work,
			 msecs_to_jiffies(sp->negotiate_timeout_msec));

	WARN_ON_ONCE(sc->status != SMBDIRECT_SOCKET_RDMA_CONNECT_NEEDED);
	sc->status = SMBDIRECT_SOCKET_RDMA_CONNECT_RUNNING;
	ret = rdma_accept(sc->rdma.cm_id, &conn_param);
	if (ret) {
		pr_err("error at rdma_accept: %d\n", ret);
		return ret;
	}
	return 0;
}

static int smb_direct_prepare_negotiation(struct smbdirect_socket *sc)
{
	struct smbdirect_recv_io *recvmsg;
	bool recv_posted = false;
	int ret;

	WARN_ON_ONCE(sc->status != SMBDIRECT_SOCKET_CREATED);
	sc->status = SMBDIRECT_SOCKET_RDMA_CONNECT_NEEDED;

	sc->recv_io.expected = SMBDIRECT_EXPECT_NEGOTIATE_REQ;

	recvmsg = get_free_recvmsg(sc);
	if (!recvmsg)
		return -ENOMEM;
	recvmsg->cqe.done = smb_direct_negotiate_recv_done;

	ret = smb_direct_post_recv(sc, recvmsg);
	if (ret) {
		pr_err("Can't post recv: %d\n", ret);
		goto out_err;
	}
	recv_posted = true;

	ret = smb_direct_accept_client(sc);
	if (ret) {
		pr_err("Can't accept client\n");
		goto out_err;
	}

	return 0;
out_err:
	/*
	 * If the recv was never posted, return it to the free list.
	 * If it was posted, leave it alone so disconnect teardown can
	 * drain the QP and complete it (flush) and the completion path
	 * will unmap it exactly once.
	 */
	if (!recv_posted)
		put_recvmsg(sc, recvmsg);
	return ret;
}

static int smb_direct_init_params(struct smbdirect_socket *sc)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	int max_send_sges;
	unsigned int maxpages;

	/* need 3 more sge. because a SMB_DIRECT header, SMB2 header,
	 * SMB2 response could be mapped.
	 */
	max_send_sges = DIV_ROUND_UP(sp->max_send_size, PAGE_SIZE) + 3;
	if (max_send_sges > SMBDIRECT_SEND_IO_MAX_SGE) {
		pr_err("max_send_size %d is too large\n", sp->max_send_size);
		return -EINVAL;
	}

	atomic_set(&sc->send_io.lcredits.count, sp->send_credit_target);

	maxpages = DIV_ROUND_UP(sp->max_read_write_size, PAGE_SIZE);
	sc->rw_io.credits.max = rdma_rw_mr_factor(sc->ib.dev,
						  sc->rdma.cm_id->port_num,
						  maxpages);
	sc->rw_io.credits.num_pages = DIV_ROUND_UP(maxpages, sc->rw_io.credits.max);
	/* add one extra in order to handle unaligned pages */
	sc->rw_io.credits.max += 1;

	sc->recv_io.credits.target = 1;

	atomic_set(&sc->rw_io.credits.count, sc->rw_io.credits.max);

	return 0;
}

static void smb_direct_destroy_pools(struct smbdirect_socket *sc)
{
	struct smbdirect_recv_io *recvmsg;

	while ((recvmsg = get_free_recvmsg(sc)))
		mempool_free(recvmsg, sc->recv_io.mem.pool);

	mempool_destroy(sc->recv_io.mem.pool);
	sc->recv_io.mem.pool = NULL;

	kmem_cache_destroy(sc->recv_io.mem.cache);
	sc->recv_io.mem.cache = NULL;

	mempool_destroy(sc->send_io.mem.pool);
	sc->send_io.mem.pool = NULL;

	kmem_cache_destroy(sc->send_io.mem.cache);
	sc->send_io.mem.cache = NULL;
}

static int smb_direct_create_pools(struct smbdirect_socket *sc)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	char name[80];
	int i;
	struct smbdirect_recv_io *recvmsg;

	snprintf(name, sizeof(name), "smbdirect_send_io_pool_%p", sc);
	sc->send_io.mem.cache = kmem_cache_create(name,
					     sizeof(struct smbdirect_send_io) +
					      sizeof(struct smbdirect_negotiate_resp),
					     0, SLAB_HWCACHE_ALIGN, NULL);
	if (!sc->send_io.mem.cache)
		return -ENOMEM;

	sc->send_io.mem.pool = mempool_create(sp->send_credit_target,
					    mempool_alloc_slab, mempool_free_slab,
					    sc->send_io.mem.cache);
	if (!sc->send_io.mem.pool)
		goto err;

	snprintf(name, sizeof(name), "smbdirect_recv_io_pool_%p", sc);
	sc->recv_io.mem.cache = kmem_cache_create(name,
					     sizeof(struct smbdirect_recv_io) +
					     sp->max_recv_size,
					     0, SLAB_HWCACHE_ALIGN, NULL);
	if (!sc->recv_io.mem.cache)
		goto err;

	sc->recv_io.mem.pool =
		mempool_create(sp->recv_credit_max, mempool_alloc_slab,
			       mempool_free_slab, sc->recv_io.mem.cache);
	if (!sc->recv_io.mem.pool)
		goto err;

	for (i = 0; i < sp->recv_credit_max; i++) {
		recvmsg = mempool_alloc(sc->recv_io.mem.pool, KSMBD_DEFAULT_GFP);
		if (!recvmsg)
			goto err;
		recvmsg->socket = sc;
		recvmsg->sge.length = 0;
		list_add(&recvmsg->list, &sc->recv_io.free.list);
	}

	return 0;
err:
	smb_direct_destroy_pools(sc);
	return -ENOMEM;
}

static u32 smb_direct_rdma_rw_send_wrs(struct ib_device *dev, const struct ib_qp_init_attr *attr)
{
	/*
	 * This could be split out of rdma_rw_init_qp()
	 * and be a helper function next to rdma_rw_mr_factor()
	 *
	 * We can't check unlikely(rdma_rw_force_mr) here,
	 * but that is most likely 0 anyway.
	 */
	u32 factor;

	WARN_ON_ONCE(attr->port_num == 0);

	/*
	 * Each context needs at least one RDMA READ or WRITE WR.
	 *
	 * For some hardware we might need more, eventually we should ask the
	 * HCA driver for a multiplier here.
	 */
	factor = 1;

	/*
	 * If the device needs MRs to perform RDMA READ or WRITE operations,
	 * we'll need two additional MRs for the registrations and the
	 * invalidation.
	 */
	if (rdma_protocol_iwarp(dev, attr->port_num) || dev->attrs.max_sgl_rd)
		factor += 2;	/* inv + reg */

	return factor * attr->cap.max_rdma_ctxs;
}

static int smb_direct_create_qpair(struct smbdirect_socket *sc)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	int ret;
	struct ib_qp_cap qp_cap;
	struct ib_qp_init_attr qp_attr;
	u32 max_send_wr;
	u32 rdma_send_wr;

	/*
	 * Note that {rdma,ib}_create_qp() will call
	 * rdma_rw_init_qp() if cap->max_rdma_ctxs is not 0.
	 * It will adjust cap->max_send_wr to the required
	 * number of additional WRs for the RDMA RW operations.
	 * It will cap cap->max_send_wr to the device limit.
	 *
	 * +1 for ib_drain_qp
	 */
	qp_cap.max_send_wr = sp->send_credit_target + 1;
	qp_cap.max_recv_wr = sp->recv_credit_max + 1;
	qp_cap.max_send_sge = SMBDIRECT_SEND_IO_MAX_SGE;
	qp_cap.max_recv_sge = SMBDIRECT_RECV_IO_MAX_SGE;
	qp_cap.max_inline_data = 0;
	qp_cap.max_rdma_ctxs = sc->rw_io.credits.max;

	/*
	 * Find out the number of max_send_wr
	 * after rdma_rw_init_qp() adjusted it.
	 *
	 * We only do it on a temporary variable,
	 * as rdma_create_qp() will trigger
	 * rdma_rw_init_qp() again.
	 */
	memset(&qp_attr, 0, sizeof(qp_attr));
	qp_attr.cap = qp_cap;
	qp_attr.port_num = sc->rdma.cm_id->port_num;
	rdma_send_wr = smb_direct_rdma_rw_send_wrs(sc->ib.dev, &qp_attr);
	max_send_wr = qp_cap.max_send_wr + rdma_send_wr;

	if (qp_cap.max_send_wr > sc->ib.dev->attrs.max_cqe ||
	    qp_cap.max_send_wr > sc->ib.dev->attrs.max_qp_wr) {
		pr_err("Possible CQE overrun: max_send_wr %d\n",
		       qp_cap.max_send_wr);
		pr_err("device %.*s reporting max_cqe %d max_qp_wr %d\n",
		       IB_DEVICE_NAME_MAX,
		       sc->ib.dev->name,
		       sc->ib.dev->attrs.max_cqe,
		       sc->ib.dev->attrs.max_qp_wr);
		pr_err("consider lowering send_credit_target = %d\n",
		       sp->send_credit_target);
		return -EINVAL;
	}

	if (qp_cap.max_rdma_ctxs &&
	    (max_send_wr >= sc->ib.dev->attrs.max_cqe ||
	     max_send_wr >= sc->ib.dev->attrs.max_qp_wr)) {
		pr_err("Possible CQE overrun: rdma_send_wr %d + max_send_wr %d = %d\n",
		       rdma_send_wr, qp_cap.max_send_wr, max_send_wr);
		pr_err("device %.*s reporting max_cqe %d max_qp_wr %d\n",
		       IB_DEVICE_NAME_MAX,
		       sc->ib.dev->name,
		       sc->ib.dev->attrs.max_cqe,
		       sc->ib.dev->attrs.max_qp_wr);
		pr_err("consider lowering send_credit_target = %d, max_rdma_ctxs = %d\n",
		       sp->send_credit_target, qp_cap.max_rdma_ctxs);
		return -EINVAL;
	}

	if (qp_cap.max_recv_wr > sc->ib.dev->attrs.max_cqe ||
	    qp_cap.max_recv_wr > sc->ib.dev->attrs.max_qp_wr) {
		pr_err("Possible CQE overrun: max_recv_wr %d\n",
		       qp_cap.max_recv_wr);
		pr_err("device %.*s reporting max_cqe %d max_qp_wr %d\n",
		       IB_DEVICE_NAME_MAX,
		       sc->ib.dev->name,
		       sc->ib.dev->attrs.max_cqe,
		       sc->ib.dev->attrs.max_qp_wr);
		pr_err("consider lowering receive_credit_max = %d\n",
		       sp->recv_credit_max);
		return -EINVAL;
	}

	if (qp_cap.max_send_sge > sc->ib.dev->attrs.max_send_sge ||
	    qp_cap.max_recv_sge > sc->ib.dev->attrs.max_recv_sge) {
		pr_err("device %.*s max_send_sge/max_recv_sge = %d/%d too small\n",
		       IB_DEVICE_NAME_MAX,
		       sc->ib.dev->name,
		       sc->ib.dev->attrs.max_send_sge,
		       sc->ib.dev->attrs.max_recv_sge);
		return -EINVAL;
	}

	sc->ib.pd = ib_alloc_pd(sc->ib.dev, 0);
	if (IS_ERR(sc->ib.pd)) {
		pr_err("Can't create RDMA PD\n");
		ret = PTR_ERR(sc->ib.pd);
		sc->ib.pd = NULL;
		return ret;
	}

	sc->ib.send_cq = ib_alloc_cq_any(sc->ib.dev, sc,
					 max_send_wr,
					 IB_POLL_WORKQUEUE);
	if (IS_ERR(sc->ib.send_cq)) {
		pr_err("Can't create RDMA send CQ\n");
		ret = PTR_ERR(sc->ib.send_cq);
		sc->ib.send_cq = NULL;
		goto err;
	}

	sc->ib.recv_cq = ib_alloc_cq_any(sc->ib.dev, sc,
					 qp_cap.max_recv_wr,
					 IB_POLL_WORKQUEUE);
	if (IS_ERR(sc->ib.recv_cq)) {
		pr_err("Can't create RDMA recv CQ\n");
		ret = PTR_ERR(sc->ib.recv_cq);
		sc->ib.recv_cq = NULL;
		goto err;
	}

	/*
	 * We reset completely here!
	 * As the above use was just temporary
	 * to calc max_send_wr and rdma_send_wr.
	 *
	 * rdma_create_qp() will trigger rdma_rw_init_qp()
	 * again if max_rdma_ctxs is not 0.
	 */
	memset(&qp_attr, 0, sizeof(qp_attr));
	qp_attr.event_handler = smb_direct_qpair_handler;
	qp_attr.qp_context = sc;
	qp_attr.cap = qp_cap;
	qp_attr.sq_sig_type = IB_SIGNAL_REQ_WR;
	qp_attr.qp_type = IB_QPT_RC;
	qp_attr.send_cq = sc->ib.send_cq;
	qp_attr.recv_cq = sc->ib.recv_cq;
	qp_attr.port_num = ~0;

	ret = rdma_create_qp(sc->rdma.cm_id, sc->ib.pd, &qp_attr);
	if (ret) {
		pr_err("Can't create RDMA QP: %d\n", ret);
		goto err;
	}

	sc->ib.qp = sc->rdma.cm_id->qp;
	sc->rdma.cm_id->event_handler = smb_direct_cm_handler;

	return 0;
err:
	if (sc->ib.qp) {
		sc->ib.qp = NULL;
		rdma_destroy_qp(sc->rdma.cm_id);
	}
	if (sc->ib.recv_cq) {
		ib_destroy_cq(sc->ib.recv_cq);
		sc->ib.recv_cq = NULL;
	}
	if (sc->ib.send_cq) {
		ib_destroy_cq(sc->ib.send_cq);
		sc->ib.send_cq = NULL;
	}
	if (sc->ib.pd) {
		ib_dealloc_pd(sc->ib.pd);
		sc->ib.pd = NULL;
	}
	return ret;
}

static int smb_direct_prepare(struct ksmbd_transport *t)
{
	struct smb_direct_transport *st = SMBD_TRANS(t);
	struct smbdirect_socket *sc = &st->socket;
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	struct smbdirect_recv_io *recvmsg;
	struct smbdirect_negotiate_req *req;
	unsigned long flags;
	int ret;

	/*
	 * We are waiting to pass the following states:
	 *
	 * SMBDIRECT_SOCKET_RDMA_CONNECT_NEEDED
	 * SMBDIRECT_SOCKET_RDMA_CONNECT_RUNNING
	 * SMBDIRECT_SOCKET_NEGOTIATE_NEEDED
	 *
	 * To finally get to SMBDIRECT_SOCKET_NEGOTIATE_RUNNING
	 * in order to continue below.
	 *
	 * Everything else is unexpected and an error.
	 */
	ksmbd_debug(RDMA, "Waiting for SMB_DIRECT negotiate request\n");
	ret = wait_event_interruptible_timeout(sc->status_wait,
					sc->status != SMBDIRECT_SOCKET_RDMA_CONNECT_NEEDED &&
					sc->status != SMBDIRECT_SOCKET_RDMA_CONNECT_RUNNING &&
					sc->status != SMBDIRECT_SOCKET_NEGOTIATE_NEEDED,
					msecs_to_jiffies(sp->negotiate_timeout_msec));
	if (ret <= 0 || sc->status != SMBDIRECT_SOCKET_NEGOTIATE_RUNNING)
		return ret < 0 ? ret : -ETIMEDOUT;

	recvmsg = get_first_reassembly(sc);
	if (!recvmsg)
		return -ECONNABORTED;

	ret = smb_direct_check_recvmsg(recvmsg);
	if (ret)
		goto put;

	req = (struct smbdirect_negotiate_req *)recvmsg->packet;
	sp->max_recv_size = min_t(int, sp->max_recv_size,
				  le32_to_cpu(req->preferred_send_size));
	sp->max_send_size = min_t(int, sp->max_send_size,
				  le32_to_cpu(req->max_receive_size));
	sp->max_fragmented_send_size =
		le32_to_cpu(req->max_fragmented_size);
	sp->max_fragmented_recv_size =
		(sp->recv_credit_max * sp->max_recv_size) / 2;
	sc->recv_io.credits.target = le16_to_cpu(req->credits_requested);
	sc->recv_io.credits.target = min_t(u16, sc->recv_io.credits.target, sp->recv_credit_max);
	sc->recv_io.credits.target = max_t(u16, sc->recv_io.credits.target, 1);

put:
	spin_lock_irqsave(&sc->recv_io.reassembly.lock, flags);
	sc->recv_io.reassembly.queue_length--;
	list_del(&recvmsg->list);
	spin_unlock_irqrestore(&sc->recv_io.reassembly.lock, flags);
	put_recvmsg(sc, recvmsg);

	if (ret == -ECONNABORTED)
		return ret;

	if (ret)
		goto respond;

	/*
	 * We negotiated with success, so we need to refill the recv queue.
	 * We do that with sc->idle.immediate_work still being disabled
	 * via smbdirect_socket_init(), so that queue_work(sc->workqueue,
	 * &sc->idle.immediate_work) in smb_direct_post_recv_credits()
	 * is a no-op.
	 *
	 * The message that grants the credits to the client is
	 * the negotiate response.
	 */
	INIT_WORK(&sc->recv_io.posted.refill_work, smb_direct_post_recv_credits);
	smb_direct_post_recv_credits(&sc->recv_io.posted.refill_work);
	if (unlikely(sc->first_error))
		return sc->first_error;
	INIT_WORK(&sc->idle.immediate_work, smb_direct_send_immediate_work);

respond:
	ret = smb_direct_send_negotiate_response(sc, ret);

	return ret;
}

static int smb_direct_connect(struct smbdirect_socket *sc)
{
	struct smbdirect_recv_io *recv_io;
	int ret;

	ret = smb_direct_init_params(sc);
	if (ret) {
		pr_err("Can't configure RDMA parameters\n");
		return ret;
	}

	ret = smb_direct_create_pools(sc);
	if (ret) {
		pr_err("Can't init RDMA pool: %d\n", ret);
		return ret;
	}

	list_for_each_entry(recv_io, &sc->recv_io.free.list, list)
		recv_io->cqe.done = recv_done;

	ret = smb_direct_create_qpair(sc);
	if (ret) {
		pr_err("Can't accept RDMA client: %d\n", ret);
		return ret;
	}

	ret = smb_direct_prepare_negotiation(sc);
	if (ret) {
		pr_err("Can't negotiate: %d\n", ret);
		return ret;
	}
	return 0;
}

static bool rdma_frwr_is_supported(struct ib_device_attr *attrs)
{
	if (!(attrs->device_cap_flags & IB_DEVICE_MEM_MGT_EXTENSIONS))
		return false;
	if (attrs->max_fast_reg_page_list_len == 0)
		return false;
	return true;
}

static int smb_direct_handle_connect_request(struct rdma_cm_id *new_cm_id,
					     struct rdma_cm_event *event)
=======
static int smb_direct_cm_handler(struct rdma_cm_id *cm_id,
				 struct rdma_cm_event *event)
{
	struct smbdirect_socket *sc = cm_id->context;
	unsigned long flags;

	ksmbd_debug(RDMA, "RDMA CM event. cm_id=%p event=%s (%d)\n",
		    cm_id, rdma_event_msg(event->event), event->event);

	switch (event->event) {
	case RDMA_CM_EVENT_ESTABLISHED: {
		/*
		 * Some drivers (at least mlx5_ib and irdma in roce mode)
		 * might post a recv completion before RDMA_CM_EVENT_ESTABLISHED,
		 * we need to adjust our expectation in that case.
		 *
		 * If smb_direct_negotiate_recv_done was called first
		 * it initialized sc->connect.work only for us to
		 * start, so that we turned into
		 * SMBDIRECT_SOCKET_NEGOTIATE_NEEDED, before
		 * smb_direct_negotiate_recv_work() runs.
		 *
		 * If smb_direct_negotiate_recv_done didn't happen
		 * yet. sc->connect.work is still be disabled and
		 * queue_work() is a no-op.
		 */
		if (SMBDIRECT_CHECK_STATUS_DISCONNECT(sc, SMBDIRECT_SOCKET_RDMA_CONNECT_RUNNING))
			break;
		sc->status = SMBDIRECT_SOCKET_NEGOTIATE_NEEDED;
		spin_lock_irqsave(&sc->connect.lock, flags);
		if (!sc->first_error)
			queue_work(sc->workqueue, &sc->connect.work);
		spin_unlock_irqrestore(&sc->connect.lock, flags);
		wake_up(&sc->status_wait);
		break;
	}
	case RDMA_CM_EVENT_DEVICE_REMOVAL:
	case RDMA_CM_EVENT_DISCONNECTED: {
		sc->status = SMBDIRECT_SOCKET_DISCONNECTED;
		smb_direct_disconnect_rdma_work(&sc->disconnect_work);
		if (sc->ib.qp)
			ib_drain_qp(sc->ib.qp);
		break;
	}
	case RDMA_CM_EVENT_CONNECT_ERROR: {
		sc->status = SMBDIRECT_SOCKET_DISCONNECTED;
		smb_direct_disconnect_rdma_work(&sc->disconnect_work);
		break;
	}
	default:
		pr_err("Unexpected RDMA CM event. cm_id=%p, event=%s (%d)\n",
		       cm_id, rdma_event_msg(event->event),
		       event->event);
		break;
	}
	return 0;
}

static void smb_direct_qpair_handler(struct ib_event *event, void *context)
{
	struct smbdirect_socket *sc = context;

	ksmbd_debug(RDMA, "Received QP event. cm_id=%p, event=%s (%d)\n",
		    sc->rdma.cm_id, ib_event_msg(event->event), event->event);

	switch (event->event) {
	case IB_EVENT_CQ_ERR:
	case IB_EVENT_QP_FATAL:
		smb_direct_disconnect_rdma_connection(sc);
		break;
	default:
		break;
	}
}

static int smb_direct_send_negotiate_response(struct smbdirect_socket *sc,
					      int failed)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	struct smbdirect_send_io *sendmsg;
	struct smbdirect_negotiate_resp *resp;
	int ret;

	sendmsg = smb_direct_alloc_sendmsg(sc);
	if (IS_ERR(sendmsg))
		return -ENOMEM;

	resp = (struct smbdirect_negotiate_resp *)sendmsg->packet;
	if (failed) {
		memset(resp, 0, sizeof(*resp));
		resp->min_version = SMB_DIRECT_VERSION_LE;
		resp->max_version = SMB_DIRECT_VERSION_LE;
		resp->status = STATUS_NOT_SUPPORTED;

		sc->status = SMBDIRECT_SOCKET_NEGOTIATE_FAILED;
	} else {
		resp->status = STATUS_SUCCESS;
		resp->min_version = SMB_DIRECT_VERSION_LE;
		resp->max_version = SMB_DIRECT_VERSION_LE;
		resp->negotiated_version = SMB_DIRECT_VERSION_LE;
		resp->reserved = 0;
		resp->credits_requested =
				cpu_to_le16(sp->send_credit_target);
		resp->credits_granted = cpu_to_le16(manage_credits_prior_sending(sc));
		resp->max_readwrite_size = cpu_to_le32(sp->max_read_write_size);
		resp->preferred_send_size = cpu_to_le32(sp->max_send_size);
		resp->max_receive_size = cpu_to_le32(sp->max_recv_size);
		resp->max_fragmented_size =
				cpu_to_le32(sp->max_fragmented_recv_size);

		atomic_set(&sc->send_io.bcredits.count, 1);
		sc->recv_io.expected = SMBDIRECT_EXPECT_DATA_TRANSFER;
		sc->status = SMBDIRECT_SOCKET_CONNECTED;
	}

	sendmsg->sge[0].addr = ib_dma_map_single(sc->ib.dev,
						 (void *)resp, sizeof(*resp),
						 DMA_TO_DEVICE);
	ret = ib_dma_mapping_error(sc->ib.dev, sendmsg->sge[0].addr);
	if (ret) {
		smb_direct_free_sendmsg(sc, sendmsg);
		return ret;
	}

	sendmsg->num_sge = 1;
	sendmsg->sge[0].length = sizeof(*resp);
	sendmsg->sge[0].lkey = sc->ib.pd->local_dma_lkey;

	ret = post_sendmsg(sc, NULL, sendmsg);
	if (ret) {
		smb_direct_free_sendmsg(sc, sendmsg);
		return ret;
	}

	wait_event(sc->send_io.pending.zero_wait_queue,
		   atomic_read(&sc->send_io.pending.count) == 0 ||
		   sc->status != SMBDIRECT_SOCKET_CONNECTED);
	if (sc->status != SMBDIRECT_SOCKET_CONNECTED)
		return -ENOTCONN;

	return 0;
}

static int smb_direct_accept_client(struct smbdirect_socket *sc)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	struct rdma_conn_param conn_param;
	__be32 ird_ord_hdr[2];
	int ret;

	/*
	 * smb_direct_handle_connect_request()
	 * already negotiated sp->initiator_depth
	 * and sp->responder_resources
	 */
	memset(&conn_param, 0, sizeof(conn_param));
	conn_param.initiator_depth = sp->initiator_depth;
	conn_param.responder_resources = sp->responder_resources;

	if (sc->rdma.legacy_iwarp) {
		ird_ord_hdr[0] = cpu_to_be32(conn_param.responder_resources);
		ird_ord_hdr[1] = cpu_to_be32(conn_param.initiator_depth);
		conn_param.private_data = ird_ord_hdr;
		conn_param.private_data_len = sizeof(ird_ord_hdr);
	} else {
		conn_param.private_data = NULL;
		conn_param.private_data_len = 0;
	}
	conn_param.retry_count = SMB_DIRECT_CM_RETRY;
	conn_param.rnr_retry_count = SMB_DIRECT_CM_RNR_RETRY;
	conn_param.flow_control = 0;

	/*
	 * start with the negotiate timeout and SMBDIRECT_KEEPALIVE_PENDING
	 * so that the timer will cause a disconnect.
	 */
	sc->idle.keepalive = SMBDIRECT_KEEPALIVE_PENDING;
	mod_delayed_work(sc->workqueue, &sc->idle.timer_work,
			 msecs_to_jiffies(sp->negotiate_timeout_msec));

	WARN_ON_ONCE(sc->status != SMBDIRECT_SOCKET_RDMA_CONNECT_NEEDED);
	sc->status = SMBDIRECT_SOCKET_RDMA_CONNECT_RUNNING;
	ret = rdma_accept(sc->rdma.cm_id, &conn_param);
	if (ret) {
		pr_err("error at rdma_accept: %d\n", ret);
		return ret;
	}
	return 0;
}

static int smb_direct_prepare_negotiation(struct smbdirect_socket *sc)
{
	struct smbdirect_recv_io *recvmsg;
	bool recv_posted = false;
	int ret;

	WARN_ON_ONCE(sc->status != SMBDIRECT_SOCKET_CREATED);
	sc->status = SMBDIRECT_SOCKET_RDMA_CONNECT_NEEDED;

	sc->recv_io.expected = SMBDIRECT_EXPECT_NEGOTIATE_REQ;

	recvmsg = get_free_recvmsg(sc);
	if (!recvmsg)
		return -ENOMEM;
	recvmsg->cqe.done = smb_direct_negotiate_recv_done;

	ret = smb_direct_post_recv(sc, recvmsg);
	if (ret) {
		pr_err("Can't post recv: %d\n", ret);
		goto out_err;
	}
	recv_posted = true;

	ret = smb_direct_accept_client(sc);
	if (ret) {
		pr_err("Can't accept client\n");
		goto out_err;
	}

	return 0;
out_err:
	/*
	 * If the recv was never posted, return it to the free list.
	 * If it was posted, leave it alone so disconnect teardown can
	 * drain the QP and complete it (flush) and the completion path
	 * will unmap it exactly once.
	 */
	if (!recv_posted)
		put_recvmsg(sc, recvmsg);
	return ret;
}

static int smb_direct_init_params(struct smbdirect_socket *sc)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	int max_send_sges;
	unsigned int maxpages;

	/* need 3 more sge. because a SMB_DIRECT header, SMB2 header,
	 * SMB2 response could be mapped.
	 */
	max_send_sges = DIV_ROUND_UP(sp->max_send_size, PAGE_SIZE) + 3;
	if (max_send_sges > SMBDIRECT_SEND_IO_MAX_SGE) {
		pr_err("max_send_size %d is too large\n", sp->max_send_size);
		return -EINVAL;
	}

	atomic_set(&sc->send_io.lcredits.count, sp->send_credit_target);

	maxpages = DIV_ROUND_UP(sp->max_read_write_size, PAGE_SIZE);
	sc->rw_io.credits.max = rdma_rw_mr_factor(sc->ib.dev,
						  sc->rdma.cm_id->port_num,
						  maxpages);
	sc->rw_io.credits.num_pages = DIV_ROUND_UP(maxpages, sc->rw_io.credits.max);
	/* add one extra in order to handle unaligned pages */
	sc->rw_io.credits.max += 1;

	sc->recv_io.credits.target = 1;

	atomic_set(&sc->rw_io.credits.count, sc->rw_io.credits.max);

	return 0;
}

static void smb_direct_destroy_pools(struct smbdirect_socket *sc)
{
	struct smbdirect_recv_io *recvmsg;

	while ((recvmsg = get_free_recvmsg(sc)))
		mempool_free(recvmsg, sc->recv_io.mem.pool);

	mempool_destroy(sc->recv_io.mem.pool);
	sc->recv_io.mem.pool = NULL;

	kmem_cache_destroy(sc->recv_io.mem.cache);
	sc->recv_io.mem.cache = NULL;

	mempool_destroy(sc->send_io.mem.pool);
	sc->send_io.mem.pool = NULL;

	kmem_cache_destroy(sc->send_io.mem.cache);
	sc->send_io.mem.cache = NULL;
}

static int smb_direct_create_pools(struct smbdirect_socket *sc)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	char name[80];
	int i;
	struct smbdirect_recv_io *recvmsg;

	snprintf(name, sizeof(name), "smbdirect_send_io_pool_%p", sc);
	sc->send_io.mem.cache = kmem_cache_create(name,
					     sizeof(struct smbdirect_send_io) +
					      sizeof(struct smbdirect_negotiate_resp),
					     0, SLAB_HWCACHE_ALIGN, NULL);
	if (!sc->send_io.mem.cache)
		return -ENOMEM;

	sc->send_io.mem.pool = mempool_create(sp->send_credit_target,
					    mempool_alloc_slab, mempool_free_slab,
					    sc->send_io.mem.cache);
	if (!sc->send_io.mem.pool)
		goto err;

	snprintf(name, sizeof(name), "smbdirect_recv_io_pool_%p", sc);
	sc->recv_io.mem.cache = kmem_cache_create(name,
					     sizeof(struct smbdirect_recv_io) +
					     sp->max_recv_size,
					     0, SLAB_HWCACHE_ALIGN, NULL);
	if (!sc->recv_io.mem.cache)
		goto err;

	sc->recv_io.mem.pool =
		mempool_create(sp->recv_credit_max, mempool_alloc_slab,
			       mempool_free_slab, sc->recv_io.mem.cache);
	if (!sc->recv_io.mem.pool)
		goto err;

	for (i = 0; i < sp->recv_credit_max; i++) {
		recvmsg = mempool_alloc(sc->recv_io.mem.pool, KSMBD_DEFAULT_GFP);
		if (!recvmsg)
			goto err;
		recvmsg->socket = sc;
		recvmsg->sge.length = 0;
		list_add(&recvmsg->list, &sc->recv_io.free.list);
	}

	return 0;
err:
	smb_direct_destroy_pools(sc);
	return -ENOMEM;
}

static u32 smb_direct_rdma_rw_send_wrs(struct ib_device *dev, const struct ib_qp_init_attr *attr)
{
	/*
	 * This could be split out of rdma_rw_init_qp()
	 * and be a helper function next to rdma_rw_mr_factor()
	 *
	 * We can't check unlikely(rdma_rw_force_mr) here,
	 * but that is most likely 0 anyway.
	 */
	u32 factor;

	WARN_ON_ONCE(attr->port_num == 0);

	/*
	 * Each context needs at least one RDMA READ or WRITE WR.
	 *
	 * For some hardware we might need more, eventually we should ask the
	 * HCA driver for a multiplier here.
	 */
	factor = 1;

	/*
	 * If the device needs MRs to perform RDMA READ or WRITE operations,
	 * we'll need two additional MRs for the registrations and the
	 * invalidation.
	 */
	if (rdma_protocol_iwarp(dev, attr->port_num) || dev->attrs.max_sgl_rd)
		factor += 2;	/* inv + reg */

	return factor * attr->cap.max_rdma_ctxs;
}

static int smb_direct_create_qpair(struct smbdirect_socket *sc)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	int ret;
	struct ib_qp_cap qp_cap;
	struct ib_qp_init_attr qp_attr;
	u32 max_send_wr;
	u32 rdma_send_wr;

	/*
	 * Note that {rdma,ib}_create_qp() will call
	 * rdma_rw_init_qp() if cap->max_rdma_ctxs is not 0.
	 * It will adjust cap->max_send_wr to the required
	 * number of additional WRs for the RDMA RW operations.
	 * It will cap cap->max_send_wr to the device limit.
	 *
	 * +1 for ib_drain_qp
	 */
	qp_cap.max_send_wr = sp->send_credit_target + 1;
	qp_cap.max_recv_wr = sp->recv_credit_max + 1;
	qp_cap.max_send_sge = SMBDIRECT_SEND_IO_MAX_SGE;
	qp_cap.max_recv_sge = SMBDIRECT_RECV_IO_MAX_SGE;
	qp_cap.max_inline_data = 0;
	qp_cap.max_rdma_ctxs = sc->rw_io.credits.max;

	/*
	 * Find out the number of max_send_wr
	 * after rdma_rw_init_qp() adjusted it.
	 *
	 * We only do it on a temporary variable,
	 * as rdma_create_qp() will trigger
	 * rdma_rw_init_qp() again.
	 */
	memset(&qp_attr, 0, sizeof(qp_attr));
	qp_attr.cap = qp_cap;
	qp_attr.port_num = sc->rdma.cm_id->port_num;
	rdma_send_wr = smb_direct_rdma_rw_send_wrs(sc->ib.dev, &qp_attr);
	max_send_wr = qp_cap.max_send_wr + rdma_send_wr;

	if (qp_cap.max_send_wr > sc->ib.dev->attrs.max_cqe ||
	    qp_cap.max_send_wr > sc->ib.dev->attrs.max_qp_wr) {
		pr_err("Possible CQE overrun: max_send_wr %d\n",
		       qp_cap.max_send_wr);
		pr_err("device %.*s reporting max_cqe %d max_qp_wr %d\n",
		       IB_DEVICE_NAME_MAX,
		       sc->ib.dev->name,
		       sc->ib.dev->attrs.max_cqe,
		       sc->ib.dev->attrs.max_qp_wr);
		pr_err("consider lowering send_credit_target = %d\n",
		       sp->send_credit_target);
		return -EINVAL;
	}

	if (qp_cap.max_rdma_ctxs &&
	    (max_send_wr >= sc->ib.dev->attrs.max_cqe ||
	     max_send_wr >= sc->ib.dev->attrs.max_qp_wr)) {
		pr_err("Possible CQE overrun: rdma_send_wr %d + max_send_wr %d = %d\n",
		       rdma_send_wr, qp_cap.max_send_wr, max_send_wr);
		pr_err("device %.*s reporting max_cqe %d max_qp_wr %d\n",
		       IB_DEVICE_NAME_MAX,
		       sc->ib.dev->name,
		       sc->ib.dev->attrs.max_cqe,
		       sc->ib.dev->attrs.max_qp_wr);
		pr_err("consider lowering send_credit_target = %d, max_rdma_ctxs = %d\n",
		       sp->send_credit_target, qp_cap.max_rdma_ctxs);
		return -EINVAL;
	}

	if (qp_cap.max_recv_wr > sc->ib.dev->attrs.max_cqe ||
	    qp_cap.max_recv_wr > sc->ib.dev->attrs.max_qp_wr) {
		pr_err("Possible CQE overrun: max_recv_wr %d\n",
		       qp_cap.max_recv_wr);
		pr_err("device %.*s reporting max_cqe %d max_qp_wr %d\n",
		       IB_DEVICE_NAME_MAX,
		       sc->ib.dev->name,
		       sc->ib.dev->attrs.max_cqe,
		       sc->ib.dev->attrs.max_qp_wr);
		pr_err("consider lowering receive_credit_max = %d\n",
		       sp->recv_credit_max);
		return -EINVAL;
	}

	if (qp_cap.max_send_sge > sc->ib.dev->attrs.max_send_sge ||
	    qp_cap.max_recv_sge > sc->ib.dev->attrs.max_recv_sge) {
		pr_err("device %.*s max_send_sge/max_recv_sge = %d/%d too small\n",
		       IB_DEVICE_NAME_MAX,
		       sc->ib.dev->name,
		       sc->ib.dev->attrs.max_send_sge,
		       sc->ib.dev->attrs.max_recv_sge);
		return -EINVAL;
	}

	sc->ib.pd = ib_alloc_pd(sc->ib.dev, 0);
	if (IS_ERR(sc->ib.pd)) {
		pr_err("Can't create RDMA PD\n");
		ret = PTR_ERR(sc->ib.pd);
		sc->ib.pd = NULL;
		return ret;
	}

	sc->ib.send_cq = ib_alloc_cq_any(sc->ib.dev, sc,
					 max_send_wr,
					 IB_POLL_WORKQUEUE);
	if (IS_ERR(sc->ib.send_cq)) {
		pr_err("Can't create RDMA send CQ\n");
		ret = PTR_ERR(sc->ib.send_cq);
		sc->ib.send_cq = NULL;
		goto err;
	}

	sc->ib.recv_cq = ib_alloc_cq_any(sc->ib.dev, sc,
					 qp_cap.max_recv_wr,
					 IB_POLL_WORKQUEUE);
	if (IS_ERR(sc->ib.recv_cq)) {
		pr_err("Can't create RDMA recv CQ\n");
		ret = PTR_ERR(sc->ib.recv_cq);
		sc->ib.recv_cq = NULL;
		goto err;
	}

	/*
	 * We reset completely here!
	 * As the above use was just temporary
	 * to calc max_send_wr and rdma_send_wr.
	 *
	 * rdma_create_qp() will trigger rdma_rw_init_qp()
	 * again if max_rdma_ctxs is not 0.
	 */
	memset(&qp_attr, 0, sizeof(qp_attr));
	qp_attr.event_handler = smb_direct_qpair_handler;
	qp_attr.qp_context = sc;
	qp_attr.cap = qp_cap;
	qp_attr.sq_sig_type = IB_SIGNAL_REQ_WR;
	qp_attr.qp_type = IB_QPT_RC;
	qp_attr.send_cq = sc->ib.send_cq;
	qp_attr.recv_cq = sc->ib.recv_cq;
	qp_attr.port_num = ~0;

	ret = rdma_create_qp(sc->rdma.cm_id, sc->ib.pd, &qp_attr);
	if (ret) {
		pr_err("Can't create RDMA QP: %d\n", ret);
		goto err;
	}

	sc->ib.qp = sc->rdma.cm_id->qp;
	sc->rdma.cm_id->event_handler = smb_direct_cm_handler;

	return 0;
err:
	if (sc->ib.qp) {
		sc->ib.qp = NULL;
		rdma_destroy_qp(sc->rdma.cm_id);
	}
	if (sc->ib.recv_cq) {
		ib_destroy_cq(sc->ib.recv_cq);
		sc->ib.recv_cq = NULL;
	}
	if (sc->ib.send_cq) {
		ib_destroy_cq(sc->ib.send_cq);
		sc->ib.send_cq = NULL;
	}
	if (sc->ib.pd) {
		ib_dealloc_pd(sc->ib.pd);
		sc->ib.pd = NULL;
	}
	return ret;
}

static int smb_direct_prepare(struct ksmbd_transport *t)
{
	struct smb_direct_transport *st = SMBD_TRANS(t);
	struct smbdirect_socket *sc = &st->socket;
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	struct smbdirect_recv_io *recvmsg;
	struct smbdirect_negotiate_req *req;
	unsigned long flags;
	int ret;

	/*
	 * We are waiting to pass the following states:
	 *
	 * SMBDIRECT_SOCKET_RDMA_CONNECT_NEEDED
	 * SMBDIRECT_SOCKET_RDMA_CONNECT_RUNNING
	 * SMBDIRECT_SOCKET_NEGOTIATE_NEEDED
	 *
	 * To finally get to SMBDIRECT_SOCKET_NEGOTIATE_RUNNING
	 * in order to continue below.
	 *
	 * Everything else is unexpected and an error.
	 */
	ksmbd_debug(RDMA, "Waiting for SMB_DIRECT negotiate request\n");
	ret = wait_event_interruptible_timeout(sc->status_wait,
					sc->status != SMBDIRECT_SOCKET_RDMA_CONNECT_NEEDED &&
					sc->status != SMBDIRECT_SOCKET_RDMA_CONNECT_RUNNING &&
					sc->status != SMBDIRECT_SOCKET_NEGOTIATE_NEEDED,
					msecs_to_jiffies(sp->negotiate_timeout_msec));
	if (ret <= 0 || sc->status != SMBDIRECT_SOCKET_NEGOTIATE_RUNNING)
		return ret < 0 ? ret : -ETIMEDOUT;

	recvmsg = get_first_reassembly(sc);
	if (!recvmsg)
		return -ECONNABORTED;

	ret = smb_direct_check_recvmsg(recvmsg);
	if (ret)
		goto put;

	req = (struct smbdirect_negotiate_req *)recvmsg->packet;
	sp->max_recv_size = min_t(u32, sp->max_recv_size,
				  le32_to_cpu(req->preferred_send_size));
	sp->max_send_size = min_t(u32, sp->max_send_size,
				  le32_to_cpu(req->max_receive_size));
	sp->max_fragmented_send_size =
		le32_to_cpu(req->max_fragmented_size);
	sp->max_fragmented_recv_size =
		(sp->recv_credit_max * sp->max_recv_size) / 2;
	sc->recv_io.credits.target = le16_to_cpu(req->credits_requested);
	sc->recv_io.credits.target = min_t(u16, sc->recv_io.credits.target, sp->recv_credit_max);
	sc->recv_io.credits.target = max_t(u16, sc->recv_io.credits.target, 1);

put:
	spin_lock_irqsave(&sc->recv_io.reassembly.lock, flags);
	sc->recv_io.reassembly.queue_length--;
	list_del(&recvmsg->list);
	spin_unlock_irqrestore(&sc->recv_io.reassembly.lock, flags);
	put_recvmsg(sc, recvmsg);

	if (ret == -ECONNABORTED)
		return ret;

	if (ret)
		goto respond;

	/*
	 * We negotiated with success, so we need to refill the recv queue.
	 * We do that with sc->idle.immediate_work still being disabled
	 * via smbdirect_socket_init(), so that queue_work(sc->workqueue,
	 * &sc->idle.immediate_work) in smb_direct_post_recv_credits()
	 * is a no-op.
	 *
	 * The message that grants the credits to the client is
	 * the negotiate response.
	 */
	INIT_WORK(&sc->recv_io.posted.refill_work, smb_direct_post_recv_credits);
	smb_direct_post_recv_credits(&sc->recv_io.posted.refill_work);
	if (unlikely(sc->first_error))
		return sc->first_error;
	INIT_WORK(&sc->idle.immediate_work, smb_direct_send_immediate_work);

respond:
	ret = smb_direct_send_negotiate_response(sc, ret);

	return ret;
}

static int smb_direct_connect(struct smbdirect_socket *sc)
{
	struct smbdirect_recv_io *recv_io;
	int ret;

	ret = smb_direct_init_params(sc);
	if (ret) {
		pr_err("Can't configure RDMA parameters\n");
		return ret;
	}

	ret = smb_direct_create_pools(sc);
	if (ret) {
		pr_err("Can't init RDMA pool: %d\n", ret);
		return ret;
	}

	list_for_each_entry(recv_io, &sc->recv_io.free.list, list)
		recv_io->cqe.done = recv_done;

	ret = smb_direct_create_qpair(sc);
	if (ret) {
		pr_err("Can't accept RDMA client: %d\n", ret);
		return ret;
	}

	ret = smb_direct_prepare_negotiation(sc);
	if (ret) {
		pr_err("Can't negotiate: %d\n", ret);
		return ret;
	}
	return 0;
}

static bool rdma_frwr_is_supported(struct ib_device_attr *attrs)
{
	if (!(attrs->device_cap_flags & IB_DEVICE_MEM_MGT_EXTENSIONS))
		return false;
	if (attrs->max_fast_reg_page_list_len == 0)
		return false;
	return true;
}

static int smb_direct_handle_connect_request(struct rdma_cm_id *new_cm_id,
					     struct rdma_cm_event *event)
>>>>>>> hardened/6.19
{
	struct smb_direct_transport *t;
	struct task_struct *handler;
	int ret;

	t = alloc_transport(client_sc);
	if (!t) {
		smbdirect_socket_release(client_sc);
		return -ENOMEM;
	}

	handler = kthread_run(ksmbd_conn_handler_loop,
			      KSMBD_TRANS(t)->conn, "ksmbd:r%u",
			      listener->port);
	if (IS_ERR(handler)) {
		ret = PTR_ERR(handler);
		pr_err("Can't start thread\n");
		goto out_err;
	}

	return 0;
out_err:
	free_transport(t);
	return ret;
}

static int smb_direct_listener_kthread_fn(void *p)
{
	struct smb_direct_listener *listener = (struct smb_direct_listener *)p;
	struct smbdirect_socket *client_sc = NULL;

	while (!kthread_should_stop()) {
		struct proto_accept_arg arg = { .err = -EINVAL, };
		long timeo = MAX_SCHEDULE_TIMEOUT;

		if (!listener->socket)
			break;
		client_sc = smbdirect_socket_accept(listener->socket, timeo, &arg);
		if (!client_sc && arg.err == -EINVAL)
			break;
		if (!client_sc)
			continue;

		ksmbd_debug(CONN, "connect success: accepted new connection\n");
		smb_direct_new_connection(listener, client_sc);
	}

	ksmbd_debug(CONN, "releasing socket\n");
	return 0;
}

static void smb_direct_listener_destroy(struct smb_direct_listener *listener)
{
	int ret;

	if (listener->socket)
		smbdirect_socket_shutdown(listener->socket);

	if (listener->thread) {
		ret = kthread_stop(listener->thread);
		if (ret)
			pr_err("failed to stop forker thread\n");
		listener->thread = NULL;
	}

	if (listener->socket) {
		smbdirect_socket_release(listener->socket);
		listener->socket = NULL;
	}

	listener->port = 0;
}

static int smb_direct_listen(struct smb_direct_listener *listener,
			     int port)
{
	struct net *net = current->nsproxy->net_ns;
	struct task_struct *kthread;
	struct sockaddr_in sin = {
		.sin_family		= AF_INET,
		.sin_addr.s_addr	= htonl(INADDR_ANY),
		.sin_port		= htons(port),
	};
	struct smbdirect_socket_parameters init_params = {};
	struct smbdirect_socket_parameters *sp;
	struct smbdirect_socket *sc;
	u64 port_flags = 0;
	int ret;

	switch (port) {
	case SMB_DIRECT_PORT_IWARP:
		/*
		 * only allow iWarp devices
		 * for port 5445.
		 */
		port_flags |= SMBDIRECT_FLAG_PORT_RANGE_ONLY_IW;
		break;
	case SMB_DIRECT_PORT_INFINIBAND:
		/*
		 * only allow InfiniBand, RoCEv1 or RoCEv2
		 * devices for port 445.
		 *
		 * (Basically don't allow iWarp devices)
		 */
		port_flags |= SMBDIRECT_FLAG_PORT_RANGE_ONLY_IB;
		break;
	default:
		pr_err("unsupported smbdirect port=%d!\n", port);
		return -ENODEV;
	}

	ret = smbdirect_socket_create_kern(net, &sc);
	if (ret) {
		pr_err("smbdirect_socket_create_kern() failed: %d %1pe\n",
		       ret, ERR_PTR(ret));
		return ret;
	}

	/*
	 * Create the initial parameters
	 */
	sp = &init_params;
	sp->flags |= port_flags;
	sp->negotiate_timeout_msec = SMB_DIRECT_NEGOTIATE_TIMEOUT * 1000;
	sp->initiator_depth = SMB_DIRECT_CM_INITIATOR_DEPTH;
	sp->responder_resources = 1;
	sp->recv_credit_max = smb_direct_receive_credit_max;
	sp->send_credit_target = smb_direct_send_credit_target;
	sp->max_send_size = smb_direct_max_send_size;
	sp->max_fragmented_recv_size = smb_direct_max_fragmented_recv_size;
	sp->max_recv_size = smb_direct_max_receive_size;
	sp->max_read_write_size = smb_direct_max_read_write_size;
	sp->keepalive_interval_msec = SMB_DIRECT_KEEPALIVE_SEND_INTERVAL * 1000;
	sp->keepalive_timeout_msec = SMB_DIRECT_KEEPALIVE_RECV_TIMEOUT * 1000;

	smbdirect_socket_set_logging(sc, NULL,
				     smb_direct_logging_needed,
				     smb_direct_logging_vaprintf);
	ret = smbdirect_socket_set_initial_parameters(sc, sp);
	if (ret) {
		pr_err("Failed smbdirect_socket_set_initial_parameters(): %d %1pe\n",
		       ret, ERR_PTR(ret));
		goto err;
	}
	ret = smbdirect_socket_set_kernel_settings(sc, IB_POLL_WORKQUEUE, KSMBD_DEFAULT_GFP);
	if (ret) {
		pr_err("Failed smbdirect_socket_set_kernel_settings(): %d %1pe\n",
		       ret, ERR_PTR(ret));
		goto err;
	}

	ret = smbdirect_socket_bind(sc, (struct sockaddr *)&sin);
	if (ret) {
		pr_err("smbdirect_socket_bind() failed: %d %1pe\n",
		       ret, ERR_PTR(ret));
		goto err;
	}

	ret = smbdirect_socket_listen(sc, 10);
	if (ret) {
		pr_err("Port[%d] smbdirect_socket_listen() failed: %d %1pe\n",
		       port, ret, ERR_PTR(ret));
		goto err;
	}

	listener->port = port;
	listener->socket = sc;

	kthread = kthread_run(smb_direct_listener_kthread_fn,
			      listener,
			      "ksmbd-smbdirect-listener-%u", port);
	if (IS_ERR(kthread)) {
		ret = PTR_ERR(kthread);
		pr_err("Can't start ksmbd listen kthread: %d %1pe\n",
		       ret, ERR_PTR(ret));
		goto err;
	}

	listener->thread = kthread;
	return 0;
err:
	smb_direct_listener_destroy(listener);
	return ret;
}

int ksmbd_rdma_init(void)
{
	int ret;

	smb_direct_ib_listener = smb_direct_iw_listener = (struct smb_direct_listener) {
		.socket = NULL,
	};

	ret = smb_direct_listen(&smb_direct_ib_listener,
				SMB_DIRECT_PORT_INFINIBAND);
	if (ret) {
		pr_err("Can't listen on InfiniBand/RoCEv1/RoCEv2: %d\n", ret);
		goto err;
	}

	ksmbd_debug(RDMA, "InfiniBand/RoCEv1/RoCEv2 RDMA listener. socket=%p\n",
		    smb_direct_ib_listener.socket);

	ret = smb_direct_listen(&smb_direct_iw_listener,
				SMB_DIRECT_PORT_IWARP);
	if (ret) {
		pr_err("Can't listen on iWarp: %d\n", ret);
		goto err;
	}

	ksmbd_debug(RDMA, "iWarp RDMA listener. socket=%p\n",
		    smb_direct_iw_listener.socket);

	return 0;
err:
	ksmbd_rdma_stop_listening();
	return ret;
}

void ksmbd_rdma_stop_listening(void)
{
	smb_direct_listener_destroy(&smb_direct_ib_listener);
	smb_direct_listener_destroy(&smb_direct_iw_listener);
}

bool ksmbd_rdma_capable_netdev(struct net_device *netdev)
{
	u8 node_type = smbdirect_netdev_rdma_capable_node_type(netdev);

	return node_type != RDMA_NODE_UNSPECIFIED;
}

static const struct ksmbd_transport_ops ksmbd_smb_direct_transport_ops = {
	.disconnect	= smb_direct_disconnect,
	.shutdown	= smb_direct_shutdown,
	.writev		= smb_direct_writev,
	.read		= smb_direct_read,
	.rdma_read	= smb_direct_rdma_read,
	.rdma_write	= smb_direct_rdma_write,
	.free_transport = smb_direct_free_transport,
};
