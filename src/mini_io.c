#include <fcntl.h>
#include "mini_lib.h"
#include <unistd.h>

/**
* @author: Junior Sedogbo
     * @date: 13/11/2022
     * @version: 1.0
Implémentation des fonctions de gestion des flux d'entrée et de sortie
*/
const int IOBUFFER_SIZE=2048;

OPEN_FILES* OPEN_FILES_LISTE=(void*)-1;

#ifdef DEBUG
    static void debug(char* prefixe){
        OPEN_FILES* actuel=OPEN_FILES_LISTE;
        int count=0;
        while (actuel!=(void*)-1)
        {
            count++;
            actuel=actuel->suivant;
        }
        mini_printf(prefixe);
        mini_printf(": ");
        char tmp[32];
        mini_printf(mini_itoa(count,tmp));
        mini_printf(" fichiers ouverts\n");
    }
#endif
MYFILE* mini_fopen(char* file, char mode){
    int fd;
    int converted_flag;

    switch (mode)
    {
    case 'a':
        converted_flag= O_APPEND|O_WRONLY;
        break;
    case 'r':
        converted_flag=O_RDONLY;
        break;
    case 'w':
        converted_flag= O_WRONLY;
        break;
    case 'b':
        converted_flag=O_RDWR;
        break;
    case 'c':
        converted_flag=O_WRONLY|O_CREAT;
        break;
    default:
        converted_flag=O_RDONLY;
        break;
    }
    if(converted_flag==(O_WRONLY|O_CREAT)){
        if((fd=open(file,converted_flag,S_IRUSR | S_IWUSR |S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))==-1){ 
            mini_printf("cannot open ");
            mini_perror(file);
            return (void*)-1;
        }
    }
    else{
        if((fd=open(file,converted_flag))==-1){ 
            mini_perror("Erreur d'ouverture du fichier");
            return (void*)-1;
        }
    }

    MYFILE* the_file= mini_calloc(sizeof(MYFILE),1);
    if (the_file!=(void*)-1){
        the_file->fd=fd;
        the_file->ind_read=-1;
        the_file->ind_write=-1;
    }
     //On ajoute le fichier à la liste des fichiers à flush
    OPEN_FILES* new_file=mini_calloc(sizeof(OPEN_FILES),1);
    if(new_file!=(void*)-1){
        new_file->cellule=the_file;
        new_file->suivant=(void*)-1;
        
        if(OPEN_FILES_LISTE==(void*)-1){
            OPEN_FILES_LISTE=mini_calloc(sizeof(OPEN_FILES),1);
            if(OPEN_FILES_LISTE!=(void*)-1){
                OPEN_FILES_LISTE=new_file;
            }
        }
        else{
            OPEN_FILES* fichier_actuel=OPEN_FILES_LISTE;
            while (fichier_actuel->suivant!=(void*)-1)
            {
                fichier_actuel=fichier_actuel->suivant;
            }
            fichier_actuel->suivant=new_file;     
        }
    }
    
    #ifdef DEBUG
        debug("open");
    #endif

    return the_file;
}  

int mini_fread(void* buffer, int size_element, int number_element, MYFILE* file){
    /*
    Lit IOBUFFER_SIZE elements avec read et les stocke dans buffer_read.
    Les éléments de buffer_read sont recopiés dans buffer
    Un autre read est déclenché que si ind_read est arrivé à la fin de buffer_read
    Renvoie le nombre de bytes lus
    */
   //On caste les pointeurs void en char* pour pouvoir lire ou écrire byte par byte 
   char* char_buffer_read;
   char* char_buffer=(char*)buffer;

   if(file->ind_read ==-1){
        //1ere lecture dans le fichier file
        if((file->buffer_read=mini_calloc(size_element,IOBUFFER_SIZE))==(void*)-1){
            mini_perror("Erreur allocation mémoire du buffer read");
            return -1;
        }
        file->ind_read=0;
        char_buffer_read=(char*)file->buffer_read;

        int count_bytes=read(file->fd,file->buffer_read,IOBUFFER_SIZE*size_element);
        if(count_bytes==-1){
            mini_perror("Erreur de lecture du fichier");
            return -1;
        }
        
   }
        
    int i=0;   
    char_buffer_read=(char*)file->buffer_read;
    
    //Si size_element vaut 1 je vais supposer qu'on veut lire des chaines de caractère; et donc que le fichier contient du texte.
    //Ainsi quand buffer_read[file->ind_read] vaut \0, j'ai attend la fin du fichier
    if(size_element==1){
        while(i<number_element*size_element && char_buffer_read[file->ind_read]!='\0'){
            if(file->ind_read==IOBUFFER_SIZE*size_element){
                //On est arrivé à la fin du buffer read, on le recharge avec un read
                int count_bytes=read(file->fd,file->buffer_read,IOBUFFER_SIZE*size_element);
                if(count_bytes==-1){
                    mini_perror("Erreur de lecture du fichier");
                    return -1;
                }
                file->ind_read=0; 
            }

            char_buffer[i]=char_buffer_read[file->ind_read];
            file->ind_read++;
            i++;
        }
        char_buffer[i]='\0';
    }
    //Pour les autres types de données, on lit la taille de bytes demandés.
    //Je me suis rendu compte que la décomposition d'un int en bytes contenait des \0 et que ça ne signifie pas la fin du fichier.
    else{
        while(i<number_element*size_element){
            if(file->ind_read==IOBUFFER_SIZE*size_element){
                //On est arrivé à la fin du buffer read, on le recharge avec un read
                int count_bytes=read(file->fd,file->buffer_read,IOBUFFER_SIZE*size_element);
                if(count_bytes==-1){
                    mini_perror("Erreur de lecture du fichier");
                    return -1;
                }
                file->ind_read=0; 
            }

            char_buffer[i]=char_buffer_read[file->ind_read];
            file->ind_read++;
            i++;
        }
    }
    return i;

}

int mini_freadline(char* buffer, MYFILE* file){

    /*
    Cette fonction lit une ligne à partir de la position du curseur dans le fichier file.
    Le fichier file doit contenir des chaines de caractère.
    Pour cela elle lit byte par byte le fichier jusqu'à tomber sur un \n ou un \0
    La fonction renvoie le nombre de bytes écrits.
    */
    if(file->ind_read ==-1){
        //1ere lecture dans le fichier file
        if((file->buffer_read=mini_calloc(sizeof(char),IOBUFFER_SIZE))==(void*)-1){
            mini_perror("Erreur allocation mémoire du buffer read");
            return -1;
        }
        file->ind_read=0;
        
        if(read(file->fd,file->buffer_read,IOBUFFER_SIZE*sizeof(char))==-1){
            mini_perror("Erreur de lecture du fichier");
            return -1;
        }    
   }

    int i=0;
    char* char_buffer_read=(char*)file->buffer_read;
    char* char_buffer=(char*)buffer;

        while(char_buffer_read[file->ind_read]!='\n'&& char_buffer_read[file->ind_read]!='\0'){
            if(file->ind_read==IOBUFFER_SIZE*sizeof(char)){
                //On est arrivé à la fin du buffer read, on le recharge avec un read
                if(read(file->fd,file->buffer_read,IOBUFFER_SIZE*sizeof(char))==-1){
                    mini_perror("Erreur de lecture du fichier");
                    return -1;
                }
                file->ind_read=0;    
            }

            char_buffer[i]=char_buffer_read[file->ind_read];
            file->ind_read+=sizeof(char);
            i+=sizeof(char);
        }
        if(char_buffer_read[file->ind_read]=='\n'){
            //On est arrivé à la fin d'une ligne, on ne copie pas le '\n',on le saute
            file->ind_read+=1;
        }
        char_buffer[i]='\0';
        return i;
}

int mini_fwrite(void* buffer, int size_element, int number_element, MYFILE* file){
    /*
    Recopie les éléments de buffer_write et met à jour ind_write
    UN write est déclenché que si buffer_write est plein
    Renvoie le nombre de caractères écrits ou -1
    */
    
    if(file->ind_write==-1){
        //1ere écriture dans le fichier file
        if((file->buffer_write=mini_calloc(size_element,IOBUFFER_SIZE))==(void*)-1)
            return -1;
        
        file->ind_write=0;   
    }
    //On caste ensuite les pointeurs void* vers des  pointeurs char* pour pouvoir accéder byte byte aux zones mémoires
    char* char_buffer=(char*)buffer;
    char* char_buffer_write=(char*)file->buffer_write;
    int i=0;

    //Si size_element vaut 1 je vais supposer qu'on veut écrire des chaines de caractère; et donc que le fichier contient du texte.
    //Ainsi je peux arrêter ma boucle si buffer[i] vaut \0 et qu'on n'a pas encore la taille de bytes demandés
    //Cela peut arriver si le buffer a une taille réelle plus petite que number_element*size_element
    if(size_element==1){
        while(i<number_element*size_element && char_buffer[i]!='\0'){
            if(file->ind_write==IOBUFFER_SIZE*size_element){
                //Le buffer write est rempli
                if(write(file->fd,file->buffer_write,IOBUFFER_SIZE*size_element)==-1){
                    mini_perror("Erreur d'ecriture dans le fichier");
                    return -1;      
                }
                file->ind_write=0;
                
            }   
            char_buffer_write[file->ind_write]=char_buffer[i];
            file->ind_write++;
            i++;
        }
    }
    else{
        while(i<number_element*size_element){
            if(file->ind_write==IOBUFFER_SIZE*size_element){
                //Le buffer write est rempli
                if(write(file->fd,file->buffer_write,IOBUFFER_SIZE*size_element)==-1){
                    mini_perror("Erreur d'ecriture dans le fichier");
                    return -1;      
                }
                file->ind_write=0;
                
            }   
            char_buffer_write[file->ind_write]=char_buffer[i];
            file->ind_write++;
            i++;
        }
    }
    return i;
}

int mini_fflush(MYFILE* file){
    /*
    Force l'écriture des données non écrites présentes dans buffer_write
    Renvoie -1 en cas d'erreur ou le nombre de bytes écrits
    */
    int char_number=0;
    if(file->ind_write!=0 && file->ind_write!=-1){
        char_number=write(file->fd,file->buffer_write,file->ind_write);
        if(char_number==-1)
            mini_perror("Flush non effectue");
    }
    return char_number;
}


int mini_fclose(MYFILE* file){
    /*
    Ferme le fichier: le flush le supprime de la liste
    et utilise l'appel système close pour le fermer 
    */
    if(mini_fflush(file)!=-1){
        OPEN_FILES* actuel= OPEN_FILES_LISTE;
        if(OPEN_FILES_LISTE!=(void*)-1){

            if(OPEN_FILES_LISTE->cellule==file)
                OPEN_FILES_LISTE=OPEN_FILES_LISTE->suivant;
            
            else{
                while(actuel->suivant!=(void*)-1){
                    if(actuel->suivant->cellule==file){
                        actuel->suivant=actuel->suivant->suivant;
                        break;
                    }
                    actuel=actuel->suivant;
                }
            }
            #ifdef DEBUG
               debug("close");
            #endif
        }
        else{
            mini_printf("Aucun fichier n a ete ouvert\n");
        }
        
        if(close(file->fd)==-1){
            mini_perror("Erreur de fermeture fichier");
            return -1;
        }
        return 0;
    }
    return -1;

}
int mini_fseek(MYFILE* file,int offset,int whence){
    /*
    Déplace le curseur d'un fichier ouvert
    Renvoie la nouvelle position
    */
    int position;
    if((position=(int)lseek(file->fd,offset,whence))==-1)
        mini_perror("Erreur de deplacement du curseur");
    return position;
}  
int mini_countlines(char* file){
    /*
    Cette fonction compte le nombre de lignes dans un fichier et le renvoie.
    Le fichier doit contenir des chaines de caractère
    Pour cela, elle incrémente un compteur à chaque lecture de ligne que je fais grâce à la fonction mini_freadline()
    */
    int count_caracters;
    int count_lines=0;
    char* buffer=mini_calloc(sizeof(char), IOBUFFER_SIZE);
    MYFILE* my_file=mini_fopen(file,'r');

    if(my_file!=(void*)-1 && buffer!=(void*)-1){
        count_caracters=mini_freadline(buffer,my_file);
        while(count_caracters!=-1 && count_caracters!=0){
            count_lines++;
            count_caracters=mini_freadline(buffer,my_file);
        }
        mini_free(buffer);
        mini_fclose(my_file);
        return count_lines;
    }
    return -1;
}

char mini_fgetc(MYFILE* file){
    /*
    Renvoie un caractère lu dans un fichier contenant des chaines de caractère, -1 en cas d'erreur
    */
    char tempo_buffer[1+1];
        char character;
        if(mini_fread(tempo_buffer,sizeof(char),1,file)!=-1){
            character=tempo_buffer[0];
            return character;   
        }
    return -1;
}

int mini_fputc(MYFILE* file, char c){
    /*
    Ecrit un caractère dans un fichier 
    renvoie -1 en cas d'erreur
    */
    char tempo_buffer[2];
    if(tempo_buffer!=(void*)-1){
        tempo_buffer[0]=c;
        tempo_buffer[1]='\0';
        if(mini_fwrite(tempo_buffer,sizeof(char),1,file)!=-1)
            return 0;
    }
    return -1;
}

