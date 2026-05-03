// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *   Copyright (C) 2017, Microsoft Corporation.
 *
 *   Author(s): Long Li <longli@microsoft.com>
 */

#include "smbdirect.h"
#include "cifs_debug.h"
#include "cifsproto.h"
#include "smb2proto.h"
<<<<<<< HEAD
#include "../smbdirect/public.h"
||||||| 05f7e89ab9731

const struct smbdirect_socket_parameters *smbd_get_parameters(struct smbd_connection *conn)
{
	struct smbdirect_socket *sc = &conn->socket;

	return &sc->parameters;
}

static struct smbdirect_recv_io *get_receive_buffer(
		struct smbdirect_socket *sc);
static void put_receive_buffer(
		struct smbdirect_socket *sc,
		struct smbdirect_recv_io *response);
static int allocate_receive_buffers(struct smbdirect_socket *sc, int num_buf);
static void destroy_receive_buffers(struct smbdirect_socket *sc);

static void enqueue_reassembly(
		struct smbdirect_socket *sc,
		struct smbdirect_recv_io *response, int data_length);
static struct smbdirect_recv_io *_get_first_reassembly(
		struct smbdirect_socket *sc);

static int smbd_post_recv(
		struct smbdirect_socket *sc,
		struct smbdirect_recv_io *response);

static int smbd_post_send_empty(struct smbdirect_socket *sc);

static void destroy_mr_list(struct smbdirect_socket *sc);
static int allocate_mr_list(struct smbdirect_socket *sc);

struct smb_extract_to_rdma {
	struct ib_sge		*sge;
	unsigned int		nr_sge;
	unsigned int		max_sge;
	struct ib_device	*device;
	u32			local_dma_lkey;
	enum dma_data_direction	direction;
};
static ssize_t smb_extract_iter_to_rdma(struct iov_iter *iter, size_t len,
					struct smb_extract_to_rdma *rdma);
=======

const struct smbdirect_socket_parameters *smbd_get_parameters(struct smbd_connection *conn)
{
	struct smbdirect_socket *sc = &conn->socket;

	return &sc->parameters;
}

static struct smbdirect_recv_io *get_receive_buffer(
		struct smbdirect_socket *sc);
static void put_receive_buffer(
		struct smbdirect_socket *sc,
		struct smbdirect_recv_io *response);
static int allocate_receive_buffers(struct smbdirect_socket *sc, int num_buf);
static void destroy_receive_buffers(struct smbdirect_socket *sc);

static void enqueue_reassembly(
		struct smbdirect_socket *sc,
		struct smbdirect_recv_io *response, int data_length);
static struct smbdirect_recv_io *_get_first_reassembly(
		struct smbdirect_socket *sc);

static int smbd_post_send(struct smbdirect_socket *sc,
			  struct smbdirect_send_batch *batch,
			  struct smbdirect_send_io *request);

static int smbd_post_recv(
		struct smbdirect_socket *sc,
		struct smbdirect_recv_io *response);

static int smbd_post_send_empty(struct smbdirect_socket *sc);

static void destroy_mr_list(struct smbdirect_socket *sc);
static int allocate_mr_list(struct smbdirect_socket *sc);

struct smb_extract_to_rdma {
	struct ib_sge		*sge;
	unsigned int		nr_sge;
	unsigned int		max_sge;
	struct ib_device	*device;
	u32			local_dma_lkey;
	enum dma_data_direction	direction;
};
static ssize_t smb_extract_iter_to_rdma(struct iov_iter *iter, size_t len,
					struct smb_extract_to_rdma *rdma);
>>>>>>> hardened/6.19

/* Port numbers for SMBD transport */
#define SMB_PORT	445
#define SMBD_PORT	5445

/* Address lookup and resolve timeout in ms */
#define RDMA_RESOLVE_TIMEOUT	5000

/* SMBD negotiation timeout in seconds */
#define SMBD_NEGOTIATE_TIMEOUT	120

/* The timeout to wait for a keepalive message from peer in seconds */
#define KEEPALIVE_RECV_TIMEOUT 5

/*
 * Default maximum number of RDMA read/write outstanding on this connection
 * This value is possibly decreased during QP creation on hardware limit
 */
#define SMBD_CM_RESPONDER_RESOURCES	32

/*
 * User configurable initial values per SMBD transport connection
 * as defined in [MS-SMBD] 3.1.1.1
 * Those may change after a SMBD negotiation
 */
/* The local peer's maximum number of credits to grant to the peer */
int smbd_receive_credit_max = 255;

/* The remote peer's credit request of local peer */
int smbd_send_credit_target = 255;

/* The maximum single message size can be sent to remote peer */
int smbd_max_send_size = 1364;

/*
 * The maximum fragmented upper-layer payload receive size supported
 *
 * Assume max_payload_per_credit is
 * smbd_max_receive_size - 24 = 1340
 *
 * The maximum number would be
 * smbd_receive_credit_max * max_payload_per_credit
 *
 *                       1340 * 255 = 341700 (0x536C4)
 *
 * The minimum value from the spec is 131072 (0x20000)
 *
 * For now we use the logic we used in ksmbd before:
 *                 (1364 * 255) / 2 = 173910 (0x2A756)
 */
int smbd_max_fragmented_recv_size = (1364 * 255) / 2;

/*  The maximum single-message size which can be received */
int smbd_max_receive_size = 1364;

/* The timeout to initiate send of a keepalive message on idle */
int smbd_keep_alive_interval = 120;

/*
 * User configurable initial values for RDMA transport
 * The actual values used may be lower and are limited to hardware capabilities
 */
/* Default maximum number of pages in a single RDMA write/read */
int smbd_max_frmr_depth = 2048;

/* If payload is less than this byte, use RDMA send/recv not read/write */
int rdma_readwrite_threshold = 4096;

/* Transport logging functions
 * Logging are defined as classes. They can be OR'ed to define the actual
 * logging level via module parameter smbd_logging_class
 * e.g. cifs.smbd_logging_class=0xa0 will log all log_rdma_recv() and
 * log_rdma_event()
 */
#define LOG_OUTGOING			0x1
#define LOG_INCOMING			0x2
#define LOG_READ			0x4
#define LOG_WRITE			0x8
#define LOG_RDMA_SEND			0x10
#define LOG_RDMA_RECV			0x20
#define LOG_KEEP_ALIVE			0x40
#define LOG_RDMA_EVENT			0x80
#define LOG_RDMA_MR			0x100
static unsigned int smbd_logging_class;
module_param(smbd_logging_class, uint, 0644);
MODULE_PARM_DESC(smbd_logging_class,
	"Logging class for SMBD transport 0x0 to 0x100");

#define ERR		0x0
#define INFO		0x1
static unsigned int smbd_logging_level = ERR;
module_param(smbd_logging_level, uint, 0644);
MODULE_PARM_DESC(smbd_logging_level,
	"Logging level for SMBD transport, 0 (default): error, 1: info");

static bool smbd_logging_needed(struct smbdirect_socket *sc,
				void *private_ptr,
				unsigned int lvl,
				unsigned int cls)
{
#define BUILD_BUG_SAME(x) BUILD_BUG_ON(x != SMBDIRECT_LOG_ ##x)
	BUILD_BUG_SAME(ERR);
	BUILD_BUG_SAME(INFO);
#undef BUILD_BUG_SAME
#define BUILD_BUG_SAME(x) BUILD_BUG_ON(x != SMBDIRECT_ ##x)
	BUILD_BUG_SAME(LOG_OUTGOING);
	BUILD_BUG_SAME(LOG_INCOMING);
	BUILD_BUG_SAME(LOG_READ);
	BUILD_BUG_SAME(LOG_WRITE);
	BUILD_BUG_SAME(LOG_RDMA_SEND);
	BUILD_BUG_SAME(LOG_RDMA_RECV);
	BUILD_BUG_SAME(LOG_KEEP_ALIVE);
	BUILD_BUG_SAME(LOG_RDMA_EVENT);
	BUILD_BUG_SAME(LOG_RDMA_MR);
#undef BUILD_BUG_SAME

	if (lvl <= smbd_logging_level || cls & smbd_logging_class)
		return true;
	return false;
}

static void smbd_logging_vaprintf(struct smbdirect_socket *sc,
				  const char *func,
				  unsigned int line,
				  void *private_ptr,
				  unsigned int lvl,
				  unsigned int cls,
				  struct va_format *vaf)
{
	cifs_dbg(VFS, "%s:%u %pV", func, line, vaf);
}

#define log_rdma(level, class, fmt, args...)				\
do {									\
	if (level <= smbd_logging_level || class & smbd_logging_class)	\
		cifs_dbg(VFS, "%s:%d " fmt, __func__, __LINE__, ##args);\
} while (0)

#define log_outgoing(level, fmt, args...) \
		log_rdma(level, LOG_OUTGOING, fmt, ##args)
#define log_incoming(level, fmt, args...) \
		log_rdma(level, LOG_INCOMING, fmt, ##args)
#define log_read(level, fmt, args...)	log_rdma(level, LOG_READ, fmt, ##args)
#define log_write(level, fmt, args...)	log_rdma(level, LOG_WRITE, fmt, ##args)
#define log_rdma_send(level, fmt, args...) \
		log_rdma(level, LOG_RDMA_SEND, fmt, ##args)
#define log_rdma_recv(level, fmt, args...) \
		log_rdma(level, LOG_RDMA_RECV, fmt, ##args)
#define log_keep_alive(level, fmt, args...) \
		log_rdma(level, LOG_KEEP_ALIVE, fmt, ##args)
#define log_rdma_event(level, fmt, args...) \
		log_rdma(level, LOG_RDMA_EVENT, fmt, ##args)
#define log_rdma_mr(level, fmt, args...) \
		log_rdma(level, LOG_RDMA_MR, fmt, ##args)

<<<<<<< HEAD
||||||| 05f7e89ab9731
static void smbd_disconnect_wake_up_all(struct smbdirect_socket *sc)
{
	/*
	 * Wake up all waiters in all wait queues
	 * in order to notice the broken connection.
	 */
	wake_up_all(&sc->status_wait);
	wake_up_all(&sc->send_io.lcredits.wait_queue);
	wake_up_all(&sc->send_io.credits.wait_queue);
	wake_up_all(&sc->send_io.pending.dec_wait_queue);
	wake_up_all(&sc->send_io.pending.zero_wait_queue);
	wake_up_all(&sc->recv_io.reassembly.wait_queue);
	wake_up_all(&sc->mr_io.ready.wait_queue);
	wake_up_all(&sc->mr_io.cleanup.wait_queue);
}

static void smbd_disconnect_rdma_work(struct work_struct *work)
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
	disable_work(&sc->recv_io.posted.refill_work);
	disable_work(&sc->mr_io.recovery_work);
	disable_work(&sc->idle.immediate_work);
	disable_delayed_work(&sc->idle.timer_work);

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
		 * rdma_connect() never reached
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
	smbd_disconnect_wake_up_all(sc);
}

static void smbd_disconnect_rdma_connection(struct smbdirect_socket *sc)
{
	if (sc->first_error == 0)
		sc->first_error = -ECONNABORTED;

	/*
	 * make sure other work (than disconnect_work) is
	 * not queued again but here we don't block and avoid
	 * disable[_delayed]_work_sync()
	 */
	disable_work(&sc->recv_io.posted.refill_work);
	disable_work(&sc->mr_io.recovery_work);
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
	smbd_disconnect_wake_up_all(sc);

	queue_work(sc->workqueue, &sc->disconnect_work);
}

/* Upcall from RDMA CM */
static int smbd_conn_upcall(
		struct rdma_cm_id *id, struct rdma_cm_event *event)
{
	struct smbdirect_socket *sc = id->context;
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	const char *event_name = rdma_event_msg(event->event);
	u8 peer_initiator_depth;
	u8 peer_responder_resources;

	log_rdma_event(INFO, "event=%s status=%d\n",
		event_name, event->status);

	switch (event->event) {
	case RDMA_CM_EVENT_ADDR_RESOLVED:
		if (SMBDIRECT_CHECK_STATUS_DISCONNECT(sc, SMBDIRECT_SOCKET_RESOLVE_ADDR_RUNNING))
			break;
		sc->status = SMBDIRECT_SOCKET_RESOLVE_ROUTE_NEEDED;
		wake_up(&sc->status_wait);
		break;

	case RDMA_CM_EVENT_ROUTE_RESOLVED:
		if (SMBDIRECT_CHECK_STATUS_DISCONNECT(sc, SMBDIRECT_SOCKET_RESOLVE_ROUTE_RUNNING))
			break;
		sc->status = SMBDIRECT_SOCKET_RDMA_CONNECT_NEEDED;
		wake_up(&sc->status_wait);
		break;

	case RDMA_CM_EVENT_ADDR_ERROR:
		log_rdma_event(ERR, "connecting failed event=%s\n", event_name);
		sc->status = SMBDIRECT_SOCKET_RESOLVE_ADDR_FAILED;
		smbd_disconnect_rdma_work(&sc->disconnect_work);
		break;

	case RDMA_CM_EVENT_ROUTE_ERROR:
		log_rdma_event(ERR, "connecting failed event=%s\n", event_name);
		sc->status = SMBDIRECT_SOCKET_RESOLVE_ROUTE_FAILED;
		smbd_disconnect_rdma_work(&sc->disconnect_work);
		break;

	case RDMA_CM_EVENT_ESTABLISHED:
		log_rdma_event(INFO, "connected event=%s\n", event_name);

		/*
		 * Here we work around an inconsistency between
		 * iWarp and other devices (at least rxe and irdma using RoCEv2)
		 */
		if (rdma_protocol_iwarp(id->device, id->port_num)) {
			/*
			 * iWarp devices report the peer's values
			 * with the perspective of the peer here.
			 * Tested with siw and irdma (in iwarp mode)
			 * We need to change to our perspective here,
			 * so we need to switch the values.
			 */
			peer_initiator_depth = event->param.conn.responder_resources;
			peer_responder_resources = event->param.conn.initiator_depth;
		} else {
			/*
			 * Non iWarp devices report the peer's values
			 * already changed to our perspective here.
			 * Tested with rxe and irdma (in roce mode).
			 */
			peer_initiator_depth = event->param.conn.initiator_depth;
			peer_responder_resources = event->param.conn.responder_resources;
		}
		if (rdma_protocol_iwarp(id->device, id->port_num) &&
		    event->param.conn.private_data_len == 8) {
			/*
			 * Legacy clients with only iWarp MPA v1 support
			 * need a private blob in order to negotiate
			 * the IRD/ORD values.
			 */
			const __be32 *ird_ord_hdr = event->param.conn.private_data;
			u32 ird32 = be32_to_cpu(ird_ord_hdr[0]);
			u32 ord32 = be32_to_cpu(ird_ord_hdr[1]);

			/*
			 * cifs.ko sends the legacy IRD/ORD negotiation
			 * event if iWarp MPA v2 was used.
			 *
			 * Here we check that the values match and only
			 * mark the client as legacy if they don't match.
			 */
			if ((u32)event->param.conn.initiator_depth != ird32 ||
			    (u32)event->param.conn.responder_resources != ord32) {
				/*
				 * There are broken clients (old cifs.ko)
				 * using little endian and also
				 * struct rdma_conn_param only uses u8
				 * for initiator_depth and responder_resources,
				 * so we truncate the value to U8_MAX.
				 *
				 * smb_direct_accept_client() will then
				 * do the real negotiation in order to
				 * select the minimum between client and
				 * server.
				 */
				ird32 = min_t(u32, ird32, U8_MAX);
				ord32 = min_t(u32, ord32, U8_MAX);

				sc->rdma.legacy_iwarp = true;
				peer_initiator_depth = (u8)ird32;
				peer_responder_resources = (u8)ord32;
			}
		}

		/*
		 * negotiate the value by using the minimum
		 * between client and server if the client provided
		 * non 0 values.
		 */
		if (peer_initiator_depth != 0)
			sp->initiator_depth =
					min_t(u8, sp->initiator_depth,
					      peer_initiator_depth);
		if (peer_responder_resources != 0)
			sp->responder_resources =
					min_t(u8, sp->responder_resources,
					      peer_responder_resources);

		if (SMBDIRECT_CHECK_STATUS_DISCONNECT(sc, SMBDIRECT_SOCKET_RDMA_CONNECT_RUNNING))
			break;
		sc->status = SMBDIRECT_SOCKET_NEGOTIATE_NEEDED;
		wake_up(&sc->status_wait);
		break;

	case RDMA_CM_EVENT_CONNECT_ERROR:
	case RDMA_CM_EVENT_UNREACHABLE:
	case RDMA_CM_EVENT_REJECTED:
		log_rdma_event(ERR, "connecting failed event=%s\n", event_name);
		sc->status = SMBDIRECT_SOCKET_RDMA_CONNECT_FAILED;
		smbd_disconnect_rdma_work(&sc->disconnect_work);
		break;

	case RDMA_CM_EVENT_DEVICE_REMOVAL:
	case RDMA_CM_EVENT_DISCONNECTED:
		/* This happens when we fail the negotiation */
		if (sc->status == SMBDIRECT_SOCKET_NEGOTIATE_FAILED) {
			log_rdma_event(ERR, "event=%s during negotiation\n", event_name);
		}

		sc->status = SMBDIRECT_SOCKET_DISCONNECTED;
		smbd_disconnect_rdma_work(&sc->disconnect_work);
		break;

	default:
		log_rdma_event(ERR, "unexpected event=%s status=%d\n",
			       event_name, event->status);
		break;
	}

	return 0;
}

/* Upcall from RDMA QP */
static void
smbd_qp_async_error_upcall(struct ib_event *event, void *context)
{
	struct smbdirect_socket *sc = context;

	log_rdma_event(ERR, "%s on device %s socket %p\n",
		ib_event_msg(event->event), event->device->name, sc);

	switch (event->event) {
	case IB_EVENT_CQ_ERR:
	case IB_EVENT_QP_FATAL:
		smbd_disconnect_rdma_connection(sc);
		break;

	default:
		break;
	}
}

static inline void *smbdirect_send_io_payload(struct smbdirect_send_io *request)
{
	return (void *)request->packet;
}

static inline void *smbdirect_recv_io_payload(struct smbdirect_recv_io *response)
{
	return (void *)response->packet;
}

/* Called when a RDMA send is done */
static void send_done(struct ib_cq *cq, struct ib_wc *wc)
{
	int i;
	struct smbdirect_send_io *request =
		container_of(wc->wr_cqe, struct smbdirect_send_io, cqe);
	struct smbdirect_socket *sc = request->socket;
	int lcredits = 0;

	log_rdma_send(INFO, "smbdirect_send_io 0x%p completed wc->status=%s\n",
		request, ib_wc_status_msg(wc->status));

	for (i = 0; i < request->num_sge; i++)
		ib_dma_unmap_single(sc->ib.dev,
			request->sge[i].addr,
			request->sge[i].length,
			DMA_TO_DEVICE);
	mempool_free(request, sc->send_io.mem.pool);
	lcredits += 1;

	if (wc->status != IB_WC_SUCCESS || wc->opcode != IB_WC_SEND) {
		if (wc->status != IB_WC_WR_FLUSH_ERR)
			log_rdma_send(ERR, "wc->status=%s wc->opcode=%d\n",
				ib_wc_status_msg(wc->status), wc->opcode);
		smbd_disconnect_rdma_connection(sc);
		return;
	}

	atomic_add(lcredits, &sc->send_io.lcredits.count);
	wake_up(&sc->send_io.lcredits.wait_queue);

	if (atomic_dec_and_test(&sc->send_io.pending.count))
		wake_up(&sc->send_io.pending.zero_wait_queue);

	wake_up(&sc->send_io.pending.dec_wait_queue);
}

static void dump_smbdirect_negotiate_resp(struct smbdirect_negotiate_resp *resp)
{
	log_rdma_event(INFO, "resp message min_version %u max_version %u negotiated_version %u credits_requested %u credits_granted %u status %u max_readwrite_size %u preferred_send_size %u max_receive_size %u max_fragmented_size %u\n",
		       resp->min_version, resp->max_version,
		       resp->negotiated_version, resp->credits_requested,
		       resp->credits_granted, resp->status,
		       resp->max_readwrite_size, resp->preferred_send_size,
		       resp->max_receive_size, resp->max_fragmented_size);
}

/*
 * Process a negotiation response message, according to [MS-SMBD]3.1.5.7
 * response, packet_length: the negotiation response message
 * return value: true if negotiation is a success, false if failed
 */
static bool process_negotiation_response(
		struct smbdirect_recv_io *response, int packet_length)
{
	struct smbdirect_socket *sc = response->socket;
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	struct smbdirect_negotiate_resp *packet = smbdirect_recv_io_payload(response);

	if (packet_length < sizeof(struct smbdirect_negotiate_resp)) {
		log_rdma_event(ERR,
			"error: packet_length=%d\n", packet_length);
		return false;
	}

	if (le16_to_cpu(packet->negotiated_version) != SMBDIRECT_V1) {
		log_rdma_event(ERR, "error: negotiated_version=%x\n",
			le16_to_cpu(packet->negotiated_version));
		return false;
	}

	if (packet->credits_requested == 0) {
		log_rdma_event(ERR, "error: credits_requested==0\n");
		return false;
	}
	sc->recv_io.credits.target = le16_to_cpu(packet->credits_requested);
	sc->recv_io.credits.target = min_t(u16, sc->recv_io.credits.target, sp->recv_credit_max);

	if (packet->credits_granted == 0) {
		log_rdma_event(ERR, "error: credits_granted==0\n");
		return false;
	}
	atomic_set(&sc->send_io.lcredits.count, sp->send_credit_target);
	atomic_set(&sc->send_io.credits.count, le16_to_cpu(packet->credits_granted));

	if (le32_to_cpu(packet->preferred_send_size) > sp->max_recv_size) {
		log_rdma_event(ERR, "error: preferred_send_size=%d\n",
			le32_to_cpu(packet->preferred_send_size));
		return false;
	}
	sp->max_recv_size = le32_to_cpu(packet->preferred_send_size);

	if (le32_to_cpu(packet->max_receive_size) < SMBD_MIN_RECEIVE_SIZE) {
		log_rdma_event(ERR, "error: max_receive_size=%d\n",
			le32_to_cpu(packet->max_receive_size));
		return false;
	}
	sp->max_send_size = min_t(u32, sp->max_send_size,
				  le32_to_cpu(packet->max_receive_size));

	if (le32_to_cpu(packet->max_fragmented_size) <
			SMBD_MIN_FRAGMENTED_SIZE) {
		log_rdma_event(ERR, "error: max_fragmented_size=%d\n",
			le32_to_cpu(packet->max_fragmented_size));
		return false;
	}
	sp->max_fragmented_send_size =
		le32_to_cpu(packet->max_fragmented_size);


	sp->max_read_write_size = min_t(u32,
			le32_to_cpu(packet->max_readwrite_size),
			sp->max_frmr_depth * PAGE_SIZE);
	sp->max_frmr_depth = sp->max_read_write_size / PAGE_SIZE;

	sc->recv_io.expected = SMBDIRECT_EXPECT_DATA_TRANSFER;
	return true;
}

static void smbd_post_send_credits(struct work_struct *work)
{
	int rc;
	struct smbdirect_recv_io *response;
	struct smbdirect_socket *sc =
		container_of(work, struct smbdirect_socket, recv_io.posted.refill_work);

	if (sc->status != SMBDIRECT_SOCKET_CONNECTED) {
		return;
	}

	if (sc->recv_io.credits.target >
		atomic_read(&sc->recv_io.credits.count)) {
		while (true) {
			response = get_receive_buffer(sc);
			if (!response)
				break;

			response->first_segment = false;
			rc = smbd_post_recv(sc, response);
			if (rc) {
				log_rdma_recv(ERR,
					"post_recv failed rc=%d\n", rc);
				put_receive_buffer(sc, response);
				break;
			}

			atomic_inc(&sc->recv_io.posted.count);
		}
	}

	/* Promptly send an immediate packet as defined in [MS-SMBD] 3.1.1.1 */
	if (atomic_read(&sc->recv_io.credits.count) <
		sc->recv_io.credits.target - 1) {
		log_keep_alive(INFO, "schedule send of an empty message\n");
		queue_work(sc->workqueue, &sc->idle.immediate_work);
	}
}

/* Called from softirq, when recv is done */
static void recv_done(struct ib_cq *cq, struct ib_wc *wc)
{
	struct smbdirect_data_transfer *data_transfer;
	struct smbdirect_recv_io *response =
		container_of(wc->wr_cqe, struct smbdirect_recv_io, cqe);
	struct smbdirect_socket *sc = response->socket;
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	u16 old_recv_credit_target;
	u32 data_offset = 0;
	u32 data_length = 0;
	u32 remaining_data_length = 0;
	bool negotiate_done = false;

	log_rdma_recv(INFO,
		      "response=0x%p type=%d wc status=%s wc opcode %d byte_len=%d pkey_index=%u\n",
		      response, sc->recv_io.expected,
		      ib_wc_status_msg(wc->status), wc->opcode,
		      wc->byte_len, wc->pkey_index);

	if (wc->status != IB_WC_SUCCESS || wc->opcode != IB_WC_RECV) {
		if (wc->status != IB_WC_WR_FLUSH_ERR)
			log_rdma_recv(ERR, "wc->status=%s opcode=%d\n",
				ib_wc_status_msg(wc->status), wc->opcode);
		goto error;
	}

	ib_dma_sync_single_for_cpu(
		wc->qp->device,
		response->sge.addr,
		response->sge.length,
		DMA_FROM_DEVICE);

	/*
	 * Reset timer to the keepalive interval in
	 * order to trigger our next keepalive message.
	 */
	sc->idle.keepalive = SMBDIRECT_KEEPALIVE_NONE;
	mod_delayed_work(sc->workqueue, &sc->idle.timer_work,
			 msecs_to_jiffies(sp->keepalive_interval_msec));

	switch (sc->recv_io.expected) {
	/* SMBD negotiation response */
	case SMBDIRECT_EXPECT_NEGOTIATE_REP:
		dump_smbdirect_negotiate_resp(smbdirect_recv_io_payload(response));
		sc->recv_io.reassembly.full_packet_received = true;
		negotiate_done =
			process_negotiation_response(response, wc->byte_len);
		put_receive_buffer(sc, response);
		if (SMBDIRECT_CHECK_STATUS_WARN(sc, SMBDIRECT_SOCKET_NEGOTIATE_RUNNING))
			negotiate_done = false;
		if (!negotiate_done) {
			sc->status = SMBDIRECT_SOCKET_NEGOTIATE_FAILED;
			smbd_disconnect_rdma_connection(sc);
		} else {
			sc->status = SMBDIRECT_SOCKET_CONNECTED;
			wake_up(&sc->status_wait);
		}

		return;

	/* SMBD data transfer packet */
	case SMBDIRECT_EXPECT_DATA_TRANSFER:
		data_transfer = smbdirect_recv_io_payload(response);

		if (wc->byte_len <
		    offsetof(struct smbdirect_data_transfer, padding))
			goto error;

		remaining_data_length = le32_to_cpu(data_transfer->remaining_data_length);
		data_offset = le32_to_cpu(data_transfer->data_offset);
		data_length = le32_to_cpu(data_transfer->data_length);
		if (wc->byte_len < data_offset ||
		    (u64)wc->byte_len < (u64)data_offset + data_length)
			goto error;

		if (remaining_data_length > sp->max_fragmented_recv_size ||
		    data_length > sp->max_fragmented_recv_size ||
		    (u64)remaining_data_length + (u64)data_length > (u64)sp->max_fragmented_recv_size)
			goto error;

		if (data_length) {
			if (sc->recv_io.reassembly.full_packet_received)
				response->first_segment = true;

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
		if (le16_to_cpu(data_transfer->credits_granted)) {
			atomic_add(le16_to_cpu(data_transfer->credits_granted),
				&sc->send_io.credits.count);
			/*
			 * We have new send credits granted from remote peer
			 * If any sender is waiting for credits, unblock it
			 */
			wake_up(&sc->send_io.credits.wait_queue);
		}

		log_incoming(INFO, "data flags %d data_offset %d data_length %d remaining_data_length %d\n",
			     le16_to_cpu(data_transfer->flags),
			     le32_to_cpu(data_transfer->data_offset),
			     le32_to_cpu(data_transfer->data_length),
			     le32_to_cpu(data_transfer->remaining_data_length));

		/* Send an immediate response right away if requested */
		if (le16_to_cpu(data_transfer->flags) &
				SMBDIRECT_FLAG_RESPONSE_REQUESTED) {
			log_keep_alive(INFO, "schedule send of immediate response\n");
			queue_work(sc->workqueue, &sc->idle.immediate_work);
		}

		/*
		 * If this is a packet with data playload place the data in
		 * reassembly queue and wake up the reading thread
		 */
		if (data_length) {
			if (sc->recv_io.credits.target > old_recv_credit_target)
				queue_work(sc->workqueue, &sc->recv_io.posted.refill_work);

			enqueue_reassembly(sc, response, data_length);
			wake_up(&sc->recv_io.reassembly.wait_queue);
		} else
			put_receive_buffer(sc, response);

		return;

	case SMBDIRECT_EXPECT_NEGOTIATE_REQ:
		/* Only server... */
		break;
	}

	/*
	 * This is an internal error!
	 */
	log_rdma_recv(ERR, "unexpected response type=%d\n", sc->recv_io.expected);
	WARN_ON_ONCE(sc->recv_io.expected != SMBDIRECT_EXPECT_DATA_TRANSFER);
error:
	put_receive_buffer(sc, response);
	smbd_disconnect_rdma_connection(sc);
}

static struct rdma_cm_id *smbd_create_id(
		struct smbdirect_socket *sc,
		struct sockaddr *dstaddr, int port)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	struct rdma_cm_id *id;
	int rc;
	__be16 *sport;

	id = rdma_create_id(&init_net, smbd_conn_upcall, sc,
		RDMA_PS_TCP, IB_QPT_RC);
	if (IS_ERR(id)) {
		rc = PTR_ERR(id);
		log_rdma_event(ERR, "rdma_create_id() failed %i\n", rc);
		return id;
	}

	if (dstaddr->sa_family == AF_INET6)
		sport = &((struct sockaddr_in6 *)dstaddr)->sin6_port;
	else
		sport = &((struct sockaddr_in *)dstaddr)->sin_port;

	*sport = htons(port);

	WARN_ON_ONCE(sc->status != SMBDIRECT_SOCKET_RESOLVE_ADDR_NEEDED);
	sc->status = SMBDIRECT_SOCKET_RESOLVE_ADDR_RUNNING;
	rc = rdma_resolve_addr(id, NULL, (struct sockaddr *)dstaddr,
		sp->resolve_addr_timeout_msec);
	if (rc) {
		log_rdma_event(ERR, "rdma_resolve_addr() failed %i\n", rc);
		goto out;
	}
	rc = wait_event_interruptible_timeout(
		sc->status_wait,
		sc->status != SMBDIRECT_SOCKET_RESOLVE_ADDR_RUNNING,
		msecs_to_jiffies(sp->resolve_addr_timeout_msec));
	/* e.g. if interrupted returns -ERESTARTSYS */
	if (rc < 0) {
		log_rdma_event(ERR, "rdma_resolve_addr timeout rc: %i\n", rc);
		goto out;
	}
	if (sc->status == SMBDIRECT_SOCKET_RESOLVE_ADDR_RUNNING) {
		rc = -ETIMEDOUT;
		log_rdma_event(ERR, "rdma_resolve_addr() completed %i\n", rc);
		goto out;
	}
	if (sc->status != SMBDIRECT_SOCKET_RESOLVE_ROUTE_NEEDED) {
		rc = -EHOSTUNREACH;
		log_rdma_event(ERR, "rdma_resolve_addr() completed %i\n", rc);
		goto out;
	}

	WARN_ON_ONCE(sc->status != SMBDIRECT_SOCKET_RESOLVE_ROUTE_NEEDED);
	sc->status = SMBDIRECT_SOCKET_RESOLVE_ROUTE_RUNNING;
	rc = rdma_resolve_route(id, sp->resolve_route_timeout_msec);
	if (rc) {
		log_rdma_event(ERR, "rdma_resolve_route() failed %i\n", rc);
		goto out;
	}
	rc = wait_event_interruptible_timeout(
		sc->status_wait,
		sc->status != SMBDIRECT_SOCKET_RESOLVE_ROUTE_RUNNING,
		msecs_to_jiffies(sp->resolve_route_timeout_msec));
	/* e.g. if interrupted returns -ERESTARTSYS */
	if (rc < 0)  {
		log_rdma_event(ERR, "rdma_resolve_addr timeout rc: %i\n", rc);
		goto out;
	}
	if (sc->status == SMBDIRECT_SOCKET_RESOLVE_ROUTE_RUNNING) {
		rc = -ETIMEDOUT;
		log_rdma_event(ERR, "rdma_resolve_route() completed %i\n", rc);
		goto out;
	}
	if (sc->status != SMBDIRECT_SOCKET_RDMA_CONNECT_NEEDED) {
		rc = -ENETUNREACH;
		log_rdma_event(ERR, "rdma_resolve_route() completed %i\n", rc);
		goto out;
	}

	return id;

out:
	rdma_destroy_id(id);
	return ERR_PTR(rc);
}

/*
 * Test if FRWR (Fast Registration Work Requests) is supported on the device
 * This implementation requires FRWR on RDMA read/write
 * return value: true if it is supported
 */
static bool frwr_is_supported(struct ib_device_attr *attrs)
{
	if (!(attrs->device_cap_flags & IB_DEVICE_MEM_MGT_EXTENSIONS))
		return false;
	if (attrs->max_fast_reg_page_list_len == 0)
		return false;
	return true;
}

static int smbd_ia_open(
		struct smbdirect_socket *sc,
		struct sockaddr *dstaddr, int port)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	int rc;

	WARN_ON_ONCE(sc->status != SMBDIRECT_SOCKET_CREATED);
	sc->status = SMBDIRECT_SOCKET_RESOLVE_ADDR_NEEDED;

	sc->rdma.cm_id = smbd_create_id(sc, dstaddr, port);
	if (IS_ERR(sc->rdma.cm_id)) {
		rc = PTR_ERR(sc->rdma.cm_id);
		goto out1;
	}
	sc->ib.dev = sc->rdma.cm_id->device;

	if (!frwr_is_supported(&sc->ib.dev->attrs)) {
		log_rdma_event(ERR, "Fast Registration Work Requests (FRWR) is not supported\n");
		log_rdma_event(ERR, "Device capability flags = %llx max_fast_reg_page_list_len = %u\n",
			       sc->ib.dev->attrs.device_cap_flags,
			       sc->ib.dev->attrs.max_fast_reg_page_list_len);
		rc = -EPROTONOSUPPORT;
		goto out2;
	}
	sp->max_frmr_depth = min_t(u32,
		sp->max_frmr_depth,
		sc->ib.dev->attrs.max_fast_reg_page_list_len);
	sc->mr_io.type = IB_MR_TYPE_MEM_REG;
	if (sc->ib.dev->attrs.kernel_cap_flags & IBK_SG_GAPS_REG)
		sc->mr_io.type = IB_MR_TYPE_SG_GAPS;

	return 0;

out2:
	rdma_destroy_id(sc->rdma.cm_id);
	sc->rdma.cm_id = NULL;

out1:
	return rc;
}

/*
 * Send a negotiation request message to the peer
 * The negotiation procedure is in [MS-SMBD] 3.1.5.2 and 3.1.5.3
 * After negotiation, the transport is connected and ready for
 * carrying upper layer SMB payload
 */
static int smbd_post_send_negotiate_req(struct smbdirect_socket *sc)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	struct ib_send_wr send_wr;
	int rc = -ENOMEM;
	struct smbdirect_send_io *request;
	struct smbdirect_negotiate_req *packet;

	request = mempool_alloc(sc->send_io.mem.pool, GFP_KERNEL);
	if (!request)
		return rc;

	request->socket = sc;

	packet = smbdirect_send_io_payload(request);
	packet->min_version = cpu_to_le16(SMBDIRECT_V1);
	packet->max_version = cpu_to_le16(SMBDIRECT_V1);
	packet->reserved = 0;
	packet->credits_requested = cpu_to_le16(sp->send_credit_target);
	packet->preferred_send_size = cpu_to_le32(sp->max_send_size);
	packet->max_receive_size = cpu_to_le32(sp->max_recv_size);
	packet->max_fragmented_size =
		cpu_to_le32(sp->max_fragmented_recv_size);

	request->num_sge = 1;
	request->sge[0].addr = ib_dma_map_single(
				sc->ib.dev, (void *)packet,
				sizeof(*packet), DMA_TO_DEVICE);
	if (ib_dma_mapping_error(sc->ib.dev, request->sge[0].addr)) {
		rc = -EIO;
		goto dma_mapping_failed;
	}

	request->sge[0].length = sizeof(*packet);
	request->sge[0].lkey = sc->ib.pd->local_dma_lkey;

	ib_dma_sync_single_for_device(
		sc->ib.dev, request->sge[0].addr,
		request->sge[0].length, DMA_TO_DEVICE);

	request->cqe.done = send_done;

	send_wr.next = NULL;
	send_wr.wr_cqe = &request->cqe;
	send_wr.sg_list = request->sge;
	send_wr.num_sge = request->num_sge;
	send_wr.opcode = IB_WR_SEND;
	send_wr.send_flags = IB_SEND_SIGNALED;

	log_rdma_send(INFO, "sge addr=0x%llx length=%u lkey=0x%x\n",
		request->sge[0].addr,
		request->sge[0].length, request->sge[0].lkey);

	atomic_inc(&sc->send_io.pending.count);
	rc = ib_post_send(sc->ib.qp, &send_wr, NULL);
	if (!rc)
		return 0;

	/* if we reach here, post send failed */
	log_rdma_send(ERR, "ib_post_send failed rc=%d\n", rc);
	atomic_dec(&sc->send_io.pending.count);
	ib_dma_unmap_single(sc->ib.dev, request->sge[0].addr,
		request->sge[0].length, DMA_TO_DEVICE);

	smbd_disconnect_rdma_connection(sc);

dma_mapping_failed:
	mempool_free(request, sc->send_io.mem.pool);
	return rc;
}

/*
 * Extend the credits to remote peer
 * This implements [MS-SMBD] 3.1.5.9
 * The idea is that we should extend credits to remote peer as quickly as
 * it's allowed, to maintain data flow. We allocate as much receive
 * buffer as possible, and extend the receive credits to remote peer
 * return value: the new credtis being granted.
 */
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

	return new_credits;
}

/*
 * Check if we need to send a KEEP_ALIVE message
 * The idle connection timer triggers a KEEP_ALIVE message when expires
 * SMBDIRECT_FLAG_RESPONSE_REQUESTED is set in the message flag to have peer send
 * back a response.
 * return value:
 * 1 if SMBDIRECT_FLAG_RESPONSE_REQUESTED needs to be set
 * 0: otherwise
 */
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

/* Post the send request */
static int smbd_post_send(struct smbdirect_socket *sc,
		struct smbdirect_send_io *request)
{
	struct ib_send_wr send_wr;
	int rc, i;

	for (i = 0; i < request->num_sge; i++) {
		log_rdma_send(INFO,
			"rdma_request sge[%d] addr=0x%llx length=%u\n",
			i, request->sge[i].addr, request->sge[i].length);
		ib_dma_sync_single_for_device(
			sc->ib.dev,
			request->sge[i].addr,
			request->sge[i].length,
			DMA_TO_DEVICE);
	}

	request->cqe.done = send_done;

	send_wr.next = NULL;
	send_wr.wr_cqe = &request->cqe;
	send_wr.sg_list = request->sge;
	send_wr.num_sge = request->num_sge;
	send_wr.opcode = IB_WR_SEND;
	send_wr.send_flags = IB_SEND_SIGNALED;

	rc = ib_post_send(sc->ib.qp, &send_wr, NULL);
	if (rc) {
		log_rdma_send(ERR, "ib_post_send failed rc=%d\n", rc);
		smbd_disconnect_rdma_connection(sc);
		rc = -EAGAIN;
	}

	return rc;
}

static int smbd_post_send_iter(struct smbdirect_socket *sc,
			       struct iov_iter *iter,
			       int *_remaining_data_length)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	int i, rc;
	int header_length;
	int data_length;
	struct smbdirect_send_io *request;
	struct smbdirect_data_transfer *packet;
	int new_credits = 0;

wait_lcredit:
	/* Wait for local send credits */
	rc = wait_event_interruptible(sc->send_io.lcredits.wait_queue,
		atomic_read(&sc->send_io.lcredits.count) > 0 ||
		sc->status != SMBDIRECT_SOCKET_CONNECTED);
	if (rc)
		goto err_wait_lcredit;

	if (sc->status != SMBDIRECT_SOCKET_CONNECTED) {
		log_outgoing(ERR, "disconnected not sending on wait_credit\n");
		rc = -EAGAIN;
		goto err_wait_lcredit;
	}
	if (unlikely(atomic_dec_return(&sc->send_io.lcredits.count) < 0)) {
		atomic_inc(&sc->send_io.lcredits.count);
		goto wait_lcredit;
	}

wait_credit:
	/* Wait for send credits. A SMBD packet needs one credit */
	rc = wait_event_interruptible(sc->send_io.credits.wait_queue,
		atomic_read(&sc->send_io.credits.count) > 0 ||
		sc->status != SMBDIRECT_SOCKET_CONNECTED);
	if (rc)
		goto err_wait_credit;

	if (sc->status != SMBDIRECT_SOCKET_CONNECTED) {
		log_outgoing(ERR, "disconnected not sending on wait_credit\n");
		rc = -EAGAIN;
		goto err_wait_credit;
	}
	if (unlikely(atomic_dec_return(&sc->send_io.credits.count) < 0)) {
		atomic_inc(&sc->send_io.credits.count);
		goto wait_credit;
	}

	request = mempool_alloc(sc->send_io.mem.pool, GFP_KERNEL);
	if (!request) {
		rc = -ENOMEM;
		goto err_alloc;
	}

	request->socket = sc;
	memset(request->sge, 0, sizeof(request->sge));

	/* Map the packet to DMA */
	header_length = sizeof(struct smbdirect_data_transfer);
	/* If this is a packet without payload, don't send padding */
	if (!iter)
		header_length = offsetof(struct smbdirect_data_transfer, padding);

	packet = smbdirect_send_io_payload(request);
	request->sge[0].addr = ib_dma_map_single(sc->ib.dev,
						 (void *)packet,
						 header_length,
						 DMA_TO_DEVICE);
	if (ib_dma_mapping_error(sc->ib.dev, request->sge[0].addr)) {
		rc = -EIO;
		goto err_dma;
	}

	request->sge[0].length = header_length;
	request->sge[0].lkey = sc->ib.pd->local_dma_lkey;
	request->num_sge = 1;

	/* Fill in the data payload to find out how much data we can add */
	if (iter) {
		struct smb_extract_to_rdma extract = {
			.nr_sge		= request->num_sge,
			.max_sge	= SMBDIRECT_SEND_IO_MAX_SGE,
			.sge		= request->sge,
			.device		= sc->ib.dev,
			.local_dma_lkey	= sc->ib.pd->local_dma_lkey,
			.direction	= DMA_TO_DEVICE,
		};
		size_t payload_len = umin(*_remaining_data_length,
					  sp->max_send_size - sizeof(*packet));

		rc = smb_extract_iter_to_rdma(iter, payload_len,
					      &extract);
		if (rc < 0)
			goto err_dma;
		data_length = rc;
		request->num_sge = extract.nr_sge;
		*_remaining_data_length -= data_length;
	} else {
		data_length = 0;
	}

	/* Fill in the packet header */
	packet->credits_requested = cpu_to_le16(sp->send_credit_target);

	new_credits = manage_credits_prior_sending(sc);
	atomic_add(new_credits, &sc->recv_io.credits.count);
	packet->credits_granted = cpu_to_le16(new_credits);

	packet->flags = 0;
	if (manage_keep_alive_before_sending(sc))
		packet->flags |= cpu_to_le16(SMBDIRECT_FLAG_RESPONSE_REQUESTED);

	packet->reserved = 0;
	if (!data_length)
		packet->data_offset = 0;
	else
		packet->data_offset = cpu_to_le32(24);
	packet->data_length = cpu_to_le32(data_length);
	packet->remaining_data_length = cpu_to_le32(*_remaining_data_length);
	packet->padding = 0;

	log_outgoing(INFO, "credits_requested=%d credits_granted=%d data_offset=%d data_length=%d remaining_data_length=%d\n",
		     le16_to_cpu(packet->credits_requested),
		     le16_to_cpu(packet->credits_granted),
		     le32_to_cpu(packet->data_offset),
		     le32_to_cpu(packet->data_length),
		     le32_to_cpu(packet->remaining_data_length));

	/*
	 * Now that we got a local and a remote credit
	 * we add us as pending
	 */
	atomic_inc(&sc->send_io.pending.count);

	rc = smbd_post_send(sc, request);
	if (!rc)
		return 0;

	if (atomic_dec_and_test(&sc->send_io.pending.count))
		wake_up(&sc->send_io.pending.zero_wait_queue);

	wake_up(&sc->send_io.pending.dec_wait_queue);

err_dma:
	for (i = 0; i < request->num_sge; i++)
		if (request->sge[i].addr)
			ib_dma_unmap_single(sc->ib.dev,
					    request->sge[i].addr,
					    request->sge[i].length,
					    DMA_TO_DEVICE);
	mempool_free(request, sc->send_io.mem.pool);

	/* roll back the granted receive credits */
	atomic_sub(new_credits, &sc->recv_io.credits.count);

err_alloc:
	atomic_inc(&sc->send_io.credits.count);
	wake_up(&sc->send_io.credits.wait_queue);

err_wait_credit:
	atomic_inc(&sc->send_io.lcredits.count);
	wake_up(&sc->send_io.lcredits.wait_queue);

err_wait_lcredit:
	return rc;
}

/*
 * Send an empty message
 * Empty message is used to extend credits to peer to for keep live
 * while there is no upper layer payload to send at the time
 */
static int smbd_post_send_empty(struct smbdirect_socket *sc)
{
	int remaining_data_length = 0;

	sc->statistics.send_empty++;
	return smbd_post_send_iter(sc, NULL, &remaining_data_length);
}

=======
static void smbd_disconnect_wake_up_all(struct smbdirect_socket *sc)
{
	/*
	 * Wake up all waiters in all wait queues
	 * in order to notice the broken connection.
	 */
	wake_up_all(&sc->status_wait);
	wake_up_all(&sc->send_io.lcredits.wait_queue);
	wake_up_all(&sc->send_io.credits.wait_queue);
	wake_up_all(&sc->send_io.pending.dec_wait_queue);
	wake_up_all(&sc->send_io.pending.zero_wait_queue);
	wake_up_all(&sc->recv_io.reassembly.wait_queue);
	wake_up_all(&sc->mr_io.ready.wait_queue);
	wake_up_all(&sc->mr_io.cleanup.wait_queue);
}

static void smbd_disconnect_rdma_work(struct work_struct *work)
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
	disable_work(&sc->recv_io.posted.refill_work);
	disable_work(&sc->mr_io.recovery_work);
	disable_work(&sc->idle.immediate_work);
	disable_delayed_work(&sc->idle.timer_work);

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
		 * rdma_connect() never reached
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
	smbd_disconnect_wake_up_all(sc);
}

static void smbd_disconnect_rdma_connection(struct smbdirect_socket *sc)
{
	if (sc->first_error == 0)
		sc->first_error = -ECONNABORTED;

	/*
	 * make sure other work (than disconnect_work) is
	 * not queued again but here we don't block and avoid
	 * disable[_delayed]_work_sync()
	 */
	disable_work(&sc->recv_io.posted.refill_work);
	disable_work(&sc->mr_io.recovery_work);
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
	smbd_disconnect_wake_up_all(sc);

	queue_work(sc->workqueue, &sc->disconnect_work);
}

/* Upcall from RDMA CM */
static int smbd_conn_upcall(
		struct rdma_cm_id *id, struct rdma_cm_event *event)
{
	struct smbdirect_socket *sc = id->context;
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	const char *event_name = rdma_event_msg(event->event);
	u8 peer_initiator_depth;
	u8 peer_responder_resources;

	log_rdma_event(INFO, "event=%s status=%d\n",
		event_name, event->status);

	switch (event->event) {
	case RDMA_CM_EVENT_ADDR_RESOLVED:
		if (SMBDIRECT_CHECK_STATUS_DISCONNECT(sc, SMBDIRECT_SOCKET_RESOLVE_ADDR_RUNNING))
			break;
		sc->status = SMBDIRECT_SOCKET_RESOLVE_ROUTE_NEEDED;
		wake_up(&sc->status_wait);
		break;

	case RDMA_CM_EVENT_ROUTE_RESOLVED:
		if (SMBDIRECT_CHECK_STATUS_DISCONNECT(sc, SMBDIRECT_SOCKET_RESOLVE_ROUTE_RUNNING))
			break;
		sc->status = SMBDIRECT_SOCKET_RDMA_CONNECT_NEEDED;
		wake_up(&sc->status_wait);
		break;

	case RDMA_CM_EVENT_ADDR_ERROR:
		log_rdma_event(ERR, "connecting failed event=%s\n", event_name);
		sc->status = SMBDIRECT_SOCKET_RESOLVE_ADDR_FAILED;
		smbd_disconnect_rdma_work(&sc->disconnect_work);
		break;

	case RDMA_CM_EVENT_ROUTE_ERROR:
		log_rdma_event(ERR, "connecting failed event=%s\n", event_name);
		sc->status = SMBDIRECT_SOCKET_RESOLVE_ROUTE_FAILED;
		smbd_disconnect_rdma_work(&sc->disconnect_work);
		break;

	case RDMA_CM_EVENT_ESTABLISHED:
		log_rdma_event(INFO, "connected event=%s\n", event_name);

		/*
		 * Here we work around an inconsistency between
		 * iWarp and other devices (at least rxe and irdma using RoCEv2)
		 */
		if (rdma_protocol_iwarp(id->device, id->port_num)) {
			/*
			 * iWarp devices report the peer's values
			 * with the perspective of the peer here.
			 * Tested with siw and irdma (in iwarp mode)
			 * We need to change to our perspective here,
			 * so we need to switch the values.
			 */
			peer_initiator_depth = event->param.conn.responder_resources;
			peer_responder_resources = event->param.conn.initiator_depth;
		} else {
			/*
			 * Non iWarp devices report the peer's values
			 * already changed to our perspective here.
			 * Tested with rxe and irdma (in roce mode).
			 */
			peer_initiator_depth = event->param.conn.initiator_depth;
			peer_responder_resources = event->param.conn.responder_resources;
		}
		if (rdma_protocol_iwarp(id->device, id->port_num) &&
		    event->param.conn.private_data_len == 8) {
			/*
			 * Legacy clients with only iWarp MPA v1 support
			 * need a private blob in order to negotiate
			 * the IRD/ORD values.
			 */
			const __be32 *ird_ord_hdr = event->param.conn.private_data;
			u32 ird32 = be32_to_cpu(ird_ord_hdr[0]);
			u32 ord32 = be32_to_cpu(ird_ord_hdr[1]);

			/*
			 * cifs.ko sends the legacy IRD/ORD negotiation
			 * event if iWarp MPA v2 was used.
			 *
			 * Here we check that the values match and only
			 * mark the client as legacy if they don't match.
			 */
			if ((u32)event->param.conn.initiator_depth != ird32 ||
			    (u32)event->param.conn.responder_resources != ord32) {
				/*
				 * There are broken clients (old cifs.ko)
				 * using little endian and also
				 * struct rdma_conn_param only uses u8
				 * for initiator_depth and responder_resources,
				 * so we truncate the value to U8_MAX.
				 *
				 * smb_direct_accept_client() will then
				 * do the real negotiation in order to
				 * select the minimum between client and
				 * server.
				 */
				ird32 = min_t(u32, ird32, U8_MAX);
				ord32 = min_t(u32, ord32, U8_MAX);

				sc->rdma.legacy_iwarp = true;
				peer_initiator_depth = (u8)ird32;
				peer_responder_resources = (u8)ord32;
			}
		}

		/*
		 * negotiate the value by using the minimum
		 * between client and server if the client provided
		 * non 0 values.
		 */
		if (peer_initiator_depth != 0)
			sp->initiator_depth =
					min_t(u8, sp->initiator_depth,
					      peer_initiator_depth);
		if (peer_responder_resources != 0)
			sp->responder_resources =
					min_t(u8, sp->responder_resources,
					      peer_responder_resources);

		if (SMBDIRECT_CHECK_STATUS_DISCONNECT(sc, SMBDIRECT_SOCKET_RDMA_CONNECT_RUNNING))
			break;
		sc->status = SMBDIRECT_SOCKET_NEGOTIATE_NEEDED;
		wake_up(&sc->status_wait);
		break;

	case RDMA_CM_EVENT_CONNECT_ERROR:
	case RDMA_CM_EVENT_UNREACHABLE:
	case RDMA_CM_EVENT_REJECTED:
		log_rdma_event(ERR, "connecting failed event=%s\n", event_name);
		sc->status = SMBDIRECT_SOCKET_RDMA_CONNECT_FAILED;
		smbd_disconnect_rdma_work(&sc->disconnect_work);
		break;

	case RDMA_CM_EVENT_DEVICE_REMOVAL:
	case RDMA_CM_EVENT_DISCONNECTED:
		/* This happens when we fail the negotiation */
		if (sc->status == SMBDIRECT_SOCKET_NEGOTIATE_FAILED) {
			log_rdma_event(ERR, "event=%s during negotiation\n", event_name);
		}

		sc->status = SMBDIRECT_SOCKET_DISCONNECTED;
		smbd_disconnect_rdma_work(&sc->disconnect_work);
		break;

	default:
		log_rdma_event(ERR, "unexpected event=%s status=%d\n",
			       event_name, event->status);
		break;
	}

	return 0;
}

/* Upcall from RDMA QP */
static void
smbd_qp_async_error_upcall(struct ib_event *event, void *context)
{
	struct smbdirect_socket *sc = context;

	log_rdma_event(ERR, "%s on device %s socket %p\n",
		ib_event_msg(event->event), event->device->name, sc);

	switch (event->event) {
	case IB_EVENT_CQ_ERR:
	case IB_EVENT_QP_FATAL:
		smbd_disconnect_rdma_connection(sc);
		break;

	default:
		break;
	}
}

static inline void *smbdirect_send_io_payload(struct smbdirect_send_io *request)
{
	return (void *)request->packet;
}

static inline void *smbdirect_recv_io_payload(struct smbdirect_recv_io *response)
{
	return (void *)response->packet;
}

static struct smbdirect_send_io *smbd_alloc_send_io(struct smbdirect_socket *sc)
{
	struct smbdirect_send_io *msg;

	msg = mempool_alloc(sc->send_io.mem.pool, GFP_KERNEL);
	if (!msg)
		return ERR_PTR(-ENOMEM);
	msg->socket = sc;
	INIT_LIST_HEAD(&msg->sibling_list);
	msg->num_sge = 0;

	return msg;
}

static void smbd_free_send_io(struct smbdirect_send_io *msg)
{
	struct smbdirect_socket *sc = msg->socket;
	size_t i;

	/*
	 * The list needs to be empty!
	 * The caller should take care of it.
	 */
	WARN_ON_ONCE(!list_empty(&msg->sibling_list));

	/*
	 * Note we call ib_dma_unmap_page(), even if some sges are mapped using
	 * ib_dma_map_single().
	 *
	 * The difference between _single() and _page() only matters for the
	 * ib_dma_map_*() case.
	 *
	 * For the ib_dma_unmap_*() case it does not matter as both take the
	 * dma_addr_t and dma_unmap_single_attrs() is just an alias to
	 * dma_unmap_page_attrs().
	 */
	for (i = 0; i < msg->num_sge; i++)
		ib_dma_unmap_page(sc->ib.dev,
				  msg->sge[i].addr,
				  msg->sge[i].length,
				  DMA_TO_DEVICE);

	mempool_free(msg, sc->send_io.mem.pool);
}

/* Called when a RDMA send is done */
static void send_done(struct ib_cq *cq, struct ib_wc *wc)
{
	struct smbdirect_send_io *request =
		container_of(wc->wr_cqe, struct smbdirect_send_io, cqe);
	struct smbdirect_socket *sc = request->socket;
	struct smbdirect_send_io *sibling, *next;
	int lcredits = 0;

	log_rdma_send(INFO, "smbdirect_send_io 0x%p completed wc->status=%s\n",
		request, ib_wc_status_msg(wc->status));

	if (unlikely(!(request->wr.send_flags & IB_SEND_SIGNALED))) {
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
		log_rdma_send(ERR,
			"unexpected send completion wc->status=%s (%d) wc->opcode=%d\n",
			ib_wc_status_msg(wc->status), wc->status, wc->opcode);
		smbd_disconnect_rdma_connection(sc);
		return;
	}

	/*
	 * Free possible siblings and then the main send_io
	 */
	list_for_each_entry_safe(sibling, next, &request->sibling_list, sibling_list) {
		list_del_init(&sibling->sibling_list);
		smbd_free_send_io(sibling);
		lcredits += 1;
	}
	/* Note this frees wc->wr_cqe, but not wc */
	smbd_free_send_io(request);
	lcredits += 1;

	if (wc->status != IB_WC_SUCCESS || wc->opcode != IB_WC_SEND) {
skip_free:
		if (wc->status != IB_WC_WR_FLUSH_ERR)
			log_rdma_send(ERR, "wc->status=%s wc->opcode=%d\n",
				ib_wc_status_msg(wc->status), wc->opcode);
		smbd_disconnect_rdma_connection(sc);
		return;
	}

	atomic_add(lcredits, &sc->send_io.lcredits.count);
	wake_up(&sc->send_io.lcredits.wait_queue);

	if (atomic_dec_and_test(&sc->send_io.pending.count))
		wake_up(&sc->send_io.pending.zero_wait_queue);

	wake_up(&sc->send_io.pending.dec_wait_queue);
}

static void dump_smbdirect_negotiate_resp(struct smbdirect_negotiate_resp *resp)
{
	log_rdma_event(INFO, "resp message min_version %u max_version %u negotiated_version %u credits_requested %u credits_granted %u status %u max_readwrite_size %u preferred_send_size %u max_receive_size %u max_fragmented_size %u\n",
		       resp->min_version, resp->max_version,
		       resp->negotiated_version, resp->credits_requested,
		       resp->credits_granted, resp->status,
		       resp->max_readwrite_size, resp->preferred_send_size,
		       resp->max_receive_size, resp->max_fragmented_size);
}

/*
 * Process a negotiation response message, according to [MS-SMBD]3.1.5.7
 * response, packet_length: the negotiation response message
 * return value: true if negotiation is a success, false if failed
 */
static bool process_negotiation_response(
		struct smbdirect_recv_io *response, int packet_length)
{
	struct smbdirect_socket *sc = response->socket;
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	struct smbdirect_negotiate_resp *packet = smbdirect_recv_io_payload(response);

	if (packet_length < sizeof(struct smbdirect_negotiate_resp)) {
		log_rdma_event(ERR,
			"error: packet_length=%d\n", packet_length);
		return false;
	}

	if (le16_to_cpu(packet->negotiated_version) != SMBDIRECT_V1) {
		log_rdma_event(ERR, "error: negotiated_version=%x\n",
			le16_to_cpu(packet->negotiated_version));
		return false;
	}

	if (packet->credits_requested == 0) {
		log_rdma_event(ERR, "error: credits_requested==0\n");
		return false;
	}
	sc->recv_io.credits.target = le16_to_cpu(packet->credits_requested);
	sc->recv_io.credits.target = min_t(u16, sc->recv_io.credits.target, sp->recv_credit_max);

	if (packet->credits_granted == 0) {
		log_rdma_event(ERR, "error: credits_granted==0\n");
		return false;
	}
	atomic_set(&sc->send_io.lcredits.count, sp->send_credit_target);
	atomic_set(&sc->send_io.credits.count, le16_to_cpu(packet->credits_granted));

	if (le32_to_cpu(packet->preferred_send_size) > sp->max_recv_size) {
		log_rdma_event(ERR, "error: preferred_send_size=%d\n",
			le32_to_cpu(packet->preferred_send_size));
		return false;
	}
	sp->max_recv_size = le32_to_cpu(packet->preferred_send_size);

	if (le32_to_cpu(packet->max_receive_size) < SMBD_MIN_RECEIVE_SIZE) {
		log_rdma_event(ERR, "error: max_receive_size=%d\n",
			le32_to_cpu(packet->max_receive_size));
		return false;
	}
	sp->max_send_size = min_t(u32, sp->max_send_size,
				  le32_to_cpu(packet->max_receive_size));

	if (le32_to_cpu(packet->max_fragmented_size) <
			SMBD_MIN_FRAGMENTED_SIZE) {
		log_rdma_event(ERR, "error: max_fragmented_size=%d\n",
			le32_to_cpu(packet->max_fragmented_size));
		return false;
	}
	sp->max_fragmented_send_size =
		le32_to_cpu(packet->max_fragmented_size);


	sp->max_read_write_size = min_t(u32,
			le32_to_cpu(packet->max_readwrite_size),
			sp->max_frmr_depth * PAGE_SIZE);
	sp->max_frmr_depth = sp->max_read_write_size / PAGE_SIZE;

	atomic_set(&sc->send_io.bcredits.count, 1);
	sc->recv_io.expected = SMBDIRECT_EXPECT_DATA_TRANSFER;
	return true;
}

static void smbd_post_send_credits(struct work_struct *work)
{
	int rc;
	struct smbdirect_recv_io *response;
	struct smbdirect_socket *sc =
		container_of(work, struct smbdirect_socket, recv_io.posted.refill_work);
	int posted = 0;

	if (sc->status != SMBDIRECT_SOCKET_CONNECTED) {
		return;
	}

	if (sc->recv_io.credits.target >
		atomic_read(&sc->recv_io.credits.count)) {
		while (true) {
			response = get_receive_buffer(sc);
			if (!response)
				break;

			response->first_segment = false;
			rc = smbd_post_recv(sc, response);
			if (rc) {
				log_rdma_recv(ERR,
					"post_recv failed rc=%d\n", rc);
				put_receive_buffer(sc, response);
				break;
			}

			atomic_inc(&sc->recv_io.posted.count);
			posted += 1;
		}
	}

	atomic_add(posted, &sc->recv_io.credits.available);

	/*
	 * If the last send credit is waiting for credits
	 * it can grant we need to wake it up
	 */
	if (posted &&
	    atomic_read(&sc->send_io.bcredits.count) == 0 &&
	    atomic_read(&sc->send_io.credits.count) == 0)
		wake_up(&sc->send_io.credits.wait_queue);

	/* Promptly send an immediate packet as defined in [MS-SMBD] 3.1.1.1 */
	if (atomic_read(&sc->recv_io.credits.count) <
		sc->recv_io.credits.target - 1) {
		log_keep_alive(INFO, "schedule send of an empty message\n");
		queue_work(sc->workqueue, &sc->idle.immediate_work);
	}
}

/* Called from softirq, when recv is done */
static void recv_done(struct ib_cq *cq, struct ib_wc *wc)
{
	struct smbdirect_data_transfer *data_transfer;
	struct smbdirect_recv_io *response =
		container_of(wc->wr_cqe, struct smbdirect_recv_io, cqe);
	struct smbdirect_socket *sc = response->socket;
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	int current_recv_credits;
	u16 old_recv_credit_target;
	u32 data_offset = 0;
	u32 data_length = 0;
	u32 remaining_data_length = 0;
	bool negotiate_done = false;

	log_rdma_recv(INFO,
		      "response=0x%p type=%d wc status=%s wc opcode %d byte_len=%d pkey_index=%u\n",
		      response, sc->recv_io.expected,
		      ib_wc_status_msg(wc->status), wc->opcode,
		      wc->byte_len, wc->pkey_index);

	if (wc->status != IB_WC_SUCCESS || wc->opcode != IB_WC_RECV) {
		if (wc->status != IB_WC_WR_FLUSH_ERR)
			log_rdma_recv(ERR, "wc->status=%s opcode=%d\n",
				ib_wc_status_msg(wc->status), wc->opcode);
		goto error;
	}

	ib_dma_sync_single_for_cpu(
		wc->qp->device,
		response->sge.addr,
		response->sge.length,
		DMA_FROM_DEVICE);

	/*
	 * Reset timer to the keepalive interval in
	 * order to trigger our next keepalive message.
	 */
	sc->idle.keepalive = SMBDIRECT_KEEPALIVE_NONE;
	mod_delayed_work(sc->workqueue, &sc->idle.timer_work,
			 msecs_to_jiffies(sp->keepalive_interval_msec));

	switch (sc->recv_io.expected) {
	/* SMBD negotiation response */
	case SMBDIRECT_EXPECT_NEGOTIATE_REP:
		dump_smbdirect_negotiate_resp(smbdirect_recv_io_payload(response));
		sc->recv_io.reassembly.full_packet_received = true;
		negotiate_done =
			process_negotiation_response(response, wc->byte_len);
		put_receive_buffer(sc, response);
		if (SMBDIRECT_CHECK_STATUS_WARN(sc, SMBDIRECT_SOCKET_NEGOTIATE_RUNNING))
			negotiate_done = false;
		if (!negotiate_done) {
			sc->status = SMBDIRECT_SOCKET_NEGOTIATE_FAILED;
			smbd_disconnect_rdma_connection(sc);
		} else {
			sc->status = SMBDIRECT_SOCKET_CONNECTED;
			wake_up(&sc->status_wait);
		}

		return;

	/* SMBD data transfer packet */
	case SMBDIRECT_EXPECT_DATA_TRANSFER:
		data_transfer = smbdirect_recv_io_payload(response);

		if (wc->byte_len <
		    offsetof(struct smbdirect_data_transfer, padding))
			goto error;

		remaining_data_length = le32_to_cpu(data_transfer->remaining_data_length);
		data_offset = le32_to_cpu(data_transfer->data_offset);
		data_length = le32_to_cpu(data_transfer->data_length);
		if (wc->byte_len < data_offset ||
		    (u64)wc->byte_len < (u64)data_offset + data_length)
			goto error;

		if (remaining_data_length > sp->max_fragmented_recv_size ||
		    data_length > sp->max_fragmented_recv_size ||
		    (u64)remaining_data_length + (u64)data_length > (u64)sp->max_fragmented_recv_size)
			goto error;

		if (data_length) {
			if (sc->recv_io.reassembly.full_packet_received)
				response->first_segment = true;

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
		if (le16_to_cpu(data_transfer->credits_granted)) {
			atomic_add(le16_to_cpu(data_transfer->credits_granted),
				&sc->send_io.credits.count);
			/*
			 * We have new send credits granted from remote peer
			 * If any sender is waiting for credits, unblock it
			 */
			wake_up(&sc->send_io.credits.wait_queue);
		}

		log_incoming(INFO, "data flags %d data_offset %d data_length %d remaining_data_length %d\n",
			     le16_to_cpu(data_transfer->flags),
			     le32_to_cpu(data_transfer->data_offset),
			     le32_to_cpu(data_transfer->data_length),
			     le32_to_cpu(data_transfer->remaining_data_length));

		/* Send an immediate response right away if requested */
		if (le16_to_cpu(data_transfer->flags) &
				SMBDIRECT_FLAG_RESPONSE_REQUESTED) {
			log_keep_alive(INFO, "schedule send of immediate response\n");
			queue_work(sc->workqueue, &sc->idle.immediate_work);
		}

		/*
		 * If this is a packet with data playload place the data in
		 * reassembly queue and wake up the reading thread
		 */
		if (data_length) {
			if (current_recv_credits <= (sc->recv_io.credits.target / 4) ||
			    sc->recv_io.credits.target > old_recv_credit_target)
				queue_work(sc->workqueue, &sc->recv_io.posted.refill_work);

			enqueue_reassembly(sc, response, data_length);
			wake_up(&sc->recv_io.reassembly.wait_queue);
		} else
			put_receive_buffer(sc, response);

		return;

	case SMBDIRECT_EXPECT_NEGOTIATE_REQ:
		/* Only server... */
		break;
	}

	/*
	 * This is an internal error!
	 */
	log_rdma_recv(ERR, "unexpected response type=%d\n", sc->recv_io.expected);
	WARN_ON_ONCE(sc->recv_io.expected != SMBDIRECT_EXPECT_DATA_TRANSFER);
error:
	put_receive_buffer(sc, response);
	smbd_disconnect_rdma_connection(sc);
}

static struct rdma_cm_id *smbd_create_id(
		struct smbdirect_socket *sc,
		struct sockaddr *dstaddr, int port)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	struct rdma_cm_id *id;
	int rc;
	__be16 *sport;

	id = rdma_create_id(&init_net, smbd_conn_upcall, sc,
		RDMA_PS_TCP, IB_QPT_RC);
	if (IS_ERR(id)) {
		rc = PTR_ERR(id);
		log_rdma_event(ERR, "rdma_create_id() failed %i\n", rc);
		return id;
	}

	if (dstaddr->sa_family == AF_INET6)
		sport = &((struct sockaddr_in6 *)dstaddr)->sin6_port;
	else
		sport = &((struct sockaddr_in *)dstaddr)->sin_port;

	*sport = htons(port);

	WARN_ON_ONCE(sc->status != SMBDIRECT_SOCKET_RESOLVE_ADDR_NEEDED);
	sc->status = SMBDIRECT_SOCKET_RESOLVE_ADDR_RUNNING;
	rc = rdma_resolve_addr(id, NULL, (struct sockaddr *)dstaddr,
		sp->resolve_addr_timeout_msec);
	if (rc) {
		log_rdma_event(ERR, "rdma_resolve_addr() failed %i\n", rc);
		goto out;
	}
	rc = wait_event_interruptible_timeout(
		sc->status_wait,
		sc->status != SMBDIRECT_SOCKET_RESOLVE_ADDR_RUNNING,
		msecs_to_jiffies(sp->resolve_addr_timeout_msec));
	/* e.g. if interrupted returns -ERESTARTSYS */
	if (rc < 0) {
		log_rdma_event(ERR, "rdma_resolve_addr timeout rc: %i\n", rc);
		goto out;
	}
	if (sc->status == SMBDIRECT_SOCKET_RESOLVE_ADDR_RUNNING) {
		rc = -ETIMEDOUT;
		log_rdma_event(ERR, "rdma_resolve_addr() completed %i\n", rc);
		goto out;
	}
	if (sc->status != SMBDIRECT_SOCKET_RESOLVE_ROUTE_NEEDED) {
		rc = -EHOSTUNREACH;
		log_rdma_event(ERR, "rdma_resolve_addr() completed %i\n", rc);
		goto out;
	}

	WARN_ON_ONCE(sc->status != SMBDIRECT_SOCKET_RESOLVE_ROUTE_NEEDED);
	sc->status = SMBDIRECT_SOCKET_RESOLVE_ROUTE_RUNNING;
	rc = rdma_resolve_route(id, sp->resolve_route_timeout_msec);
	if (rc) {
		log_rdma_event(ERR, "rdma_resolve_route() failed %i\n", rc);
		goto out;
	}
	rc = wait_event_interruptible_timeout(
		sc->status_wait,
		sc->status != SMBDIRECT_SOCKET_RESOLVE_ROUTE_RUNNING,
		msecs_to_jiffies(sp->resolve_route_timeout_msec));
	/* e.g. if interrupted returns -ERESTARTSYS */
	if (rc < 0)  {
		log_rdma_event(ERR, "rdma_resolve_addr timeout rc: %i\n", rc);
		goto out;
	}
	if (sc->status == SMBDIRECT_SOCKET_RESOLVE_ROUTE_RUNNING) {
		rc = -ETIMEDOUT;
		log_rdma_event(ERR, "rdma_resolve_route() completed %i\n", rc);
		goto out;
	}
	if (sc->status != SMBDIRECT_SOCKET_RDMA_CONNECT_NEEDED) {
		rc = -ENETUNREACH;
		log_rdma_event(ERR, "rdma_resolve_route() completed %i\n", rc);
		goto out;
	}

	return id;

out:
	rdma_destroy_id(id);
	return ERR_PTR(rc);
}

/*
 * Test if FRWR (Fast Registration Work Requests) is supported on the device
 * This implementation requires FRWR on RDMA read/write
 * return value: true if it is supported
 */
static bool frwr_is_supported(struct ib_device_attr *attrs)
{
	if (!(attrs->device_cap_flags & IB_DEVICE_MEM_MGT_EXTENSIONS))
		return false;
	if (attrs->max_fast_reg_page_list_len == 0)
		return false;
	return true;
}

static int smbd_ia_open(
		struct smbdirect_socket *sc,
		struct sockaddr *dstaddr, int port)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	int rc;

	WARN_ON_ONCE(sc->status != SMBDIRECT_SOCKET_CREATED);
	sc->status = SMBDIRECT_SOCKET_RESOLVE_ADDR_NEEDED;

	sc->rdma.cm_id = smbd_create_id(sc, dstaddr, port);
	if (IS_ERR(sc->rdma.cm_id)) {
		rc = PTR_ERR(sc->rdma.cm_id);
		goto out1;
	}
	sc->ib.dev = sc->rdma.cm_id->device;

	if (!frwr_is_supported(&sc->ib.dev->attrs)) {
		log_rdma_event(ERR, "Fast Registration Work Requests (FRWR) is not supported\n");
		log_rdma_event(ERR, "Device capability flags = %llx max_fast_reg_page_list_len = %u\n",
			       sc->ib.dev->attrs.device_cap_flags,
			       sc->ib.dev->attrs.max_fast_reg_page_list_len);
		rc = -EPROTONOSUPPORT;
		goto out2;
	}
	sp->max_frmr_depth = min_t(u32,
		sp->max_frmr_depth,
		sc->ib.dev->attrs.max_fast_reg_page_list_len);
	sc->mr_io.type = IB_MR_TYPE_MEM_REG;
	if (sc->ib.dev->attrs.kernel_cap_flags & IBK_SG_GAPS_REG)
		sc->mr_io.type = IB_MR_TYPE_SG_GAPS;

	return 0;

out2:
	rdma_destroy_id(sc->rdma.cm_id);
	sc->rdma.cm_id = NULL;

out1:
	return rc;
}

/*
 * Send a negotiation request message to the peer
 * The negotiation procedure is in [MS-SMBD] 3.1.5.2 and 3.1.5.3
 * After negotiation, the transport is connected and ready for
 * carrying upper layer SMB payload
 */
static int smbd_post_send_negotiate_req(struct smbdirect_socket *sc)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	int rc;
	struct smbdirect_send_io *request;
	struct smbdirect_negotiate_req *packet;

	request = smbd_alloc_send_io(sc);
	if (IS_ERR(request))
		return PTR_ERR(request);

	packet = smbdirect_send_io_payload(request);
	packet->min_version = cpu_to_le16(SMBDIRECT_V1);
	packet->max_version = cpu_to_le16(SMBDIRECT_V1);
	packet->reserved = 0;
	packet->credits_requested = cpu_to_le16(sp->send_credit_target);
	packet->preferred_send_size = cpu_to_le32(sp->max_send_size);
	packet->max_receive_size = cpu_to_le32(sp->max_recv_size);
	packet->max_fragmented_size =
		cpu_to_le32(sp->max_fragmented_recv_size);

	request->sge[0].addr = ib_dma_map_single(
				sc->ib.dev, (void *)packet,
				sizeof(*packet), DMA_TO_DEVICE);
	if (ib_dma_mapping_error(sc->ib.dev, request->sge[0].addr)) {
		rc = -EIO;
		goto dma_mapping_failed;
	}
	request->num_sge = 1;

	request->sge[0].length = sizeof(*packet);
	request->sge[0].lkey = sc->ib.pd->local_dma_lkey;

	rc = smbd_post_send(sc, NULL, request);
	if (!rc)
		return 0;

	if (rc == -EAGAIN)
		rc = -EIO;

dma_mapping_failed:
	smbd_free_send_io(request);
	return rc;
}

/*
 * Extend the credits to remote peer
 * This implements [MS-SMBD] 3.1.5.9
 * The idea is that we should extend credits to remote peer as quickly as
 * it's allowed, to maintain data flow. We allocate as much receive
 * buffer as possible, and extend the receive credits to remote peer
 * return value: the new credtis being granted.
 */
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

/*
 * Check if we need to send a KEEP_ALIVE message
 * The idle connection timer triggers a KEEP_ALIVE message when expires
 * SMBDIRECT_FLAG_RESPONSE_REQUESTED is set in the message flag to have peer send
 * back a response.
 * return value:
 * 1 if SMBDIRECT_FLAG_RESPONSE_REQUESTED needs to be set
 * 0: otherwise
 */
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

static int smbd_ib_post_send(struct smbdirect_socket *sc,
			     struct ib_send_wr *wr)
{
	int ret;

	atomic_inc(&sc->send_io.pending.count);
	ret = ib_post_send(sc->ib.qp, wr, NULL);
	if (ret) {
		pr_err("failed to post send: %d\n", ret);
		smbd_disconnect_rdma_connection(sc);
		ret = -EAGAIN;
	}
	return ret;
}

/* Post the send request */
static int smbd_post_send(struct smbdirect_socket *sc,
			  struct smbdirect_send_batch *batch,
			  struct smbdirect_send_io *request)
{
	int i;

	for (i = 0; i < request->num_sge; i++) {
		log_rdma_send(INFO,
			"rdma_request sge[%d] addr=0x%llx length=%u\n",
			i, request->sge[i].addr, request->sge[i].length);
		ib_dma_sync_single_for_device(
			sc->ib.dev,
			request->sge[i].addr,
			request->sge[i].length,
			DMA_TO_DEVICE);
	}

	request->cqe.done = send_done;
	request->wr.next = NULL;
	request->wr.sg_list = request->sge;
	request->wr.num_sge = request->num_sge;
	request->wr.opcode = IB_WR_SEND;

	if (batch) {
		request->wr.wr_cqe = NULL;
		request->wr.send_flags = 0;
		if (!list_empty(&batch->msg_list)) {
			struct smbdirect_send_io *last;

			last = list_last_entry(&batch->msg_list,
					       struct smbdirect_send_io,
					       sibling_list);
			last->wr.next = &request->wr;
		}
		list_add_tail(&request->sibling_list, &batch->msg_list);
		batch->wr_cnt++;
		return 0;
	}

	request->wr.wr_cqe = &request->cqe;
	request->wr.send_flags = IB_SEND_SIGNALED;
	return smbd_ib_post_send(sc, &request->wr);
}

static void smbd_send_batch_init(struct smbdirect_send_batch *batch,
				 bool need_invalidate_rkey,
				 unsigned int remote_key)
{
	INIT_LIST_HEAD(&batch->msg_list);
	batch->wr_cnt = 0;
	batch->need_invalidate_rkey = need_invalidate_rkey;
	batch->remote_key = remote_key;
	batch->credit = 0;
}

static int smbd_send_batch_flush(struct smbdirect_socket *sc,
				 struct smbdirect_send_batch *batch,
				 bool is_last)
{
	struct smbdirect_send_io *first, *last;
	int ret = 0;

	if (list_empty(&batch->msg_list))
		goto release_credit;

	first = list_first_entry(&batch->msg_list,
				 struct smbdirect_send_io,
				 sibling_list);
	last = list_last_entry(&batch->msg_list,
			       struct smbdirect_send_io,
			       sibling_list);

	if (batch->need_invalidate_rkey) {
		first->wr.opcode = IB_WR_SEND_WITH_INV;
		first->wr.ex.invalidate_rkey = batch->remote_key;
		batch->need_invalidate_rkey = false;
		batch->remote_key = 0;
	}

	last->wr.send_flags = IB_SEND_SIGNALED;
	last->wr.wr_cqe = &last->cqe;

	/*
	 * Remove last from batch->msg_list
	 * and splice the rest of batch->msg_list
	 * to last->sibling_list.
	 *
	 * batch->msg_list is a valid empty list
	 * at the end.
	 */
	list_del_init(&last->sibling_list);
	list_splice_tail_init(&batch->msg_list, &last->sibling_list);
	batch->wr_cnt = 0;

	ret = smbd_ib_post_send(sc, &first->wr);
	if (ret) {
		struct smbdirect_send_io *sibling, *next;

		list_for_each_entry_safe(sibling, next, &last->sibling_list, sibling_list) {
			list_del_init(&sibling->sibling_list);
			smbd_free_send_io(sibling);
		}
		smbd_free_send_io(last);
	}

release_credit:
	if (is_last && !ret && batch->credit) {
		atomic_add(batch->credit, &sc->send_io.bcredits.count);
		batch->credit = 0;
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
				 struct smbdirect_send_batch *batch)
{
	int ret;

	if (batch->credit)
		return 0;

	ret = wait_for_credits(sc,
			       &sc->send_io.bcredits.wait_queue,
			       &sc->send_io.bcredits.count,
			       1);
	if (ret)
		return ret;

	batch->credit = 1;
	return 0;
}

static int wait_for_send_lcredit(struct smbdirect_socket *sc,
				 struct smbdirect_send_batch *batch)
{
	if (batch && (atomic_read(&sc->send_io.lcredits.count) <= 1)) {
		int ret;

		ret = smbd_send_batch_flush(sc, batch, false);
		if (ret)
			return ret;
	}

	return wait_for_credits(sc,
				&sc->send_io.lcredits.wait_queue,
				&sc->send_io.lcredits.count,
				1);
}

static int wait_for_send_credits(struct smbdirect_socket *sc,
				 struct smbdirect_send_batch *batch)
{
	if (batch &&
	    (batch->wr_cnt >= 16 || atomic_read(&sc->send_io.credits.count) <= 1)) {
		int ret;

		ret = smbd_send_batch_flush(sc, batch, false);
		if (ret)
			return ret;
	}

	return wait_for_credits(sc,
				&sc->send_io.credits.wait_queue,
				&sc->send_io.credits.count,
				1);
}

static int smbd_post_send_iter(struct smbdirect_socket *sc,
			       struct smbdirect_send_batch *batch,
			       struct iov_iter *iter,
			       int *_remaining_data_length)
{
	struct smbdirect_socket_parameters *sp = &sc->parameters;
	int rc;
	int header_length;
	int data_length;
	struct smbdirect_send_io *request;
	struct smbdirect_data_transfer *packet;
	int new_credits = 0;
	struct smbdirect_send_batch _batch;

	if (!batch) {
		smbd_send_batch_init(&_batch, false, 0);
		batch = &_batch;
	}

	rc = wait_for_send_bcredit(sc, batch);
	if (rc) {
		log_outgoing(ERR, "disconnected not sending on wait_bcredit\n");
		rc = -EAGAIN;
		goto err_wait_bcredit;
	}

	rc = wait_for_send_lcredit(sc, batch);
	if (rc) {
		log_outgoing(ERR, "disconnected not sending on wait_lcredit\n");
		rc = -EAGAIN;
		goto err_wait_lcredit;
	}

	rc = wait_for_send_credits(sc, batch);
	if (rc) {
		log_outgoing(ERR, "disconnected not sending on wait_credit\n");
		rc = -EAGAIN;
		goto err_wait_credit;
	}

	new_credits = manage_credits_prior_sending(sc);
	if (new_credits == 0 &&
	    atomic_read(&sc->send_io.credits.count) == 0 &&
	    atomic_read(&sc->recv_io.credits.count) == 0) {
		queue_work(sc->workqueue, &sc->recv_io.posted.refill_work);
		rc = wait_event_interruptible(sc->send_io.credits.wait_queue,
					      atomic_read(&sc->send_io.credits.count) >= 1 ||
					      atomic_read(&sc->recv_io.credits.available) >= 1 ||
					      sc->status != SMBDIRECT_SOCKET_CONNECTED);
		if (sc->status != SMBDIRECT_SOCKET_CONNECTED)
			rc = -ENOTCONN;
		if (rc < 0) {
			log_outgoing(ERR, "disconnected not sending on last credit\n");
			rc = -EAGAIN;
			goto err_wait_credit;
		}

		new_credits = manage_credits_prior_sending(sc);
	}

	request = smbd_alloc_send_io(sc);
	if (IS_ERR(request)) {
		rc = PTR_ERR(request);
		goto err_alloc;
	}

	memset(request->sge, 0, sizeof(request->sge));

	/* Map the packet to DMA */
	header_length = sizeof(struct smbdirect_data_transfer);
	/* If this is a packet without payload, don't send padding */
	if (!iter)
		header_length = offsetof(struct smbdirect_data_transfer, padding);

	packet = smbdirect_send_io_payload(request);
	request->sge[0].addr = ib_dma_map_single(sc->ib.dev,
						 (void *)packet,
						 header_length,
						 DMA_TO_DEVICE);
	if (ib_dma_mapping_error(sc->ib.dev, request->sge[0].addr)) {
		rc = -EIO;
		goto err_dma;
	}

	request->sge[0].length = header_length;
	request->sge[0].lkey = sc->ib.pd->local_dma_lkey;
	request->num_sge = 1;

	/* Fill in the data payload to find out how much data we can add */
	if (iter) {
		struct smb_extract_to_rdma extract = {
			.nr_sge		= request->num_sge,
			.max_sge	= SMBDIRECT_SEND_IO_MAX_SGE,
			.sge		= request->sge,
			.device		= sc->ib.dev,
			.local_dma_lkey	= sc->ib.pd->local_dma_lkey,
			.direction	= DMA_TO_DEVICE,
		};
		size_t payload_len = umin(*_remaining_data_length,
					  sp->max_send_size - sizeof(*packet));

		rc = smb_extract_iter_to_rdma(iter, payload_len,
					      &extract);
		if (rc < 0)
			goto err_dma;
		data_length = rc;
		request->num_sge = extract.nr_sge;
		*_remaining_data_length -= data_length;
	} else {
		data_length = 0;
	}

	/* Fill in the packet header */
	packet->credits_requested = cpu_to_le16(sp->send_credit_target);
	packet->credits_granted = cpu_to_le16(new_credits);

	packet->flags = 0;
	if (manage_keep_alive_before_sending(sc))
		packet->flags |= cpu_to_le16(SMBDIRECT_FLAG_RESPONSE_REQUESTED);

	packet->reserved = 0;
	if (!data_length)
		packet->data_offset = 0;
	else
		packet->data_offset = cpu_to_le32(24);
	packet->data_length = cpu_to_le32(data_length);
	packet->remaining_data_length = cpu_to_le32(*_remaining_data_length);
	packet->padding = 0;

	log_outgoing(INFO, "credits_requested=%d credits_granted=%d data_offset=%d data_length=%d remaining_data_length=%d\n",
		     le16_to_cpu(packet->credits_requested),
		     le16_to_cpu(packet->credits_granted),
		     le32_to_cpu(packet->data_offset),
		     le32_to_cpu(packet->data_length),
		     le32_to_cpu(packet->remaining_data_length));

	rc = smbd_post_send(sc, batch, request);
	if (!rc) {
		/*
		 * From here request is moved to batch
		 * and we should not free it explicitly.
		 */

		if (batch != &_batch)
			return 0;

		rc = smbd_send_batch_flush(sc, batch, true);
		if (!rc)
			return 0;

		goto err_flush;
	}

err_dma:
	smbd_free_send_io(request);

err_flush:
err_alloc:
	atomic_inc(&sc->send_io.credits.count);
	wake_up(&sc->send_io.credits.wait_queue);

err_wait_credit:
	atomic_inc(&sc->send_io.lcredits.count);
	wake_up(&sc->send_io.lcredits.wait_queue);

err_wait_lcredit:
	atomic_add(batch->credit, &sc->send_io.bcredits.count);
	batch->credit = 0;
	wake_up(&sc->send_io.bcredits.wait_queue);

err_wait_bcredit:
	return rc;
}

/*
 * Send an empty message
 * Empty message is used to extend credits to peer to for keep live
 * while there is no upper layer payload to send at the time
 */
static int smbd_post_send_empty(struct smbdirect_socket *sc)
{
	int remaining_data_length = 0;

	sc->statistics.send_empty++;
	return smbd_post_send_iter(sc, NULL, NULL, &remaining_data_length);
}

>>>>>>> hardened/6.19
static int smbd_post_send_full_iter(struct smbdirect_socket *sc,
				    struct smbdirect_send_batch *batch,
				    struct iov_iter *iter,
				    u32 remaining_data_length)
{
	int bytes = 0;

	/*
	 * smbdirect_connection_send_single_iter() respects the
	 * negotiated max_send_size, so we need to
	 * loop until the full iter is posted
	 */

	while (iov_iter_count(iter) > 0) {
<<<<<<< HEAD
		int rc;

		rc = smbdirect_connection_send_single_iter(sc,
							   batch,
							   iter,
							   0, /* flags */
							   remaining_data_length);
||||||| 05f7e89ab9731
		rc = smbd_post_send_iter(sc, iter, _remaining_data_length);
=======
		rc = smbd_post_send_iter(sc, batch, iter, _remaining_data_length);
>>>>>>> hardened/6.19
		if (rc < 0)
			return rc;
		remaining_data_length -= rc;
		bytes += rc;
	}

	return bytes;
}

/*
 * Destroy the transport and related RDMA and memory resources
 * Need to go through all the pending counters and make sure on one is using
 * the transport while it is destroyed
 */
void smbd_destroy(struct TCP_Server_Info *server)
{
	struct smbd_connection *info = server->smbd_conn;

	if (!info) {
		log_rdma_event(INFO, "rdma session already destroyed\n");
		return;
	}

	smbdirect_socket_release(info->socket);

	kfree(info);
	server->smbd_conn = NULL;
}

/*
 * Reconnect this SMBD connection, called from upper layer
 * return value: 0 on success, or actual error code
 */
int smbd_reconnect(struct TCP_Server_Info *server)
{
	log_rdma_event(INFO, "reconnecting rdma session\n");

	if (!server->smbd_conn) {
		log_rdma_event(INFO, "rdma session already destroyed\n");
		goto create_conn;
	}

	/*
	 * This is possible if transport is disconnected and we haven't received
	 * notification from RDMA, but upper layer has detected timeout
	 */
	log_rdma_event(INFO, "disconnecting transport\n");
	smbd_destroy(server);

create_conn:
	log_rdma_event(INFO, "creating rdma session\n");
	server->smbd_conn = smbd_get_connection(
		server, (struct sockaddr *) &server->dstaddr);

	if (server->smbd_conn) {
		cifs_dbg(VFS, "RDMA transport re-established\n");
		trace_smb3_smbd_connect_done(server->hostname, server->conn_id, &server->dstaddr);
		return 0;
	}
	trace_smb3_smbd_connect_err(server->hostname, server->conn_id, &server->dstaddr);
	return -ENOENT;
}

/* Create a SMBD connection, called by upper layer */
static struct smbd_connection *_smbd_get_connection(
	struct TCP_Server_Info *server, struct sockaddr *dstaddr, int port)
{
	struct net *net = cifs_net_ns(server);
	struct smbd_connection *info;
	struct smbdirect_socket *sc;
	struct smbdirect_socket_parameters init_params = {};
	struct smbdirect_socket_parameters *sp;
	__be16 *sport;
	u64 port_flags = 0;
	int ret;

	switch (port) {
	case SMBD_PORT:
		/*
		 * only allow iWarp devices
		 * for port 5445.
		 */
		port_flags |= SMBDIRECT_FLAG_PORT_RANGE_ONLY_IW;
		break;
	case SMB_PORT:
		/*
		 * only allow InfiniBand, RoCEv1 or RoCEv2
		 * devices for port 445.
		 *
		 * (Basically don't allow iWarp devices)
		 */
		port_flags |= SMBDIRECT_FLAG_PORT_RANGE_ONLY_IB;
		break;
	}

	/*
	 * Create the initial parameters
	 */
	sp = &init_params;
	sp->flags = port_flags;
	sp->resolve_addr_timeout_msec = RDMA_RESOLVE_TIMEOUT;
	sp->resolve_route_timeout_msec = RDMA_RESOLVE_TIMEOUT;
	sp->rdma_connect_timeout_msec = RDMA_RESOLVE_TIMEOUT;
	sp->negotiate_timeout_msec = SMBD_NEGOTIATE_TIMEOUT * 1000;
	sp->initiator_depth = 1;
	sp->responder_resources = SMBD_CM_RESPONDER_RESOURCES;
	sp->recv_credit_max = smbd_receive_credit_max;
	sp->send_credit_target = smbd_send_credit_target;
	sp->max_send_size = smbd_max_send_size;
	sp->max_fragmented_recv_size = smbd_max_fragmented_recv_size;
	sp->max_recv_size = smbd_max_receive_size;
	sp->max_frmr_depth = smbd_max_frmr_depth;
	sp->keepalive_interval_msec = smbd_keep_alive_interval * 1000;
	sp->keepalive_timeout_msec = KEEPALIVE_RECV_TIMEOUT * 1000;

	info = kzalloc_obj(*info);
	if (!info)
		return NULL;
	ret = smbdirect_socket_create_kern(net, &sc);
	if (ret)
		goto socket_init_failed;
	smbdirect_socket_set_logging(sc, NULL, smbd_logging_needed, smbd_logging_vaprintf);
	ret = smbdirect_socket_set_initial_parameters(sc, sp);
	if (ret)
		goto set_params_failed;
	ret = smbdirect_socket_set_kernel_settings(sc, IB_POLL_SOFTIRQ, GFP_KERNEL);
	if (ret)
		goto set_settings_failed;

	if (dstaddr->sa_family == AF_INET6)
		sport = &((struct sockaddr_in6 *)dstaddr)->sin6_port;
	else
		sport = &((struct sockaddr_in *)dstaddr)->sin_port;

	*sport = htons(port);

	ret = smbdirect_connect_sync(sc, dstaddr);
	if (ret) {
		log_rdma_event(ERR, "connect to %pISpsfc failed: %1pe\n",
			       dstaddr, ERR_PTR(ret));
		goto connect_failed;
	}

	info->socket = sc;
	return info;

connect_failed:
set_settings_failed:
set_params_failed:
	smbdirect_socket_release(sc);
socket_init_failed:
	kfree(info);
	return NULL;
}

const struct smbdirect_socket_parameters *smbd_get_parameters(struct smbd_connection *conn)
{
	if (unlikely(!conn->socket)) {
		static const struct smbdirect_socket_parameters zero_params;

		return &zero_params;
	}

	return smbdirect_socket_get_current_parameters(conn->socket);
}

struct smbd_connection *smbd_get_connection(
	struct TCP_Server_Info *server, struct sockaddr *dstaddr)
{
	struct smbd_connection *ret;
	const struct smbdirect_socket_parameters *sp;
	int port = SMBD_PORT;

try_again:
	ret = _smbd_get_connection(server, dstaddr, port);

	/* Try SMB_PORT if SMBD_PORT doesn't work */
	if (!ret && port == SMBD_PORT) {
		port = SMB_PORT;
		goto try_again;
	}
	if (!ret)
		return NULL;

	sp = smbd_get_parameters(ret);

	server->rdma_readwrite_threshold =
		rdma_readwrite_threshold > sp->max_fragmented_send_size ?
		sp->max_fragmented_send_size :
		rdma_readwrite_threshold;

	return ret;
}

/*
 * Receive data from the transport's receive reassembly queue
 * All the incoming data packets are placed in reassembly queue
 * iter: the buffer to read data into
 * size: the length of data to read
 * return value: actual data read
 *
 * Note: this implementation copies the data from reassembly queue to receive
 * buffers used by upper layer. This is not the optimal code path. A better way
 * to do it is to not have upper layer allocate its receive buffers but rather
 * borrow the buffer from reassembly queue, and return it after data is
 * consumed. But this will require more changes to upper layer code, and also
 * need to consider packet boundaries while they still being reassembled.
 */
int smbd_recv(struct smbd_connection *info, struct msghdr *msg)
{
	struct smbdirect_socket *sc = info->socket;

	if (!smbdirect_connection_is_connected(sc))
		return -ENOTCONN;

	return smbdirect_connection_recvmsg(sc, msg, 0);
}

/*
 * Send data to transport
 * Each rqst is transported as a SMBDirect payload
 * rqst: the data to write
 * return value: 0 if successfully write, otherwise error code
 */
int smbd_send(struct TCP_Server_Info *server,
	int num_rqst, struct smb_rqst *rqst_array)
{
	struct smbd_connection *info = server->smbd_conn;
	struct smbdirect_socket *sc = info->socket;
	const struct smbdirect_socket_parameters *sp = smbd_get_parameters(info);
	struct smb_rqst *rqst;
	struct iov_iter iter;
<<<<<<< HEAD
	struct smbdirect_send_batch_storage bstorage;
	struct smbdirect_send_batch *batch;
||||||| 05f7e89ab9731
=======
	struct smbdirect_send_batch batch;
>>>>>>> hardened/6.19
	unsigned int remaining_data_length, klen;
	int rc, i, rqst_idx;
	int error = 0;

	if (!smbdirect_connection_is_connected(sc))
		return -EAGAIN;

	/*
	 * Add in the page array if there is one. The caller needs to set
	 * rq_tailsz to PAGE_SIZE when the buffer has multiple pages and
	 * ends at page boundary
	 */
	remaining_data_length = 0;
	for (i = 0; i < num_rqst; i++)
		remaining_data_length += smb_rqst_len(server, &rqst_array[i]);

	if (unlikely(remaining_data_length > sp->max_fragmented_send_size)) {
		/* assertion: payload never exceeds negotiated maximum */
		log_write(ERR, "payload size %d > max size %d\n",
			remaining_data_length, sp->max_fragmented_send_size);
		return -EINVAL;
	}

	log_write(INFO, "num_rqst=%d total length=%u\n",
			num_rqst, remaining_data_length);

	rqst_idx = 0;
<<<<<<< HEAD
	batch = smbdirect_init_send_batch_storage(&bstorage, false, 0);
||||||| 05f7e89ab9731
=======
	smbd_send_batch_init(&batch, false, 0);
>>>>>>> hardened/6.19
	do {
		rqst = &rqst_array[rqst_idx];

		cifs_dbg(FYI, "Sending smb (RDMA): idx=%d smb_len=%lu\n",
			 rqst_idx, smb_rqst_len(server, rqst));
		for (i = 0; i < rqst->rq_nvec; i++)
			dump_smb(rqst->rq_iov[i].iov_base, rqst->rq_iov[i].iov_len);

		log_write(INFO, "RDMA-WR[%u] nvec=%d len=%u iter=%zu rqlen=%lu\n",
			  rqst_idx, rqst->rq_nvec, remaining_data_length,
			  iov_iter_count(&rqst->rq_iter), smb_rqst_len(server, rqst));

		/* Send the metadata pages. */
		klen = 0;
		for (i = 0; i < rqst->rq_nvec; i++)
			klen += rqst->rq_iov[i].iov_len;
		iov_iter_kvec(&iter, ITER_SOURCE, rqst->rq_iov, rqst->rq_nvec, klen);

<<<<<<< HEAD
		rc = smbd_post_send_full_iter(sc, batch, &iter, remaining_data_length);
		if (rc < 0) {
			error = rc;
||||||| 05f7e89ab9731
		rc = smbd_post_send_full_iter(sc, &iter, &remaining_data_length);
		if (rc < 0)
=======
		rc = smbd_post_send_full_iter(sc, &batch, &iter, &remaining_data_length);
		if (rc < 0) {
			error = rc;
>>>>>>> hardened/6.19
			break;
<<<<<<< HEAD
		}
		remaining_data_length -= rc;
||||||| 05f7e89ab9731
=======
		}
>>>>>>> hardened/6.19

		if (iov_iter_count(&rqst->rq_iter) > 0) {
			/* And then the data pages if there are any */
<<<<<<< HEAD
			rc = smbd_post_send_full_iter(sc, batch, &rqst->rq_iter,
						      remaining_data_length);
			if (rc < 0) {
				error = rc;
||||||| 05f7e89ab9731
			rc = smbd_post_send_full_iter(sc, &rqst->rq_iter,
						      &remaining_data_length);
			if (rc < 0)
=======
			rc = smbd_post_send_full_iter(sc, &batch, &rqst->rq_iter,
						      &remaining_data_length);
			if (rc < 0) {
				error = rc;
>>>>>>> hardened/6.19
				break;
<<<<<<< HEAD
			}
			remaining_data_length -= rc;
||||||| 05f7e89ab9731
=======
			}
>>>>>>> hardened/6.19
		}

	} while (++rqst_idx < num_rqst);

<<<<<<< HEAD
	rc = smbdirect_connection_send_batch_flush(sc, batch, true);
	if (unlikely(!rc && error))
		rc = error;

||||||| 05f7e89ab9731
=======
	rc = smbd_send_batch_flush(sc, &batch, true);
	if (unlikely(!rc && error))
		rc = error;

>>>>>>> hardened/6.19
	/*
	 * As an optimization, we don't wait for individual I/O to finish
	 * before sending the next one.
	 * Send them all and wait for pending send count to get to 0
	 * that means all the I/Os have been out and we are good to return
	 */

	error = rc;
	rc = smbdirect_connection_send_wait_zero_pending(sc);
	if (unlikely(rc && !error))
		error = -EAGAIN;

	if (unlikely(error))
		return error;

	return 0;
}

/*
 * Register memory for RDMA read/write
 * iter: the buffer to register memory with
 * writing: true if this is a RDMA write (SMB read), false for RDMA read
 * need_invalidate: true if this MR needs to be locally invalidated after I/O
 * return value: the MR registered, NULL if failed.
 */
struct smbdirect_mr_io *smbd_register_mr(struct smbd_connection *info,
				 struct iov_iter *iter,
				 bool writing, bool need_invalidate)
{
	struct smbdirect_socket *sc = info->socket;

	if (!smbdirect_connection_is_connected(sc))
		return NULL;

	return smbdirect_connection_register_mr_io(sc, iter, writing, need_invalidate);
}

void smbd_mr_fill_buffer_descriptor(struct smbdirect_mr_io *mr,
				    struct smbdirect_buffer_descriptor_v1 *v1)
{
	smbdirect_mr_io_fill_buffer_descriptor(mr, v1);
}

/*
 * Deregister a MR after I/O is done
 * This function may wait if remote invalidation is not used
 * and we have to locally invalidate the buffer to prevent data is being
 * modified by remote peer after upper layer consumes it
 */
void smbd_deregister_mr(struct smbdirect_mr_io *mr)
{
	smbdirect_connection_deregister_mr_io(mr);
}

void smbd_debug_proc_show(struct TCP_Server_Info *server, struct seq_file *m)
{
	if (!server->rdma)
		return;

	if (!server->smbd_conn) {
		seq_puts(m, "\nSMBDirect transport not available");
		return;
	}

	smbdirect_connection_legacy_debug_proc_show(server->smbd_conn->socket,
						    server->rdma_readwrite_threshold,
						    m);
}
