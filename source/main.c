#include <nds.h>
#include <math.h>

// Definições do tamanho do mapa
#define MAP_WIDTH 16
#define MAP_HEIGHT 16

// Layout do Bunker da 3ª Guerra Mundial
const int MAP[MAP_HEIGHT][MAP_WIDTH] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,0,0,0,0,0,1,1,1,0,0,1},
    {1,0,1,0,0,0,0,0,0,0,0,0,1,0,0,1},
    {1,0,1,0,0,0,0,0,0,0,0,0,1,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,1,1,1,1,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1},
    {1,0,0,0,1,1,1,1,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// Variáveis do Jogador
float playerX = 2.5f;
float playerY = 2.5f;
float playerAngle = 0.0f;

// Função para desenhar um cubo 3D (Parede do Bunker)
void drawCube(float x, float y) {
    glPushMatrix();
    glTranslatef(x, 0.0f, y);
    
    glBegin(GL_QUAD);
        // Face Frontal
        glColor3f(0.3f, 0.3f, 0.3f); // Cinza militar escuro
        glVertex3f(-0.5f, -0.5f,  0.5f);
        glVertex3f( 0.5f, -0.5f,  0.5f);
        glVertex3f( 0.5f,  0.5f,  0.5f);
        glVertex3f(-0.5f,  0.5f,  0.5f);
        // Face Traseira
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f(-0.5f,  0.5f, -0.5f);
        glVertex3f( 0.5f,  0.5f, -0.5f);
        glVertex3f( 0.5f, -0.5f, -0.5f);
    glEnd();
    
    glPopMatrix();
}

int main(void) {
    // 1. Inicializa o motor gráfico 3D na tela superior do DS
    videoSetMode(MODE_0_3D);
    glInit();
    
    // Configura a iluminação e profundidade (VRAM)
    glEnable(GL_BLEND);
    glEnable(GL_ANTIALIAS);
    glClearColor(5, 5, 5, 31); // Teto/Fundo escuro
    glClearPolyID(63);
    glClearDepth(GL_MAX_DEPTH);
    
    // Configura o Campo de Visão (Viewport e Projeção Perspectiva)
    glViewport(0, 0, 255, 191);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(70, 256.0f / 192.0f, 0.1f, 40.0f);
    
    // 2. Inicializa o Console de Texto na tela inferior do DS (HUD)
    videoSetModeSub(MODE_0_2D);
    vramSetMainBanks(VRAM_A_TEXTURE, VRAM_B_MAIN_BG, VRAM_C_SUB_BG, VRAM_D_LCD);
    consoleInit(NULL, 1, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);

    // Loop Principal de Hardware do Nintendo DS
    while(1) {
        // Ler quais botões físicos do DS foram pressionados
        scanKeys();
        uint32 keys = keysHeld();
        
        float speed = 0.08f;
        float turnSpeed = 0.05f;

        // Rotação de câmera (D-Pad Esquerda / Direita)
        if (keys & KEY_LEFT)  playerAngle -= turnSpeed;
        if (keys & KEY_RIGHT) playerAngle += turnSpeed;

        // Movimentação (D-Pad Cima / Baixo)
        if (keys & KEY_UP) {
            float nextX = playerX + cosf(playerAngle) * speed;
            float nextY = playerY + sinf(playerAngle) * speed;
            if (MAP[(int)nextY][(int)nextX] == 0) {
                playerX = nextX;
                playerY = nextY;
            }
        }
        if (keys & KEY_DOWN) {
            float nextX = playerX - cosf(playerAngle) * speed;
            float nextY = playerY - sinf(playerAngle) * speed;
            if (MAP[(int)nextY][(int)nextX] == 0) {
                playerX = nextX;
                playerY = nextY;
            }
        }

        // --- RENDERIZAÇÃO TELA SUPERIOR (3D) ---
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        // Posiciona a câmera do DS nos olhos do jogador
        float lookX = playerX + cosf(playerAngle);
        float lookY = playerY + sinf(playerAngle);
        gluLookAt(playerX, 0.0f, playerY,   lookX, 0.0f, lookY,   0.0f, 1.0f, 0.0f);

        // Renderiza o mapa ao redor do jogador
        for(int y = 0; y < MAP_HEIGHT; y++) {
            for(int x = 0; x < MAP_WIDTH; x++) {
                if(MAP[y][x] == 1) {
                    drawCube((float)x, (float)y);
                }
            }
        }

        // --- RENDERIZAÇÃO TELA INFERIOR (HUD / Texto) ---
        consoleClear();
        iprintf("\n    === WW3: OPERATION DS ===\n");
        iprintf(" --------------------------------\n");
        iprintf("  POS: X:%.2f | Y:%.2f\n", playerX, playerY);
        iprintf("  ANGULO: %.2f rad\n", playerAngle);
        iprintf(" --------------------------------\n");
        iprintf("  VIDA: 100%%  |  MUNICAO: 30/30\n");
        iprintf("\n  CONTROLES:\n");
        iprintf("  - D-Pad: Mover e Olhar\n");
        iprintf("  - Botao A: Atirar (Breve)\n");

        // Envia as coordenadas processadas para a GPU do DS desenhar na tela
        glFlush(0);
        swiWaitForVBlank();
    }

    return 0;
}
