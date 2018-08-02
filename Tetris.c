/*
  2017.1.7
  Tetris.c
*/

#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<handy.h>

#define WINDOWSIZE 600.0 //長辺

int board[12][25];                 //[x][y]
double range = WINDOWSIZE / 25.0;  //ピース一辺の長さ
int deletedLine;                   //消去する行

//参考*1--------------------
typedef struct Shape {
    int x, y;
} Shape;

typedef struct Block {
    int angle;
    Shape parts[3]; //回転中心{0,0}からの相対的なピース位置を表してブロックを作る
} Block;

Block category[8] = {  //{x, y}
    { 0, { {0,0},  {0,0}, {0,0}  } },  //null

    { 4, { {-1,-1},{-1,0},{1,0}  } },  // L         1 (1) 1
                                       //           1

    { 4, { {-1,0}, {1,0}, {1,-1} } },  //逆L        1 (1) 1
                                       //                 1

    { 4, { {-1,0}, {1,0}, {0,1}  } },  // 凸           1
                                       //           1 (1) 1

    { 2, { {-1,0}, {0,1}, {1,1}  } },  //左下右上      1  1
                                       //           1 (1)

    { 2, { {-1,1}, {0,1}, {1,0}  } },  //左上右下   1  1
                                       //             (1) 1

    { 2, { {0,-1}, {0,1}, {0,2}  } },  //bar           1
                                       //              1
                                       //             (1)
                                       //              1

    { 1, { {0,1},  {1,0}, {1,1}  } },  //box          1  1
                                       //            (1) 1
};

typedef struct Status {
    int x, y;           //現在位置（回転中心）
    int category;       //ブロック種類
    int angle;          //回転角度
} Status;

Status now;  //落下中のブロック情報
//--------------------------

void checkBoard();           //boardをターミナル表示
void setBoard();             //boardの初期化
int formBlock(int DoPoC);    //nowの位置情報とピース配置構造体からブロックを生成。
                             //移動前のブロック削除にも使用。
                             //ブロック干渉の判定にもこの関数からcheckOverlapを呼ぶことで使われる。
int checkOverlap(int nextX, int nextY);  //formBlock内で干渉判定に使用
void drawBoard(int lid);     //boardに基づいて描写する
int deleteLine();            //揃った列があるか確認
void shifter();              //行を削除して空行がある場合、その上のブロックをずらしてくる
int gameover();              //gameover判定


int main() {
    hgevent *event;
    int frames = 0;
    int fixCount = 0;   //ブロック固定までのカウント
    int exist = 0;
    doubleLayer layers;
    srandom(time(NULL));

    now.category = 1 + random() % 7; //1~7の乱数でブロックを決定
    now.angle = 100;                 //下矢印キーでのnow.angle--;処理のため、余裕のある数字で初期化
    now.x = 5;                       //出現位置
    now.y = 21;

    setBoard();
    checkBoard();

    //HgOpen(500, 700);
    HgOpen(range * 12, WINDOWSIZE);
    layers = HgWAddDoubleLayer(0);

    HgSetEventMask(HG_KEY_DOWN);

    while(1) {
        if(now.y == 21) {  //now.y==21の時（新しいブロックが出現する時）にgameover判定
            if(gameover() == 1) break;
        }
        formBlock(1);
        checkBoard();

        drawBoard(HgLSwitch(&layers));  //描画

        formBlock(0);  //移動前のブロック消去

        //入力系
        event = HgEventNonBlocking();
        if(event != NULL) {
            if(event->ch == HG_U_ARROW) {
                now.angle++;
                if(formBlock(2) == 1) now.angle--;
            } else if(event->ch == HG_D_ARROW) {
                now.y--;
                if(formBlock(2) == 1) now.y++;
                //now.angle--;
                //if(formBlock(2) == 1) now.angle++;
            } else if(event->ch == HG_L_ARROW) {
                now.x--;
                if(formBlock(2) == 1) now.x++;  //変更した座標位置が干渉するならば座標を元に
            } else if(event->ch == HG_R_ARROW) {
                now.x++;
                if(formBlock(2) == 1) now.x--;
            }
        }

        //何フレームごとにブロックが落下するか(落下速度となる)
        if(frames > 4) {
            now.y--;
            if(formBlock(2) == 1) {
                now.y++;
                fixCount++;
            }
            frames = 0;
        }

        if(fixCount > 2) {
            formBlock(1);  //ブロック書き込み（固定）
            do {
                exist = deleteLine();           //削除すべき行の確認 まだ削除すべき行がある限り回る
                drawBoard(HgLSwitch(&layers));  //描画
                if(exist == 1) {
                    shifter();                     //existが1→空行がある→ずらす
                    drawBoard(HgLSwitch(&layers));  //描画
                }
            } while(exist == 1);

            fixCount = 0;  //落下完了カウント初期化
            now.x = 5;     //現在位置初期化
            now.y = 21;
            now.category = 1 + random() % 7; //1~7の乱数でブロックを決定
            now.angle = 0; //回転角初期化
        }

        frames++;
    }

    return 0;
}

//ターミナル確認用--------------------------------------------------
void checkBoard() {
    int i, k;

    printf("\n");
    for(k=24; k>=0; k--) {
        for(i=0; i<12; i++) {
            if(k==0 && i>0) printf("\x1b[47m"); //背景色　灰
            if(k > 0 || (k==0 && (i==0 || i==11))) {
                printf("  ");
            } else {
                printf(" ");
            }
            switch(board[i][k]) {   //エスケープシーケンス
                case 1:
                    printf("\x1b[41m");         //背景色  赤
                    break;
                case 2:
                    printf("\x1b[34m");         //全景色  青
                    break;
                case 3:
                    printf("\x1b[35m");         //前景色  マゼンタ
                    break;
                case 4:
                    printf("\x1b[32m");         //前景色  緑
                    break;
                case 5:
                    printf("\x1b[31m");         //前景色  赤
                    break;
                case 6:
                    printf("\x1b[36m");         //前景色  シアン
                    break;
                case 7:
                    printf("\x1b[33m");         //前景色  黄色
                    break;
                case 99:
                    printf("\x1b[47m");         //背景色  灰
                    break;
                default:
                    printf("\x1b[39m");         //前景色  デフォルト
                    break;
            }
            printf("%d", board[i][k]);
            printf("\x1b[49m");                 //背景色  デフォルト
            printf("\x1b[39m");
        }
        printf("  __%d\n", k);
    }
}

//boardの初期化-----------------------------------------------------
void setBoard() {
    int i,k;

    for(k=0; k<25; k++) {
        for(i=0; i<12; i++) {
            if(i==0 || i==11 || k==0) {
                board[i][k] = 99;  //壁は99
            } else {
                board[i][k] = 0;   //空間は0
            }
        }
    }

}

//ブロック干渉確認--------------------------------------------------
int checkOverlap(int nextX, int nextY) {  //次の配置位置が0（空間）でなければ1を返す
    if(board[nextX][nextY] != 0) return 1;
    return 0;
}

//ブロックの配置----------------------------------------------------参考*1
int formBlock(int DoPoC) { //引数0でブロック消去、1で形成、2で配置可能かの確認
    int i, k;
    int tempX, tempY;
    int pX, pY;
    int angle = now.angle % category[now.category].angle;
    //回転パターンがそれぞれ1, 2, 4のブロックがあり、now.angleのmodをとるとそれぞれ0と0, 1と0, 1, 2, 3とangleがでる
    //now.angleに基づきcategory[8]に初期化されている状態から右に何回転したかを判定し回す

    Block rotation[8];      //category[8]自体は触りたくないので回転用に新規作成
    for(i=0; i<8; i++) {    //now.angleに基づいて1から（category[8]の状態から）回すので毎度初期化します
        rotation[i].angle = category[i].angle;
        for(k=0; k<3; k++) {
            rotation[i].parts[k].x = category[i].parts[k].x;
            rotation[i].parts[k].y = category[i].parts[k].y;
        }
    }

    for(i=0; i<3; i++) {

        for(k=0; k<angle; k++) {    //回転処理 angleが0（回転パターン1）だとfalse（回らない）
            pX = rotation[now.category].parts[i].x;
            pY = rotation[now.category].parts[i].y;

            rotation[now.category].parts[i].x = pY;
            rotation[now.category].parts[i].y = -pX;
        }

        switch(rotation[now.category].parts[i].x) {   //回転処理を施した構造体rotationに基づくピース配置
            case -1:
                tempX = now.x - 1;
                break;
            case 0:
                tempX = now.x;
                break;
            case 1:
                tempX = now.x + 1;
                break;
            case 2:
                tempX = now.x + 2;
                break;
            default:
                break;
        }
        switch(rotation[now.category].parts[i].y) {
            case -1:
                tempY = now.y - 1;
                break;
            case 0:
                tempY = now.y;
                break;
            case 1:
                tempY = now.y + 1;
                break;
            case 2:
                tempY = now.y + 2;
                break;
            default:
                break;
        }

        if(DoPoC == 0) {
            board[now.x][now.y] = 0;
            board[tempX][tempY] = 0;
        } else if(DoPoC == 1) {
            board[now.x][now.y] = now.category;
            board[tempX][tempY] = now.category;
        } else if(DoPoC == 2 && (checkOverlap(now.x, now.y) == 1 || checkOverlap(tempX, tempY) == 1)) {
            return 1;
        }
    }

    return 0;
}

//揃った行を削除する------------------------------------------------
int deleteLine() {  //削除した後1を返す。なければ0
    int  i, k;
    int flag;

    for(k=1; k<21; k++) {
        flag = 1;
        for(i=1; i<11; i++) {
            if(board[i][k]==0) {  //行に空間0があればflagは倒す
                flag = 0;
            }
        }
        if(flag==1) {
            deletedLine = k;      //kが削除すべき行を保持している
            break;
        }
    }
    if(flag==1) {
        for(i=1; i<11; i++) {
            board[i][deletedLine] = 0;
        }
        return 1;
    }


    return 0;
}

//消去した行から上のブロックをずらす--------------------------------
void shifter() {
    int i, k;
    int temp;

    for(k=deletedLine; k<22; k++) {
        for(i=1; i<11; i++) {
            temp = board[i][k + 1];
            board[i][k] = temp;
        }
    }

}

//ゲームオーバーの判定----------------------------------------------
int gameover() {  //ゲームオーバーで1を返す
    int i;

    for(i=1; i<11; i++) {
        if(board[i][21] != 0) {
            return 1;
        }
    }

    return 0;
}

//描画する----------------------------------------------------------
void drawBoard(int lid) {  //レイヤIDを引数にとる
    int i, k;

    HgLClear(lid);

    HgWSetFillColor(lid, HG_BLACK);
    HgWBoxFill(lid, 0, 0, range*12, WINDOWSIZE, 0);
    for(k=0; k<25; k++) {
        for(i=0; i<12; i++) {
            switch(board[i][k]) {
                case 0:
                    HgWSetFillColor(lid, HG_BLACK);
                    break;
                case 1:
                    HgWSetFillColor(lid, HG_ORANGE);
                    break;
                case 2:
                    HgWSetFillColor(lid, HG_BLUE);
                    break;
                case 3:
                    HgWSetFillColor(lid, HG_PURPLE);
                    break;
                case 4:
                    HgWSetFillColor(lid, HG_GREEN);
                    break;
                case 5:
                    HgWSetFillColor(lid, HG_RED);
                    break;
                case 6:
                    HgWSetFillColor(lid, HG_SKYBLUE);
                    break;
                case 7:
                    HgWSetFillColor(lid, HG_YELLOW);
                    break;
                default:
                    HgWSetFillColor(lid, HG_DBLUE);
                    break;
            }
            HgWBoxFill(lid, range*i, range*k, range, range, 0);
        }
    }

    //グリッド
    HgWSetColor(lid, HG_RED);
    for(i=0; i<26; i++) { //横線
        HgWLine(lid, 0, range * i, range * 12, range * i);
    }
    for(i=0; i<13; i++) { //縦線
        HgWLine(lid, range * i, 0, range * i, WINDOWSIZE);
    }

    //ゲームオーバライン
    HgWSetColor(lid, HG_GREEN);
    HgWLine(lid, 0, range*20, range*12, range*20);
    //HgWBox(lid, 0, 0, range*12, range*21);
}
