#ifndef RUDP_PROCESS_STRUCTS_H
#define RUDP_PROCESS_STRUCTS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/param.h>
#include <sys/time.h> 
#include <arpa/inet.h>

#include "event.h"
#include "sockaddr6.h"
#include "rudp_structs.h"
#include "getaddr.h"


/*!
 * Create a new RUDP socket
 */
struct rudp_socket * create_new_rudp_socket();

/*!
 * Create a new peer with given address
 */
struct peer * create_new_peer(struct sockaddr_in6 peer_addr);

/*!
 * Create new data node
 */
struct data_node * create_new_data_node(u_int32_t index, struct vsftp * data, int len);

/*!
 * Create new rudp packet with retry
 */
struct retry_rudp_packet * create_new_retry_rudp_packet(struct rudp_socket *rsocket, 
															struct sockaddr_in6 *dest_addr,
															char * data, int data_len);

/*!
 * Assign data to a rudp header
 */
void assign_data_rudp_header(struct rudp_hdr *header, u_int16_t version, u_int16_t type,
								u_int32_t seqno);

/*!
 * Convert a rudp header to host byte order
 */
void ntohl_rudp_header(struct rudp_hdr *header);

/*!
 * Convert a rudp header to network byte order
 */
void htonl_rudp_header(struct rudp_hdr *header);

/*!
 * Create new ACK rudp packet
 */
char *create_new_ack_rudp_packet(int seqno, int *packet_len);

/*!
 * Create new FIN rudp packet
 */
char *create_new_fin_rudp_packet(int seqno, int *packet_len);

/*!
 * Create new SYN rudp packet
 */
char *create_new_syn_rudp_packet(int seqno, int *packet_len);

/*!
 * Create a new DATA rudp packet
 */
char *create_new_data_rudp_packet(struct vsftp *data, int data_len, int seqno, int *packet_len);

/*!
 * Create a new sliding window
 */
struct sliding_window * create_new_sliding_window();

/*!
 * Check if all peers have received the file
 */
int can_close_socket(struct rudp_socket *rsocket);

/*!
 * Close a socket
 */
int close_socket(struct rudp_socket *rsocket);

/*!
 * Free a list of data node starting from the given node
 */
void free_data_node(struct data_node * cur_node);

/*!
 * Free a list of peers starting from the given peer
 */
void free_peer(struct peer * cur_peer);

/*!
 * Free a list of timeout arguments starting from the given one
 */
void free_timeout_arg(struct timeout_arg * cur_arg);

/*!
 * Find a peer in the given socket
 */
struct peer * find_peer(struct rudp_socket * rsocket, struct sockaddr_in6 *peer_addr);

/*!
 * Add a peer to the given socket
 */
struct peer * add_peer(struct rudp_socket * rsocket, struct sockaddr_in6 *peer_addr);

/*!
 * Store data inside a rudp socket
 */
int store_data(struct rudp_socket *rsocket, struct vsftp * data, int len);

int add_timeout_arg(struct peer *cur_peer, u_int32_t seqno, struct retry_rudp_packet *rpacket);

/*!
 * Get timeout for the event in form of a struct timeval
 */
struct timeval get_timeout();

/*!
 * Set timeout for sending a packet
 */
int set_timeout_callback(struct retry_rudp_packet * retry_rpacket);

/*!
 * Send a packet to the given destination using the given socket
 */
int send_packet(struct rudp_socket *rsocket, struct peer * cur_peer, 
					char * data, int data_len, u_int8_t retransmit_flag);

/*!
 * Callback when a packet doesn't receive ACK before timeout
 */
int resend_packet(int sockfd, void* arg);

/*!
 * Start transmitting data for the given peer using sliding window
 */
int start_transmitting_data(struct rudp_socket *rsocket, struct peer *cur_peer);

/*!
 * Process the received ACK packet. This must be on the sender side.
 */
int process_ack_packet(struct rudp_socket * rsocket, struct rudp_hdr * header,
							struct sockaddr_in6 *sender_addr);

/*!
 * Process the received DATA packet. This must be one the receiver side
 */
int process_data_packet(struct rudp_socket * rsocket, struct rudp_hdr * header,
							void * data, int data_len, struct sockaddr_in6 *sender_addr);

/*!
 * Process the received SYN packet. This must be on the receiver side
 */
int process_syn_packet(struct rudp_socket * rsocket, struct rudp_hdr * header,
							struct sockaddr_in6 *sender_addr);

/*!
 * Process the received FIN packet. This must be on the receiver side
 */
int process_fin_packet(struct rudp_socket * rsocket, struct rudp_hdr * header,
							struct sockaddr_in6 *sender_addr);

/*!
 * Callback when a socket receive packet.
 */
int rudp_process_recv_packet(int sockfd, void *arg);

#endif