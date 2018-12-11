#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_REC_SIZE 512
#define DELIM_STR "|"
#define DELIM '\n'
#define MAXKEYS 4
#define MINKEYS MAXKEYS/2
#define NIL -1
#define NO 0
#define YES 1

typedef struct caes{
    char id_i[5];
    char id_r[5];
    char nome[20];
    char sexo[2];
}caes;

typedef struct no{
    char id_i[5];
    char byteOffSet[7];
}no;
#define NOKEY (no){"-1","-1"}

typedef struct{
    short keycount;
    no key[MAXKEYS];
    long child[MAXKEYS+1];
}BTPAGE;
#define PAGESIZE sizeof(BTPAGE)

long root;
FILE* btfd;
FILE* infd;
int numRegs;

int readnome(FILE* fd, char* str){
    //char str[25];
    char c;
    int i = 0;
    str[0]= '\0';
    c = fgetc(fd);
    while (c!= EOF && c != DELIM){
        str[i++] = c;
        c = fgetc(fd);
    }
    str[i] = '\0';
    return i;
}

int getNumRegs(){
    FILE* arq;
    arq = fopen("individuos_num.txt", "r");
    fscanf( arq, "%d", &numRegs );
    fclose(arq);
    return numRegs;
}

int btopen(){
    btfd= fopen("btree.txt", "r+");
    return btfd!=NULL;
}

int btclose(){
    fclose(btfd);
}

long getRoot(){
    long root= 0;
    if(btfd != NULL){
        fseek(btfd, 0L, 0);
        if(!fread(&root, sizeof(long), 1, btfd)){
            printf("Erro: Incapaz de obter a raiz\n");
            exit(1);
        }
    }else{
        btopen();
        //fseek(btfd, 0L, 0);
        if(fread(&root, sizeof(long), 1, btfd) == 0){
            printf("Erro: Incapaz de obter a raiz\n");
            exit(1);
        }
        btclose();
    }
    root++;
    return root;
}

void putRoot(long raiz){
    //short r= 2*(raiz/2 + raiz%2);
    long r= raiz - 1;
    fseek(btfd, 0L, 0);
    if(btfd != NULL){
        if(!fwrite(&r, sizeof(long), 1, btfd))
            printf("Erro na escrita de Root\n");
    }else{
        btopen();
        fwrite(&r, sizeof(long), 1, btfd);
        btclose();
    }
}

long getPage(){
    long addr;
    fseek(btfd, 0L, 2) - 4L;
    addr= ftell(btfd);
    return (long)addr/PAGESIZE;
}

void pageInit(BTPAGE* p_page){
    int j;
    for(j= 0; j<MAXKEYS; j++){
        p_page->key[j]= NOKEY;
        p_page->child[j]= NIL;
    }
    p_page->child[MAXKEYS]= NIL;
}

int btread(long rrn, BTPAGE* page_ptr){
    long addr= ((long)rrn)*((long)PAGESIZE) + 4L;
    fseek(btfd, addr, 0);
    return fread(page_ptr, PAGESIZE, 1, btfd);
}

long btwrite(long rrn, BTPAGE* page_ptr){
    long addr= ((long)rrn * (long)PAGESIZE) + 4L;
    fseek(btfd, addr, 0);
    //short keycount= page_ptr.keycount;
    //short *child= page_ptr.child;
    //no key= page_ptr.key;
    //char *id= key.id_i;
    //char *offset= no.byteOffSet;
    //return fwrite()
    return fwrite(page_ptr, PAGESIZE, 1, btfd);
}

long createRoot(no key, long left, long right){
    BTPAGE page;
    long rrn= getPage();
    //printf("%d\n", rrn);
    pageInit(&page);
    page.key[0]= key;
    page.child[0]= left;
    page.child[1]= right;
    page.keycount= 1;
    btwrite(rrn, &page);
    putRoot(rrn);
    return rrn;
}

int readFieldNo(FILE* arq, no* ind){
    char str[20];
    char c;
    int i = 0;
    for(int j = 0; j < 2; j++){
        i = 0;
        str[0]= '\0';
        c = fgetc(arq);
        while (c!= EOF && c != DELIM){
            str[i++] = c;
            c = fgetc(arq);
        }
        str[i] = '\0';
        switch(j){
            case 0: strcpy(ind->id_i, str);
                    break;
            case 1: strcpy(ind->byteOffSet, str);
                    break;
        }
    }
    return i;
}



int readfield(FILE* fd, caes* ind){
    char str[20];
    char c;
    int i = 0;
    for(int j = 0; j < 4; j++){
        i = 0;
        str[0]= '\0';
        c = fgetc(fd);
        while (c!= EOF && c != DELIM){
            str[i++] = c;
            c = fgetc(fd);
        }
        str[i] = '\0';
        switch(j){
            case 0: strcpy(ind->id_i, str);
                    break;
            case 1: strcpy(ind->id_r, str);
                    break;
            case 2: strcpy(ind->nome, str);
                    break;
            case 3: strcpy(ind->sexo, str);
                    break;
        }
    }
    return i;
}


short createTree(){
    no key;
    char str[25];
    btfd= fopen("btree.txt", "w");
    fclose(btfd);
    btopen();
    FILE* ind;
    if ((ind = fopen("indices.txt", "r")) == NULL) {
        printf("Erro na leitura do arquivo Individuos--- programa abortado\n");
        exit(1);
    }
    readFieldNo(ind,&key);
    fclose(ind);
    return createRoot(key,NIL,NIL);
}


int searchNode(no key, BTPAGE* p_page, long* pos){
    int i;
    //int comp= atoi(p_page->key[0].id_i);
    for(i= 0; i<p_page->keycount && atoi(key.id_i) > atoi(p_page->key[i].id_i); i++){
        ;
    }
    *pos= i;
    if(*pos < p_page->keycount && atoi(key.id_i) == atoi(p_page->key[*pos].id_i))
        return YES;
    return NO;
}

void ins_in_page(no key, long r_child, BTPAGE* p_page){
    int i;
    for(i= p_page->keycount; atoi(key.id_i) < atoi(p_page->key[i-1].id_i) && i>0; i--){
        p_page->key[i]= p_page->key[i-1];
        p_page->child[i+1]= p_page->child[i];
    }
    p_page->keycount++;
    p_page->key[i]= key;
    p_page->child[i+1]= r_child;
}

void split(no key, long r_child, BTPAGE* p_oldpage, no* promo_key, long* promo_r_child, BTPAGE* p_newpage){
    int i;
    long mid;
    no workkeys[MAXKEYS+1];
    long workch[MAXKEYS+2];
    for(i= 0; i<MAXKEYS; i++){
        workkeys[i]= p_oldpage->key[i];
        workch[i]= p_oldpage->child[i];
    }
    workch[i]= p_oldpage->child[i];
    for(i= MAXKEYS; atoi(key.id_i) < atoi(workkeys[i-1].id_i) && i>0; i--){
        workkeys[i]= workkeys[i-1];
        workch[i+1]= workch[i];
    }
    workkeys[i]= key;
    workch[i+1]= r_child;
    *promo_r_child= getPage();
    pageInit(p_newpage);
    for(i= 0; i<MINKEYS; i++){
        p_oldpage->key[i]= workkeys[i];
        p_oldpage->child[i]= workch[i];
        p_newpage->key[i]= workkeys[i+1+MINKEYS];
        p_newpage->child[i]= workch[i+1+MINKEYS];
        p_oldpage->key[i+MINKEYS]= NOKEY;
        p_oldpage->child[i+1+MINKEYS]= NIL;
    }
    p_oldpage->child[MINKEYS]= workch[MINKEYS];
    p_newpage->child[MINKEYS]= workch[i+1+MINKEYS];
    p_newpage->keycount= MAXKEYS - MINKEYS;
    p_oldpage->keycount= MINKEYS;
    *promo_key= workkeys[MINKEYS];
}

int insert(long rrn, no key, long* promo_r_child, no* promo_key){
    BTPAGE page, newpage;
    int found,promoted;
    long pos, p_b_rrn;
    no p_b_key;
    if(rrn == NIL){
        *promo_key= key;
        *promo_r_child= NIL;
        return(YES);
    }
    btread(rrn, &page);
    if(strcmp(key.id_i, "") < 1)
        return NO;
    found= searchNode(key, &page, &pos);
    if(found){
        printf("Erro: tentativa de inserir chave duplicada: %s \n", key.id_i);
        return 0;
    }
    promoted= insert(page.child[pos], key, &p_b_rrn, &p_b_key);
    if(!promoted){
        return NO;
    }
    if(page.keycount < MAXKEYS){
        ins_in_page(p_b_key, p_b_rrn, &page);
        btwrite(rrn, &page);
        return NO;
    }else{
        split(p_b_key, p_b_rrn, &page, promo_key, promo_r_child, &newpage);
        btwrite(rrn, &page);
        btwrite(*promo_r_child, &newpage);
        return YES;
    }
}

void driver(){
    int promoted, i;
    long promoRrn;
    no promoKey, key;
    if(btopen())
        root= getRoot();
    else{
        root= createTree();
        FILE* ind;
        if ((ind = fopen("indices.txt", "r")) == NULL) {
            printf("Erro na leitura do arquivo Indices--- programa abortado\n");
            exit(1);
        }
        fscanf(ind, "%*[^\n]\n", NULL);
        fscanf(ind, "%*[^\n]\n", NULL);
        fseek(ind, sizeof(int)+PAGESIZE, 0);
        for(i= 0; i<numRegs; i++){
            readFieldNo(ind,&key);
            promoted= insert(root, key, &promoRrn, &promoKey);
            if(promoted)
                root= createRoot(promoKey, root, promoRrn);
        }
    }
    //printArvore();
    //putRoot(root);
    btclose();
}

void printPagina(long rrn){
    BTPAGE pagina, filho;
    btread(rrn, &pagina);
    printf("RRN: %d\n", rrn);
    if(pagina.keycount > 0){
        printf("Chaves: ");
        for(int i= 0; i<pagina.keycount; i++){
            printf("%s |", pagina.key[i].id_i);
        }
        printf("\nByte-offsets: ");
        for(int i= 0; i<pagina.keycount+1; i++){
            printf("%s |", pagina.key[i].byteOffSet);
        }
        printf("\nPonteiros: ");
        for(int i= 0; i<pagina.keycount+1; i++){
            printf("%d |", pagina.child[i]);
        }
    }
    printf("\n-+----------------------------+-\n");
    for(int i= 0; i<pagina.keycount+1; i++)
        if(pagina.child[i] > -1){
            //btread(pagina.child[i], &filho);
            printPagina(pagina.child[i]);
        }
        else return;
}

void printArvore(){
    if(btopen()){
        //root= getRoot();
        printf("---+- Página Raiz -+---\n");
        printPagina(root);
        btclose();
    }else{
        printf("Não existe árvore!\n");
        return;
    }
}

int vetor_aux[100];


typedef struct racas{
    int id;
    char raca[30];
}racas;



typedef struct indices{
    int id;
    long offset;
}indices;

typedef struct invertida{
    int id;
    int raca;
    long offset;
    long prox;
}invertida;

typedef struct secundaria{
    int id;
    long indice;
}secundaria;

secundaria listaSecundaria[100];
invertida listaInvertida[500];
int tamSec= 0, tamInvert= 0;
indices listaIndices[500];
int tamIndices= 0;
racas listaRacas[18];
racas listaNomeRacas[100];
int tamNomeRacas= 0;


int getline2 (char *str, int tam){
	int i = 0;
	fgets(str, tam, stdin);
	//Tira o enter do fim da string, se houver
	for (i = 0; str[i] != '\n' && str[i] != '\0'; i++);
	str[i] = '\0';
	//avanca o ponteiro do stdin para o final
	fseek(stdin, 0L, SEEK_END);
	return i;
}




void merge(indices arr[], int l, int m, int r){
    int i, j, k;
    int n1 = m - l + 1;
    int n2 =  r - m;
    indices L[n1], R[n2];
    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1+ j];
    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2){
        if (L[i].id <= R[j].id){
            arr[k] = L[i];
            i++;
        }else{
            arr[k] = R[j];
            j++;
        }
        k++;
    }
    while (i < n1){
        arr[k] = L[i];
        i++;
        k++;
    }
    while (j < n2){
        arr[k] = R[j];
        j++;
        k++;
    }
}

void mergeSort(indices arr[], int l, int r){
    if (l < r){
        int m = l+(r-l)/2;
        mergeSort(arr, l, m);
        mergeSort(arr, m+1, r);
        merge(arr, l, m, r);
    }
}

void leNomesRacas(char* nome){
    FILE* arq;
    if(!strlen(nome))
        nome="nomes-racas.txt";
    if ((arq = fopen(nome, "r")) == NULL) {
        printf("Erro na criacão do arquivo Racas: %s--- programa abortado\n", nome);
        exit(1);
    }
    char buff[40];
    while(1){
        readnome(arq, buff);
        if(!strlen(buff))
            break;
        listaNomeRacas[tamNomeRacas].id= atoi(strtok(buff, " "));
        strcpy(listaNomeRacas[tamNomeRacas].raca, strtok(NULL, "\n"));
        tamNomeRacas++;
    }
    fclose(arq);
}

char* getNomeRaca(int id){
    if(id<1 || id>tamNomeRacas)
        return "";
    return listaNomeRacas[id-1].raca;
}

int getIdRaca(char* nome){
    if(strlen(nome)){
        int len= strlen(nome);
        for(int j=0; j<tamNomeRacas; j++){
            if(!strcmp(listaNomeRacas[j].raca, nome)){
                return j+1;
            }
        }
    }
    return -1;
}

void criaIndices(FILE* base){
    /*
        Tamanho de cada registro: sizeof(int) + sizeof(long)
    */
    int tam, id= 0, id_max, continua, i= -1;
    char buff[40], *temp;
    long offset;
    fseek(base,0, SEEK_SET);
    int numRegs;
    //fread(&numRegs, sizeof(int), 1, base);
    //printf("%d\n", numRegs);
    while(1){
        i++;
        offset= ftell(base);
        continua = fread(&tam, sizeof(int), 1, base);
        if(!continua)
            break;
        fread(buff, sizeof(char), tam, base);
        buff[tam] = '\0';
        temp= strtok(buff, "|");
        id= atoi(temp);
        listaIndices[i].id= id;
        listaIndices[i].offset= offset;
        tamIndices= i;
    }
    //mergeSort(listaIndices, 0, i-1);
}

void gravaIndices(){
    FILE* ind;
    if ((ind = fopen("indices.txt", "w+")) == NULL) {
        printf("Erro na criação do arquivo Indices--- programa abortado\n");
        exit(1);
    }
    int id, num= numRegs;
    long offset;
    for(int h= 0; h< num; h++){
        id= listaIndices[h].id;
        offset= listaIndices[h].offset;
        fprintf(ind, "%d\n%d\n", id, offset);
    }
    fclose(ind);
}


void gravaInvertida(){
    FILE* ind;
    if ((ind = fopen("invertida.txt", "w+")) == NULL) {
        printf("Erro na criação do arquivo Invertida--- programa abortado\n");
        exit(1);
    }
    int id, raca, num= tamInvert;
    long offset, prox;
    fwrite(&num, sizeof(int), 1, ind);
    for(int i= 0; i<= num; i++){
        id= listaInvertida[i].id;
        raca= listaInvertida[i].raca;
        offset= listaInvertida[i].offset;
        prox= listaInvertida[i].prox;
        fprintf(ind,"%d",id);
        fprintf(ind,"|");
        fprintf(ind,"%d",raca);
        fprintf(ind,"|");
        fprintf(ind,"%d", offset);
        fprintf(ind,"|");
        fprintf(ind,"%d",prox);
        fprintf(ind,"|");
    }
}
 void gravaSecundaria(){
    FILE* ind;
    if ((ind = fopen("secundaria.txt", "w+")) == NULL) {
        printf("Erro na criação do arquivo Secundaria--- programa abortado\n");
        exit(1);
    }
    int id, num= tamNomeRacas;
    long indice;
    for(int i= 0; i<= num; i++){
        id= listaSecundaria[i].id;
        indice= listaSecundaria[i].indice;
        fprintf(ind,"%d",id);
        fprintf(ind,"|");
        fprintf(ind,"%d",indice);
        fprintf(ind,"|");
    }
}

void carregaInvertida(){
    FILE* invert;
    if ((invert = fopen("invertida.txt", "r")) == NULL) {
        printf("Erro na criação do arquivo Invertida--- programa abortado\n");
        exit(1);
    }
    int id,raca,num;
    fread(&num, sizeof(int), 1, invert);
    long offset, prox;
    for(int i= 0; i<num; i++){
        fread(&id, sizeof(int), 1, invert);
        fread(&raca, sizeof(int), 1, invert);
        fread(&offset, sizeof(long), 1, invert);
        fread(&prox, sizeof(long), 1, invert);
        listaInvertida[tamInvert].id= id;
        listaInvertida[tamInvert].raca= raca;
        listaInvertida[tamInvert].offset= offset;
        listaInvertida[tamInvert].prox= prox;
        tamIndices++;
    }
}
 void carregaSecundaria(){
    FILE* secundaria;
    if ((secundaria = fopen("secundaria.txt", "r")) == NULL) {
        printf("Erro na criação do arquivo Secundaria--- programa abortado\n");
        exit(1);
    }
    int id, num;
    fread(&num, sizeof(int), 1, secundaria);
    long indice;
    for(int i= 0; i<num; i++){
        fread(&id, sizeof(int), 1, secundaria);
        fread(&indice, sizeof(long), 1, secundaria);
        listaSecundaria[tamIndices].id= id;
        listaSecundaria[tamIndices].indice= indice;
        tamIndices++;
    }
}

void carregaIndices(){
    FILE* indices;
    if ((indices = fopen("indices.txt", "r")) == NULL) {
        printf("Erro na criação do arquivo Indices--- programa abortado\n");
        exit(1);
    }
    int id, num;
    fread(&num, sizeof(int), 1, indices);
    long offset;
    for(int i= 0; i<num; i++){
        fread(&id, sizeof(int), 1, indices);
        fread(&offset, sizeof(long), 1, indices);
        listaIndices[tamIndices].id= id;
        listaIndices[tamIndices].offset= offset;
        tamIndices++;
    }
}


void grava(){
    gravaIndices();
    gravaInvertida();
    gravaSecundaria();
}

void carrega(){
    carregaIndices();
    carregaInvertida();
}

long buscaOffsetDoIndiceNoArquivo(int id){
    FILE* indices;
    if ((indices = fopen("indices.txt", "r")) == NULL) {
        printf("Erro na criação do arquivo Indices--- programa abortado\n");
        exit(1);
    }
    fseek(indices, (id - 1)*(sizeof(int)+sizeof(long)), SEEK_SET);
    int id_lido;
    long offset;
    fread(&id_lido, sizeof(int), 1, indices);
    if(id != id_lido){
        printf("ERRO - O indivíduo com id#%d não foi encontrado", id);
        fclose(indices);
        exit(1);
    }
    fread(&offset, sizeof(long), 1, indices);
    fclose(indices);
    return offset;
}


int getIndice(int id) {
   int lowerBound = 0;
   int upperBound = tamIndices -1;
   int midPoint = -1;
   int comparisons = 0;
   int index = -1;

   while(lowerBound <= upperBound) {
      comparisons++;

      midPoint = lowerBound + (upperBound - lowerBound) / 2;
      if(listaIndices[midPoint].id == id) {
         index = midPoint;
         break;
      } else {
         if(listaIndices[midPoint].id < id) {
            lowerBound = midPoint + 1;
         }
         else {
            upperBound = midPoint -1;
         }
      }
   }
   return index;
}

long buscaOffsetDoIndice(int id){
    int index= getIndice(id);
    if(index>=0){
        return listaIndices[index].offset;
    }
    return -1;
}

struct caes buscaPorOffset(long offset){
    FILE* base;
    if ((base = fopen("base.txt", "r+")) == NULL) {
        printf("Erro na criação do arquivo Base--- programa abortado\n");
        exit(1);
    }
    char buff[40];
    int tam;
    fseek(base, offset, SEEK_SET);
    fread(&tam, sizeof(int), 1, base);
    //printf("Tam: %d\n", tam);
    fread(buff, sizeof(char), tam, base);
    //printf("Buff: %s\n", buff);
    buff[tam] = '\0';

    struct caes individuo;
    strcpy(individuo.id_i, strtok(buff, "|"));
    strcpy(individuo.id_r, strtok(NULL, "|"));
    strcpy(individuo.nome, strtok(NULL, "|"));
    strcpy(individuo.sexo, strtok(NULL, "|"));
    //Adicionar a raça depois que a busca por id_r estiver pronta
    printf("Encontrado individuo:\n\tId: %s\n\tRaca: %s\n\tNome: %s\n\tSexo: %s\n", individuo.id_i, getNomeRaca(atoi(individuo.id_r)), individuo.nome, individuo.sexo);
    fclose(base);
    return individuo;
}

struct caes buscaPorId(int id){
    printf("Offset: %d\n", buscaOffsetDoIndice(id));
    return buscaPorOffset(buscaOffsetDoIndice(id));
}


void buscaPorRaca(int id_r){
    int i = (int)listaSecundaria[id_r].indice;
    while(i > -1){
        int id = listaInvertida[i-1].id;
        buscaPorId(id);
        i = listaInvertida[i-1].prox;
    }
}

void buscaPorNomeDaRaca(char* nome){
    int id_r= getIdRaca(nome);
    if(id_r < 0)
        printf("Raça não encontrada");
    int i = (int)listaSecundaria[id_r].indice;
    while(i > -1){
        int id = listaInvertida[i-1].id;
        buscaPorId(id);
        i = listaInvertida[i-1].prox;
    }
}

void povoaArquivo(char* nome){
    caes individuo;
    FILE* input, *base;
    if(nome == ""){
        nome= "individuos_num.txt";
    }
    char s[200] = "";
    int tam;
    if ((input = fopen(nome, "r")) == NULL) {
        printf("Erro na criação do arquivo Individuo--- programa abortado\n");
        exit(1);
    }
    fscanf(input, "%*[^\n]\n", NULL);
    if ((base = fopen("base.txt", "w+")) == NULL) {
        printf("Erro na criação do arquivo Base--- programa abortado\n");
        exit(1);
    }
    fseek(base, 0, SEEK_SET);
    int fld_count = 0;
    while (readfield(input, &individuo) > 0){
          strcat(s, individuo.id_i);
          strcat(s, "|");
          strcat(s, individuo.id_r);
          strcat(s, "|");
          strcat(s, individuo.nome);
          strcat(s, "|");
          strcat(s, individuo.sexo);
          strcat(s, "|");
          tam = strlen(s);
          //printf("A\n");
          fwrite(&tam, sizeof(tam), 1, base);
          fwrite(s, sizeof(char), tam, base);
          strcpy(s, "");
    }
    fclose(input);
    fclose(base);
}

void monta_lista(){
    long offset=0;
    FILE* base;
    if ((base = fopen("base.txt", "r+")) == NULL) {
        printf("Erro na criação do arquivo Base--- programa abortado\n");
        exit(1);
    }
    struct caes individuo;
    char buff[40];
    int tam, cont=0;
    fseek(base, 0, SEEK_SET);
    int i=0, k=0, p=0;
    while(fread(&tam, sizeof(int), 1, base)){
        cont++;
        //fseek(base, offset,SEEK_CUR);
        //printf("Offset: %ld\n", offset);
        fread(buff, sizeof(char), tam, base);
        buff[tam] = '\0';
        strcpy(individuo.id_i, strtok(buff, "|"));
        strcpy(individuo.id_r, strtok(NULL, "|"));
        strcpy(individuo.nome, strtok(NULL, "|"));
        strcpy(individuo.sexo, strtok(NULL, "|"));

        listaInvertida[p].id = atoi(individuo.id_i);
        listaInvertida[p].raca = atoi(individuo.id_r);
        listaInvertida[p].offset = listaIndices[atoi(individuo.id_i)-1].offset+4;
        vetor_aux[atoi(individuo.id_r)]=listaInvertida[p].id;

        if(listaSecundaria[atoi(individuo.id_r)].id == 0){
            listaSecundaria[atoi(individuo.id_r)].id = atoi(individuo.id_r);
            listaSecundaria[atoi(individuo.id_r)].indice = p+1;
        }
        /*
        printf("i: %d\n", i);
        offset = buscaOffsetDoIndice(i+1);
        printf("Offset Novo: %ld\n");
        */
        p++;
    }
    //for(int v=1;v<=18;v++){
    //  printf("raca: %ld, %d\n",listaSecundaria[v].id,listaSecundaria[v].indice);
    //}
    int l = cont;
    while(l >= 0 ){
        if(listaInvertida[l].id == vetor_aux[listaInvertida[l].raca]){
            listaInvertida[l].prox = -1;
        }
        else if(listaInvertida[l].id < vetor_aux[listaInvertida[l].raca] ){
            listaInvertida[l].prox = vetor_aux[listaInvertida[l].raca];
            vetor_aux[listaInvertida[l].raca] = listaInvertida[l].id;
        }
        //printf("ID:%ld, PROX:%d\n", listaInvertida[l].id, listaInvertida[l].prox);
        l--;
    }
    fclose(base);

}

void printRacas(){
	for(int i=0; i< tamNomeRacas; i++){
		printf("%s -> Digite %d\n", listaNomeRacas[i].raca, listaNomeRacas[i].id);
	}
}

void resetaIndices(){
    for(int i= 0; i<tamIndices; i++){
        listaIndices[i].id= listaIndices[i].offset= 0;
    }
    tamIndices= 0;
}

void preparaParaLerOutra(){
    for(int i= 0; i<tamSec; i++){
        listaSecundaria[i].id= listaSecundaria[i].indice= 0;
    }
    for(int i= 0; i<tamInvert; i++){
        listaInvertida[i].id= listaInvertida[i].offset= listaInvertida[i].raca= 0;
        listaInvertida[i].id= -1;
    }
    for(int i= 0; i<tamIndices; i++){
        listaIndices[i].id= listaIndices[i].offset= 0;
    }
    tamSec= 0;
	tamInvert= 0;
	tamIndices= 0;
}

int searchNo(no key, BTPAGE* p_page, long* pos, long *rrn){
    int i;
    BTPAGE pagina= *p_page;
    if(pagina.keycount == 0)
        return NO;
    for(i= 0; i<pagina.keycount && atoi(key.id_i) > atoi(pagina.key[i].id_i); i++){
        ;
    }
    *pos= i;
    if(atoi(pagina.key[*pos].id_i) < 0){
        *pos= *pos - 1;
        if(*pos < 0)
            return NO;
    }
    if(*pos < pagina.keycount && atoi(key.id_i) == atoi(pagina.key[*pos].id_i))
        return YES;
    if(*pos < pagina.keycount && atoi(key.id_i) < atoi(pagina.key[*pos].id_i)){
        BTPAGE filho;
        btread(pagina.child[*pos], &filho);
        *rrn= pagina.child[*pos];
        return searchNo(key, &filho, pos, rrn);
    }
    if(*pos < pagina.keycount && atoi(key.id_i) > atoi(pagina.key[*pos].id_i)){
        BTPAGE filho;
        btread(pagina.child[*pos+1], &filho);
        *rrn= pagina.child[*pos+1];
        return searchNo(key, &filho, pos, rrn);
    }
    return NO;
}

void buscaRegistro(char *idBusca){
    btopen();
    BTPAGE pagina, nova;
    btread(root, &pagina);
    no filtro;
    strcpy(filtro.id_i, idBusca);
    long pos, rrn;
    char *offsetS;
    long offset;
    caes individuo;
    if(searchNo(filtro, &pagina, &pos, &rrn) == YES){
        printf("Sai da busca! %d, %d\n", rrn, pos);
        btread(rrn, &nova);
        printf("Li! %s\n", nova.key[pos].byteOffSet);
        offset= atol(nova.key[pos].byteOffSet);
        printf("Peguei o offset! %d %d\n", offset, buscaOffsetDoIndice(42));
        buscaPorOffset(offset);
    }else
        printf("Nenhum foi encontrado!");
    btclose();
}

void adicionaCao(){
    caes novocao;
    printf("\nInsira os dados do novo cão\n");
    printf("Insira o id do novo cão: ");
    scanf("%s", novocao.id_i); printf("\n");
    printf("Insira o id da raça do novo cão: ");
    scanf("%s", novocao.id_r); printf("\n");
    printf("Insira o nome do novo cão : ");
    scanf("%s", novocao.nome); printf("\n");
    printf("Insira o sexo do novo cão : ");
    scanf("%s", novocao.sexo); printf("\n");
    FILE* arq;
    arq = fopen("individuos_num.txt", "r+");
    fseek(arq, 0, SEEK_END);
    fprintf(arq,"%s%s%s%s", novocao.id_i, novocao.id_r, novocao.nome, novocao.sexo);

}

void trocaArquivo(char *filename, FILE *base){
    preparaParaLerOutra();
    remove("btree.txt");
    driver();
}

void dialogo(){
    printf("Bem vindo(a) a aplicacao!\n \
            Ainda nao foi cadastrado nenhum cao, por favor digite o nome de um arquivo:\n");
    char input[40];
    getline2(input, 40);
    povoaArquivo(input);
    getNumRegs();
    FILE* base;
    base = fopen("base.txt", "r");
    criaIndices(base);
    gravaIndices();
    leNomesRacas("nome-racas.txt");
    monta_lista();
    driver();
    int flag=1;
    int opcao;
    while(flag){
        printf("O que vc deseja fazer?\n \
                0: Sair\n \
                1: Adicionar um novo cão\n \
                2: Buscar um cão pelo seu id\n \
                3: Listagem da Arvore B\n \
                4: Ler um novo arquivo\n");
        input[0]= '\0';

        scanf("%d", &opcao);
        switch(opcao){
            case 0: flag=0;
                grava();
                exit(1);
            case 1:
                adicionaCao();
                break;
            case 2:
                printf("Qual e o id do cao a ser buscado?\n");
                int id;
                scanf("%d", &id);
                buscaPorId(id);
                break;
            case 3:
                printArvore();
            case 4:
                printf("Qual o nome do arquivo a ser lido?\n");
                fflush(stdin);
                getline2(input, 40);
                povoaArquivo(input);
                trocaArquivo(input, base);
                printf("\nLido segundo arquivo\n");
                monta_lista();
                break;
        }
    }
    fclose(base);
}

int main(){
    dialogo();
    /*FILE* base;
    base = fopen("base.txt", "r");
    getNumRegs();
    povoaArquivo("");
    criaIndices(base);
    gravaIndices();
    //carregaIndices();
    leNomesRacas("nomes-racas.txt");
    monta_lista();
    //buscaPorId(1);
    driver();
    printArvore();
    //trocaArquivo("individuos_num2.txt", base);
    //printArvore();
    //buscaPorId(42);
    buscaRegistro("42");
    //dialogo();
    fclose(base);*/
}
