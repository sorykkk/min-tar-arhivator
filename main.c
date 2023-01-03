#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
//memcpy, memset, strcpy considered unsafe
//looks like now all the functions in C are considered unsafe


#define BLOCK_SIZE 512
#define MIN_BLOCK_COUNT 20

typedef struct{     
                        //offset
    char name[100];     //0
    char mode[8];       //100
    char uid[8];        //108
    char gid[8];        //116
    char size[12];      //124
    char mtime[12];     //136
    char chksum[8];     //148 6 cifre terminat prin null si space
    char typeflag;      //156
    char linkname[100]; //157
    char magic[6];      //257 
    char version[2];    //263
    char uname[32];     //265
    char gname[32];     //297
    char devmajor[8];   //329
    char devminor[8];   //337
    char prefix[155];   //345
    char pad[12];       //500
}header_t;

//============hardcoding=========================//
void hardcode_header(header_t *header)
{
    memcpy(header->mode, (char[]){0x30, 0x30, 0x30, 0x30, 0x36, 0x36, 0x34, 0x0}, 8);
    header->typeflag = '0';
    memcpy(header->magic, (char[]){0x75, 0x73, 0x74, 0x61, 0x72, 0x20}, 6);
    memcpy(header->version, (char[]){0x20, 0x0}, 2);
}
//==============================================//

//returns numbers of characters in file, can set it's octal value if argument provided
long file_character_size(FILE *f, char *octal)
{//ftell interpreteaza '\n' si '\r' ca 2 caractere in dependenta de sistemul de operare
 //deci ftell arata marimea reala a fisierului pe cand in .tar se indica marimea ca numar de caractere (aparent)

    char ch;
    long size=0;
    while((ch = fgetc(f)) != EOF)
        size++;

    if(octal != NULL)
        sprintf(octal, "%011lo", size);
    rewind(f);

    return size;
}

//returns and sets chksum field
unsigned int set_checksum(header_t *h)
{
    char block[512];
    unsigned int check = 0;

    memset(h->chksum, ' ', 8);
    memcpy(block, h, sizeof(header_t));
    for(int i = 0; i < BLOCK_SIZE; i++)
        check += (unsigned char)block[i];

    sprintf(h->chksum, "%06o", check);
    h->chksum[6] = 0;
    h->chksum[7] = ' ';
    return check;
}

//returns number of allocated blocks
int process_header(FILE *archive, FILE * file, char *file_name)
{
    header_t *file_header = NULL;

    if((file_header = (header_t*)calloc(1, sizeof(header_t))) == NULL)
    {
        perror(NULL);
        exit(-1);
    }
    //hardcode some values
    hardcode_header(file_header);

    struct stat f_stat;
    if(stat(file_name, &f_stat) == -1)
    {
        perror(NULL);
        exit(-1);
    }
    //getting file name 
    strcpy(file_header->name, file_name);
    //getting file size
    file_character_size(file, file_header->size);
    
    //getting uid and gid
    sprintf(file_header->uid, "%07o", f_stat.st_uid);
    sprintf(file_header->gid, "%07o", f_stat.st_gid);

    //getting last modificated time
    sprintf(file_header->mtime, "%07lo", f_stat.st_mtime);

    //getting user name and group name
    //assuming files are created by the user, if not then it's sad :(
    strcpy(file_header->uname, getlogin());
    strcpy(file_header->gname, getlogin());
    //getting chksum filed
    set_checksum(file_header);
    
    //writting it all in the archive file
    fwrite(file_header, sizeof(header_t), 1, archive);
    free(file_header);
    return 1;
}

//returns number of allocated blocks
int process_file(FILE *archive, FILE *file)
{
    int blocks_count = 0;
    long size = file_character_size(file, NULL);//possible memory leak
    char file_buff[512];
    
    for(; size > 0; size -= BLOCK_SIZE)
    {
        memset(file_buff, 0, BLOCK_SIZE);
        fread(file_buff, sizeof(char), BLOCK_SIZE, file);
        fwrite(file_buff, sizeof(char), BLOCK_SIZE, archive);
        blocks_count++;
    }
    return blocks_count;
}

//return total number of allocated blocks
int process_archive(FILE *archive, FILE *file, char *file_name)
{
    int header_block = process_header(archive, file, file_name);
    int file_block = process_file(archive, file);

    return (header_block+file_block);
}

//returns how many files were successfuly archived, if error occured it either exit program or returns -1
//doesn't perform checking if this is a tar archive, because why would i?
int create_archive(char *archive_name, int argc, char **argv)
{
    int doc_count=0;      //document counter
    int block_count = 0;  //allocated blocks counter
    FILE *archive = NULL; //final archive file

    if((archive = fopen(archive_name, "wb")) == NULL)
    {
        fprintf(stderr, "\n# error on creating archive \'%s\' #\n", archive_name);
        exit(-1);
    }

    //I create and operate with only one content file and header at a time for every document
    //more intuitive, less space
    for(doc_count = 3; doc_count < argc; doc_count++)
    {
        FILE *file = NULL;

        char *file_name = NULL;
        if((file_name = (char*)malloc(sizeof(strlen(argv[doc_count]))+1)) == NULL)
        {
            perror(NULL);
            return -1;
        }
        strcpy(file_name, argv[doc_count]);


        if((file = fopen(file_name, "r"))==NULL)
        {
            fprintf(stderr, "\n# error on opening file \'%s\' #\n", file_name);
            exit(-1);
        }
        block_count += process_archive(archive, file, file_name);
        
        free(file_name);
        fclose(file);
    }

    //closing 2 512-bytes null blocks
    char *buff = NULL;

    if((buff = calloc(2*BLOCK_SIZE, sizeof(char))) == NULL)
    {
        perror(NULL);
        return -1;
    }

    fwrite(buff, sizeof(char), 2*BLOCK_SIZE, archive);
    free(buff);

    //completing minimum requirment
    //if i'd use realloc then i'd need aux pointer to not cause memory leaks
    block_count += 2;
    if(block_count < MIN_BLOCK_COUNT)
    {
        if((buff = calloc((MIN_BLOCK_COUNT-block_count)*BLOCK_SIZE, sizeof(char))) == NULL)
        {
            perror(NULL);
            return -1;
        }
        fwrite(buff, sizeof(char), (MIN_BLOCK_COUNT-block_count)*BLOCK_SIZE, archive);
        free(buff);
    }

    fclose(archive);

    return doc_count-3;
}

//returns number of extracted document, otherwise -1;
int extract_archive(char *archive_name)
{
    int docs_count = 0;
    FILE *archive, *file;
    if((archive = fopen(archive_name, "r")) == NULL)
    {
        fprintf(stderr, "\n# error on opening archive \'%s\' #\n", archive_name);
        exit(-1);
    }

    char buff[BLOCK_SIZE];
    
    header_t header;

    while(1)
    {
        fread(buff, sizeof(char), BLOCK_SIZE, archive);
        //checking for ending block
        if(buff[0] == 0 && !memcmp(buff, buff+1, BLOCK_SIZE-1))
            break;

        memcpy(&header, buff, BLOCK_SIZE);

        if(chown(header.name, strtol(header.uid, NULL, 8), strtol(header.gid, NULL, 8)) != 0)
        {
            perror(NULL);
            return -1;
        }

        // setting original file mtime to the created file
        //can't set it properly
        // struct utimbuf ubuf;    
        // ubuf.modtime = strtol(header.mtime, NULL, 8);
        // utime(header.name, &ubuf);

        if(docs_count == 0)
            printf("\nDocuments successfuly extracted: \n");

        if((file = fopen(header.name, "w")) == NULL)
        {
            fprintf(stderr, "\n# error on opening file \'%s\' #\n", header.name);
            exit(-1);
        }

        for(long i = 0; i < (int)ceil((double)strtol(header.size, NULL, 8)/BLOCK_SIZE); i++)
        {
            fread(buff, sizeof(char), BLOCK_SIZE, archive);
            fwrite(buff, sizeof(char), strlen(buff), file);
        }
        // fwrite('\0', sizeof(char), 1, file);
        printf("%s\n", header.name);
        docs_count++;

        fclose(file);
    }
    printf("\n");

    fclose(archive);
    return docs_count;
}

int main(int argc, char **argv)
{
    char *archive_name = NULL;

    if(argc < 2)
    {
        fprintf(stderr, "\n### usage error. Few arguments to the command line. Should be called with \'./mytar [OPERATION TYPE] [FILE...]\' ###\n");
        exit(-1);
    }

    if((archive_name = (char*)malloc(sizeof(strlen(argv[2]))+1)) == NULL)
    {
        perror(NULL);
        exit(-1);
    }
    strcpy(archive_name, argv[2]);
    
    if(strcmp(argv[1], "-c") == 0)
    {
        if(create_archive(archive_name, argc, argv) != argc-3)
        {
            fprintf(stderr, "\n# error occured during archivation #\n");
            exit(-1);
        }
        printf("\nDocuments successfuly archived:\n");
        for(int i = 3; i < argc; i++)
            printf("%s\n", argv[i]);
        printf("\n");
    }
    else if(strcmp(argv[1], "-x") ==0 )
    {
        if(extract_archive(argv[2]) == -1)
        {
            fprintf(stderr, "\n# error occured during extraction of archived files #\n");
            exit(-1);
        }
    }
    else
    {
        fprintf(stderr, "\n### program should be used only with \'-c\' (create archive) or \'-x\' (extract archive) arguments ###\n");
    }

    free(archive_name);
    return 0;
}