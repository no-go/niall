/**
 * niall.c (was part of gNiall)
 * Copyright 1999 Gary Benson <rat@spunge.org>
 *
 * Modified for CLI input by JotPe Krefeld, 2024
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 **/

/* gcc niall.c -O2 -mtune=native -Wall -o niall.exe */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

#define BUFFER_SIZE  4096

int Niall_Learn(char *Buffer, char *dictname);
void Niall_Reply(char *Buffer, int BufSize);

void Niall_NewDictionary(void);
void Niall_ListDictionary(void);
void Niall_SaveDictionary(char *file);
void Niall_LoadDictionary(char *file);

/** Definitions and structures *************************************************/

#define END_SENTENCE	NULL
#define FILE_ID		"NIALL2"

typedef struct word WORD;
typedef struct ascn ASCN;
struct word
{
	WORD	*Next;
	char	*Data;
	ASCN	*Associations;
};
struct ascn
{
	ASCN	*Next;
	int	Word;
	int	Probability;
};

WORD *WordList;

/** Linked List Handlers **************************************************/

/* Add a new word to the list */
static WORD *AddWord(char *Data)
{
	WORD *Word,*Last;

	for(Last=NULL,Word=WordList;Word;Word=Word->Next) Last=Word;

	Word=calloc(sizeof(WORD),1);
	if(!Word) return(NULL);

	Word->Data=calloc(sizeof(char),strlen(Data)+1);
	if(!Word->Data) return(NULL);
	strcpy(Word->Data,Data);

	if(Last)
		Last->Next=Word;
	else
		WordList=Word;

	return(Word);
}

/* Return pointer to a word in the list */
static WORD *FindWord(char *Data)
{
	WORD *Word;

	for(Word=WordList;Word;Word=Word->Next)
	{
		if(!strcmp(Word->Data,Data)) return(Word);
	}
	return(NULL);
}

/* Return the index of a word */
static int WordIndex(WORD *This)
{
	WORD *Word;
	int i;

	for(i=0,Word=WordList;Word;Word=Word->Next,i++)
	{
		if(Word==This) return(i);
	}
	return(-1);
}

/* Return ptr to word number # */
static WORD *GetWord(int Index)
{
	WORD *Word;
	int i;

	for(i=0,Word=WordList;Word;Word=Word->Next,i++)
	{
		if(i==Index) return(Word);
	}
	return(NULL);
}

/* Count probabilities on a word */
static int CountProbs(WORD *Word)
{
	ASCN *Assoc;
	int total;

	for(total=0,Assoc=Word->Associations;Assoc;Assoc=Assoc->Next)
	{
		total+=Assoc->Probability;
	}
	return(total);
}

/* Count the words in the dictionary */
static int CountWords(void)
{
	WORD *Word;
	int i;

	for(i=0,Word=WordList;Word;Word=Word->Next,i++);
	return(i);
}


/** Learning Functions ********************************************************/

/* Associate a word with one that follows it. */
static void Associate(WORD *Word,int Next)
{
	ASCN *Assoc;

	for(Assoc=Word->Associations;Assoc;Assoc=Assoc->Next)
	{
		if(Assoc->Word==Next)
		{
			Assoc->Probability++;
			return;
		}
	}

	Assoc=calloc(sizeof(ASCN),1);
	if(!Assoc) printf("Err: Out of memory.\n");

	Assoc->Word = Next;
	Assoc->Probability = 1;
	Assoc->Next = Word->Associations;
	Word->Associations = Assoc;
}

/* Add one word to the dictionary. */
static int IndexWord(char *Word,int Follows)
{
	WORD *thisWord,*lastWord;
	int wordIndex;

	if(Word != END_SENTENCE)
	{
		thisWord = FindWord(Word);
		if(!thisWord)
		{
			thisWord = AddWord(Word);
			if(!thisWord) printf("Err: Out of memory.\n");
		}
		wordIndex = WordIndex(thisWord);
	}
	else wordIndex = -1;

	lastWord = GetWord(Follows);
	if(!lastWord) printf("Err: Corrupted brain (Can't find last word).\n");
	Associate(lastWord,wordIndex);

	return(wordIndex);
}

/* Add the words in a processed sentence to the dictionary. */
static void AddWords(char *Buffer)
{
	int LastWord=0;
	int i;

	while(strlen(Buffer))
	{
		while(isspace(*Buffer)) Buffer++;
		for(i=0;(!isspace(Buffer[i]))&&(Buffer[i]!=0);i++);

		if(strlen(Buffer)==0)	/* No more words */
		{
			if(LastWord) IndexWord(END_SENTENCE,LastWord);
		}
		else if(Buffer[i]==0)
		{
			LastWord=IndexWord(Buffer,LastWord);
			IndexWord(END_SENTENCE,LastWord);
			Buffer += i;
		}
		else
		{
			Buffer[i] = 0;
			LastWord = IndexWord(Buffer,LastWord);
			Buffer += (i+1);
		}
	}
}

int Niall_Learn(char *Buffer, char *dictname)
{
	char c;
	int i,j;

	/* Remove trailing spaces */
	for(i=strlen(Buffer)-1;i>=0;i--)
	{
		if(!isspace(Buffer[i])) break;
		Buffer[i]=0;
	}

	/* commands */
	if (Buffer[0]==':')
	{
		/* quit */
		if (Buffer[1]=='q')
		{
			return 2;
		}
		/* save */
		else if (Buffer[1]=='s')
		{
			Niall_SaveDictionary(dictname);
		}

		/* new */
		else if (Buffer[1]=='n')
		{
			Niall_NewDictionary();
		}

		/* list */
		else if (Buffer[1]=='l')
		{
			Niall_ListDictionary();
		}

		return 1;
	}

	/* Strip punctuation and break sentence if necessary */
	for(i=0;i<strlen(Buffer);i++)
	{
		c=Buffer[i];

		if(( c=='.' )||( c==',' ))
		{
			if( i+1 < strlen(Buffer))
			{
				Buffer[i]=0;
				Niall_Learn(&Buffer[i+1], dictname);
			}
			Buffer[i]=0;
			break;
		}
		else if(isspace(c))
		{
			Buffer[i]=' ';
		}
		else if(!isalnum(c))
		{
			for(j=i+1;j<=strlen(Buffer);j++) Buffer[j-1]=Buffer[j];
			i--;
		}
		else Buffer[i]=tolower(c);
	}
	AddWords(Buffer);
	return 0;
}


/** Speaking Functions *****************************************************/

/* Add a string to the end of the buffer, checking for overflows. */
static void safeStrcat(char *Buffer,int BufSize,char *Word)
{
	int i;

	if((strlen(Buffer)+strlen(Word)+1)>BufSize)
	{
		printf("Warn: Buffer overflow - %d bytes exceeded.\n",BufSize);

		for(i=strlen(Buffer);i<BufSize;i++) Buffer[i]='<';
		Buffer[BufSize-1]=0;
	}
	else
	{
		strcat(Buffer,Word);
	}
}

/* Add the next word on to the end of the buffer */
static void StringWord(char *Buffer,int BufSize,WORD *ThisWord)
{
	int nProbs,iProb,total;
	WORD *NextWord;
	ASCN *Assoc;

	/* Randomly select an index for the next word. */
	nProbs = CountProbs(ThisWord);
	if(nProbs<1)
	{
		printf("Warn: Corrupted brain (Unlinked word).\n");
		return;
	}

	/* Taken from the rand(3) manual page... */
	iProb = (int)( (float)nProbs*(float)rand() / ((float)RAND_MAX+1.0) );

	/* Find the next word. */
	for(total=0,Assoc=ThisWord->Associations;Assoc;Assoc=Assoc->Next)
	{
		total+=Assoc->Probability;
		if(total>iProb)
		{
			NextWord = GetWord(Assoc->Word);
			if(NextWord != END_SENTENCE)
			{
				if(strlen(Buffer)) safeStrcat(Buffer,BufSize," ");
				safeStrcat(Buffer,BufSize,NextWord->Data);
				StringWord(Buffer,BufSize,NextWord);
				return;
			}
			else return;
		}
	}
	printf("Warn: Corrupted brain (Loop Overflow).\n");
}

void Niall_Reply(char *Buffer, int BufSize)
{
	/* Clear the buffer. */
	Buffer[0]=0;

	/* Check we have some words to say */
	if(WordList==NULL)
	{
		printf("Warn: Corrupted brain (Not initialised).\n");
		Niall_NewDictionary();
		return;
	}
	if(CountProbs(WordList)==0)
	{
		strcpy(Buffer,"I cannot speak yet!");
		return;
	}

	/* Speak some words of wisdom. */
	StringWord(Buffer,BufSize,WordList);
	Buffer[0]=toupper(Buffer[0]);
	safeStrcat(Buffer,BufSize,".");
}


/** Housekeeping Functions *****************************************************/

static void ClearDictionary(void)
{
	WORD *thisWord,*nextWord;
	ASCN *thisAscn,*nextAscn;

	for(thisWord=WordList;thisWord;thisWord=nextWord)
	{
		for(thisAscn=thisWord->Associations;thisAscn;thisAscn=nextAscn)
		{
			nextAscn=thisAscn->Next;
			free(thisAscn);
		}
		nextWord=thisWord->Next;
		free(thisWord);
	}
	WordList=NULL;
}

void Niall_NewDictionary(void)
{
	ClearDictionary();

	WordList=AddWord("");
	if(!WordList) printf("Err: Out of memory.\n");
}

void Niall_ListDictionary(void)
{
	WORD *Word;
	ASCN *Assoc;
	int i;

	printf("\n");
	for(i=0,Word=WordList;Word;Word=Word->Next,i++)
	{
		if(strlen(Word->Data)==0)
			printf("%6d: > %d|",i,CountProbs(Word));
		else
			printf("%6d: %s %d|",i,Word->Data,CountProbs(Word));
		for(Assoc=Word->Associations;Assoc;Assoc=Assoc->Next)
		{
			printf(" %d(%d)",Assoc->Word,Assoc->Probability);
		}
		printf("\n");
	}
	printf("\n");
}

void Niall_SaveDictionary(char *FileName)
{
	FILE *fHandle;
	int nWords;
	WORD *Word;
	ASCN *Assoc;
	int i;

	nWords=CountWords();
	if(nWords<2)
	{
		printf("Warn: No words to save.\n");
		return;
	}

	fHandle=fopen(FileName,"w");
	if(!fHandle)
	{
		printf("Warn: Can't open file %s.\n",FileName);
		return;
	}
	fprintf(fHandle,"%s %d\n",FILE_ID,nWords);

	for(i=0,Word=WordList;Word;Word=Word->Next,i++)
	{
		if(strlen(Word->Data)==0)
			fprintf(fHandle,"%6d: > %d|",i,CountProbs(Word));
		else
			fprintf(fHandle,"%6d: %s %d|",i,Word->Data,CountProbs(Word));
		for(Assoc=Word->Associations;Assoc;Assoc=Assoc->Next)
		{
			fprintf(fHandle," %d(%d)",Assoc->Word,Assoc->Probability);
		}
		fprintf(fHandle,"\n");
		if(ferror(fHandle))
		{
			printf("Warn: File %s not saved correctly.\n",FileName);
			fclose(fHandle);
			return;
		}
	}
	fclose(fHandle);
}

void Niall_LoadDictionary(char *FileName)
{
	char Buffer[BUFFER_SIZE];
	FILE *fHandle;
	int nWords,i,d;
	int nAsocs,j,w,p,k;
	WORD *Word;

	fHandle=fopen(FileName,"r");
	if(!fHandle)
	{
		printf("Warn: File %s not found. I run with clear Dictionary\n",FileName);
		ClearDictionary();
		return;
	}
	fscanf(fHandle,"%s %d\n",Buffer,&nWords);
	if((strcmp(Buffer,FILE_ID))||(nWords<2))
	{
		printf("Warn: File %s is not a valid Niall file.\n",FileName);
		return;
	}
	ClearDictionary();

	for(i=0;i<nWords;i++)
	{
		fscanf(fHandle,"%6d: %s %d|",&d,Buffer,&nAsocs);
		if((d!=i)||(nAsocs<1))
		{
			printf("Warn: Word %d is corrupted.\n",i);
			continue;
		}
		if(Buffer[0]=='>')
		{
			if(i!=0)
			{
				printf("Warn: first Word %d is corrupted.\n",i);
				Niall_NewDictionary();
				return;
			}
			Buffer[0]=0;
		}
		Word=AddWord(Buffer);
		if(Word==NULL) printf("Err: Out of memory.\n");

		for(j=0;j<nAsocs;)
		{
			fscanf(fHandle," %d(%d)",&w,&p);
			if(w>=nWords)
			{
				printf("Warn: Word %d/Assoc %d is corrupted.\n",i,j);
				j+=p;
				continue;
			}
			for(k=0;k<p;k++) Associate(Word,w);
			j+=p;
		}
	}
}

/*******************************************************************************/



int main(int argc, char *argv[])
{
	int res = 1;
	int commandRes = 0;
	char buffer[BUFFER_SIZE];
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	srand(time(NULL));
	Niall_NewDictionary();

	switch (argc)
	{
		case 3:
			// learn
			fp = fopen(argv[1], "r");
			if (fp == NULL)
			{
				res = 1;
			}
			else
			{
				while ((read = getline(&line, &len, fp)) != -1) {
					Niall_Learn(line, argv[2]);
				}

				fclose(fp);

				Niall_Reply(buffer, BUFFER_SIZE -1);
				printf("Niall: %s\n", buffer);

				Niall_SaveDictionary(argv[2]);

				if (line) free(line);
				res = 0;
			}
			break;

		case 2:
			/* chat */
			Niall_LoadDictionary(argv[1]);
			res = 0;
			printf("Commands   :q = quit\n");
			printf("           :s = save actual dictionary\n");
			printf("           :n = new/clear dictionary\n");
			printf("           :l = list dictionary\n\n");

			while (commandRes == 0) {
				char buf[BUFFER_SIZE];
				printf("User: ");

				fgets(buf, BUFFER_SIZE, stdin);
				len = strlen(buf)-1;
				if (buf[len] == '\n') buf[len] = '\0';

				commandRes = Niall_Learn(buf, argv[1]);
				if (commandRes == 0)
				{
					Niall_Reply(buf, BUFFER_SIZE -1);
					printf("Niall: %s\n", buf);
				}
				/* skip reply */
				else if (commandRes == 1) {
					commandRes = 0;
				}
				/* else 2 -> end loop */
			}
			break;

		default:
			/* error */
			printf("USEAGE: %s input-textfile output-dict-name\n", argv[0]);
			printf("        %s dict-name\n\n", argv[0]);
			break;
	}

	return res;
}
