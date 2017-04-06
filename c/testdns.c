

#include <stdio.h>
#include <stdlib.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>


#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#define BUFFSIZE 1024
void Die(char *mess) { perror(mess); exit(1); }

typedef unsigned char u_char;

void
print_hex_ascii_line(const u_char *payload, int len, int offset)
{

	int i;
	int gap;
	const u_char *ch;

	/* offset */
	printf("%05d   ", offset);
	
	/* hex */
	ch = payload;
	for(i = 0; i < len; i++) {
		printf("%02x ", *ch);
		ch++;
		/* print extra space after 8th byte for visual aid */
		if (i == 7)
			printf(" ");
	}
	/* print space to handle line less than 8 bytes */
	if (len < 8)
		printf(" ");
	
	/* fill hex gap with spaces if not full line */
	if (len < 16) {
		gap = 16 - len;
		for (i = 0; i < gap; i++) {
			printf("   ");
		}
	}
	printf("   ");
	
	/* ascii (if printable) */
	ch = payload;
	for(i = 0; i < len; i++) {
		if (isprint(*ch))
			printf("%c", *ch);
		else
			printf(".");
		ch++;
	}

	printf("\n");

return;
}



void
print_payload(const u_char *payload, int len)
{

	int len_rem = len;
	int line_width = 16;			/* number of bytes per line */
	int line_len;
	int offset = 0;					/* zero-based offset counter */
	const u_char *ch = payload;

	if (len <= 0)
		return;

	/* data fits on one line */
	if (len <= line_width) {
		print_hex_ascii_line(ch, len, offset);
		return;
	}

	/* data spans multiple lines */
	for ( ;; ) {
		/* compute current line length */
		line_len = line_width % len_rem;
		/* print line */
		print_hex_ascii_line(ch, line_len, offset);
		/* compute total remaining */
		len_rem = len_rem - line_len;
		/* shift pointer to remaining bytes to print */
		ch = ch + line_len;
		/* add offset */
		offset = offset + line_width;
		/* check if we have line width chars or less */
		if (len_rem <= line_width) {
			/* print last line and get out */
			print_hex_ascii_line(ch, len_rem, offset);
			break;
		}
	}

return;
}

int main(int argc, char *argv[]) {
	int sock;
	struct sockaddr_in echoserver;
	struct sockaddr_in echoclient;
	char buffer[BUFFSIZE];
	unsigned int len, clientlen;
	int received = 0, i;
	unsigned short tmp;

	char dns_req[] = {
		0x15,0x77,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x0c,0x73,0x61,0x66,
		0x65,0x62,0x72,0x6f,0x77,0x73,0x69,0x6e,0x67,0x07,0x63,0x6c,0x69,0x65,0x6e,0x74,
		0x73,0x06,0x67,0x6f,0x6f,0x67,0x6c,0x65,0x03,0x63,0x6f,0x6d,0x00,0x00,0x01,0x00,
		0x01
	};

	char *payload;

	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		Die("Failed to create socket");
	}

	srandom(time(NULL));

	memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */
	echoserver.sin_family = AF_INET;
	echoserver.sin_addr.s_addr = inet_addr("192.168.10.1");
	echoserver.sin_port = htons(53);

	len = sizeof(dns_req);

	for (i = 0; i < 10; i++) {
		tmp = rand() % 65536;

		printf("tmp: 0x%4x\n", tmp);
		fflush(stdout);

		dns_req[0] = (char) ((tmp & 0xff00) >> 8);
		dns_req[1] = (char) (tmp & 0xff);

		if (sendto(sock, dns_req, len, 0, 
			(struct sockaddr *) &echoserver, sizeof(echoserver)) != len) 
		{
			Die("Mismatch in number of sent bytes");
		}

		clientlen = sizeof(echoclient);

		payload = buffer;

		received = recvfrom(sock, payload, BUFFSIZE, 0,
			(struct sockaddr *) &echoclient, &clientlen);

		if (received <= 0)
			Die("recvfrom error");

		print_payload(payload, received);

		fprintf(stdout, "\n");

		sleep(1);
	}
	close(sock);
	exit(0);
}
