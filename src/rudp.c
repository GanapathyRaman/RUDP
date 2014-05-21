#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "event.h"
#include "rudp.h"
#include "rudp_api.h"
#include "rudp_structs.h"
#include "rudp_process.h"
#include "getaddr.h"
#include "sockaddr6.h"

/* 
 * rudp_socket: Create a RUDP socket. 
 * May use a random port by setting port to zero. 
 */

rudp_socket_t rudp_socket(int port) {
	int sockfd;
	struct rudp_socket *rsocket;

	sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("RUDP: create socket error\n");
        return NULL;
    }

    struct sockaddr_in6 sin;  
    memset((char *) &sin, 0, sizeof(sin));
    sin.sin6_family = AF_INET6;
    sin.sin6_addr = in6addr_any;
    sin.sin6_port = htons(port);
    //sin.sin6_scope_id = 0;
    if(bind(sockfd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
        printf("RUDP: bind error\n");
        return NULL;
    }

    // create the socket
    rsocket = create_new_rudp_socket();
    rsocket->sockfd = sockfd;

    if(event_fd(sockfd, &rudp_process_recv_packet, (void*)rsocket, "RUDP:receive_data") < 0) {
        printf("RUDP: register receive packet event error\n");
        free(rsocket);
        return NULL;
    }
	return rsocket;
}

/* 
 *rudp_close: Close socket 
 */ 

int rudp_close(rudp_socket_t rsocket_t) {
	struct rudp_socket *rsocket;
	struct peer * cur_peer;
	char *syn_packet;
	int packet_len;

	rsocket = (struct rudp_socket *) rsocket_t;
	
	// start sending data to all peers
	if (rsocket->stored_all_data == 1) {
		cur_peer = rsocket->peer_head;

		// sending SYN packet to each peer		
		while (cur_peer != NULL) {
			if (cur_peer->started == 0) {
				cur_peer->started = 1;
		
				printf("Sending SYN packet \n");
				syn_packet = create_new_syn_rudp_packet(cur_peer->base_seq_no, &packet_len);
				send_packet(rsocket, cur_peer, syn_packet, packet_len, 1);		
			}
			cur_peer = cur_peer->next;
		}
	}
	
	return 0;
}

/* 
 *rudp_recvfrom_handler: Register receive callback function 
 */ 

int rudp_recvfrom_handler(rudp_socket_t rsocket_t, 
			  int (*handler)(rudp_socket_t, struct sockaddr_in6 *, 
					 void *, int)) {
	struct rudp_socket *rsocket;

	rsocket = (struct rudp_socket *) rsocket_t;
	rsocket->recv_packet_handler = handler;
	return 0;
}

/* 
 *rudp_event_handler: Register event handler callback function 
 */ 
int rudp_event_handler(rudp_socket_t rsocket_t, 
		       int (*handler)(rudp_socket_t, rudp_event_t, 
				      struct sockaddr_in6 *)) {
	struct rudp_socket *rsocket;

	rsocket = (struct rudp_socket *) rsocket_t;
	rsocket->event_handler = handler;
	return 0;
}


/* 
 * rudp_sendto: Send a block of data to the receiver. 
 */

int rudp_sendto(rudp_socket_t rsocket_t, void* data, int len, struct sockaddr_in6* to) {
	struct rudp_socket *rsocket;
	struct peer * cur_peer;
	struct vsftp * vs_data;

	rsocket = (struct rudp_socket *) rsocket_t;
	vs_data = (struct vsftp *) data;
	vs_data->vs_type = ntohl(vs_data->vs_type);

	// find the corresponding peer
	cur_peer = find_peer(rsocket, to);

	// create a new peer if needed
	if (cur_peer == NULL) {
		// ignore if it's not VS_TYPE_BEGIN
		if (vs_data->vs_type != VS_TYPE_BEGIN) {
			printf("rudp_sendto: cannot find peer\n");
			return -1;
		}

		cur_peer = add_peer(rsocket, to);	
	}

	// if the peer is the first peer, store the data to the data list of the socket
	if (cur_peer == rsocket->peer_head) {
		store_data(rsocket, vs_data, len);
	}

	vs_data->vs_type = htonl(vs_data->vs_type);
	
	return 0;
}

