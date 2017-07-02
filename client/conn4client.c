#include "ch.h"


			/**************
			**	Funtions **
			**************/
//Patrick:
int main(int argc, char *argv[])
{
	// get portno:
	int portno = 15000;
	if(argc!=3){
		printf("Syntax: ./c4client <host> <portno>\n");
		_Exit(2);
	}
	else{
		portno = atoi(argv[2]);
	}
	
	struct sockaddr_in servaddr;

	printf("Attempting to connect to server...\n");

	// socket:
	if((sockfd=socket(AF_INET,SOCK_STREAM,0)) < 0)
		err_handle("main::socket");
	
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(portno);
	if( inet_pton(AF_INET, argv[1], &servaddr.sin_addr) < 0 )
		err_handle("main::inet_pton");
	
	// connect:
	if( (connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr))) < 0 )
		err_handle("main::bind");
	
	printf("Requesting to join game...\n");
	
	char * writebuf = "1";
	write(sockfd,writebuf,1);
	
	while(1){
	  get_and_parse_msg();
		
	}//end while
	
	//close(sockfd);
	
	return 0;
}

//eof
