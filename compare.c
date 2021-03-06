#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include <math.h>
#include <sys/queue.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
//#ifndef QSIZE
//#define QSIZE 8
//#endif







int QSIZE = 20;

typedef struct {
	char** data;
	unsigned count;
	unsigned head;
	int open;
	pthread_mutex_t lock;
	pthread_cond_t read_ready;
	pthread_cond_t write_ready;
} queue_t;


int init(queue_t *Q)
{
    Q->data = malloc(sizeof(char*)*QSIZE);
	Q->count = 0;
	Q->head = 0;
	Q->open = 1;
	pthread_mutex_init(&Q->lock, NULL);
	pthread_cond_init(&Q->read_ready, NULL);
	pthread_cond_init(&Q->write_ready, NULL);
	
	return 0;
}

int destroy(queue_t *Q)
{
	pthread_mutex_destroy(&Q->lock);
	pthread_cond_destroy(&Q->read_ready);
	pthread_cond_destroy(&Q->write_ready);

	return 0;
}
int get_qCount(queue_t *Q){
    pthread_mutex_lock(&Q->lock);
    unsigned int x = Q->count;
    pthread_mutex_unlock(&Q->lock);
    return x;
}


// add item to end of queue
// if the queue is full, block until space becomes available
int enqueue(queue_t *Q, char* item)
{
	pthread_mutex_lock(&Q->lock);
	
	while (Q->count == QSIZE && Q->open) {
        
		pthread_cond_wait(&Q->write_ready, &Q->lock);
	}
	if (!Q->open) {
		pthread_mutex_unlock(&Q->lock);
		return -1;
	}
	
	unsigned i = Q->head + Q->count;
   
	if (i >= QSIZE-1){
        
        QSIZE *= 2;
        char** temp = realloc(Q->data, sizeof(char*) * QSIZE);
       
        Q->data = temp;
        
    }
	
    //testing-kevin
    Q->data[i] = malloc(sizeof(char)*(strlen(item)+1));
    strcpy(Q->data[i],item);
    //test

	//Q->data[i] = item;
	++Q->count;
	
	pthread_cond_signal(&Q->read_ready);
	
	pthread_mutex_unlock(&Q->lock);
	
	return 0;
}


int dequeue(queue_t *Q, char* item)
{
	pthread_mutex_lock(&Q->lock);
	
	while (Q->count == 0 && Q->open) {
        
		pthread_cond_wait(&Q->read_ready, &Q->lock);
	}
	if (Q->count == 0) {
		pthread_mutex_unlock(&Q->lock);
		return -1;
	}
	
    //item = Q->data[Q->head];
    strcpy(item, Q->data[Q->head]);
    free(Q->data[Q->head]);
    //printf("i = %d queue data :%s",Q->head,item);
	--Q->count;
	++Q->head;
	if (Q->head == QSIZE) Q->head = 0;
	
	pthread_cond_signal(&Q->write_ready);
	
	pthread_mutex_unlock(&Q->lock);
	
	return 0;
}

int qclose(queue_t *Q)
{
	pthread_mutex_lock(&Q->lock);
	Q->open = 0;
    free(Q->data);
	pthread_cond_broadcast(&Q->read_ready);
	pthread_cond_broadcast(&Q->write_ready);
	pthread_mutex_unlock(&Q->lock);	

	return 0;
}
int length(queue_t *Q){
    pthread_mutex_lock(&Q->lock);
    
    int p = strlen(Q->data[Q->head]);
    pthread_mutex_unlock(&Q->lock);
    return p;
}
void close_q(queue_t *Q){
    pthread_mutex_lock(&Q->lock);
    Q->open = 0;
    
    pthread_mutex_unlock(&Q->lock);

}




/* ******** 
 * Example client code for synchronized queue 
 */

struct targs {
	queue_t *Q;
	int id;
	int max;
	int wait;
};




typedef struct{//arguments to pass into directory thread function
    queue_t *arg_d_q;
    queue_t *arg_f_q;
    char* suff;
}queues_struct;

typedef struct Node{//Node for linked list of words, there will be 1 linked list for each file
  double freq;//frequency of the word in the file
  char* word;//name of the word
  struct Node *next;
} Node;

typedef struct{//this will be used to create an array of structs, each index containing a file name and its linked list of word frequencies
  Node *llHead;//pointer to head of linked list containing words w/ frequencies in alphabetical order
  char* fileName;//name of file
  int word_count;
} FileNode;

typedef struct{//arguments to pass into file thread function
    queue_t *file_arg_q;
    queue_t *dir_arg_q;
    pthread_t *id;
    pthread_t *array_id;
    FileNode *wsdFiles;
    int wsdCounter;
    pthread_mutex_t lock;
    int wsd_size;
    FileNode **final_wsd;

}file_args;

typedef struct{
    char *file1;
    char *file2;
    double jsdValue;
    int cwc;//combined word count
}JSD;

typedef struct{
    FileNode* wfdArrayHead;
    JSD* jsdArrayHead;
    int lo;
    int hi;
    int wsdLength;
}analysis_args;

void finit(file_args *f){
    f->wsdCounter =0;
    pthread_mutex_init(&f->lock, NULL);
}

int getwsdCounter(file_args *f){
    pthread_mutex_lock(&f->lock);
    int x = f->wsdCounter;
    pthread_mutex_unlock(&f->lock);
    return x;
}

void increment(file_args *f,FileNode* arr){
    pthread_mutex_lock(&f->lock);
    f->wsdCounter = f->wsdCounter +1;
    /**
    FileNode* temparray;
    if(f->wsdCounter>f->wsd_size-2){
        printf("O0QAWJDMQWIODMQWOIDMWQOIDMWQIO");
        f-> wsd_size  = f->wsd_size * 2;
        int temp = f->wsd_size;
        //FileNode* temp = realloc(f->wsdFiles, sizeof(FileNode) * f->wsd_size); 
        temparray = realloc(arr, sizeof(FileNode) * temp);
        //FileNode* temp = realloc(f->wsdFiles, sizeof(FileNode*) * f->wsd_size);
        //f->wsdFiles=  temp;
        f->wsdFiles = temparray;
        arr = temparray;
    }
    **/
    
    
    
    pthread_mutex_unlock(&f->lock);
    

}
FileNode* get_array(file_args *f){
    pthread_mutex_lock(&f->lock);
    FileNode* x = f->wsdFiles;
    pthread_mutex_unlock(&f->lock);
    return x;
}


/**
double comp(const void *a, const void* b){
    JSD *j1 = (JSD *)a;
    JSD *j2 = (JSD *)b;
    return (j2->cwc - j1->cwc);
}
**/
int comp(const void *a, const void *b)
{
    
    JSD *j1 = (JSD *)a;
    JSD *j2 = (JSD *)b;
   if (j1->cwc > j2->cwc)
      return -1;
   else if (j1->cwc < j2->cwc)
      return 1;
   else
      return 0;
}



void freeList(Node* head_ref){
 
    /* deref head_ref to get the real head */
    Node* current = head_ref;
    Node* next = NULL;
 
    while (current != NULL)
    {
        next = current->next;
        free(current->word);
        free(current);
        current = next;
    }
 
    
    head_ref = NULL;
}

void displayList(Node* h_ref){
    Node* current = h_ref;
    printf("DISPLAYING LIST\n");
    double total = 0;
    while (current != NULL)
    {
        total+=current->freq;
        printf("Node name: %s Frequency: %f\n ",current->word,current->freq);
        current = current->next;
    }
    printf("total freq: %f\n",total);
    
}






Node * insertNode(char *word, Node *h,int size){

    Node *newNode = NULL;
    

    Node *previous = NULL;

    Node *current = h;


    newNode = malloc(sizeof(Node));
    newNode->freq = 0;
    newNode->word = NULL;
    newNode->next = NULL;

    if(h==NULL ){
        
        h=malloc(sizeof(Node));
        h->word = malloc(sizeof(char)*(size+1));
        h->word = strncpy(h->word, word,size+1);
        h->freq = 1;
        h->next = NULL;
        free(newNode);
    }

    else{
   
        newNode->word = malloc(sizeof(char)*(size+1));
        newNode->word = strncpy(newNode->word, word,size+1);
        newNode->freq = 1;
        //int x = strncmp(word, current->word,size);
        //printf(" size: %d word: %s ||curr->word: %s\n",size,word,current->word);
   //figure out where to insert in linked list
   //&& strncmp(word,current->word,size)>0
        while(current !=NULL && strncmp(word,current->word,size+1)>0){
            //printf(" size: %d word: %s ||curr->word:%s \n",size,word,current->word);
    
            previous = current;
      
            current = current->next;

        }
     
        if(previous==NULL){
            newNode->next = current;
            
         
            h = newNode;
        }
        else{
      
            previous->next = newNode;
            newNode->next = current;
        }
    }


    return h;
}




//dir checker
int isdir(char *name){
  struct stat data;
  int err = stat(name, &data);
  // should confirm err == 0
  if (err){
    perror(name);
    return 0;
  }

  if(S_ISDIR(data.st_mode)){
    return 1;
  
  }
  return 0;
}

//directory thread
//file thread
void *file_thread(void *temp){
    file_args *fstruct= (file_args *) temp;
    queue_t *f_q = (queue_t *)fstruct->file_arg_q;
    //queue_t *d_q = (queue_t *)fstruct->dir_arg_q;
    FileNode *fileArray = (FileNode *)fstruct->wsdFiles;
    //int wfdSizeTemp = (int)fstruct->wsdCounter;
    pthread_t *join_id = (pthread_t *) fstruct->id;
    pthread_t * stop_id = (pthread_t * )fstruct->array_id;
    //  || d_q->open == 1
    if(pthread_self() == stop_id[0]){
        pthread_join(*join_id, NULL);
    }
    
    while(get_qCount(f_q)>0){
        int totalWords = 0;

        Node *head = NULL;
        //head->freq = 0;
        //head->word = NULL;
        //head->next = NULL;
        
        int file_len = length(f_q);
        char*file_name = malloc(sizeof(char)* (file_len +10));
        
        //char* dir_name;
            
        dequeue(f_q,file_name);

        //printf("filethread name: %s\n",file_name);
       
        int fdr;
        fdr = open(file_name,O_RDONLY);
        char c;
        char prev =' ';
        char* buffer = malloc(sizeof(char)*10);
        int buff_max_size = 10;
        int buff_curr_size = 0;
        while(read(fdr,&c,1) != 0){
            
            if (buff_curr_size+5 >= buff_max_size){
                buffer = realloc(buffer,sizeof(char)*(buff_max_size*2));
        
                buff_max_size *= 2;
        
            }
            if( (isalpha(c)|| c =='-') ||  (isdigit(c)||0  ) ){
                //printf("adding character to buffer");
                buffer[buff_curr_size] = tolower(c);
                buff_curr_size+=1;
                
            }
            //space
            else if(isspace(c) && isspace(prev) == 0){
                totalWords++;
                //printf("GOT HERE");
                //put into node
                buffer[buff_curr_size] = '\0';
                //first check if word exists in linked list
                //if yes increment count by 1
                //if not make new node and insert it in alphabetical order
                //
                //int inList = 0;

                
                Node *ptr = head;
                int orig = 1;
                //printf("word found");
                
                while(ptr != NULL){
                    if (ptr->word != NULL){
                        if(strcmp(buffer, ptr->word) == 0){
                            orig = 0;
                            ptr->freq +=1;
                            //inList = 1;
                            break;
                        }
                    }
                    
                    
                    ptr = ptr->next;
                }
                
                

                if(orig){
                    head = insertNode(buffer,head,buff_curr_size);
                    
                }
                
                //printf("word: %s  \n",buffer);
                for(int i=0; i<buff_curr_size; i++){//resets word buffer to be empty
                    buffer[i] = '\0';
                }
                


                buff_curr_size=0;



            }
            //punc
            else{
                continue;
            }
            prev = c;
            


            //printf("%c",c);
        }
        

        if (buff_curr_size != 0){
            totalWords++;
            buffer[buff_curr_size] = '\0';

            Node *ptr = head;
            int orig = 1;
            //printf("word found");
                
            while(ptr != NULL){
                if (ptr->word != NULL){
                     if(strcmp(buffer, ptr->word) == 0){
                          orig = 0;
                         ptr->freq +=1;
                        //inList = 1;
                         break;
                      }
                 }    
                ptr = ptr->next;
             }
                
            if(orig){
                head = insertNode(buffer,head,buff_curr_size);    
            }
        }
        

        //filearray[fstruct->wsdCounter]->llHead = malloc(sizeof(FileNode));
        int wsdcount = getwsdCounter(fstruct);
        
        increment(fstruct,fileArray);
        /**
        pthread_mutex_lock(&fstruct->lock);
        if(wsdcount>fstruct->wsd_size-2)
        {

            printf("O0QAWJDMQWIODMQWOIDMWQOIDMWQIO");
            fstruct-> wsd_size  = fstruct->wsd_size * 2;
            
            fileArray = realloc(fileArray, sizeof(FileNode) * fstruct->wsd_size);
            
            fstruct->wsdFiles = fileArray;
        }
        pthread_mutex_unlock(&fstruct->lock);
        **/
       
        
        
        

       //fileArray = get_array(fstruct);
      //  fstruct->wsdFiles = fileArray;
        


       // printf("wsdcount:%d\n",wsdcount);
       // printf("wsd_max_size: %d\n",fstruct->wsd_size);

        //fstruct->wsdFiles = fileArray;
        fileArray[wsdcount].llHead = head;
        //fstruct->wsdFiles = fileArray;
        fileArray[wsdcount].fileName = malloc(sizeof(char) * (strlen(file_name)+1));
        
        //printf("TESTING: %s\n",file_name);
        strcpy(fileArray[wsdcount].fileName,file_name);
        fileArray[wsdcount].word_count = totalWords;
        Node *freqPtr = head;
        float temp_freq =0;
        while(freqPtr != NULL){
            //printf("here\n");
            //printf("word count");
            temp_freq = freqPtr->freq;//gets word count
            temp_freq /= totalWords;//divides word count by total # of words
            freqPtr->freq = temp_freq; 
            
            freqPtr = freqPtr->next;
        }
        //displayList(head);

        
        
        //freeList(head);
        close(fdr);
        free(buffer);
        //free(head);
        free(file_name);
        
    } 
    //fstruct->wsdFiles = fileArray;  
    
    return NULL;
}
void *dir_thread(void *q){
    queues_struct *qstruct = (queues_struct *) q; 
    queue_t *d_q = (queue_t *)qstruct->arg_d_q;
    queue_t *f_q = (queue_t *)qstruct->arg_f_q;
    char *suffixX = (char *)qstruct->suff;
    //printf("dir_thread: %s\n", d_q->data[0]);
    while(get_qCount(d_q)>0){
        
        
        
        
        //int dir_len = strlen(d_q->data[d_q->head]);
        int dir_len = length(d_q);


        char*dir_name = malloc(sizeof(char)* (dir_len +10));
        char*original_dir_name = malloc(sizeof(char)* (dir_len +10));
        //char* dir_name;
        
        dequeue(d_q,dir_name);


        
        strcpy(original_dir_name,dir_name);
        //printf("o_dir_name:%s \n",dir_name);
        
        
        
        
        struct dirent *dp;
        DIR *dfd;
        //error if cannot open directory?
        
        dfd = opendir(dir_name);
        
        
        
        while((dp= readdir(dfd))!=NULL){
            
            strcpy(dir_name,original_dir_name);
            if(strcmp(dp->d_name,".")==0||strcmp(dp->d_name,"..")==0){
                //printf("dont actually read \n");
                continue;
            }
            int sufSwitch = 1;
            if(dp->d_name[0] == '.'){
                sufSwitch = 0;
            }

            //printf("f in d : %s",dp->d_name);
            dir_name = realloc(dir_name,sizeof(char) * (dir_len + strlen(dp->d_name)+ 20));
            strcat(dir_name,"/");
            strcat(dir_name,dp->d_name);
            if(isdir(dir_name)){
                
               // printf("DIRECTORY FOUND, name created is: %s\n",dir_name);
                enqueue(d_q,dir_name);
                continue;
            }
            else{
                
                //if(dir_name[0] == '.'){
                   // sufSwitch = 0;
                //}
                
                int sufLen = strlen(suffixX)-1;
                int fileLen = strlen(dir_name)-1;
                //printf("sufLen: %d, fileLen: %d\n", sufLen, fileLen);
                if(sufLen > 1){
                    for (int j=sufLen; j>1; j--){
                        if (suffixX[j] != dir_name[fileLen] && fileLen >=0){
                            sufSwitch = 0;
                            break;
                        }
                        fileLen--;
                    }
                }

                
                if (sufSwitch == 1){
                    //printf("dir thread adds file to file q: %s\n",dir_name);
                    enqueue(f_q,dir_name);
                }
                
            }


        }

        closedir(dfd);

        free(original_dir_name);
        free(dir_name);
    }
    //close_q(d_q);
    return NULL;
}


int count_func(queue_t *s_q,int file_count,char* sufix_fix){
    
  
    char *suffixX = sufix_fix;
    //printf("dir_thread: %s\n", d_q->data[0]);
    while(get_qCount(s_q)>0){
        
        
        
        
        //int dir_len = strlen(d_q->data[d_q->head]);
        int dir_len = length(s_q);


        char*dir_name = malloc(sizeof(char)* (dir_len +10));
        char*original_dir_name = malloc(sizeof(char)* (dir_len +10));
        //char* dir_name;
        
        dequeue(s_q,dir_name);


        
        strcpy(original_dir_name,dir_name);
        //printf("o_dir_name:%s \n",dir_name);
        
        
        
        
        struct dirent *dp;
        DIR *dfd;
        //error if cannot open directory?
        
        dfd = opendir(dir_name);
        
        
        
        while((dp= readdir(dfd))!=NULL){
            
            strcpy(dir_name,original_dir_name);
            if(strcmp(dp->d_name,".")==0||strcmp(dp->d_name,"..")==0){
                //printf("dont actually read \n");
                continue;
            }

            //printf("f in d : %s",dp->d_name);
            dir_name = realloc(dir_name,sizeof(char) * (dir_len + strlen(dp->d_name)+ 20));
            strcat(dir_name,"/");
            strcat(dir_name,dp->d_name);
            if(isdir(dir_name)){
                
               // printf("DIRECTORY FOUND, name created is: %s\n",dir_name);
                enqueue(s_q,dir_name);
                continue;
            }
            else{
                int sufSwitch = 1;
                if(dir_name[0] == '.'){
                    sufSwitch = 0;
                }
                
                int sufLen = strlen(suffixX)-1;
                int fileLen = strlen(dir_name)-1;
                //printf("sufLen: %d, fileLen: %d\n", sufLen, fileLen);
                if(sufLen > 1){
                    for (int j=sufLen; j>1; j--){
                        if (suffixX[j] != dir_name[fileLen] && fileLen >=0){
                            sufSwitch = 0;
                            break;
                        }
                        fileLen--;
                    }
                }

                
                if (sufSwitch == 1){
                    //printf("dir thread adds file to file q: %s\n",dir_name);
                    file_count++;
                }
                
            }


        }

        closedir(dfd);

        free(original_dir_name);
        free(dir_name);
    }
    //close_q(d_q);
    return file_count;
}

void* analysis_thread(void * jsd_struct){
    analysis_args *a_input = (analysis_args *) jsd_struct;

    JSD *mathArray = (JSD *) a_input->jsdArrayHead;
    
    FileNode *info = (FileNode *) a_input->wfdArrayHead;
    
    char *ffile_name;
    char *sfile_name;

    
    Node* head1;
    Node* head2;
    Node* ptr1;
    Node* ptr2;
    
    char* word1;
    char* word2;
    int index1;
    int index2;
    
    
    for(int i = a_input->lo;i<a_input->hi;i++){
        Node *average = malloc(sizeof(Node));
        Node *aptr;
        average->word = NULL;
        average->next = NULL;
        average->freq = 0;
        int cmp= 0;
        double avgfreq= 0;
        Node* left = NULL;
        ffile_name = mathArray[i].file1;
        sfile_name = mathArray[i].file2;
        for(int z = 0;z<a_input->wsdLength;z++){//finds the indecies of the two files we are comparing in WFD array 
            if(strcmp(info[z].fileName,ffile_name)==0){
               // printf("index found");
                index1 = z;
            }
            if(strcmp(info[z].fileName,sfile_name)==0){
               // printf("index found");
                index2 = z;
            }
        }
        head1 = info[index1].llHead;
        head2 = info[index2].llHead;
        ptr1= head1;
        ptr2= head2;
        aptr = average;
        int sumWords = info[index1].word_count + info[index2].word_count;
        while(ptr1!=NULL&&ptr2!=NULL){
            word1 = ptr1->word;
            word2 = ptr2->word;
            cmp = strcmp(word1,word2);
            if(cmp == 0){//words are equal
                avgfreq = (ptr1->freq + ptr2->freq)/2;
                aptr->word = word1;
             

                ptr1 = ptr1->next;
                ptr2 = ptr2->next;

            }
            
            
            else{//words are not equal
                if (cmp<0){//word1 is alphabetically first
                    //compute mean frequency for word1
                    aptr->word = word1;
                    avgfreq = (ptr1->freq) / 2;
                    ptr1 = ptr1->next;
                }
                else{//word2 is alphabetically first
                    //compute mean frequency for word2
                    aptr->word = word2;
                    avgfreq = (ptr2->freq) / 2;
                    ptr2 = ptr2->next;
                }
            }

            aptr->freq = avgfreq;
            aptr->next = malloc(sizeof(Node));
            aptr = aptr->next;
            aptr->word = NULL;
            aptr->next = NULL;
            aptr->freq = 0;


        }
        if(ptr1==NULL&&ptr2!=NULL){
            left = ptr2;
            
        }
        if(ptr1!=NULL&&ptr2==NULL){
            left = ptr1;
            
        }
        if(left!=NULL){
            while(left!=NULL){
                aptr->word = left->word;
                avgfreq = (left->freq) / 2;
                
                aptr->freq = avgfreq;
                aptr->next = malloc(sizeof(Node));
                aptr = aptr->next;
                aptr->word = NULL;
                aptr->next = NULL;
                aptr->freq = 0;
                left = left->next;
            }
        }

        double kld1 = 0;//KLD for file1
        double kld2 = 0;//KLD for file2
        double inside_log = 0;
        double actual_log = 0;
        double final_log = 0;
        //increment file1/file2 ptr by 1, iterate through mean list until we find matching word
        //compute KLD for word and add it to total
        //do this once for both files
        ptr1= head1;
        ptr2= head2;
        aptr = average;
        while(ptr1!=NULL){//compute KLD for file1
            word1 = ptr1->word;
            while(strcmp(word1,aptr->word)!=0){//while words are not equal
                aptr = aptr->next;
            }
            inside_log = ptr1->freq/aptr->freq;
            actual_log = log10(inside_log) / log10(2);
            final_log = ptr1->freq * actual_log;
            kld1 += final_log;
            ptr1 = ptr1->next;
        }

        aptr = average;//resets aptr to head
        while(ptr2!=NULL){//compute KLD for file2
            word2 = ptr2->word;
            while(strcmp(word2,aptr->word)!=0){//while words are not equal
                aptr = aptr->next;
            }
            inside_log = ptr2->freq/aptr->freq;
            actual_log = log10(inside_log) / log10(2);
            final_log = ptr2->freq * actual_log;
            kld2 += final_log;
            ptr2 = ptr2->next;
        }
    
        double final_jsd = 0;
        final_jsd = sqrt((.5 * kld1) + (.5 *kld2));
        mathArray[i].cwc = sumWords;
        mathArray[i].jsdValue = final_jsd;
        //printf("JSD: %f",final_jsd);
        //printf("ffile: %s sfile: %s\n",ffile_name,sfile_name);
        /* deref head_ref to get the real head */
        Node* current = average;
        Node* avgNext = NULL;
 
        while (current != NULL)
        {
            avgNext = current->next;
            free(current);
            current = avgNext;
        }
    }
    
    

    
    return NULL;
}

int main(int argc, char **argv){

    //printf("OQAWDJMIQOWD");
    queue_t size_q;
    queue_t dir_q;
    queue_t file_q;
    init(&size_q);
    init(&dir_q);
    init(&file_q);
    int wsdSizeFinal = 0;

    queues_struct queues;
    queues.arg_d_q = &dir_q;
    queues.arg_f_q = &file_q;
    
    int isSuffix = 0;
    int dN = 1; //directory threads
    int fN = 1; //file threads
    int aN = 1; //analysis threads
    char *suffix = NULL; //file name suffix to look for
    //pthread_t thread_id;//id for threads (idk yet)
    
    /**
    int fSize = 50;//number of filesmak
   
    FileNode *WSD = (FileNode *)malloc(sizeof(FileNode) * fSize);//initializes array of linked lists w/ file names
    
    file_args file_thread_input;
    file_thread_input.file_arg_q = &file_q;
    file_thread_input.dir_arg_q = &dir_q;
    file_thread_input.wsdFiles = WSD;
    
    file_thread_input.wsd_size = fSize;
    //printf("file_thread_input.wsd_siz: %d\n",file_thread_input.wsd_size);
    file_thread_input.wsdCounter = 0;
    finit(&file_thread_input);
    **/
    //WSD = (FileNode *)realloc(WSD, sizeof(FileNode) * yex); 
    //file_thread_input.wsdFiles = WSD;
    
    //Checking parameters
    for(int i = 1;i<argc;i++){
        
        
        //if parameter
        //printf("i is: %d\n", i);
        if(argv[i][0]== '-'){
            if (argv[i][1] == 'd' || argv[i][1] == 'f' || argv[i][1] == 'a'){
                int total = 0;
                
            
                for (int j=2; j<strlen(argv[i]); j++){
                    int power = pow(10, strlen(argv[i])-j-1);
                    //printf("power is: %d\n", power);
                    if (isdigit(argv[i][j]) == 0){
                        //free(WSD);
                        qclose(&dir_q);
                        qclose(&file_q);
                        perror("invalid parameters");
                        return EXIT_FAILURE;
                    }
                     
                    //printf("digit: %c\n",argv[i][j]);
                    int tempNum = argv[i][j]- '0';
                    //printf("tempNUm is: %d\n", tempNum);
                    total += (tempNum * power);
                    //power = power / 10;
                }
                if (total <= 0){
                    qclose(&dir_q);
                    qclose(&file_q);
                    //free(WSD);
                    
                    perror("invalid parameters");
                    return EXIT_FAILURE;
                } 
                else if (argv[i][1] == 'd') dN = total;
                else if (argv[i][1] == 'f') fN = total;
                else aN = total;
            }
            else if (argv[i][1] == 's'){
                isSuffix = 1;
                suffix = malloc(sizeof(char) * (strlen(argv[i])+1));
                //memcpy(&suffix, &argv[i][2], strlen(argv[i]));
                strcpy(suffix, argv[i]);
            }
            else{
                //free(WSD);
                qclose(&dir_q);
                qclose(&file_q);
                printf("Error reading parameters");
                return EXIT_FAILURE;
            }

            continue;
        }
        //queue
        //it is a directory
        if(isdir(argv[i])){
            enqueue(&size_q,argv[i]);
            enqueue(&dir_q,argv[i]);
            
            
        }
        //not a directory
        else{
            FILE* fp = fopen(argv[i], "r");
            if(fp != NULL){
                //add to file to file queue
                wsdSizeFinal++;
                enqueue(&file_q,argv[i]);
                fclose(fp);
            }
            
            
        }
        
    }
    if (isSuffix == 0){
        suffix = malloc(sizeof(char) * 5);
        strcpy(suffix, ".txt");
    }

    queues.suff = suffix;




    int c = count_func(&size_q, wsdSizeFinal, suffix);
    //printf("file count is: %d\n", c);

    FileNode *WSD = (FileNode *)malloc(sizeof(FileNode) * c);//initializes array of linked lists w/ file names
    
    file_args file_thread_input;
    file_thread_input.file_arg_q = &file_q;
    file_thread_input.dir_arg_q = &dir_q;
    file_thread_input.wsdFiles = WSD;
    
    file_thread_input.wsd_size = c;
    //printf("file_thread_input.wsd_siz: %d\n",file_thread_input.wsd_size);
    file_thread_input.wsdCounter = 0;
    finit(&file_thread_input);



    
    /**
    for (int i=0; i<dir_q.count; i++){
        printf("Directory is: %s\n", dir_q.data[i]);
        
        pthread_create(&thread_id,NULL,dir_thread,&dir_q);
        pthread_join(thread_id, NULL);
    }
    
    while(dir_q.count>0){
        pthread_create(&thread_id,NULL,dir_thread,&queues);
        pthread_join(thread_id, NULL);
    }
    **/
    //unsigned int ids[dN];
    pthread_t tids[dN];
    pthread_t fids[fN];
    for(int i =0;i<dN;i++){
        pthread_create(&tids[i],NULL,dir_thread,&queues);
       
    }
    file_thread_input.id = &tids[0];
    file_thread_input.array_id = fids;
    for(int i =0;i<fN;i++){
        pthread_create(&fids[i],NULL,file_thread,&file_thread_input);
       
    }




    for(int i =1;i<dN;i++){
        
        pthread_join(tids[i], NULL);
    }
    for(int i =0;i<fN;i++){
        
        pthread_join(fids[i], NULL);
    }

    
    /**
    while(file_q.count>0){
        //printf("File is: %s\n", file_q.data[i]);
        
        pthread_create(&thread_id,NULL,file_thread,&file_thread_input);
        pthread_join(thread_id, NULL);
    }
    **/

    if (file_thread_input.wsdCounter < 2){
        perror("Less than two files found, exiting\n");
        for(int i=0; i<file_thread_input.wsdCounter; i++){
        FileNode *nHead = &WSD[i];
        freeList(nHead->llHead);
        free(nHead->fileName);
        }
        free(WSD);
        free(suffix);
        qclose(&dir_q);
        qclose(&file_q);
        return EXIT_FAILURE;
    }
    
    //printf("dN is: %d\n", dN);
    //printf("fN is: %d\n", fN);
    //printf("aN is: %d\n", aN);
    //printf("sN is: %s\n", suffix);
    //temp
    
    //char* printme = malloc(sizeof(char) * 100);
    //int temp_count = getwsdCounter(&file_thread_input);
    //printf("temp count: %d\n",temp_count);
    //WSD = file_thread_input.wsdFiles;
    //FileNode *nHead;
    //for(int i=0; i<temp_count; i++){
       // printf("i count: %d\n",i);
        //FileNode *nHead = &WSD[i];
      //  printf("\n\nFile Name: %s\n",nHead->fileName);
        //displayList(nHead->llHead);
       // printf("total words: %d\n",nHead->word_count);
    //}
    
    
    int n = file_thread_input.wsdCounter;
    int jsdSize = (n * (n - 1)) / 2;
    JSD *jsdArray = malloc(sizeof(JSD) * jsdSize);
    //printf("jsdSize: %d, n: %d",jsdSize,n);

    

    FileNode *fHead;
    FileNode *sHead;
    int jsdArray_counter = 0;
    for(int i=0; i<file_thread_input.wsdCounter; i++){
        fHead = &WSD[i];
        for(int z=i+1; z<file_thread_input.wsdCounter; z++){
            sHead = &WSD[z];

            //printf("\n\n %s||%s\n",fHead->fileName,sHead->fileName);
            jsdArray[jsdArray_counter].file1 =  fHead->fileName;
            jsdArray[jsdArray_counter].file2 =  sHead->fileName;
            jsdArray_counter++;
        }
    }

    
    if(aN > jsdSize){
        aN = jsdSize;
    }
    int block = jsdSize/aN;
   // printf("block:%d\n",block);
    int lower = 0;
    

    int upper = block;
    int pekoraCounter = 1;
    /**
    for (int i=0; i<jsdArray_counter; i++){
        printf("%s || %s\n ", jsdArray[i].file1, jsdArray[i].file2);
        
        
    }   
    **/
    
    
    //analysis_args analysis_thread_input;
    //analysis_thread_input.wfdArrayHead = WSD;
    //analysis_thread_input.jsdArrayHead = jsdArray;
    //analysis_thread_input.lo = lower;
    //analysis_thread_input.hi = jsdSize;
    //int i =0;
    //pthread_create(&aids[i],NULL,analysis_thread,&analysis_thread_input);
    pthread_t aids[aN];
    analysis_args *analysis_thread_input_array = malloc(sizeof(analysis_args) * aN);
    for (int i=0; i<aN; i++){
        analysis_thread_input_array[i].wfdArrayHead = WSD;
        analysis_thread_input_array[i].jsdArrayHead = jsdArray;
        analysis_thread_input_array[i].lo = lower;
        //int wsdLen = file_thread_input.wsdCounter;
        analysis_thread_input_array[i].wsdLength = n;
        if (i != aN-1){
            analysis_thread_input_array[i].hi = upper * pekoraCounter;
           // printf("%d to %d\n",lower,upper*pekoraCounter);
        }
        else{
            analysis_thread_input_array[i].hi = jsdSize;
           // printf("%d to %d\n",lower,jsdSize);
        }
        pekoraCounter++;
        lower += block;
    }
    
    for (int i=0; i<aN; i++){
        
        //printf("%s || %s ||||| ", jsdArray[i].file1, jsdArray[i].file2);
        //analysis_args analysis_thread_input;
        //analysis_thread_input.wfdArrayHead = WSD;
        //analysis_thread_input.jsdArrayHead = jsdArray;
        //analysis_thread_input.lo = lower;
        
        
        pthread_create(&aids[i],NULL,analysis_thread,&analysis_thread_input_array[i]);

        
        
    }
    for(int i =0;i<aN;i++){
        
        pthread_join(aids[i], NULL);
    }
    
   
    
   // printf("jsdSize: %d\n", jsdSize);
    qsort(jsdArray,jsdSize,sizeof(JSD),comp);

    for(int i=0; i<jsdSize; i++){
        printf("%f %s %s\n", jsdArray[i].jsdValue, jsdArray[i].file1, jsdArray[i].file2);
    }

    
   
    for(int i=0; i<file_thread_input.wsdCounter; i++){
        FileNode *nHead = &WSD[i];
        freeList(nHead->llHead);
        free(nHead->fileName);
    }
    
    
    
    free(analysis_thread_input_array);
    free(WSD);
    free(jsdArray);
    free(suffix);
    qclose(&dir_q);
    qclose(&file_q);

    return EXIT_SUCCESS;
}