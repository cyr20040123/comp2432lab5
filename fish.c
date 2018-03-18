#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>

/* define information codes */
#define p_DEAL 1000
#define p_TOPLAY 1010
#define c_REQUESTING 1020
#define p_REQUESTCARD 1030
#define c_GIVECARD 1040
#define c_GOFISH 1041
#define p_YOURCARD 1050
#define p_FISH 1051
#define p_NOMOREFISH 1052
#define c_FINISHED 10000

#define SPADE 400
#define HEART 300
#define CLUB 200
#define DIAMOND 100

int pid,ppid,fd[100][2],cpid[100],number,countc,handcount;
char a[1000][256];
int nextcard;
int seed;

struct Card{
	int type;
	int value;
}hand[7];

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
	char s[3];
	s[3]=0;
	switch(t.type){
		case SPADE: s[0]='S'; break;
		case HEART: s[0]='H'; break;
		case CLUB: s[0]='C'; break;
		case DIAMOND: s[0]='D'; break;
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

void readCards()
{
    char t[256];
    int i;
    for(i=0;scanf("%s",t)==2;i++){
        strcpy(a[i],t);
    }
    return;
}

void initProcess(int n)
{
	int i,tpid;
	for(i=1;i<=n;i++){
		if(pipe(fd[i])!=0){
			printf("[ERROR] Pipe failed.");
			exit(1);
		}
		tpid=fork();
		if(tpid==0){//Child process
			ppid=getppid();
			number=i;
			initrand(number);
			close(fd[number][1]);//1 for out
			return;
		}
		else{//Parent process
			ppid=-1;
			number=0;
			cpid[i]=tpid;
			close(fd[number][0]);//0 for in
		}
	}
    return;
}

void deal()
{
	int i,j;
	struct Message tm;
	tm.code = DEAL;
	nextcard = 0;
	if(countc <= 4) j = 7;
	else j = 5;
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
			write(fd[i][1], tm, sizeof(tm));
		}
	}
	return;
}

void sorthand()
{
	int i,j,handcards;
	struct Card t;
	handcards = countc <= 4 ? 5 : 7;
	for(i = 0; i < handcards - 1; i++) {
		for(j = i+1; j < handcards; j++) {
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

void gethand()
{
	int i, tlen;
	struct Message t;
	i = handcount;
	while(i--){
		if(tlen = read(fd[number][0], t, sizeof(struct Message)) != sizeof(struct Message)){
			printf("[ERROR] Error in get hand (read).\n");
			exit(1);
		}
		hand[i] = t.card;
	}
	sorthand();
   	printf("Child %d, pid %d: initial hand <", count, pid);
   	for(i = 0; i < handcount-1; i++)
   		printf("%s ",toString(hand[i]));
   	printf("%s>\n",toString(hand[i]));
	return ;
}

int main(int argc,char* argv[])
{
	int i;
	countc = str2num(argv[1]);
	handcount = countc <= 4 ? 5 : 7;
    initProcess(countc);
    if(ppid == -1){
    	deal();
    	printf("Parent: the child players are");
    	for(i = 1; i <= countc; i++) printf(" %d",cpid[i]);
    	printf("\n");
    }
    else{
    	gethand();
    }
    wait(NULL);
    return 0;
}

