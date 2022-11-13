#include "mini_lib.h"
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

/*
Programme qui prend en paramètre un ou plusieurs fichiers
Le(s) crée s'il n'existe(nt) pas ou le(s) remet(s) à zéro s'il(s) existe(nt)
*/

int main(int argc, char** argv){
    if(argc>1){
        for(int i=1;i<argc;i++){
            char* file_name=argv[i];
            if(access(file_name,F_OK)==-1){
                //Fichier n'existe pas      
                int tempo;
                if((tempo=open(file_name,O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH))!=-1)
                    close(tempo);
                else
                    mini_perror("Erreur d'ouverture de fichier");       
            }   
            else{
                int tempo;
                if((tempo=open(file_name,O_TRUNC))!=-1)
                        close(tempo);
                else
                    mini_perror("Erreur d'ouverture du fichier");         
            }
        }   
    }
    mini_exit();
    
}