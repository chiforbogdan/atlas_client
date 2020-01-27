#ifndef CLIENT_H
#define CLIENT_H

void error(char *msg);

void atlas_init();

void write_to_file(char *file, char *type, int RX);

void packets_number_per_second();

void parse_packets(u_char *args, const struct pcap_pkthdr* pkthdr, const u_char* packet);

void *packets();

void *send_values_to_atlas_client();


#endif

