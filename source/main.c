using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading;

namespace WW3DoomClone
{
    class Program
    {
        // Configurações de Tela do Console
        private const int ScreenWidth = 120;
        private const int ScreenHeight = 40;

        // Propriedades do Jogador (Posição e Ângulo de Visão)
        private static float playerX = 2.5f;
        private static float playerY = 2.5f;
        private static float playerA = 0.0f; // Ângulo em Radianos

        // Estatísticas do Jogador
        private static int playerHealth = 100;
        private static int playerAmmo = 30;
        private static bool isShooting = false;
        private static int shootAnimTicks = 0;

        // Configurações do Campo de Visão (FOV) e Profundidade
        private const float FOV = MathF.PI / 4.0f; // 45 graus para cada lado
        private const float MaxDepth = 16.0f;

        // Dimensões do Mapa do Bunker (WW3)
        private const int MapHeight = 16;
        private const int MapWidth = 16;

        // Layout do Mapa: '#' = Paredes do Bunker, '.' = Espaço Vazio, 'E' = Inimigo Inicial
        private static string map = "";

        // Lista para rastrear os inimigos em tempo real
        private static List<Enemy> enemies = new List<Enemy>();

        // Estrutura de dados para os inimigos (Soldados/Drones Invasores)
        class Enemy
        {
            public float X;
            public float Y;
            public int Health;
            public bool IsDead;

            public Enemy(float x, float y)
            {
                X = x;
                Y = y;
                Health = 2; // Morre com 2 tiros
                IsDead = false;
            }
        }

        static void Main(string[] args)
        {
            // Inicializa o design do mapa de combate
            map += "################";
            map += "#..............#";
            map += "#..###....###..#";
            map += "#..#........#..#";
            map += "#..#........#..#";
            map += "#..............#";
            map += "######....######";
            map += "#..............#";
            map += "#..............#";
            map += "######....######";
            map += "#..............#";
            map += "#....####......#";
            map += "#.......#......#";
            map += "#....####......#";
            map += "#..............#";
            map += "################";

            // Spawnar inimigos em posições estratégicas do bunker
            enemies.Add(new Enemy(8.5f, 4.5f));
            enemies.Add(new Enemy(4.5f, 12.5f));
            enemies.Add(new Enemy(12.5f, 12.5f));

            // Configura o tamanho do buffer do terminal do Windows
            try
            {
                Console.SetWindowSize(ScreenWidth, ScreenHeight);
                Console.SetBufferSize(ScreenWidth, ScreenHeight);
            }
            catch
            {
                // Ignora se o S.O. ou terminal não der suporte direto via código
            }

            Console.CursorVisible = false;
            char[] screen = new char[ScreenWidth * ScreenHeight];

            Stopwatch clock = new Stopwatch();
            clock.Start();
            long lastTime = clock.ElapsedMilliseconds;

            // Loop Principal do Jogo (Game Loop)
            while (playerHealth > 0)
            {
                // Controle de tempo (Delta Time) para movimento suave
                long currentTime = clock.ElapsedMilliseconds;
                float elapsedTime = (currentTime - lastTime) / 1000.0f;
                lastTime = currentTime;

                // --- 1. ENTRADA DE CONTROLES DO JOGADOR (INPUT) ---
                if (Console.KeyAvailable)
                {
                    ConsoleKeyInfo keyInfo = Console.ReadKey(true);

                    // Rotacionar Câmera (Esquerda/Direita)
                    if (keyInfo.Key == ConsoleKey.A) playerA -= 2.5f * elapsedTime;
                    if (keyInfo.Key == ConsoleKey.D) playerA += 2.5f * elapsedTime;

                    // Movimentar para Frente/Trás com detecção simples de colisão
                    if (keyInfo.Key == ConsoleKey.W)
                    {
                        float nextX = playerX + MathF.Cos(playerA) * 4.0f * elapsedTime;
                        float nextY = playerY + MathF.Sin(playerA) * 4.0f * elapsedTime;

                        if (map[(int)nextY * MapWidth + (int)nextX] != '#')
                        {
                            playerX = nextX;
                            playerY = nextY;
                        }
                    }
                    if (keyInfo.Key == ConsoleKey.S)
                    {
                        float nextX = playerX - MathF.Cos(playerA) * 4.0f * elapsedTime;
                        float nextY = playerY - MathF.Sin(playerA) * 4.0f * elapsedTime;

                        if (map[(int)nextY * MapWidth + (int)nextX] != '#')
                        {
                            playerX = nextX;
                            playerY = nextY;
                        }
                    }

                    // Ação de Disparo (Atirar)
                    if (keyInfo.Key == ConsoleKey.Spacebar && playerAmmo > 0 && !isShooting)
                    {
                        isShooting = true;
                        shootAnimTicks = 5; // Duração da animação de tiro
                        playerAmmo--;
                        ProcessPlayerShoot();
                    }
                }

                // --- 2. INTELIGÊNCIA ARTIFICIAL E LÓGICA DOS INIMIGOS ---
                UpdateEnemies(elapsedTime);

                // --- 3. MOTOR DE RENDEREZAÇÃO 3D (RAYCASTING) ---
                // Criar array para guardar a distância das paredes de cada coluna (usado para renderizar sprites)
                float[] depthBuffer = new float[ScreenWidth];

                for (int x = 0; x < ScreenWidth; x++)
                {
                    // Para cada coluna da tela, calcula o ângulo do raio correspondente
                    float rayAngle = (playerA - FOV / 2.0f) + ((float)x / (float)ScreenWidth) * FOV;

                    float distanceToWall = 0.0f;
                    bool hitWall = false;

                    float eyeX = MathF.Cos(rayAngle); // Vetor unitário do raio
                    float eyeY = MathF.Sin(rayAngle);

                    while (!hitWall && distanceToWall < MaxDepth)
                    {
                        distanceToWall += 0.1f;

                        int testX = (int)(playerX + eyeX * distanceToWall);
                        int testY = (int)(playerY + eyeY * distanceToWall);

                        // Testar se o raio saiu dos limites do mapa
                        if (testX < 0 || testX >= MapWidth || testY < 0 || testY >= MapHeight)
                        {
                            hitWall = true;
                            distanceToWall = MaxDepth;
                        }
                        else
                        {
                            // Testar se colidiu com uma parede do bunker
                            if (map[testY * MapWidth + testX] == '#')
                            {
                                hitWall = true;
                            }
                        }
                    }

                    depthBuffer[x] = distanceToWall;

                    // Calcular a altura da parede baseada na distância (Efeito de perspectiva)
                    int ceiling = (int)((ScreenHeight / 2.0) - ScreenHeight / distanceToWall);
                    int floor = ScreenHeight - ceiling;

                    // Desenhar a coluna na tela (Teto, Parede e Chão)
                    for (int y = 0; y < ScreenHeight; y++)
                    {
                        if (y < ceiling)
                        {
                            screen[y * ScreenWidth + x] = ' '; // Teto vazio/escuro
                        }
                        else if (y >= ceiling && y <= floor)
                        {
                            // Sombreamento por caracteres baseado na distância física
                            if (distanceToWall <= MaxDepth * 0.25f) screen[y * ScreenWidth + x] = '█'; // Muito perto
                            else if (distanceToWall < MaxDepth * 0.5f) screen[y * ScreenWidth + x] = '▓';
                            else if (distanceToWall < MaxDepth * 0.75f) screen[y * ScreenWidth + x] = '▒';
                            else screen[y * ScreenWidth + x] = '░'; // Muito longe
                        }
                        else
                        {
                            // Chão com degradê de distância
                            float b = 1.0f - (((float)y - ScreenHeight / 2.0f) / ((float)ScreenHeight / 2.0f));
                            if (b < 0.25) screen[y * ScreenWidth + x] = '#';
                            else if (b < 0.5) screen[y * ScreenWidth + x] = 'x';
                            else if (b < 0.75) screen[y * ScreenWidth + x] = '.';
                            else screen[y * ScreenWidth + x] = ' ';
                        }
                    }
                }

                // --- 4. RENDERIZAR OS INIMIGOS 3D (SPRITES) ---
                RenderEnemies(screen, depthBuffer);

                // --- 5. RENDERIZAR ARMA E HUD EM ASCII ---
                DrawWeaponAndHUD(screen);

                // --- 6. IMPRIMIR BUFFER NA TELA ---
                Console.SetCursorPosition(0, 0);
                Console.Write(screen);

                // Gerenciar temporizador da animação de tiro
                if (isShooting)
                {
                    shootAnimTicks--;
                    if (shootAnimTicks <= 0) isShooting = false;
                }

                // Pequena pausa para economizar processamento e estabilizar FPS
                Thread.Sleep(15);
            }

            // Fim de jogo
Use o código com cuidado.
Console.Clear();
Console.ForegroundColor = ConsoleColor.Red;
Console.WriteLine("\n\n\n\t\t=== VOCÊ FOI ABATIDO NO BUNKER ===");
Console.WriteLine("\t\t       FIM DA LINHA NA WW3");
Console.ResetColor();
Console.ReadLine();
}
private static void UpdateEnemies(float elapsedTime)
{
foreach (var enemy in enemies)
{
if (enemy.IsDead) continue;
// Calcular distância do inimigo até o jogador
float vecX = playerX - enemy.X;
float vecY = playerY - enemy.Y;
float distance = MathF.Sqrt(vecX * vecX + vecY * vecY);
// Se o inimigo estiver muito perto, ataca o jogador
if (distance < 0.6f)
{
playerHealth -= 10; // Causa dano
if (playerHealth < 0) playerHealth = 0;
}
// Se o jogador estiver na linha de visão (raio de 7 blocos), o inimigo persegue
else if (distance < 7.0f)
{
enemy.X += (vecX / distance) * 1.5f * elapsedTime;
enemy.Y += (vecY / distance) * 1.5f * elapsedTime;
}
}
}
private static void ProcessPlayerShoot()
{
// O tiro checa se há um inimigo diretamente na mira (centro da tela / playerA)
foreach (var enemy in enemies)
{
if (enemy.IsDead) continue;
float vecX = enemy.X - playerX;
float vecY = enemy.Y - playerY;
float distance = MathF.Sqrt(vecX * vecX + vecY * vecY);
// Calcular ângulo absoluto do inimigo em relação ao jogador
float enemyAngle = MathF.Atan2(vecY, vecX);
if (enemyAngle < 0) enemyAngle += 2 * MathF.PI;
// Diferença entre o ângulo do jogador e o ângulo do inimigo
float diffAngle = playerA - enemyAngle;
while (diffAngle < -MathF.PI) diffAngle += 2 * MathF.PI;
while (diffAngle > MathF.PI) diffAngle -= 2 * MathF.PI;
// Se a diferença de ângulo for pequena e o inimigo estiver próximo, registra o impacto
if (MathF.Abs(diffAngle) < 0.15f && distance < MaxDepth)
{
enemy.Health--;
if (enemy.Health <= 0)
{
enemy.IsDead = true;
}
break; // Acerta apenas o primeiro inimigo na linha
}
}
}
private static void RenderEnemies(char[] screen, float[] depthBuffer)
{
foreach (var enemy in enemies)
{
if (enemy.IsDead) continue;
// Posição relativa ao jogador
float spriteX = enemy.X - playerX;
float spriteY = enemy.Y - playerY;
float distance = MathF.Sqrt(spriteX * spriteX + spriteY * spriteY);
// Ângulo relativo ao vetor de visão do jogador
float spriteAngle = MathF.Atan2(spriteY, spriteX) - playerA;
while (spriteAngle < -MathF.PI) spriteAngle += 2 * MathF.PI;
while (spriteAngle > MathF.PI) spriteAngle -= 2 * MathF.PI;
// Verificar se o inimigo está dentro do FOV do jogador
if (MathF.Abs(spriteAngle) < FOV / 2.0f && distance > 0.5f && distance < MaxDepth)
{
// Calcular posição X do sprite na projeção da tela
float screenX = (ScreenHeight / 2.0f) - (ScreenHeight / distance);
int spriteScreenX = (int)((ScreenWidth / 2.0f) + (spriteAngle / (FOV / 2.0f)) * (ScreenWidth / 2.0f));
// Tamanho do objeto baseado na proximidade
int spriteHeight = (int)(ScreenHeight / distance);
if (spriteHeight > ScreenHeight) spriteHeight = ScreenHeight;
int ceiling = (int)((ScreenHeight / 2.0f) - (ScreenHeight / distance));
int floor = ScreenHeight - ceiling;
// Desenhar colunas verticais do sprite do inimigo
int spriteWidth = spriteHeight / 2;
for (int sx = spriteScreenX - spriteWidth / 2; sx < spriteScreenX + spriteWidth / 2; sx++)
{
if (sx >= 0 && sx < ScreenWidth)
{
// Só desenha o inimigo se ele não estiver atrás de uma parede resistente (Z-Buffer Check)
if (depthBuffer[sx] >= distance)
{
for (int sy = ceiling; sy < floor; sy++)
{
if (sy >= 0 && sy < ScreenHeight)
{
// 'E' representa o soldado hostil
screen[sy * ScreenWidth + sx] = 'E';
}
}
}
}
}
}
}
}
private static void DrawWeaponAndHUD(char[] screen)
{
// --- DESENHAR ARMA (Rifle Tático) Centralizado na parte inferior ---
int startY = ScreenHeight - 12;
int startX = (ScreenWidth / 2) - 10;
string[] weaponAscii;
if (isShooting)
{
// Frame de Disparo (com Muzzle Flash / Clarão de fogo)
weaponAscii = new string[]
{
"       *  (!!!)  *      ",
"        \  |||  /       ",
"         |#####|        ",
"         |#####|        ",
"         |#####|        ",
"        /=======\       ",
"       /  WWIII  \      ",
"      |           |     ",
"      |  [M4A1]   |     ",
"      |           |     "
};
}
else
{
// Frame Padrão (Arma em repouso)
weaponAscii = new string[]
{
"          |||           ",
"          |||           ",
"         |#####|        ",
"         |#####|        ",
"         |#####|        ",
"        /=======\       ",
"       /  WWIII  \      ",
"      |           |     ",
"      |  [M4A1]   |     ",
"      |           |     "
};
}
for (int i = 0; i < weaponAscii.Length; i++)
{
for (int j = 0; j < weaponAscii[i].Length; j++)
{
if (weaponAscii[i][j] != ' ')
{
screen[(startY + i) * ScreenWidth + (startX + j)] = weaponAscii[i][j];
}
}
}
// --- DESENHAR HUD (Interface com informações de combate) ---
string hudText = $" [WW3 BUNKER ALPHA] | VIDA: {playerHealth}% | MUNIÇÃO: {playerAmmo}/30 | CONTROLES: W/S (Mover) A/D (Olhar) ESPAÇO (Atirar) ";
// Centralizar e sobrepor o texto na última linha da tela
int hudStart = (ScreenWidth - hudText.Length) / 2;
for (int i = 0; i < hudText.Length; i++)
{
screen[(ScreenHeight - 1) * ScreenWidth + (hudStart + i)] = hudText[i];
}
}
}
}

<FollowUp>
Se você precisar de ajuda para estender esse sistema, avise-me! Podemos implementar **portas funcionais**, **mecanismos de chaves para abrir novas áreas**, ou **itens coletáveis (como kits médicos ou pacotes de munição)** no mapa. O que você gostaria de adicionar primeiro?
</FollowUp>
