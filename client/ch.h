#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <termios.h>

#define ERR_OPCODE	0
#define BEGIN_OPCODE	1
#define MOVE_OPCODE	2
#define WAIT_OPCODE	3
#define STR_OPCODE	4
#define BOARD_OPCODE	5
#define CLOSE_OPCODE	6
#define WIN_OPCODE	7

#define MAXLINE 	512
#define bufsize		20

#define ROW		6
#define COL		7

			 /**************************
			 **  GLOBAL	VAR & ERRORS  **
			 **************************/

const char *program_name = "conn4client.c";
int sockfd;

void err_handle(char * arg)
{
	printf("Error at %s\n",arg);
	perror(program_name);
	_Exit(2);
}

			/***************
			**	FUNCTIONS **
			***************/
//patrick:
void sendMove(int move)
{
	char writebuf[bufsize+1];
	bzero(&writebuf,bufsize+1);
	int wbSize;
	//send player's move to server
	wbSize=snprintf(writebuf,bufsize,"%d_%d\n",MOVE_OPCODE,move);
	if( send(sockfd,writebuf,wbSize,0)<0 )
		err_handle("ch.h::sendMove::write");
	return;
}

/*	Patrick: wrote pseudocode
 *	Shota: implemented code
 *	reads in one single move from stdin and runs sendMove(move)
 */
void getAndSendMove()
{
	char charbuf[2];
	int move;
	// flush stdin: patrick's tcflush
	tcflush(STDIN_FILENO,TCIFLUSH);
	while(1){
		printf(": ");
		//get input
		if(fread(charbuf,2,1,stdin)<=0)
			err_handle("fread");
		char temp;
		//if second input is not a newline or end of string, try again
		if(charbuf[1]!=10&&charbuf[1]!=0)
		{
			//discard all other inputs
			while((temp=getchar())!='\n' && temp!=EOF)
				;
			printf("Error. Enter ONLY ONE integer between 0 and 6");
			continue;
		}
		//check if move is valid
		move=(charbuf[0])-48;
		if(move<=6 && move >=0)
			break;
		printf("Error. ColumnNumber must be between 0 and 6! Try again");
	}
	sendMove(move);
	return;	
}

/*
 *	Prints board from argument boardstring
 */
void printBoard(char * boardstring)
{
	printf("%s\n",boardstring);
	return;
}

/*  
 *	Patrick:
 *	gets and parses a message from CFD
 *	calls the correct function to handle the type of message.
 */

void get_and_parse_msg()
{
	//printf("________________\n");
	char readbuf[MAXLINE+1];
	bzero(&readbuf,MAXLINE+1);
	char *readstring = (char*)&readbuf;
	int nread=0;
	//printf("blocking for read.\n");
	//receive packet from server
	//Shota: changed read to recv
	if((nread=recv(sockfd,readbuf,MAXLINE,0))<0)
		err_handle("ch.h::get_and_parse_msg::read");
	readbuf[nread]=0;
	int i=0;
	//printf("nread = %d\nreadbuf=%s\n",nread,readbuf);
	//parse message
	char * op_str = strsep(&readstring,"_");
	//printf("op_str = %s\n",op_str);
	int opt = atoi(op_str);
	//printf("block stopped. opt = %d\n",opt);
	switch(opt){
		case STR_OPCODE:
			printf("%s\n",readstring);
			break;
		case MOVE_OPCODE:
			printf("%s",readstring);
			getAndSendMove();
			break;
		case BOARD_OPCODE:
			printBoard(readstring);
			break;
		case WIN_OPCODE:
			printf("%s\n",readstring);
		case CLOSE_OPCODE:
			close(sockfd);
			printf("Game closed by server.\n");
			_Exit(0);
			break;
		default:
			break;
	}//end switch
	return;
}
