/* udp_server.c */
/* Programmed by Christopher-Neil Mendoza and David Lizotte */
/* Due October 21, 2021 */

#include <ctype.h>          /* for toupper */
#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset */
#include <sys/socket.h>     /* for socket, sendto, and recvfrom */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */
#include <time.h>

#define STRING_SIZE 1024

/* SERV_UDP_PORT is the port number on which the server listens for
   incoming messages from clients. You should change this to a different
   number to prevent conflicts with others in the class. */

#define SERV_UDP_PORT 65151

//struct of request packet
struct ReqPacket {
   unsigned short id;
   unsigned short count;
};

//struct of response packet
struct ResponsePacket {
   unsigned short int id;
   unsigned short int seqNum;
   unsigned short int last;
   unsigned short int count;
   unsigned int payload[25]; /*" It is acceptable to have a static array with the maximum possible size
                              allowed in the project and not do any dynamic memory allocation"*/
};

void prepareResponse(struct ReqPacket* reqBuffer, struct ResponsePacket* response) {
   response->id = ntohs(reqBuffer->id);
   response->id = htons(response->id);
   response->seqNum = htons((unsigned short int)1);
}
				

int main(void) {

   srand(time(0)); //for random number generation

   int sock_server;  /* Socket on which server listens to clients */

   struct sockaddr_in server_addr;  /* Internet address structure that
                                        stores server address */
   unsigned short server_port;  /* Port number used by server (local port) */

   struct sockaddr_in client_addr;  /* Internet address structure that
                                        stores client address */
   unsigned int client_addr_len;  /* Length of client address structure */

   char sentence[STRING_SIZE];  /* receive message */
   char modifiedSentence[STRING_SIZE]; /* send message */
   unsigned int msg_len;  /* length of message */
   int bytes_sent, bytes_recd; /* number of bytes sent or received */
   unsigned int i;  /* temporary loop variable */

   /* open a socket */
   if ((sock_server = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
      perror("Server: can't open datagram socket\n");
      exit(1);
   }

   /* initialize server address information */
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = htonl (INADDR_ANY);  /* This allows choice of
                                        any host interface, if more than one
                                        are present */
   server_port = SERV_UDP_PORT; /* Server will listen on this port */
   server_addr.sin_port = htons(server_port);

   /* bind the socket to the local server port */

   if (bind(sock_server, (struct sockaddr *) &server_addr,
                                    sizeof (server_addr)) < 0) {
      perror("Server: can't bind to local address\n");
      close(sock_server);
      exit(1);
   }

   struct ReqPacket reqBuffer;
   struct ResponsePacket response;

   /* wait for incoming messages in an indefinite loop */
   printf("Waiting for incoming messages on port %hu\n\n", 
                           server_port);

   client_addr_len = sizeof (client_addr);

   for (;;) {

      /* receive request */
      bytes_recd = recvfrom(sock_server, &reqBuffer, 4, 0,
                     (struct sockaddr *) &client_addr, &client_addr_len);
      
      /* prepare the message to send */
      prepareResponse(&reqBuffer, &response);
      unsigned short int packetsLeftToSend = htons(reqBuffer.count); 
      bytes_sent = 0;
      int packets_sent = 0;
      unsigned long int seqSum = 0;
      unsigned int intsum = 0;
      while(packetsLeftToSend > 0) {
         if(packetsLeftToSend <= 25) { //this will be the last packet, must set last bit to 1 and correctly set number of data values
            response.count = htons(packetsLeftToSend);
            response.last = htons(1);
            packetsLeftToSend = 0;
         }
         else {
            packetsLeftToSend -= 25;
            response.count = htons(25);
            response.last = htons(0);
         }
         unsigned int data[25];
         for(int i = 0; i < ntohs(response.count); i++) { //give each data value a randomized value
            data[i] = htonl((unsigned int)(rand() % 4294967296)); 
            unsigned int tempsum =ntohl(data[i]);
            intsum = intsum + tempsum;
         }
         memcpy(response.payload,data,4*(ntohs(response.count))); //assign payload array to the data values

         /* send message*/
         bytes_sent += sendto(sock_server, &response, 108, 0,
               (struct sockaddr*) &client_addr, client_addr_len);
         packets_sent++;
         seqSum = seqSum + ntohs(response.seqNum);
         int tempSeqNum = ntohs(response.seqNum);
         tempSeqNum++;
         response.seqNum = htons(tempSeqNum);
      }
      printf("Request ID: %d\t", ntohs(response.id));
      printf("Count: %d\n", ntohs(reqBuffer.count));
      printf("Total number of packets sent: %d\n", packets_sent);
      printf("total number of bytes sent %d\n",bytes_sent);
      printf("Sequence number sum %lu\n",seqSum);
      printf("Checksum: %u\n\n", (unsigned)intsum);
   }
}
