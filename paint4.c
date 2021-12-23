#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h> // for error catch
#include <math.h>



// Structure for canvas
typedef struct
{
    int width;
    int height;
    char **canvas;
    int **colormap;
    char pen;
    int color;
} Canvas;

// Command 構造体と History構造体
// [*]
typedef struct command Command;
struct command{
    char *str;
    size_t bufsize;
    Command *next;
};

typedef struct{
    Command *begin;
    size_t bufsize; // [*] : この方が効率的ですね。一部の方から指摘ありました。
} History;

// functions for Canvas type
Canvas *init_canvas(int width, int height, char pen);
void reset_canvas(Canvas *c);
void print_canvas(Canvas *c);
void free_canvas(Canvas *c);

// display functions
void rewind_screen(unsigned int line); 
void clear_command(void);
void clear_screen(void);

// enum for interpret_command results
typedef enum res{ EXIT, LINE, RECT, TRI, CIRCLE, STAR, UNDO, LOAD, SAVE, CHPEN, CHCOL, UNKNOWN, ERRNONINT, ERRLACKARGS, NOCOMMAND, NOFILE, NOCOLOR} Result;
// Result 型に応じて出力するメッセージを返す
char *strresult(Result res);

int max(const int a, const int b);
void draw_line(Canvas *c, const int x0, const int y0, const int x1, const int y1);
Result interpret_command(const char *command, History *his, Canvas *c);
void save_history(const char *filename, History *his);

// [*] list.c のpush_backと同じ
Command *push_command(History *his, const char *str);

int main(int argc, char **argv)
{
    //for history recording
    
    const int bufsize = 1000;
    
    // [*]
    History his = (History){ .begin = NULL, .bufsize = bufsize};
    
    int width;
    int height;
    if (argc != 3){
        fprintf(stderr,"usage: %s <width> <height>\n",argv[0]);
        return EXIT_FAILURE;
    } else{
        char *e;
        long w = strtol(argv[1],&e,10);
        if (*e != '\0'){
            fprintf(stderr, "%s: irregular character found %s\n", argv[1],e);
            return EXIT_FAILURE;
        }
        long h = strtol(argv[2],&e,10);
        if (*e != '\0'){
            fprintf(stderr, "%s: irregular character found %s\n", argv[2],e);
            return EXIT_FAILURE;
        }
        width = (int)w;
        height = (int)h;    
    }
    char pen = '*';
    

    char buf[bufsize];

    Canvas *c = init_canvas(width,height, pen);
    
    printf("\n"); // required especially for windows env

    while(1){
        // [*]
        // hsize はひとまずなし
        // 作る場合はリスト長を調べる関数を作っておく
        print_canvas(c);
        printf("* > ");
        if(fgets(buf, bufsize, stdin) == NULL) break;
        
        const Result r = interpret_command(buf, &his, c);

        if (r == EXIT) break;

        // 返ってきた結果に応じてコマンド結果を表示
        clear_command();
        printf("%s\n",strresult(r));
        // LINEの場合はHistory構造体に入れる
        if (r == LINE || r == RECT || r == TRI || r == STAR || r == CIRCLE || r == CHPEN || r == CHCOL) {
            // [*]
            push_command(&his,buf);
        }
        
        rewind_screen(2); // command results
        clear_command(); // command itself
        rewind_screen(height+2); // rewind the screen to command input
	
    }
    
    clear_screen();
    free_canvas(c);
    
    return 0;
}

Canvas *init_canvas(int width,int height, char pen)
{
    Canvas *new = (Canvas *)malloc(sizeof(Canvas));
    new->width = width;
    new->height = height;
    new->canvas = (char **)malloc(width * sizeof(char *));
    new->colormap = (int **)malloc(width * sizeof(int *));
    
    char *tmp = (char *)malloc(width*height*sizeof(char));
    memset(tmp, ' ', width*height*sizeof(char));
    for (int i = 0 ; i < width ; i++){
	    new->canvas[i] = tmp + i * height;
    }

    int *ctmp = (int *)malloc(width*height*sizeof(int));
    memset(ctmp, 0, width*height*sizeof(char));
    for (int i = 0 ; i < width ; i++){
	    new->colormap[i] = ctmp + i * height;
    }
    
    new->pen = pen;
    new->color=0;
    return new;
}

void reset_canvas(Canvas *c)
{
    const int width = c->width;
    const int height = c->height;
    c->pen='*';
    c->color=0;
    memset(c->canvas[0], ' ', width*height*sizeof(char));
    memset(c->colormap[0], 0, width*height*sizeof(int));
}


void print_canvas(Canvas *c)
{
    const int height = c->height;
    const int width = c->width;
    char **canvas = c->canvas;
    int **colormap = c->colormap;
    
    // 上の壁
    printf("+");
    for (int x = 0 ; x < width ; x++){
	    printf("-");
    }
    printf("+\n");
    
    // 外壁と内側
    for (int y = 0 ; y < height ; y++) {
        printf("|");
        for (int x = 0 ; x < width; x++){
            const char c = canvas[x][y];
            printf("\x1b[%dm",colormap[x][y]);
            putchar(c);
            printf("\x1b[0m");
        }
        printf("|\n");
    }
    
    // 下の壁
    printf( "+");
    for (int x = 0 ; x < width ; x++){
	    printf("-");
    }
    printf("+\n");
    fflush(stdout);
}

void free_canvas(Canvas *c){
    free(c->canvas[0]); //  for 2-D array free
    free(c->canvas);
    free(c);
}

void rewind_screen(unsigned int line){
    printf("\e[%dA",line);
}

void clear_command(void){
    printf("\e[2K");
}

void clear_screen(void){
    printf( "\e[2J");
}


int max(const int a, const int b){
    return (a > b) ? a : b;
}

void draw_line(Canvas *c, const int x0, const int y0, const int x1, const int y1){
    const int width = c->width;
    const int height = c->height;
    char pen = c->pen;
    int color=c->color;
    
    const int n = max(abs(x1 - x0), abs(y1 - y0));
    if ( (x0 >= 0) && (x0 < width) && (y0 >= 0) && (y0 < height)){
        c->canvas[x0][y0] = pen;
        c->colormap[x0][y0]=color;
    }
    for (int i = 1; i <= n; i++) {
        const int x = x0 + i * (x1 - x0) / n;
        const int y = y0 + i * (y1 - y0) / n;
        if ( (x >= 0) && (x< width) && (y >= 0) && (y < height)){
            c->canvas[x][y] = pen;
            c->colormap[x][y] = color;
        }
    }
}

void draw_circle(Canvas *c, const int x0, const int y0, const int a, const int b, const int incolor){
    const int width = c->width;
    const int height = c->height;
    char pen=c->pen;
    int color=c->color;
    for(int x=0; x<width; x++){
        for(int y=0; y<height; y++){
            if(pow(a-0.5,2) < pow(x-x0,2)+pow((y-y0)*a/b,2) && pow(x-x0,2)+pow((y-y0)*a/b,2)<pow(a+0.5,2)){
                c->canvas[x][y]=pen;
                c->colormap[x][y]=color;
            }else if(incolor && pow(x-x0,2)+pow((y-y0)*a/b,2)<=pow(a-0.5,2)){
                c->canvas[x][y]=' ';
                c->colormap[x][y]=incolor;
            }
        }
    }
}

void save_history(const char *filename, History *his){
    const char *default_history_file = "history.txt";
    if (filename == NULL)
	filename = default_history_file;
    
    FILE *fp;
    if ((fp = fopen(filename, "w")) == NULL) {
	fprintf(stderr, "error: cannot open %s.\n", filename);
	return;
    }
    // [*] 線形リスト版
    for (Command *p = his->begin ; p != NULL ; p = p->next){
	fprintf(fp, "%s", p->str);
    }
    
    fclose(fp);
}

Result interpret_command(const char *command, History *his, Canvas *c)
{
    char buf[his->bufsize];
    strcpy(buf, command);
    if(buf[strlen(buf) - 1]=='\n'){
        buf[strlen(buf) - 1]=0;
    } // remove the newline character at the end
    
    const char *s = strtok(buf, " ");
    if (s == NULL){ // 改行だけ入力された場合
	    return UNKNOWN;
    }
    // The first token corresponds to command
    if (strcmp(s, "line") == 0) {
        int p[4] = {0}; // p[0]: x0, p[1]: y0, p[2]: x1, p[3]: y1 
        char *b[4];
        for (int i = 0 ; i < 4; i++){
            b[i] = strtok(NULL, " ");
            if (b[i] == NULL){
                return ERRLACKARGS;
            }
        }
        for (int i = 0 ; i < 4 ; i++){
            char *e;
            long v = strtol(b[i],&e, 10);
            if (*e != '\0'){
                return ERRNONINT;
            }
            p[i] = (int)v;
        }
        
        draw_line(c,p[0],p[1],p[2],p[3]);
        return LINE;
    }
    if(strcmp(s, "rect")==0){
        int p[4] = {0}; // p[0]: x0, p[1]: y0, p[2]: width, p[3]: height 
        char *b[4];
        char *color;
        int incheck=0;
        int incolor=0;
        for (int i = 0 ; i < 4; i++){
            b[i] = strtok(NULL, " ");
            if (b[i] == NULL){
                return ERRLACKARGS;
            }
        }
        if((color=strtok(NULL, ""))!=NULL){
            char colorlist[8]={'k','r','g','y','b','m','c','w'};
            int colornum[8]={40,41,42,43,44,45,46,47};
            for(int i=0; i<8; i++){
                if(color[0]==colorlist[i]){
                    incolor=colornum[i];
                    incheck=1;
                    break;
                }
            }
            if(!incheck){
                return NOCOLOR;
            }
        }
        for (int i = 0 ; i < 4 ; i++){
            char *e;
            long v = strtol(b[i],&e, 10);
            if (*e != '\0'){
                return ERRNONINT;
            }
            p[i] = (int)v;
        }
        draw_line(c,p[0],p[1],p[0]+p[2]-1, p[1]);
        draw_line(c,p[0]+p[2]-1, p[1],p[0]+p[2]-1, p[1]+p[3]-1);
        draw_line(c,p[0]+p[2]-1, p[1]+p[3]-1, p[0], p[1]+p[3]-1);
        draw_line(c,p[0], p[1]+p[3]-1, p[0],p[1]);

        if(incheck){
            for(int x=p[0]+1; x<p[0]+p[2]-1; x++){
                for(int y=p[1]+1; y<p[1]+p[3]-1; y++){
                    if(0<=x && x< c->width && 0<= y && y < c->height){
                        c->canvas[x][y]=' ';
                        c->colormap[x][y]=incolor;
                    }
                }
            }
        }
        return RECT;
    }

    if(strcmp(s,"tri")==0){
        int p[4] = {0}; // p[0]:x0 , p[1]: y0, p[2]: width, p[3]: height
        char *b[4];
        char *color;
        int incheck=0;
        int incolor=0;
        for (int i = 0 ; i < 4; i++){
            b[i] = strtok(NULL, " ");
            if (b[i] == NULL){
                return ERRLACKARGS;
            }
        }
        if((color=strtok(NULL, ""))!=NULL){
            char colorlist[8]={'k','r','g','y','b','m','c','w'};
            int colornum[8]={40,41,42,43,44,45,46,47};
            for(int i=0; i<8; i++){
                if(color[0]==colorlist[i]){
                    incolor=colornum[i];
                    incheck=1;
                    break;
                }
            }
            if(!incheck){
                return NOCOLOR;
            }
        }
        for (int i = 0 ; i < 4 ; i++){
            char *e;
            long v = strtol(b[i],&e, 10);
            if (*e != '\0'){
                return ERRNONINT;
            }
            p[i] = (int)v;
        }
        if(incheck){
            int n = max(p[2]/2, p[3]-1);
            for (int i = 1; i <= n; i++) {
                int minx = p[0] - i * p[2]/2 / n;
                int maxx = p[0] + i * p[2]/2 /n;
                int y = p[1] + i * (p[3]-1) / n;
                for(int x=minx+1; x<maxx; x++){
                    if(0<=x && x< c->width && 0<= y && y < c->height){
                        c->canvas[x][y]=' ';
                        c->colormap[x][y]=incolor;
                    }
                }
            }
        }

        draw_line(c,p[0],p[1],p[0]-p[2]/2, p[1]+p[3]-1);
        draw_line(c,p[0],p[1],p[0]+p[2]/2, p[1]+p[3]-1);
        draw_line(c,p[0]-p[2]/2, p[1]+p[3]-1,p[0]+p[2]/2, p[1]+p[3]-1);

        return TRI;
    }

    if(strcmp(s,"circle")==0){
        int p[4] = {0}; // p[0]: x0, p[1]: y0, p[2]: a(横), p[3]:b(縦) 
        char *b[4];
        char *color;
        int incheck=0;
        int incolor=0;
        for (int i = 0 ; i < 4; i++){
            b[i] = strtok(NULL, " ");
            if (b[i] == NULL){
                return ERRLACKARGS;
            }
        }
        if((color=strtok(NULL, ""))!=NULL){
            char colorlist[8]={'k','r','g','y','b','m','c','w'};
            int colornum[8]={40,41,42,43,44,45,46,47};
            
            for(int i=0; i<8; i++){
                if(color[0]==colorlist[i]){
                    incolor=colornum[i];
                    incheck=1;
                    break;
                }
            }
            if(!incheck){
                return NOCOLOR;
            }
        }
        for (int i = 0 ; i < 4 ; i++){
            char *e;
            long v = strtol(b[i],&e, 10);
            if (*e != '\0'){
                return ERRNONINT;
            }
            p[i] = (int)v;
        }
        draw_circle(c,p[0],p[1],p[2],p[3], incolor);
        return CIRCLE;
    }

    if(strcmp(s,"star")==0){
        int p[4] = {0}; // p[0]: x0, p[1]: y0, p[2]: a(横), p[3]:b(縦) 
        char *b[4];
        for (int i = 0 ; i < 4; i++){
            b[i] = strtok(NULL, " ");
            if (b[i] == NULL){
                return ERRLACKARGS;
            }
        }
        for (int i = 0 ; i < 4 ; i++){
            char *e;
            long v = strtol(b[i],&e, 10);
            if (*e != '\0'){
                return ERRNONINT;
            }
            p[i] = (int)v;
        }

        double ratio=2/(3+sqrt(5));
        int out[5]={18,90,162,234,306};
        int in[5]={54,126,198,270,342};
        double rad=3.1415/180;
        for(int i=0; i<5; i++){
            draw_line(c,(int)(p[0]+p[2]*cos(out[i]*rad)),(int)(p[1]-p[3]*sin(out[i]*rad)),(int)(p[0]+ratio*p[2]*cos(in[i]*rad)),(int)(p[1]-ratio*p[3]*sin(in[i]*rad)));
            if(i!=4){
                draw_line(c,(int)(p[0]+p[2]*cos(out[i+1]*rad)),(int)(p[1]-p[3]*sin(out[i+1]*rad)),(int)(p[0]+ratio*p[2]*cos(in[i]*rad)),(int)(p[1]-ratio*p[3]*sin(in[i]*rad)));
            }else{
                draw_line(c,(int)(p[0]+p[2]*cos(out[0]*rad)),(int)(p[1]-p[3]*sin(out[0]*rad)),(int)(p[0]+ratio*p[2]*cos(in[i]*rad)),(int)(p[1]-ratio*p[3]*sin(in[i]*rad)));
            }
        }

        return STAR;
    }

    if(strcmp(s,"load")==0){
        FILE *fp;
        char *ftmp;
        char filename[100];
        if((ftmp=strtok(NULL, " "))==NULL){
            strcpy(filename, "history.txt");
        }else{
            strcpy(filename, ftmp);
        }
        
        if((fp=fopen(filename, "r"))==NULL){
	        return NOFILE;
        }else{
            while(1){
                
                if(fgets(buf, his->bufsize, fp) == NULL){
                    break;
                }
                const Result r = interpret_command(buf, his, c);
                if (r == EXIT){
                    break;
                }
                // LINEの場合はHistory構造体に入れる
                if (r == LINE || r == RECT || r == TRI || r == STAR || r == CIRCLE || r == CHPEN || r == CHCOL) {
                    push_command(his,buf);
                }
            }
        }
        fclose(fp);
        return LOAD;
    }

    if(strcmp(s, "chpen")==0){   
        char *pen;
        pen = strtok(NULL, " ");
        if (pen == NULL){
            return ERRLACKARGS;
        }
        c->pen=pen[0];  
        
        return CHPEN;
    }

    if(strcmp(s,"chcol")==0){
        char colorlist[8]={'k','r','g','y','b','m','c','w'};
        int colornum[8]={30,31,32,33,34,35,36,37};
        char *color;
        if((color=strtok(NULL, ""))!=NULL){
            int check=0;
            for(int i=0; i<8; i++){
                if(color[0]==colorlist[i]){
                    c->color=colornum[i];
                    check=1;
                    break;
                }
            }
            if(!check){
                return NOCOLOR;
            }
        }else{
            return ERRLACKARGS;
        }

        return CHCOL;
    }
    if (strcmp(s, "save") == 0) {
        s = strtok(NULL, " ");
        save_history(s, his);
        return SAVE;
    }
    
    if (strcmp(s, "undo") == 0) {
        reset_canvas(c);
        //[*] 線形リストの先頭からスキャンして逐次実行
        // pop_back のスキャン中にinterpret_command を絡めた感じ
        Command *p = his->begin;
        if (p == NULL){
            return NOCOMMAND;
        }
        else{
            Command *q = NULL; // 新たな終端を決める時に使う
            while (p->next != NULL){ // 終端でないコマンドは実行して良い
            interpret_command(p->str, his, c);
            q = p;
            p = p->next;
            }
            // 1つしかないコマンドのundoではリストの先頭を変更する
            if (q == NULL) {
            his->begin = NULL;
            }
            else{
            q->next = NULL;
            }
            free(p->str);
            free(p);	
            return UNDO;
        }  
    }
    
    if (strcmp(s, "quit") == 0) {
	    return EXIT;
    }

    return UNKNOWN;
}


// [*] 線形リストの末尾にpush する
Command *push_command(History *his, const char *str){
    Command *c = (Command*)malloc(sizeof(Command));
    char *s = (char*)malloc(his->bufsize);
    strcpy(s, str);
    
    *c = (Command){ .str = s, .bufsize = his->bufsize, .next = NULL};
    
    Command *p = his->begin;
    
    if ( p == NULL) {
	his->begin = c;
    }
    else{
	while (p->next != NULL){
	    p = p->next;
	}
	p->next = c;
    }
    return c;
}

char *strresult(Result res){
    switch(res) {
    case EXIT:
	break;
    case SAVE:
	return "history saved";
    case LINE:
	return "1 line drawn";
    case RECT:
    return "1 rectangle drawn";
    case TRI:
    return "1 triangle drawn";
    case CIRCLE:
    return "1 circle drawn";
    case STAR:
    return "1 star drawn";
    case LOAD:
    return "successfully loaded";
    case CHPEN:
    return "pen changed";
    case CHCOL:
    return "color changed";
    case UNDO:
	return "undo!";
    case UNKNOWN:
	return "error: unknown command";
    case ERRNONINT:
	return "Non-int value is included";
    case ERRLACKARGS:
	return "Too few arguments";
    case NOCOMMAND:
	return "No command in history";
    case NOFILE:
    return "no such file";
    case NOCOLOR:
    return "no such color: choose from {k,r,g,y,b,m,c,w}";
    }
    return NULL;
}

