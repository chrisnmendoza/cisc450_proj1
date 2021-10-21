/* udp_client.c */ 
/* Programmed by Christopher-Neil Mendoza and David Lizotte */
/* Due October 21, 2021 */

#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset, memcpy, and strlen */
#include <netdb.h>          /* for struct hostent and gethostbyname */
#include <sys/socket.h>     /* for socket, sendto, and recvfrom */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */
#include <string.h>

#define STRING_SIZE 1024

//struct of request packet
struct ReqPacket {
   unsigned short int id;
   unsigned short int count;
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

unsigned short csum(unsigned short *ptr, int nbytes){
	register long sum;
	unsigned short oddbyte;
	register short answer;

	sum=0;
	while(nbytes>1){
		sum+=*ptr++;
		nbytes-=2;
	}
	if(nbytes==1){
		oddbyte=0;
		*((u_char*)&oddbyte)=*(u_char*)ptr;
		sum+=oddbyte;
	}
	sum = (sum>>16)+(sum & 0xffff);
	sum = sum + (sum>>16);
	answer=(short)~sum;

	return(answer);
}

int promptContinue(void) {
   char userInput[STRING_SIZE];
   scanf("%s",userInput);
   if(strcmp("continue",userInput) == 0) {
      printf("continuing\n");
      return 1;
   }
   else{
      printf("exiting");
      printf("\n");
      return 0;
   }
}

int promptInput(void) {
   while(1 == 1) { //continues until valid number is received
      printf("Please input count of integers:\n");
      int num = 0; 
      scanf("%d", &num);
      if(num < 1 || num > 65535) {
         printf("invalid number\n");
      }
      else {
         return num;
      }
   }
}

int main(void) {

   int sock_client;  /* Socket used by client */ 

   struct sockaddr_in client_addr;  /* Internet address structure that
                                        stores client address */
   unsigned short client_port;  /* Port number used by client (local port) */

   struct sockaddr_in server_addr;  /* Internet address structure that
                                        stores server address */
   struct hostent * server_hp;      /* Structure to store server's IP
                                        address */
   char server_hostname[STRING_SIZE]; /* Server's hostname */
   unsigned short server_port;  /* Port number used by server (remote port) */

   char sentence[STRING_SIZE];  /* send message */
   char modifiedSentence[STRING_SIZE]; /* receive message */
   unsigned int msg_len;  /* length of message */
   int bytes_sent, bytes_recd; /* number of bytes sent or received */

   /* open a socket */

   if ((sock_client = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
      perror("Client: can't open datagram socket\n");
      exit(1);
   }

   /* Note: there is no need to initialize local client address information
            unless you want to specify a specific local port.
            The local address initialization and binding is done automatically
            when the sendto function is called later, if the socket has not
            already been bound. 
            The code below illustrates how to initialize and bind to a
            specific local port, if that is desired. */

   /* initialize client address information */

   client_port = 0;   /* This allows choice of any available local port */

   /* Uncomment the lines below if you want to specify a particular 
             local port: */
   /*
   printf("Enter port number for client: ");
   scanf("%hu", &client_port);
   */

   /* clear client address structure and initialize with client address */
   memset(&client_addr, 0, sizeof(client_addr));
   client_addr.sin_family = AF_INET;
   client_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* This allows choice of
                                        any host interface, if more than one 
                                        are present */
   client_addr.sin_port = htons(client_port);

   /* bind the socket to the local client port */

   if (bind(sock_client, (struct sockaddr *) &client_addr,
                                    sizeof (client_addr)) < 0) {
      perror("Client: can't bind to local address\n");
      close(sock_client);
      exit(1);
   }

   /* end of local address initialization and binding */

   /* initialize server address information */

   printf("Enter hostname of server: ");
   scanf("%s", server_hostname);
   if ((server_hp = gethostbyname(server_hostname)) == NULL) {
      perror("Client: invalid server hostname\n");
      close(sock_client);
      exit(1);
   }
   printf("Enter port number for server: ");
   scanf("%hu", &server_port);

   /* Clear server address structure and initialize with server address */
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   memcpy((char *)&server_addr.sin_addr, server_hp->h_addr,
                                    server_hp->h_length);
   server_addr.sin_port = htons(server_port);

   struct ReqPacket* pkt = malloc(sizeof(struct ReqPacket));
   struct ResponsePacket* responseBuffer = malloc(sizeof(struct ResponsePacket));
   pkt->id = htons(0);
   int shouldContinue = 1;
   while(shouldContinue == 1) {
      /* user interface */
      int num = promptInput();

      /* prepare request */
      int tempId = ntohs(pkt->id);
      tempId++;
      pkt->id = htons(tempId);
      pkt->count = htons((unsigned short)num);

      /* send message */
      bytes_sent = sendto(sock_client, pkt, 4, 0,
               (struct sockaddr *) &server_addr, sizeof (server_addr));

      /* get response from server */
      printf("Waiting for response from server...\n\n");
      int bytesReceived = 0;
      int packetsReceived = 0;
      long int seqSum = 0;
      unsigned int checksum = 0;
      int shouldStop = 0;
      int index = 0;
      while(shouldStop == 0) { //shouldStop is the value of the last bit in the response packet
         bytes_recd = recvfrom(sock_client, responseBuffer, 108, 0,
            (struct sockaddr *) 0, (int *) 0);
         if(ntohs(responseBuffer->id) != ntohs(pkt->id)) {
            printf("something went wrong, wrong response number\n");
            exit(1);
         }
         bytesReceived += bytes_recd;
         packetsReceived++;
         int tempSeq = ntohs(responseBuffer->seqNum); //assign temporary seq value to assign to seqSum after ntohs call
         seqSum += tempSeq;
         shouldStop = ntohs(responseBuffer->last);
         for(int i =0; i< ntohs(responseBuffer->count);i++){ //calculate checksum by adding all data integers
            checksum = checksum + ntohl(responseBuffer->payload[i]);
         }
         index++;
      }
      printf("Request ID: %d\t",ntohs(pkt->id));
      printf("Count: %d\n",ntohs(pkt->count));
      printf("total number of response packets received: %d\n",packetsReceived);
      printf("total number of bytes received: %d\n",bytesReceived);
      printf("Sequence Number sum: %lu\n",seqSum);
      printf("checksum: %u\n",checksum);
      printf("Type \"continue\" to continue, type anything else to exit\n");
      
      shouldContinue = promptContinue();
   }
   
   

   /* close the socket */
   close (sock_client);
   free(pkt);
   free(responseBuffer);
}
