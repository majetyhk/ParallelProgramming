#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<dirent.h>
#include<math.h>
#include "mpi.h"
#include <sys/time.h>
#include <stddef.h>

#define MAX_WORDS_IN_CORPUS 32
#define MAX_FILEPATH_LENGTH 16
#define MAX_WORD_LENGTH 16
#define MAX_DOCUMENT_NAME_LENGTH 8
#define MAX_STRING_LENGTH 64

typedef char word_document_str[MAX_STRING_LENGTH];

typedef struct o {
	char word[32];
	char document[8];
	int wordCount;
	int docSize;
	int numDocs;
	int numDocsWithWord;
} obj;

typedef struct w {
	char word[32];
	int numDocsWithWord;
	int currDoc;
} u_w;

static int myCompare (const void * a, const void * b)
{
    return strcmp (a, b);
}

int main(int argc , char *argv[]){
	DIR* files;
	struct dirent* file;
	int i,j;
	int numDocs = 0, docSize, contains;
	char filename[MAX_FILEPATH_LENGTH], word[MAX_WORD_LENGTH], document[MAX_DOCUMENT_NAME_LENGTH];
	
	// Will hold all TFIDF objects for all documents
	obj TFIDF[MAX_WORDS_IN_CORPUS];
	int TF_idx = 0;
	memset(TFIDF, -1, sizeof(TFIDF));
	// Will hold all unique words in the corpus and the number of documents with that word
	u_w unique_words[MAX_WORDS_IN_CORPUS];
	int uw_idx = 0;
	memset(unique_words, -1, sizeof(unique_words));
	
	// Will hold the final strings that will be printed out
	word_document_str strings[MAX_WORDS_IN_CORPUS];
	
	
	//Count numDocs
	if((files = opendir("input")) == NULL){
		printf("Directory failed to open\n");
		exit(1);
	}
	while((file = readdir(files))!= NULL){
		// On linux/Unix we don't want current and parent directories
		if(!strcmp(file->d_name, "."))	 continue;
		if(!strcmp(file->d_name, "..")) continue;
		numDocs++;
	}
	
	//----------------MPI Setup------------------------??
	int   numproc, rank, len, source, dest, tag;
	struct timeval t1,t2;                 // Time Variables for Performance Monitoring
	/* initialize MPI */
    MPI_Init(&argc, &argv);

    /* get the number of procs in the comm */
    MPI_Comm_size(MPI_COMM_WORLD, &numproc);

    //---------------------------MPI_Datatype_Creation----------//
    
    //*******************************obj************************//
    const int nitems=6;
    int          blocklengths[6] = {32,8,1,1,1,1};
    MPI_Datatype types[6] = {MPI_CHAR, MPI_CHAR, MPI_INT, MPI_INT, MPI_INT, MPI_INT};
    MPI_Datatype mpi_obj_type;
    MPI_Aint     offsets[6];

    offsets[0] = offsetof(obj, word);
    offsets[1] = offsetof(obj, document);
    offsets[2] = offsetof(obj, wordCount);
    offsets[3] = offsetof(obj, docSize);
    offsets[4] = offsetof(obj, numDocs);
    offsets[5] = offsetof(obj, numDocsWithWord);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_obj_type);
    MPI_Type_commit(&mpi_obj_type);

    //*******************************unique words************************//
    const int nitems2=3;
    int          blocklengths2[3] = {32,1,1};
    MPI_Datatype types2[3] = {MPI_CHAR, MPI_INT, MPI_INT};
    MPI_Datatype mpi_uw_type;
    MPI_Aint     offsets2[3];

    offsets2[0] = offsetof(u_w, word);
    offsets2[1] = offsetof(u_w, numDocsWithWord);
    offsets2[2] = offsetof(u_w, currDoc);

    MPI_Type_create_struct(nitems2, blocklengths2, offsets2, types2, &mpi_uw_type);
    MPI_Type_commit(&mpi_uw_type);

    //---------------------------MPI_Datatype_Creation----------//

    /* get my rank in the comm */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    obj *TFIDF_BUFF;
    u_w *unique_words_BUFF;

    //----------------MPI Setup------------------------//

    if(rank != 0){
    	// Loop through each document and gather TFIDF variables for each word
		for(i=rank; i<=numDocs; i = i+numproc-1){
			sprintf(document, "doc%d", i);
			sprintf(filename,"input/%s",document);
			FILE* fp = fopen(filename, "r");
			if(fp == NULL){
				printf("Error Opening File: %s\n", filename);
				exit(0);
			}
			
			// Get the document size
			docSize = 0;
			while((fscanf(fp,"%s",word))!= EOF)
				docSize++;
			
			// For each word in the document
			fseek(fp, 0, SEEK_SET);
			while((fscanf(fp,"%s",word))!= EOF){
				contains = 0;
				
				// If TFIDF array already contains the word@document, just increment wordCount and break
				for(j=0; j<TF_idx; j++) {
					if(!strcmp(TFIDF[j].word, word) && !strcmp(TFIDF[j].document, document)){
						contains = 1;
						TFIDF[j].wordCount++;
						break;
					}
				}
				
				//If TFIDF array does not contain it, make a new one with wordCount=1
				if(!contains) {
					strcpy(TFIDF[TF_idx].word, word);
					strcpy(TFIDF[TF_idx].document, document);
					TFIDF[TF_idx].wordCount = 1;
					TFIDF[TF_idx].docSize = docSize;
					TFIDF[TF_idx].numDocs = numDocs;
					TF_idx++;
				}
				
				contains = 0;
				// If unique_words array already contains the word, just increment numDocsWithWord
				for(j=0; j<uw_idx; j++) {
					if(!strcmp(unique_words[j].word, word)){
						contains = 1;
						if(unique_words[j].currDoc != i) {
							unique_words[j].numDocsWithWord++;
							unique_words[j].currDoc = i;
						}
						break;
					}
				}
				
				// If unique_words array does not contain it, make a new one with numDocsWithWord=1 
				if(!contains) {
					strcpy(unique_words[uw_idx].word, word);
					unique_words[uw_idx].numDocsWithWord = 1;
					unique_words[uw_idx].currDoc = i;
					uw_idx++;
				}
			}
			fclose(fp);
		}
		MPI_Gather(TFIDF,MAX_WORDS_IN_CORPUS,mpi_obj_type,TFIDF_BUFF,MAX_WORDS_IN_CORPUS, mpi_obj_type,0,MPI_COMM_WORLD );
		MPI_Gather(unique_words,MAX_WORDS_IN_CORPUS,mpi_uw_type,unique_words_BUFF,MAX_WORDS_IN_CORPUS, mpi_uw_type,0,MPI_COMM_WORLD );
    }else{

    	TFIDF_BUFF= (obj *)malloc(sizeof(obj) * MAX_WORDS_IN_CORPUS*numproc);
    	//memset(TFIDF_BUFF, -1, sizeof(TFIDF_BUFF));
    	unique_words_BUFF = (u_w *)malloc(sizeof(u_w) * MAX_WORDS_IN_CORPUS*numproc);

    	MPI_Gather(TFIDF,MAX_WORDS_IN_CORPUS, mpi_obj_type,TFIDF_BUFF,MAX_WORDS_IN_CORPUS,mpi_obj_type,0,MPI_COMM_WORLD );
    	MPI_Gather(unique_words,MAX_WORDS_IN_CORPUS,mpi_uw_type,unique_words_BUFF,MAX_WORDS_IN_CORPUS, mpi_uw_type,0,MPI_COMM_WORLD );
    	// Print TF job similar to HW4/HW5 (For debugging purposes)
		/*printf("-------------TF Job-------------\n");
		for(j=0; j<TF_idx; j++)
			printf("%s@%s\t%d/%d\n", TFIDF[j].word, TFIDF[j].document, TFIDF[j].wordCount, TFIDF[j].docSize);*/
		/*int TFIndex = TF_idx;
		int uwIndex = uw_idx;*/
		for(j=MAX_WORDS_IN_CORPUS; j<MAX_WORDS_IN_CORPUS*numproc; j++){
			
			if(TFIDF_BUFF[j].wordCount!=-1){
				printf("%s@%s\t%d/%d\n", TFIDF_BUFF[j].word, TFIDF_BUFF[j].document, TFIDF_BUFF[j].wordCount, TFIDF_BUFF[j].docSize);
				//TFIDF[TF_idx] = TFIDF_BUFF[j];
				strcpy(TFIDF[TF_idx].word, TFIDF_BUFF[j].word);
				strcpy(TFIDF[TF_idx].document, TFIDF_BUFF[j].document);
				TFIDF[TF_idx].wordCount = TFIDF_BUFF[j].wordCount;
				TFIDF[TF_idx].docSize = TFIDF_BUFF[j].docSize;
				TFIDF[TF_idx].numDocs = TFIDF_BUFF[j].numDocs;
				TF_idx++;
			}

			if(unique_words_BUFF[j].currDoc != -1 ){
				printf("%s@%d\t%d\n", unique_words_BUFF[j].word, j/MAX_WORDS_IN_CORPUS, unique_words_BUFF[j].numDocsWithWord);
				int k = 0; //!strcmp(unique_words_BUFF[j].word, unique_words[k].word)
				
				contains = 0;
				// If unique_words array already contains the word, just increment numDocsWithWord
				for(k=0; k<uw_idx; k++) {
					if(!strcmp(unique_words_BUFF[j].word, unique_words[k].word)){
						contains = 1;
						
						unique_words[k].numDocsWithWord += unique_words_BUFF[j].numDocsWithWord ;
						unique_words[k].currDoc = j/MAX_WORDS_IN_CORPUS;
						
						break;
					}
				}
				
				// If unique_words array does not contain it, make a new one with numDocsWithWord=1 
				if(!contains) {
					strcpy(unique_words[uw_idx].word, unique_words_BUFF[j].word);
					unique_words[uw_idx].numDocsWithWord = 1;
					unique_words[uw_idx].currDoc = j/MAX_WORDS_IN_CORPUS;
					uw_idx++;
				}
			}


		}
		printf("-------------TF Job-------------\n");
		for(j=0; j<TF_idx; j++)
			printf("%s@%s\t%d/%d\n", TFIDF[j].word, TFIDF[j].document, TFIDF[j].wordCount, TFIDF[j].docSize);

		// Use unique_words array to populate TFIDF objects with: numDocsWithWord
		for(i=0; i<TF_idx; i++) {
			for(j=0; j<uw_idx; j++) {
				if(!strcmp(TFIDF[i].word, unique_words[j].word)) {
					TFIDF[i].numDocsWithWord = unique_words[j].numDocsWithWord;	
					break;
				}
			}
		}
		
		// Print IDF job similar to HW4/HW5 (For debugging purposes)
		printf("------------IDF Job-------------\n");
		for(j=0; j<TF_idx; j++)
			printf("%s@%s\t%d/%d\n", TFIDF[j].word, TFIDF[j].document, TFIDF[j].numDocs, TFIDF[j].numDocsWithWord);
			
		// Calculates TFIDF value and puts: "document@word\tTFIDF" into strings array
		for(j=0; j<TF_idx; j++) {
			double TF = 1.0 * TFIDF[j].wordCount / TFIDF[j].docSize;
			double IDF = log(1.0 * TFIDF[j].numDocs / TFIDF[j].numDocsWithWord);
			double TFIDF_value = TF * IDF;
			sprintf(strings[j], "%s@%s\t%.16f", TFIDF[j].document, TFIDF[j].word, TFIDF_value);
		}
		
		// Sort strings and print to file
		qsort(strings, TF_idx, sizeof(char)*MAX_STRING_LENGTH, myCompare);
		FILE* fp = fopen("output.txt", "w");
		if(fp == NULL){
			printf("Error Opening File: output.txt\n");
			exit(0);
		}
		for(i=0; i<TF_idx; i++)
			fprintf(fp, "%s\n", strings[i]);
		fclose(fp);
		

    }
	MPI_Type_free(&mpi_obj_type);
	MPI_Type_free(&mpi_uw_type);
	MPI_Finalize();
	return 0;	
}
