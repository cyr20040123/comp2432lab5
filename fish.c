#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>

/* define information codes */
#define p_DEAL 1000
#define p_TOPLAY 1010
#define c_REQUESTPERSON 1019
#define c_REQUESTING 1020
#define p_REQUESTCARD 1030
#define c_GIVECARD 1040
#define c_GOFISH 1041
#define p_YOURCARD 1050
#define p_FISH 1051
#define p_NOMOREFISH 1052
#define c_FINISHED 10000
#define p_FINISHED 10001

#define SPADE 400
#define HEART 300
#define CLUB 200
#define DIAMOND 100

#define READ 0
#define WRITE 1
#define PARENT 0
#define CHILD 1
//fd[number][P/C][R/W]

int pid,ppid,fd[100][2][2],cpid[100],number,countc,handcount;
char a[1000][256];
int nextcard,totalcards,paircount=0;
int status[200], playeronline;
int seed;

struct Card{
	int type;
	int value;
}hand[7],pairs[52];

struct Message{
	int code;
	int person;
	struct Card card;
};

void initrand(int myid)
{
	seed = myid - 1;
}

int myrand()
{
	return (seed++ % 99) + 1;
}

int str2num(char* t)
{
    int s=0,i;
    for(i=0;i<strlen(t);i++){
        s*=10;
        s+=t[i]-'0';
    }
    return s;
}

char* toString(struct Card t){
	static char s[256];
	s[2]=0;
	switch(t.type){
		case SPADE: s[0]='S'; break;
		case HEART: s[0]='H'; break;
		case CLUB: s[0]='C'; break;
		case DIAMOND: s[0]='D'; break;
		default: s[0]='X';
	}
	switch(t.value){
		case 10: s[1]='T'; break;
		case 11: s[1]='J'; break;
		case 12: s[1]='Q'; break;
		case 13: s[1]='K'; break;
		case 14: s[1]='A'; break;
		default: s[1]=t.value+'0';
	}
	return s;
}

void printInfo(char* command)
{
	printf("Child %d, pid %d: %s ", number, pid, command);
}

void initProcess(int n)
{
	int i,tpid;
	for(i = 1; i <= n; i ++){
		if(pipe(fd[i][PARENT]) != 0 || pipe(fd[i][CHILD]) != 0){
			printf("[ERROR] Pipe failed.\n");
			exit(1);
		}
		tpid = fork();
		if(tpid < 0){
			printf("[ERROR] Fork failed.\n");
			exit(1);
		}
		if(tpid == 0){//Child process
			pid = getpid();
			ppid = getppid();
			number = i;
			initrand(number);
			close(fd[number][PARENT][WRITE]);//1 for out
			printf("child %i created\n",i);
			return;
		}
		else{//Parent process
			ppid = -1;
			number = 0;
			cpid[i] = tpid;
			close(fd[i][CHILD][WRITE]);//0 for in
		}
	}
    return;
}

void readCards()
{
    char t[256];
    int i;
    for(i=0;~scanf("%s",t);i++){
    	if(strlen(t)!=2)break;
        strcpy(a[i],t);
    }
    printf("%d cards read.\n",i);
    totalcards=i;
    return;
}

void deal()
{
	int i,j;
	struct Message tm;
	tm.code = p_DEAL;
	nextcard = 0;
	j = handcount;
	while(j--){
		for(i = 1; i <= countc; i ++){
			switch(a[nextcard][0]){
				case 'S': case 's': tm.card.type = SPADE; break;
				case 'H': case 'h': tm.card.type = HEART; break;
				case 'C': case 'c': tm.card.type = CLUB; break;
				case 'D': case 'd': tm.card.type = DIAMOND; break;
				default: printf("[ERROR] Error in reading cards.\n"); exit(1);
			}
			switch(a[nextcard][1]){
				case 'T': case 't': tm.card.value = 10; break;
				case 'J': case 'j': tm.card.value = 11; break;
				case 'Q': case 'q': tm.card.value = 12; break;
				case 'K': case 'k': tm.card.value = 13; break;
				case 'A': case 'a': tm.card.value = 14; break;
				default: tm.card.value = a[nextcard][1]-'0';
			}
			nextcard++;
			write(fd[i][PARENT][WRITE], &tm, sizeof(tm));
		}
	}
	return;
}

void sortHand()
{
	int i,j;
	struct Card t;
	for(i = 0; i < handcount - 1; i++) {
		for(j = i+1; j < handcount; j++) {
			if (hand[i].value * 10000 + hand[i].type < hand[j].value * 10000 + hand[j].type)
			{
				t = hand[i];
				hand[i] = hand[j];
				hand[j] = t;
			}
		}
	}
	return ;
}

void printHand()
{
	int i;
	printf("<");
	for(i = 0; i < handcount-1; i++)
	{
		printf("%s ",toString(hand[i]));
	}
	printf("%s>\n",toString(hand[i]));
	return ;
}

void reduceHand()
{
	int i = 0, j = 0, ohandcount = handcount;
	for(i = 0; i < ohandcount; i++){
		if (i < ohandcount - 1 && hand[i].value == hand[i+1].value){
			pairs[paircount++] = hand[i];
			pairs[paircount++] = hand[i+1];
			handcount -= 2;
			i += 1;
			continue;
		}
		hand[j++] = hand[i];
	}
	if(ohandcount != handcount){
		printInfo("reduced hand");
		printHand();
	}
	return ;
}

void getHand()
{
	int i, tlen;
	struct Message t;
	i = handcount;
	while(i--){
		if(tlen = read(fd[number][PARENT][READ], &t, sizeof(struct Message)) != sizeof(struct Message)){
			printf("[ERROR] Error in get hand (read): t.code = %d, len = %d\n",t.code,tlen);
			exit(1);
		}
		hand[i] = t.card;
	}
	sortHand();
	printInfo("initial hand");
	printHand();
	reduceHand();
	return ;
}

void waitForSignal()
{
	int timesout = 100000, who, tcard, i;
	struct Message msgIn, msgOut;
	while(timesout--){
		if(read(fd[number][PARENT][READ], &msgIn, sizeof(struct Message)) != sizeof(struct Message)){
			printf("[ERROR] Error in read message, child %d.\n",number);
			exit(1);
		}
		switch(msgIn.code){
			case p_FINISHED:
				status[msgIn.person] = 0;
				playeronline --;
				break;
			case p_TOPLAY:
				printInfo("my hand");
				printHand();
				if(playeronline <= 1){
					msgOut.code = c_GOFISH;
					read(fd[number][PARENT][READ], &msgIn, sizeof(struct Message));
					printInfo("only me, go fish directly, drawn %s\n",toString(msgIn.card));
				}
				who = myrand();
				tcard = myrand();
				msgOut.code = c_REQUESTING;
				msgOut.card = hand[tcard % handcount];
				for(i = 1, j = 1; i <= countc && j != who % playeronline; i ++){
					if(status[i] && i != number) j++;
				}
				
				
				msgOut.code = c_REQUESTPERSON;
				msgOut.person = who;
				//msgOut.card = hand[tcard % handcount];
				write(fd[number][CHILD][WRITE], &msgOut, sizeof(struct Message));
				read(fd[number][PARENT][READ], &msgIn, sizeof(struct Message));
				if(msgIn.person <= 0){
					//no other player left
					msgOut.code = c_GOFISH;
					write(fd[number][CHILD][WRITE], &msgOut, sizeof(struct Message));
					read(fd[number][CHILD][READ], &msgIn, sizeof(struct Message));
					if(msgIn.code == p_NOMOREFISH){/*end*/;}
					else{
						printInfo("only me, go fish directly, drawn %s\n",toString(msgIn.card));
						////
					}
				}
				else{
					printInfo("random numbers %d %d, asking child %d for rank %d\n", who, tcard, msgIn.person, hand[tcard % handcount].value);
					msgOut.code = c_REQUESTING;
					msgOut.person = msgIn.person;
					msgOut.card = hand[tcard % handcount];
					write(fd[number][CHILD][WRITE], &msgOut, sizeof(struct Message));
					read(fd[number][PARENT][READ], &msgIn, sizeof(struct Message));
					switch(msgIn.code){//////
						case p_FISH:
							printInfo("drawn");
							printf(" %s\n",toString(msgIn.card));
							hand[handcount++] = msgIn.card;
							break;
						case p_YOURCARD:
							printInfo("receive");
							printf(" %s from child %d\n", toString(msgIn.card), msgOut.person);
							hand[handcount++] = msgIn.card;
							break;
						case p_NOMOREFISH:
							break;
						default:
							printf("[ERROR] Invalid state after requesting.\n");
							exit(1);
					}
					//fish or yourcard or NOMOREFISH
					////
				}
				break;
			case p_REQUESTCARD:
				break;
			/*case p_YOURCARD:
				break;
			case p_FISH:
				break;
			case p_NOMOREFISH:
				break;*/
			
			#define c_REQUESTING 1020
			#define  1030
			#define c_GIVECARD 1040
			#define c_GOFISH 1041
			#define  1050
			#define  1051
			#define p_NOMOREFISH 1052
			#define c_FINISHED 10000
		}
	}
	if(timesout <= 0)printf("[ERROR] Terminated in waitForSignal, child %d.\n",number);
	//check handcount
	return ;
}

void gameStart()
{
	int i, playerleft = countc;
	struct Message msgIn, msgOut;
	i = 0;
	while(nextcard != totalcards && playerleft > 0)
	{
		do{
			i = i % countc + 1;
		}while(status[i] != 1);
		msgOut.code = p_TOPLAY;
		write(fd[i][PARENT][WRITE], &msgOut, sizeof(msg));
		
	}
	return ;
}

int main(int argc,char* argv[])
{
	int i;
	countc = str2num(argv[1]);
	handcount = countc <= 4 ? 7 : 5;
	for(i = 1; i <= countc; i++) status[i] = 1;
	playeronline = countc;
    initProcess(countc);
    if(ppid == -1){
    	readCards();
    	deal();
    	printf("Parent: the child players are");
    	for(i = 1; i <= countc; i++) printf(" %d",cpid[i]);
    	printf("\n");
    	gameStart();
    }
    else{
    	getHand();
    	waitForSignal();
    	return 0;
    }
    for(i = 1; i <= countc; i++) wait(cpid[i]);
    return 0;
}

