#include "rudp_process.h"


/*!
 * Create a new RUDP socket
 */
struct rudp_socket * create_new_rudp_socket() {
	struct rudp_socket *rsocket;

	rsocket = (struct rudp_socket *) malloc(sizeof (struct rudp_socket));
    rsocket->recv_packet_handler = NULL;
    rsocket->event_handler = NULL;
    rsocket->data_head = NULL;
    rsocket->data_tail = NULL;
    rsocket->peer_head = NULL;
    rsocket->stored_all_data = 0;
    rsocket->closed = 0;

    return rsocket;
}

/*!
 * Create a new peer with given address
 */
struct peer * create_new_peer(struct sockaddr_in6 peer_addr) {
	struct peer * new_peer;

	new_peer = (struct peer *) malloc(sizeof(struct peer));
	new_peer->addr = peer_addr;
	new_peer->next = NULL;
	new_peer->has_syn = 0;
	new_peer->has_vs_begin = 0;
	new_peer->started = 0;
	new_peer->finished = 0;
	new_peer->timeout_arg_head = NULL;
	srand(time(NULL));
	new_peer->base_seq_no = rand() % 50000;
	//new_peer->base_seq_no = 0;

	return new_peer;
}

/*!
 * Create new data node
 */
struct data_node * create_new_data_node(u_int32_t index, struct vsftp * data, int len) {
	struct data_node * new_data_node;

	new_data_node = (struct data_node *) malloc(sizeof(struct data_node));
	new_data_node->index = index;
	new_data_node->data = *data;
	new_data_node->data_len = len;
	new_data_node->next = NULL;

	return new_data_node;
}

/*!
 * Create new rudp packet with retry
 */
struct retry_rudp_packet * create_new_retry_rudp_packet(struct rudp_socket *rsocket, 
															struct sockaddr_in6 *dest_addr,
															char * data, int data_len) {
	struct retry_rudp_packet * packet;

	packet = (struct retry_rudp_packet *) malloc(sizeof(struct retry_rudp_packet));
	packet->n_retries = 0;
	packet->data = data;
	packet->data_len = data_len;
	packet->rsocket = rsocket;
	packet->dest_addr = dest_addr;

	return packet;
}

/*!
 * Assign data to a rudp header
 */
void assign_data_rudp_header(struct rudp_hdr *header, u_int16_t version, u_int16_t type,
								u_int32_t seqno) {
	/*header->version = version;
	header->type = type;
	header->seqno = seqno;*/

	header->version = htons(version);
	header->type = htons(type);
	header->seqno = htonl(seqno);
}

/*!
 * Convert a rudp header to host byte order
 */
void ntohl_rudp_header(struct rudp_hdr *header) {
	header->version = ntohs(header->version);
	header->type = ntohs(header->type);
	header->seqno = ntohl(header->seqno);
}

/*!
 * Convert a rudp header to network byte order
 */
/*void htonl_rudp_header(struct rudp_hdr *header) {
	header->version = htonl(header->version);
	header->type = htonl(header->type);
	header->seqno = htonl(header->seqno);
}*/

/*!
 * Create new ACK rudp packet
 */
char *create_new_ack_rudp_packet(int seqno, int *packet_len) {
	char * new_rudp_packet;

	*packet_len = (int) sizeof(struct rudp_hdr);
	new_rudp_packet = malloc(*packet_len);
	assign_data_rudp_header((struct rudp_hdr *)new_rudp_packet, RUDP_VERSION, RUDP_ACK, seqno);

	return new_rudp_packet;
}

/*!
 * Create new FIN rudp packet
 */
char *create_new_fin_rudp_packet(int seqno, int *packet_len) {
	char * new_rudp_packet;

	*packet_len = (int) sizeof(struct rudp_hdr);
	new_rudp_packet = malloc(*packet_len);
	assign_data_rudp_header((struct rudp_hdr *)new_rudp_packet, RUDP_VERSION, RUDP_FIN, seqno);

	return new_rudp_packet;
}

/*!
 * Create new SYN rudp packet
 */
char *create_new_syn_rudp_packet(int seqno, int *packet_len) {
	char * new_rudp_packet;

	*packet_len = (int) sizeof(struct rudp_hdr);
	new_rudp_packet = malloc(*packet_len);

	assign_data_rudp_header((struct rudp_hdr *)new_rudp_packet, RUDP_VERSION, RUDP_SYN, seqno);

	return new_rudp_packet;
}

/*!
 * Create a new DATA rudp packet
 */
char * create_new_data_rudp_packet(struct vsftp *data, int data_len, int seqno, int *packet_len) {
	char * new_rudp_packet;

	*packet_len = (int) sizeof(struct rudp_hdr) + data_len;
	new_rudp_packet = malloc(*packet_len);

	// copy header
	assign_data_rudp_header((struct rudp_hdr *)new_rudp_packet, RUDP_VERSION, RUDP_DATA, seqno);
	// copy data
	data->vs_type = htonl(data->vs_type);
	memcpy(new_rudp_packet + sizeof(struct rudp_hdr), (char *) data, data_len);
	data->vs_type = ntohl(data->vs_type);
	
	return new_rudp_packet;	
}

/*!
 * Create a new sliding window
 */
struct sliding_window * create_new_sliding_window() {
	struct sliding_window * swindow;

	swindow = (struct sliding_window *) malloc(sizeof(struct sliding_window));
	memset(swindow, 0, sizeof(struct sliding_window));
	swindow->end_data_node = NULL;

	return swindow;
}

/*!
 * Free a list of data node starting from the given node
 */
void free_data_node(struct data_node * cur_node) {
	if (cur_node == NULL) {
		return;
	}
	free_data_node(cur_node->next);
	free(cur_node);
}

/*!
 * Free a list of peers starting from the given peer
 */
void free_peer(struct peer * cur_peer) {
	if (cur_peer == NULL) {
		return;
	}
	free_peer(cur_peer->next);

	free_timeout_arg(cur_peer->timeout_arg_head);
	free(cur_peer);
}

/*!
 * Free a list of timeout arguments starting from the given one
 */
void free_timeout_arg(struct timeout_arg * cur_arg) {
	struct retry_rudp_packet * rpacket;
	if (cur_arg == NULL) {
		return;
	}

	free_timeout_arg(cur_arg->next);

	rpacket = cur_arg->rpacket_addr;
	free(rpacket->data);
	free(cur_arg);
}

/*!
 * Check if all peers have received the file
 */
int can_close_socket(struct rudp_socket *rsocket) {
	struct peer * cur_peer;

	cur_peer = rsocket->peer_head;
	while (cur_peer != NULL) {
		if (cur_peer->finished == 0) {
			return 0;
		}
		cur_peer = cur_peer->next;
	}

	return 1;
}

/*!
 * Close a socket
 */
int close_socket(struct rudp_socket *rsocket) {
	if (rsocket == NULL || rsocket->closed == 1) {
		return -1;
	}
	printf("Closing socket \n");

	// delete callback for the socket when there is packet available
	event_fd_delete(&rudp_process_recv_packet, (void*)rsocket);

	// mark that it has been closed
	rsocket->closed = 1;
	// free the data nodes
	free_data_node(rsocket->data_head);
	// free the list of peers
	free_peer(rsocket->peer_head);

	// call the event handler of the application if any
	if (rsocket->event_handler != NULL) {
		rsocket->event_handler(rsocket, RUDP_EVENT_CLOSED, NULL);
	}

	return 0;
}

/*!
 * Find a peer in the given socket
 */
struct peer * find_peer(struct rudp_socket * rsocket, struct sockaddr_in6 *peer_addr) {
	struct peer * tmp;

	tmp = rsocket->peer_head;
	while (tmp != NULL) {
		if (sockaddr6_cmp(&tmp->addr, peer_addr) == 0) {
			return tmp;
		}
		tmp = tmp->next;
	}
	return NULL;
}

/*!
 * Add a peer to the given socket
 */
struct peer * add_peer(struct rudp_socket * rsocket, struct sockaddr_in6 *peer_addr) {
	struct peer * tmp;
	struct peer * new_peer;

	new_peer = create_new_peer(*peer_addr);
	if (rsocket->peer_head == NULL) {
		rsocket->peer_head = new_peer;
	} else {
		tmp = rsocket->peer_head;
		while (tmp->next != NULL) {
			tmp = tmp->next;
		}
		tmp->next = new_peer;	
	}

	return new_peer;
}

/*!
 * Store data inside a rudp socket
 */
int store_data(struct rudp_socket *rsocket, struct vsftp * data, int len) {
	struct data_node * new_data_node;
	struct data_node * tmp;

	if (rsocket->data_tail == NULL) {
		new_data_node = create_new_data_node(0, data, len);
		rsocket->data_head = new_data_node;

	} else {
		tmp = rsocket->data_tail;
		new_data_node = create_new_data_node(tmp->index+1, data, len);
		tmp->next = new_data_node;
	}

	// mark if it's the last packet
	if (data->vs_type == VS_TYPE_END) {
		rsocket->stored_all_data = 1;
	} else {
		// the data_tail point to the last data packet which is not TYPE_END
		rsocket->data_tail = new_data_node;	
	}
	
	return 0;
}

int add_timeout_arg(struct peer *cur_peer, u_int32_t seqno, struct retry_rudp_packet *rpacket) {
	struct timeout_arg * targ;

	targ = (struct timeout_arg *) malloc(sizeof(struct timeout_arg));
	targ->seqno = seqno;
	targ->rpacket_addr = rpacket;

	targ->next = cur_peer->timeout_arg_head;
	cur_peer->timeout_arg_head = targ;

	return 0;
}

int delete_timeout_callback(struct peer * cur_peer, struct timeout_arg * cur_head, u_int32_t seqno) {
	struct timeout_arg * targ;
	struct timeout_arg * next;

	if (cur_head == NULL) {
		return 0;
	}

	targ = cur_head;

	if (targ->seqno < seqno) {
		event_timeout_delete(&resend_packet, targ->rpacket_addr);
		cur_peer->timeout_arg_head = targ->next;
		free(targ);
		targ = NULL;
		return delete_timeout_callback(cur_peer, cur_peer->timeout_arg_head, seqno);
	}

	while (targ->next != NULL) {
		next = targ->next;
		if (next->seqno < seqno) {
			event_timeout_delete(&resend_packet, next->rpacket_addr);			

			targ->next = next->next;
			free(next);
			next = NULL;
			return delete_timeout_callback(cur_peer, targ, seqno);
		} else {
			targ = targ->next;	
		}
	}
	return 0;
}

/*!
 * Get timeout for the event in form of a struct timeval
 */
struct timeval get_timeout() {
	struct timeval cur_time;
	struct timeval timeout;
	struct timeval res_time;

	// get the current time
	gettimeofday(&cur_time, NULL);

	// convert the RUDP timeout to timeval
	timeout.tv_sec = RUDP_TIMEOUT / 1000;               // get second
    timeout.tv_usec = (RUDP_TIMEOUT % 1000) * 1000;     // get the remaining microseconds

    // get the time by adding the timeout to the current time
    timeradd(&cur_time, &timeout, &res_time);

	return res_time;
}

/*!
 * Set timeout for sending a packet
 */
int set_timeout_callback(struct retry_rudp_packet * retry_rpacket) {
	struct timeval timeout = get_timeout();
	if (event_timeout(timeout, &resend_packet, retry_rpacket, "Packet timeout") == -1) {
		printf("RUDP set_timeout: cannot set timeout\n");
		return -1;
	} 
	return 0;
}

/*!
 * Send a packet to the given destination using the given socket
 */
int send_packet(struct rudp_socket *rsocket, struct peer * cur_peer, 
					char * rpacket, int packet_len, 
						u_int8_t retransmit_flag) {
	struct retry_rudp_packet *retry_rpacket;
	struct rudp_hdr *header;

	// send the packet
	if (sendto(rsocket->sockfd, rpacket, packet_len, 0, 
			(struct sockaddr *) &cur_peer->addr, sizeof(cur_peer->addr)) == -1) {
		printf("RUDP: sending error");
		return -1;
	}

	// set timeout
	if (retransmit_flag != 0) {
		retry_rpacket = create_new_retry_rudp_packet(rsocket, &cur_peer->addr, 
														rpacket, packet_len);
		header = (struct rudp_hdr *) rpacket;
		ntohl_rudp_header(header);
		add_timeout_arg(cur_peer, header->seqno, retry_rpacket);

		if (set_timeout_callback(retry_rpacket) == -1) {
			free(retry_rpacket);
			return -1;	
		}	
	}
	
	
	return 0;
}

/*!
 * Callback when a packet doesn't receive ACK before timeout
 */
int resend_packet(int sockfd, void* arg) {
	struct retry_rudp_packet *retry_rpacket;

	// delete callback timeout function
	event_timeout_delete(&resend_packet, arg);

	// increate the number of retries
	retry_rpacket = (struct retry_rudp_packet *) arg;
	retry_rpacket->n_retries ++;

	// if the packet can still be resent
	if (retry_rpacket->n_retries <= RUDP_MAXRETRANS) {
		struct rudp_hdr * header = (struct rudp_hdr *) retry_rpacket->data;
		ntohl_rudp_header(header);

		printf("Resending a packet (seq = %d) to %s:%s\n", header->seqno, getnameinfohost(retry_rpacket->dest_addr), 
												getnameinfoserv(retry_rpacket->dest_addr));
		
		// send the packet
		if (sendto(retry_rpacket->rsocket->sockfd, retry_rpacket->data, retry_rpacket->data_len, 0, 
					(struct sockaddr *) retry_rpacket->dest_addr, 
					sizeof(*retry_rpacket->dest_addr)) == -1) {
			printf("RUDP: sending error");
			return -1;
		}

		set_timeout_callback(retry_rpacket);

	// reach the maximum number of retransmission
	} else {
		printf("Reached maximum retries, stopping... \n");
		close_socket(retry_rpacket->rsocket);
		return -1;
	}
	return 0;
}

/*!
 * Start transmitting data for the given peer using sliding window
 */
int start_transmitting_data(struct rudp_socket *rsocket, struct peer *cur_peer) {
	struct sliding_window *swindow;
	struct data_node * tmp;
	char *rpacket;
	int packet_len;
	u_int32_t seqno;
	int i;

	swindow = &cur_peer->window_info.swindow;
	swindow->begin_index = 1;
	swindow->end_index = MIN(RUDP_WINDOW, rsocket->data_tail->index);

	// send all packets in the window
	tmp = rsocket->data_head->next;
	i = swindow->begin_index; 
	while (1) {
		// create a new data rudp packet and send it
		seqno = tmp->index + cur_peer->base_seq_no + 1;
		rpacket = create_new_data_rudp_packet(&tmp->data, tmp->data_len, seqno, &packet_len);
		if (send_packet(rsocket, cur_peer, rpacket, packet_len, 1) == -1) {
			return -1;
		}

		if (i == swindow->end_index) {
			break;
		}
		
		i++;
		tmp = tmp->next;
	}
	swindow->end_data_node = tmp;
	
	return 0;
}

/*!
 * Process the received ACK packet. This must be on the sender side.
 */
int process_ack_packet(struct rudp_socket * rsocket, struct rudp_hdr * header,
							struct sockaddr_in6 *sender_addr) {
	struct peer * cur_peer;
	char *rpacket;
	int packet_len;
	struct data_node * dnode;
	struct sliding_window *swindow;
	u_int32_t new_begin;
	u_int32_t new_end;
	//u_int8_t tmp_has_sent[RUDP_WINDOW];
	int i;
	struct data_node * tmp;
	u_int32_t seqno;

	cur_peer = find_peer(rsocket, sender_addr);
	if (cur_peer == NULL) {
		printf("RUDP process_ack_packet: packet from unknown source\n");
		return -1;
	}
	// delete callback timeout
	 delete_timeout_callback(cur_peer, cur_peer->timeout_arg_head, header->seqno);

	// ACK for SYN
	if (cur_peer->has_syn == 0 && header->seqno == cur_peer->base_seq_no+1) {
		printf("Received ACK for SYN - %d\n", header->seqno);

		cur_peer->has_syn = 1;

		// send the VS_TYPE_BEGIN packet
		dnode = rsocket->data_head;
		printf("Filename: %s\n", dnode->data.vs_info.vs_filename);
		rpacket = create_new_data_rudp_packet(&dnode->data, dnode->data_len, 
												cur_peer->base_seq_no+1, &packet_len);
		if (send_packet(rsocket, cur_peer, rpacket, packet_len, 1) == -1) {
			return -1;
		}

		// mark that VS_TYPE_BEGIN has been sent
		cur_peer->has_vs_begin = 1;
		return 0;
	}

	// ACK for VS_TYPE_BEGIN
	if (cur_peer->has_vs_begin == 1 && header->seqno == cur_peer->base_seq_no+2) {
		printf("Received ACK for VS_TYPE_BEGIN - %d\n", header->seqno);

		// Start transmitting data
		start_transmitting_data(rsocket, cur_peer);
		return 0;
	}

	// ACK for VS_TYPE_END
	if (rsocket->stored_all_data == 1 
			&& header->seqno == cur_peer->base_seq_no + rsocket->data_tail->index + 3) {
		printf("Received ACK for VS_TYPE_END - %d\n", header->seqno);
		printf("Sending FIN packet\n");
		rpacket = create_new_fin_rudp_packet(header->seqno, &packet_len);
		if (send_packet(rsocket, cur_peer, rpacket, packet_len, 1) == -1) {
			return -1;
		}
		return 0;
	}

	// ACK for FIN
	if (rsocket->stored_all_data == 1 
			&& header->seqno == cur_peer->base_seq_no + rsocket->data_tail->index + 4) {
		printf("Received ACK for FIN %d %d\n", rsocket->data_tail->index, header->seqno);
		cur_peer->finished = 1;

		// close the socket if everything has finished
		if (can_close_socket(rsocket) == 1) {
			close_socket(rsocket);
		}
		return 0;
	}

	// ACK for VS_TYPE_DATA
	printf("Received ACK for VS_TYPE_DATA\n");
	swindow = &cur_peer->window_info.swindow;
	
	seqno = header->seqno - cur_peer->base_seq_no - 1;

	// if has sent all data, send VS_TYPE_END
	if (rsocket->stored_all_data == 1 && seqno == rsocket->data_tail->index + 1) {
		printf("Sending VS_TYPE_END\n");
		dnode = rsocket->data_tail->next;
		rpacket = create_new_data_rudp_packet(&dnode->data, dnode->data_len, 
												cur_peer->base_seq_no + dnode->index + 1,
												&packet_len);
		if (send_packet(rsocket, cur_peer, rpacket, packet_len, 1) == -1){
			return -1;
		}
		return 0;
	}

	// only process ACK for the one within the sliding window
	if (seqno >= swindow->begin_index + 1 && seqno <= swindow->end_index + 1) {
		new_begin = seqno;
		new_end = MIN(new_begin + RUDP_WINDOW - 1, rsocket->data_tail->index); 

		//printf("Sending VS_TYPE_DATA %d - %d; %d - %d - %d - %d\n", swindow->begin_index, swindow->end_index, new_begin, new_end, cur_peer->base_seq_no, rsocket->data_tail->next->index);
		printf("Sending VS_TYPE_DATA\n");

		if (new_end > swindow->end_index) {
			tmp = swindow->end_data_node->next;
			i = swindow->end_index + 1; 
			while (1) {
				// create a new data rudp packet and send it
				seqno = tmp->index + cur_peer->base_seq_no + 1;
				rpacket = create_new_data_rudp_packet(&tmp->data, tmp->data_len, seqno, &packet_len);
				if (send_packet(rsocket, cur_peer, rpacket, packet_len, 1) == -1) {
					return -1;
				}

				if (i == new_end) {
					break;
				}
				
				i++;
				tmp = tmp->next;
			}
			swindow->end_data_node = tmp;	
		}
		
		swindow->begin_index = new_begin;
		swindow->end_index = new_end;
	}

	return 0;	
}

/*!
 * Process the received DATA packet. This must be one the receiver side
 */
int process_data_packet(struct rudp_socket * rsocket, struct rudp_hdr * header,
							void * data, int data_len, struct sockaddr_in6 *sender_addr) {
	printf("Received vsftp\n");
	struct peer * cur_peer;
	char *rpacket;
	int packet_len;

	cur_peer = find_peer(rsocket, sender_addr);
	if (cur_peer == NULL) {
		printf("RUDP process_data_packet: receive packet from unknown sender\n");
		return -1;
	}

	if (header->seqno == cur_peer->window_info.latest_data_index + 1) {
		rpacket = create_new_ack_rudp_packet(header->seqno + 1, &packet_len);
		if (send_packet(rsocket, cur_peer, rpacket, packet_len, 0) == -1) {
			return -1;
		}

		cur_peer->window_info.latest_data_index ++;

		if (rsocket->recv_packet_handler != NULL) {
			rsocket->recv_packet_handler(rsocket, sender_addr, data, data_len);	
		}	
	} else {
		printf("RUDP process_data_packet: drop packet not in order\n");

		rpacket = create_new_ack_rudp_packet(cur_peer->window_info.latest_data_index, &packet_len);
		if (send_packet(rsocket, cur_peer, rpacket, packet_len, 0) == -1) {
			return -1;
		}
	}

	return 0;
}

/*!
 * Process the received SYN packet. This must be on the receiver side
 */
int process_syn_packet(struct rudp_socket * rsocket, struct rudp_hdr * header,
							struct sockaddr_in6 *sender_addr) {
	printf("Received SYN\n");
	struct peer * cur_peer;
	char *rpacket;
	int packet_len;

	cur_peer = find_peer(rsocket, sender_addr);
	if (cur_peer == NULL) {
		cur_peer = add_peer(rsocket, sender_addr);
		cur_peer->window_info.latest_data_index = header->seqno;
	}

	cur_peer->has_syn = 1;

	rpacket = create_new_ack_rudp_packet(header->seqno + 1, &packet_len);
	if (send_packet(rsocket, cur_peer, rpacket, packet_len, 0) == -1) {
		return -1;
	}

	return 0;
}

/*!
 * Process the received FIN packet. This could be on the both sides
 */
int process_fin_packet(struct rudp_socket * rsocket, struct rudp_hdr * header,
							struct sockaddr_in6 *sender_addr) {
	printf("Received FIN\n");
	struct peer * cur_peer;
	char *rpacket;
	int packet_len;

	cur_peer = find_peer(rsocket, sender_addr);
	if (cur_peer == NULL) {
		printf("RUDP process_fin_packet: receive packet from unknown sender\n");
		return -1;
	}
	if (header->seqno != cur_peer->window_info.latest_data_index + 1) {
		printf("RUDP process_fin_packet: invalid sequence number\n");
		return -1;
	}

	rpacket = create_new_ack_rudp_packet(header->seqno + 1, &packet_len);
	if (send_packet(rsocket, cur_peer, rpacket, packet_len, 0) == -1) {
		return -1;
	}

	return 0;
}

/*!
 * Callback when a socket receive packet.
 */
int rudp_process_recv_packet(int sockfd, void *arg) {
	struct rudp_socket * rsocket = (struct rudp_socket *) arg;
	int read_bytes;
	struct sockaddr_in6 *sender_addr;
	int sender_addr_len = sizeof(struct sockaddr_in6);
	char buf[RUDP_MAXPKTSIZE+1];
	struct rudp_hdr * header;
	void * data;
	int data_len;

	sender_addr = (struct sockaddr_in6 *) malloc(sizeof(struct sockaddr_in6));
	read_bytes = recvfrom(rsocket->sockfd, buf, RUDP_MAXPKTSIZE, 0, 
							(struct sockaddr *) sender_addr, 
							(socklen_t *) &sender_addr_len);
	if (read_bytes < sizeof(struct rudp_hdr)) {
		printf("RUDP: receive packet error\n");
		return 0;
	}

	header = (struct rudp_hdr *) buf;
	ntohl_rudp_header(header);
	printf("Received packet - Type: %d; Seq: %d\n", header->type, header->seqno);

	// validate header
	if (header->version != RUDP_VERSION) {
		printf("RUDP: invalid RUDP version\n");
		return 0;
	}

	switch (header->type) {
		case RUDP_ACK:
			return process_ack_packet(rsocket, header, sender_addr);
		case RUDP_DATA:
			data_len = read_bytes - sizeof(struct rudp_hdr);
			if (data_len == 0) {
				printf("RUDP: receive packet error\n");
				return 0;
			}
			data = (void *) (buf + sizeof(struct rudp_hdr));
			return process_data_packet(rsocket, header, data, data_len, sender_addr);
		case RUDP_SYN:
			return process_syn_packet(rsocket, header, sender_addr);
		case RUDP_FIN:
			return process_fin_packet(rsocket, header, sender_addr);
	}

	printf("RUDP: receive unknown packet type \n");
	return 0;
}