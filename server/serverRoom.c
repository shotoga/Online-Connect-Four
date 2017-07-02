#include "sr.h"

#define listenQueue		10
#define MAX_PLAYERS		6
#define MAX_IN_GAME		2
#define REQ_SIZE		20

const char *program_name = "serverRoom.c";
void *thread_handle_request(void * arg);

pthread_mutex_t setplayer_mutex = PTHREAD_MUTEX_INITIALIZER;

void err_handle(char *arg)
{
	printf("Error at %s\n",arg);
	perror(program_name);
	_Exit(2);
}

//Patrick:
int main(int argc, char *argv[])
{
	uint16_t servport = 15000;
	if(argc != 2)
		printf("No port supplied. Using port number %d\n",servport);
	else
		servport = atoi(argv[1]);
	
	int i = 0;
			/*
			// create players:
			struct Player players[MAX_PLAYERS];
			for(i = 0; i < MAX_PLAYERS; ++i)
				bzero(&players[i],sizeof(struct Player));
			*/
	
	// socket:
	int sockfd;
	if( (sockfd=socket(AF_INET,SOCK_STREAM,0)) < 0 )
		err_handle("serverRoom.c::main::socket");
	
	// bind:
	socklen_t addrlen = sizeof(struct sockaddr_in);
	struct sockaddr_in servaddr;
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(servport);
	servaddr.sin_family = PF_INET;
	if( bind(sockfd,(struct sockaddr *)&servaddr,addrlen) < 0 )
		err_handle("serverRoom.c::main::bind");
	
	// listen:
	if( listen(sockfd,listenQueue) < 0 )
		err_handle("serverroom.c::main::listen");
	
	//variables:
	pid_t pid;
	pthread_t tid[MAX_IN_GAME];
	int pthread_iterator=0, temp=0, other_iterator=0;
	ssize_t cfd;
	
	// start the while loop to get new clients:
	while(1){
		//printf("ptit = %d\n",pthread_iterator);
		// check if were have the max requests;
		if(pthread_iterator >= (MAX_IN_GAME)){
			//printf("about to join\n");
			for(i=0;i<MAX_IN_GAME;++i)
				pthread_join(tid[i],NULL);
			//printf("join done\n");
			if(player1.inUse == false && pthread_iterator>0)
				--pthread_iterator;
			if(player2.inUse == false && pthread_iterator>0)
				--pthread_iterator;
		}
		
		// check if game is ready
		if(player1.inUse == true && player2.inUse == true){
			pthread_iterator = 0;
			// start game
			pid = fork();
			if(pid == 0){
			printf("player1.cfd=%d\nplayer2.cfd=%d\n",player1.cfd,player2.cfd);
				printf("startGame()...\n");
				startGame();
				close_both();
				_Exit(2);
			}//end if
			else{	// reset everything:
				close_both();
				init_players();
				cfd = 0;
				pthread_iterator = 0;
				other_iterator = 0;
				continue;
			}//end else
		}// end if
		
		// accept:
		struct sockaddr_in cliaddr;
		bzero(&cliaddr,sizeof(cliaddr));
		
		if(pthread_iterator == 0)
			printf("Creating a new room...\n");
			
		if( (cfd=accept(sockfd,(struct sockaddr *)&cliaddr,&addrlen)) < 0 )
				err_handle("serverRoom.c::main::accept");
		
		// process request:
		pthread_create(&tid[pthread_iterator],NULL,thread_handle_request,(void*)cfd);
		++pthread_iterator;
		
		cfd = 0;
	}//end while
	
	return 0;
}

/*
 *	Takes cfd from accept, and the cliaddrPtr as argument.
 *	sets player class accordingly.
 */
void *thread_handle_request(void * arg)
{
	// variables:
	char err_string[] = "0_Unable to parse request.\n";
	char readbuf[REQ_SIZE+1];
	char * tempbuf;
	int in_opcode=0;
	
	ssize_t cfd = (ssize_t)arg;
	bzero(&readbuf,REQ_SIZE+1);
	read(cfd,readbuf,REQ_SIZE);
	printf("Request received: %s\n",readbuf);
	
	// parse request:
	in_opcode = atoi(readbuf);
	
	// process request:
	switch(in_opcode){
	case BEGIN_OPCODE: // if proper request:
		pthread_mutex_lock(&setplayer_mutex);//lock
		if(player1.inUse == false){
		  player1.symbol='X';
			set_player(&player1,cfd/*,cliaddrPtr*/);
			tempbuf = "4_You are player1. Your symbol: X\nWaiting for other players...\n";
			if( write(player1.cfd,tempbuf,strlen(tempbuf)) < 0 )
				return NULL;
		}//endif
		else if(player1.inUse == true && player2.inUse == false){
		  player2.symbol='O';
			set_player(&player2,cfd/*,cliaddrPtr*/);
			tempbuf = "4_You are player2. Your symbol: O\nWaiting for other players...\n";
			if( write(player2.cfd,tempbuf,strlen(tempbuf)) < 0 )
				return NULL;
		}//end elseif
		pthread_mutex_unlock(&setplayer_mutex);//unlock
		break;
	default: // if not proper request:
		write(cfd,err_string,strlen(err_string));
		close(cfd);
	}//end switch
	return NULL;
}

