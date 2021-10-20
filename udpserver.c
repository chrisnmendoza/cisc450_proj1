/* udp_server.c */
/* Programmed by Adarsh Sethi */
/* Sept. 19, 2021 */

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
#define RAND_MAX 4294967295

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

   /* wait for incoming messages in an indefinite loop */
   printf("size of unsigned short int: %d\n",(int)sizeof(unsigned short int));
   printf("size of unsigned int: %d\n",(int)sizeof(unsigned int));
   printf("size of unsigned long int: %d\n",(int)sizeof(unsigned long int));
   printf("size of unsigned int array: %d\n",(int)sizeof(unsigned int[25]));
   printf("size of responsePacket: %d\n",(int)sizeof(struct ResponsePacket));
   printf("size of reqPacket: %d\n",(int)sizeof(struct ReqPacket));
   printf("size of reqPacket pointer: %d\n",(int)sizeof(struct ReqPacket*));

   struct ReqPacket* reqBuffer = malloc(sizeof(struct ReqPacket));
   struct ResponsePacket* response = malloc(sizeof(struct ResponsePacket));

   printf("Waiting for incoming messages on port %hu\n\n", 
                           server_port);

   client_addr_len = sizeof (client_addr);

   for (;;) {

      bytes_recd = recvfrom(sock_server, reqBuffer, 4, 0,
                     (struct sockaddr *) &client_addr, &client_addr_len);
      printf("count is: %d\n     with id %d\n\n",
                         (int)reqBuffer->count, (int)reqBuffer->id);
      /* prepare the message to send */

      response->id = reqBuffer->id;
      response->seqNum = (unsigned short int)1;
      unsigned short int packetsLeftToSend = reqBuffer->count; 
      bytes_sent = 0;
      int packets_sent = 0;
      unsigned long int seqSum = 0;
      while(packetsLeftToSend > 0) {
         printf("packets left to send: %d\n",packetsLeftToSend);
         if(packetsLeftToSend <= 25) {
            response->count = packetsLeftToSend;
            response->last = 1;
            packetsLeftToSend = 0;
         }
         else {
            packetsLeftToSend -= 25;
            response->count = 25;
            response->last = 0;
         }
         unsigned int data[25];
         for(int i = 0; i < response->count; i++) {
            data[i] = (unsigned int)(rand() % 4294967296);
         }
         memcpy(response->payload,data,response->count);
         bytes_sent += sendto(sock_server, response, 108, 0,
               (struct sockaddr*) &client_addr, client_addr_len);
         packets_sent++;
         seqSum += response->seqNum;
         printf("bytes sent: %d\n",bytes_sent);
         response->seqNum += 1;
      }


      /*
      msg_len = bytes_recd;
      for (i=0; i<msg_len; i++)
         modifiedSentence[i] = toupper (sentence[i]);

      //send message
 
      bytes_sent = sendto(sock_server, modifiedSentence, msg_len, 0,
               (struct sockaddr*) &client_addr, client_addr_len);

      */
   }
}
