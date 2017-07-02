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
#include <pthread.h>
#include <sys/select.h>
#include <time.h>
#include <termios.h>
#define true		1
#define false		0

#define ERR_OPCODE	0
#define BEGIN_OPCODE	1
#define MOVE_OPCODE	2
#define WAIT_OPCODE	3
#define STR_OPCODE	4
#define BOARD_OPCODE	5
#define CLOSE_OPCODE	6
#define WIN_OPCODE	7

				/**********
				** GLOBAL**
				**********/
/*
 *	Patrick:
 *	added boolean int inUse
 *	removed int inGame
 *	removed int lookingForGame, int sockfd
 */
struct Player {
	struct sockaddr_in cliaddr;
	int cfd;
	int inGame;
	int inUse;
	int lookingForGame;
	char symbol;
} player1, player2;
// *************************Shota's declarations*******************************//
//need check if 4, switch players
//???each fork(game) gives 2 threads(players)
#define ROW 6
#define COL 7
int gameOver=0;
char board[ROW][COL];
int colSize[COL];
void sysErr(char *err);
void initBoard();
void printBoard();
void getInput(struct Player *player);
void placeInput(char player, int col);
void checkConnect(char player,int row, int col);
void playerWin(char player);
void checkPosDiag(char player,int count, int row, int col);
void checkNegDiag(char player,int count, int row, int col);
void checkBoardFull();
// ********************end of Shota's declarations*********************************** //
/*
 * Patrick:
 *	initialize player struct, takes poiter to struct
 *	and the sockfd
 *
 *	changed sockfd and cfd = 0 to = -1
 *	added client->inUse = false
 */
void init_player(struct Player *client)
{
	//bzero(&(client->cliaddr),sizeof(client->cliaddr));
	client->cfd = 0;
	client->inUse = false;
	return;
}

/*
 *	Patrick:
 *	inits both player 1 and player 2
 *	sets player1 to X and player2 to O
 */
void init_players()
{
	init_player(&player1);
	init_player(&player2);
	player1.symbol='X';
	player2.symbol='O';
	return;
}

/*
 *	Patrick:
 *	Sets a player based off arguments
 */
void set_player(struct Player *playPtr, int cfd/*, struct sockaddr_in *caddrPtr*/)
{
	playPtr->cfd = cfd;
	//strncpy((char*)&(playPtr->cliaddr),(char*)caddrPtr,sizeof(struct sockaddr_in));
	playPtr->inUse = true;
	return;
}
//Patrick:
void send_both_msg(char *arg, int argSize)
{
	write(player1.cfd,arg,argSize);
	write(player2.cfd,arg,argSize);
	sleep(1);
	return;
}
//Patrick:
void close_both()
{
	close(player1.cfd);
	close(player2.cfd);
	return;
}


// *********************Shota's code from here down******************************* //
void startGame()
{
	initBoard();
	//gameOver is modified after checkConnect (when a player wins).
	while(!gameOver)
	{
		getInput(&player1);
		getInput(&player2);
	}
	return;
}

void sysErr(char *err)
{
	perror(err);
	exit(1);
};
void initBoard()
{
	//set all positions to indicate free space
	memset(board,32,sizeof(board[0][0])*ROW*COL);
	printBoard();
};

/*
 *	Patrick:
 *	Rewrote printBoard using strncat. snprint method left '\0's all over the place.
 */
 //Shota:
void printBoard()
{
	char boardbuf[500];
	char tempbuf[101];
	int psize;
	bzero(&boardbuf,sizeof(boardbuf));
	bzero(&tempbuf,101);

	//start message with board_opcode, then send board as a string
	snprintf(boardbuf,11,"%d_col:    ",BOARD_OPCODE);
	int i;

	//attach column numbers
	for(i=0;i<COL;++i){
		bzero(&tempbuf,101);	
		psize = sprintf(tempbuf,"%d   ",i);
		strncat(boardbuf,tempbuf,psize);
	}

	//begin attaching row numbers
	strncat(boardbuf,"\nrow:0 |",9);

	int j;
	//attach first row contents
	for(j=0;j<COL;++j){
		bzero(&tempbuf,101);
		psize = snprintf(tempbuf,6," %c |",board[0][j]);
		strncat(boardbuf,tempbuf,psize);
	}
	strncat(boardbuf,"\n      |---------------------------|\n",38);
	
	//attach rest of row numbers and row contents
	for(i=1;i<ROW;++i){
		bzero(&tempbuf,100);
		psize = snprintf(tempbuf,8,"    %d |",i);
		strncat(boardbuf,tempbuf,psize);
		for(j=0;j<COL;++j){
			bzero(&tempbuf,101);
			psize = snprintf(tempbuf,5," %c |",board[i][j]);
			strncat(boardbuf,tempbuf,psize);
		}
		strncat(boardbuf,"\n      |---------------------------|\n",38);
	}

	send_both_msg(boardbuf,sizeof(boardbuf));
	sleep(1);
};

/*
//brute forced print of the board-- maybe optimize to look better if have time
void printBoard()
{
	char tempBoard[500];
	int bSize;
	bSize=snprintf(tempBoard,9,"%d col:    ",BOARD_OPCODE);
	int i;
	for(i=0;i<COL;i++)
		bSize+=snprintf(tempBoard+bSize,5,"%d   ",i);
	bSize+=snprintf(tempBoard+bSize,9,"\nrow:0 |");
	int j;
	for(j=0;j<COL;j++)
		bSize+=snprintf(tempBoard+bSize,5," %c |",board[0][j]);
	bSize+=snprintf(tempBoard+bSize,38,"\n      |---------------------------|\n");
	for(i=1;i<ROW;i++)
	{
		bSize+=snprintf(tempBoard+bSize,8,"    %d |",i);
		for(j=0;j<COL;j++)
			bSize+=snprintf(tempBoard+bSize,5," %c |",board[i][j]);
		bSize+=snprintf(tempBoard+bSize,38,"\n      |---------------------------|\n");
    }
	
	//print the board to each player
	send_both_msg(tempBoard,bSize);
	//printf("board:\n%s\n",tempBoard);
};
*/

void getInput(struct Player *playerPtr)
{
	char temp[31];
	int tSize;
	//tell player its his/her turn
	tSize=snprintf(temp,30,"%d_Player %c, it is your turn. ",STR_OPCODE,playerPtr->symbol);
	temp[tSize+1]=0;
	if(write(playerPtr->cfd,temp,sizeof(temp))<0)
		sysErr("getInput::write");
	sleep(1);
	int columnNumber=-1;

	//check for valid inputs
	while(1)
	{
		sleep(1);
		char temp1[35];
		bzero(&temp1,35);
		int t1Size;
		//tell player to make a move
		t1Size=snprintf(temp1,34,"%d_Enter a integer between 0 and 6",MOVE_OPCODE);
		temp1[t1Size]=0;
		if(write(playerPtr->cfd,temp1,35)<0)
			sysErr("getInput::write2");
		sleep(1);
		//receive move
		char temp2[5];
		int n=0;
		//check if opcode is right.
		if((n=recv(playerPtr->cfd,temp2,4,0))<0)
			sysErr("getInput::read");
		temp2[n]=0;
		if((temp2[0]-48)!=MOVE_OPCODE)
			continue;
		char *tempPtr = (char *)&temp2;

		//parse columnNumber
		strsep(&tempPtr,"_");
		columnNumber=atoi(tempPtr);
		//check if column is full
		if(colSize[columnNumber]>=6)
		{
			char temp3[19];
			tSize=snprintf(temp3,18,"%d_Column is full!\n", STR_OPCODE);
			temp3[tSize+1]=0;
			if(write(playerPtr->cfd,temp3,19)<0)
				sysErr("getInput::write2");
			sleep(1);
			continue;
		}
		//check if column number is between 0 and 6 (already implemented in client)
		if(columnNumber<7&&columnNumber>-1)
		{
			break;
		}
	}
	placeInput(playerPtr->symbol,columnNumber);
};

//place input in board
void placeInput(char player, int col)
{
	//start from lowest row, try column; if full, go to higher row
	int full=1;
	int row=0;
	int i;
	for(i=ROW-1;i>=0;i--)
	{
		//check if empty
		if(board[i][col]==' ')
		{
			board[i][col]=player;
			colSize[col]++;
			row=i;
			full=0;
			break;
		}
	}
	printBoard();
	//check if player won
	checkConnect(player,row,col);
	checkBoardFull();
}

//check if player has 4 connected after each input
void checkConnect(char player, int row, int col)
{
	int connected=0;
	//check col
	int i;
	for(i=0;i<ROW;i++)
	{
		if(board[i][col]==player)
			connected++;
		else
			connected=0;
		if(connected==4)
			playerWin(player);
	}
	connected=0;
	//check row
	for(i=0;i<COL;i++)
	{
		if(board[row][i]==player)
			connected++;
		else
			connected=0;
		if(connected==4)
			playerWin(player);
	}
	connected=0;
	//check pos diag (think slope)
	int j;
	//start from bottom left corner, check each column, then go up a row
	for(i=ROW-1;i>2;i--)
		for(j=0;j<COL-2;j++)
			checkPosDiag(player,connected,i,j);
	//check neg diag (think slope)
	//start from top right corner, check each column, then go down a row
	for(i=0;i<ROW-2;i++)
		for(j=0;j<COL-2;j++)
			checkNegDiag(player,connected,i,j);
}

void checkPosDiag(char player,int count, int row, int col)
{
	//base cases
	if(row>=ROW||col>=COL)
		return;
	if(board[row][col]!=player)
		return;
	count++;
	if(count==4)
		playerWin(player);
	//check up
	checkPosDiag(player,count,--row,++col);
}

void checkNegDiag(char player,int count, int row, int col)
{
	//base cases
	if(row>=ROW||col>=COL)
		return;
	if(board[row][col]!=player)
		return;
	count++;
	if(count==4)
		playerWin(player);
	//check down
	checkNegDiag(player,count,++row,++col);
}
void playerWin(char player)
{
	char buf[17];
	int bSize;
	//send both players win message
	bSize=snprintf(buf,17,"%d_Player %c WINS!!\n",WIN_OPCODE,player);
	send_both_msg(buf,bSize);
	//tell server game is over
	gameOver=1;
	return;
}
//checks if whole board is full before a player wins
void checkBoardFull()
{
	int i=0;
	int nFull=0;
	//check each columnSize
	for(i=0;i<COL;i++)
	{
		//number of full columns
		if(colSize[i]==ROW)
			nFull++;
	}
	//if all columns full, send msg, and close
	if(nFull==COL)
	{
		send_both_msg("%d_Board is full. Draw!\n",STR_OPCODE);
		sleep(1);
		send_both_msg("%d_",CLOSE_OPCODE);
	}
}
//eof
