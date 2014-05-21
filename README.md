#  Project -  RUDP
#  Preferred platform for running the server - Any Linux Machine
#  Dependencies - 
#	Any C compiler preferably (Gcc) 
#
#  +----------+----------------------+-------------------------------------------+
#  | Authors |  Tien Thanh Bui       | Ganapathy Raman Madanagopal               |
#  +----------+----------------------+-------------------------------------------+
#  |  Email  |    <ttbu@kth.se>      |               <grma@kth.se>               |
#  +---------+-----------------------+-------------------------------------------+
#
#  The following step can be use to run the RUDP based application
# 1. The src folder consist of 
#          * Seven C Source files - 
#		event.c  getaddr.c  rudp.c  rudp_process.c  sockaddr6.c  vs_recv.c  vs_send.c
#	* Eight C header files -
#		event.c  getaddr.c  rudp.c  rudp_process.c  sockaddr6.c  vs_recv.c  vs_send.c
#          * Other required files
#		Makefile - Compile all the sources file and produces two executable files “vs_send” 
#		and “vs_recv”
#		Compile - Internally calls the Makefile and moves vs_recv into a folder “receiver”.
#	vs_send.c and vs_recv.c are the application used to send and receive files respectively.
# 2. If required change the permission of the src folder using the command (chmod 777 -R src).
# 3. Compile the files using the using the file “./compile” or “sh compile”.
# 4. On the end receiver
#	* Go to folder “receiver” and the run the following command
#	“./vs_recv [-d] port”
#	where option -d to enable debugging
#	port to open the port for receiving the  data.
# 5. Note: Always start the receiver before sender.
# 6. On Sender, to send the files to all the receivers, run the following command
#	“vs_send [-d] host1:port1 [host2:port2] ... file1 [file2]...”
#	where option -d to enable debugging
#	host1, host2... denotes the IPv4 address of the receivers.
#	port1, port2… denotes the ports of the receiver respectively.
#	For IPv6 address use, place IPv6 address inside the square brackets, 
#	       i.e “./vs_send ipv4_address1:port [IPv6_address2:port2] file1 file2”

