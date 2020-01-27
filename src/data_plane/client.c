#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdlib.h>
#include <string.h>
#include <pcap.h> 
#include <errno.h> 
#include <arpa/inet.h> 
#include <netinet/if_ether.h>
#include <unistd.h>
#include <pthread.h>

#include "../logger/atlas_logger.h"

#define SLEEPTIME 5
#define IP "host 192.168.1.103"
#define HOSTNAME "raspberrypi"
#define PORTNO 51718

int  sockfd;
struct sockaddr_in serv_addr;


void atlas_init(char* username, int client_id, char* policy){

    ATLAS_LOGGER_DEBUG("DP: Register to atlas_client");

    char buffer[256];
    sprintf(buffer, "%s|%d|%s", username, client_id, policy);

    int n;
    struct hostent *server;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        ATLAS_LOGGER_ERROR("DP: ERROR opening socket");
    server = gethostbyname(HOSTNAME);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(PORTNO);
    write_to_socket(buffer);
}

void write_to_socket(char*buffer){

   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
        ATLAS_LOGGER_ERROR("DP: ERROR connecting to atlas_client.");

    int n = write(sockfd,buffer,strlen(buffer));
    if (n < 0) 
         ATLAS_LOGGER_ERROR("DP: ERROR writing to socket");
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0) 
         ATLAS_LOGGER_ERROR("DP: ERROR reading from socket");
    else
         printf("%s\n", buffer);
}

void write_to_file(char *file, char *type, int RX){
	FILE * f = fopen(file, type);	
	fprintf(f, "%d\n", RX);
	fclose(f);
}	
void packets_number_per_second(){
	int R1, R2, T1, T2, RX, TX;
	while(1){
	    FILE * rx_file = fopen("/sys/class/net/wlan0/statistics/rx_packets", "r");
	    FILE * tx_file = fopen("/sys/class/net/wlan0/statistics/tx_packets", "r");
	    if(rx_file == 0 || tx_file == 0){
		ATLAS_LOGGER_ERROR("DP: Failed to open file to read statistics");
		exit(1);
	    }
	    else{
		fscanf(rx_file, "%d", &R1);
		fscanf(tx_file, "%d", &T1);
		fclose(rx_file);
		fclose(tx_file);
		sleep(1);
		rx_file = fopen("/sys/class/net/wlan0/statistics/rx_packets", "r");
		tx_file = fopen("/sys/class/net/wlan0/statistics/tx_packets", "r");
		fscanf(rx_file, "%d", &R2);
		fscanf(tx_file, "%d", &T2);
		RX = R2-R1;
		TX = T2-T1;
		write_to_file("packets_per_seconds.txt", "w", RX); 
		printf("RX: %d,  TX: %d\n", RX, TX);
	    }
	    if(RX > 5){
		ATLAS_LOGGER_DEBUG("DP: The number of pachets per seconds was exceeded.\n");
		//break;
	    }
	}
}

void parse_packets(u_char *args, const struct pcap_pkthdr* pkthdr, const u_char* 
    packet) 
{ 
    static int count = 1;
    write_to_file("packets_number.txt", "w", count);
    write_to_file("packets_length.txt", "a", pkthdr->len);
    count++;
}

void *packets(){
    int i;
    char *dev; 
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* descr; 
    const u_char *packet; 
    struct pcap_pkthdr hdr;
    struct ether_header *eptr;    
    struct bpf_program fp;        
    bpf_u_int32 maskp;            
    bpf_u_int32 netp;            

    dev = pcap_lookupdev(errbuf); 
     
    if(dev == NULL) {
        
        ATLAS_LOGGER_ERROR(errbuf);
        exit(1);
    } 
    pcap_lookupnet(dev, &netp, &maskp, errbuf); 
 
    descr = pcap_open_live(dev, BUFSIZ, 1,1000, errbuf); 
    if(descr == NULL) {
        ATLAS_LOGGER_ERROR(errbuf);
        exit(1);
    } 

    if(pcap_compile(descr, &fp, IP, 0, netp) == -1) {
        ATLAS_LOGGER_ERROR("DP: Error calling pcap_compile\n");
        exit(1);
    } 
 
    if(pcap_setfilter(descr, &fp) == -1) {
        ATLAS_LOGGER_ERROR("DP: Error setting filter\n");
        exit(1);
    } 

    pcap_loop(descr, -1, parse_packets, NULL); 
}

void *send_values_to_atlas_client(){
	while (1){
		sleep(SLEEPTIME);
		FILE *f ;
		if ((f = fopen("packets_length.txt", "r")) == NULL){
			ATLAS_LOGGER_ERROR("DP: No packets received");
		}
		else{
			int data;
			int i = 0, pacLen;
			while(!feof(f)){
			    fscanf(f, "%d", &pacLen);
			    data=data+pacLen;
			    i++;
			}
			data=data/i;
			char buff[256];
			sprintf(buff, "averageLength %d", data);
			write_to_socket(buff);
			fclose(f);
		}		
	}
}


