#ifndef RUDP_STRUCTS_H
#define	RUDP_STRUCTS_H

#include "rudp.h"
#include "rudp_api.h"
#include "vsftp.h"

struct data_node {
	u_int32_t index;
	struct vsftp data;
	int data_len;
	struct data_node * next;
};

struct retry_rudp_packet {
	char *data;
	int data_len;
	u_int8_t n_retries;
	struct rudp_socket *rsocket;
	struct sockaddr_in6 *dest_addr;
};

struct timeout_arg {
	u_int32_t seqno;
	struct retry_rudp_packet * rpacket_addr;	
	struct timeout_arg * next;
};

struct sliding_window {
	u_int32_t begin_index;
	u_int32_t end_index;
	//u_int8_t has_sent[RUDP_WINDOW];
	struct data_node *end_data_node;
};

struct peer {
	struct sockaddr_in6 addr;

	union {
		u_int32_t latest_data_index;
		struct sliding_window swindow;
	} window_info;
	
	u_int8_t has_syn;
	u_int8_t has_vs_begin;
	u_int32_t base_seq_no;

	u_int8_t started;
	u_int8_t finished;

	struct timeout_arg * timeout_arg_head;

	struct peer * next;
};

struct rudp_socket {
	int sockfd; 

	int (*recv_packet_handler)(rudp_socket_t, struct sockaddr_in6 *, void *, int);
	int (*event_handler)(rudp_socket_t, rudp_event_t, struct sockaddr_in6 *);

	struct data_node *data_head;
	struct data_node *data_tail;
	u_int8_t stored_all_data;
	u_int8_t closed;

	struct peer * peer_head;
};

#endif /* RUDP_STRUCTS_H */