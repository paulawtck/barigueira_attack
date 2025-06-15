#include "raylib.h"
#include <stdbool.h> 
#include <string.h> 
#include <stdio.h>
#include <stdlib.h> // Fornece funções utilitárias como alocação de memória dinâmica, usado para gerenciar as capivaras.
#include <time.h> // Permite manipular e medir tempo, além de inicializar o gerador de números aleatórios.

// ENUMS
typedef enum Tela {
    TELA_INICIAL,              // Tela de abertura do jogo
    TELA_MENU,                 // Menu principal
    TELA_CREDITOS,             // Tela que exibe os nomes dos criadores e informações
    TELA_JOGO,                 // Tela onde o jogo acontece
    TELA_SELECAO_DIFICULDADE,  // Tela para o jogador escolher o nível de dificuldade.
    TELA_PAUSA                 // Tela exibida quando o jogo está pausado.
} Tela;

typedef enum Dificuldade {
    FACIL,  
    MEDIO, 
    DIFICIL
} Dificuldade;

typedef enum TipoCapivara {
    NORMAL,  // Capivara comum, dá pontos positivos.
    DOURADA, // Capivara dourada, dá mais pontos.
    CUTIA    // Cutia resulta em penalidade de pontos e ativa o efeito de chuva no modo dificil.
} TipoCapivara;

// STRUCTS
typedef struct {
    Rectangle rect;          // Posição e tamanho do animal na tela (x, y, largura, altura). Usado para desenho e detecção de colisão.
    bool visivel;            // Indica se o animal está atualmente visível para ser clicado.
    double tempoVisivel;     // Tempo decorrido desde que o animal ficou visível (para controle de duração).
    double tempoTotalVisivel; // Tempo total que o animal deve permanecer visível antes de desaparecer.
    bool hit;                // Indica se o animal foi clicado (acertado).
    double tempoAcertada;    // Tempo decorrido desde que o animal foi acertado (para animação de "atordoado").
    bool machucada;          // Indica se o animal está no estado de "atordoado" após ser acertado.
    TipoCapivara tipo;       // O tipo específico do animal (NORMAL, DOURADA, CUTIA), para determinar a textura e a pontuação.
}CAPIVARA;

// --- CONSTANTES DE DIFICULDADE ---
// Estes arrays armazenam valores específicos para cada nível de dificuldade(FACIL = 0, MEDIO = 1, DIFICIL = 2).

const int CAPIVARAS_POR_DIFICULDADE[] = {3, 4, 5};       // Número total de posições para capivaras por dificuldade.
const int MAX_SIMULTANEAS_INICIAL[] = {1, 2, 3};     // Máximo de capivaras visíveis simultaneamente no início do jogo.
const float INTERVALO_MIN[] = {1.5f, 1.0f, 1.0f};    // Intervalo mínimo de tempo para uma nova capivara aparecer.
const float INTERVALO_MAX[] = {3.0f, 2.0f, 1.5f};    // Intervalo máximo de tempo para uma nova capivara aparecer.
const float TEMPO_VISIVEL_MIN[] = {1.0f, 0.8f, 0.5f}; // Tempo mínimo que uma capivara permanece visível.
const float TEMPO_VISIVEL_MAX[] = {1.8f, 1.5f, 0.8f}; // Tempo máximo que uma capivara permanece visível.
const int PENALIDADE_CUTIA[] = {-1, -2, -3};         // Pontos perdidos ao acertar uma cutia, por dificuldade.

// --- VARIÁVEIS GLOBAIS DE TEXTURA ---
// As texturas são carregadas uma única vez na memória da GPU no início do programa para otimização.

Texture2D fundoJogo;                 // Textura para o fundo da tela de jogo.
Texture2D texturaCapivaraLevantada;  // Textura da capivara normal aparecendo.
Texture2D texturaAnimalAtordoado;    // Textura genérica para qualquer animal que foi acertado.
Texture2D texturaCutia;              // Textura da cutia.
Texture2D texturaChuva;              // Textura do efeito de chuva.
Texture2D texturaCapivaraDourada;    // Textura da capivara dourada.

// --- VARIÁVEIS GLOBAIS DE ESTADO DO JOGO ---
// Estas variáveis controlam o estado atual e os dados gerais do jogo.

Capivara* capivaras;             // Ponteiro para um array dinâmico de estruturas Capivara. Será alocado dinamicamente com base no número de capivaras por dificuldade.

Dificuldade dificuldadeAtual;    // Armazena a dificuldade selecionada para a partida atual.
double tempoRestanteJogo;        // Tempo restante da partida em segundos.
int pontos;                      // Pontuação atual do jogador.
bool jogoIniciado;               // true se o jogo começou após a contagem regressiva, false caso contrário.
bool jogoAcabou;                 // true se o tempo de jogo acabou, false caso contrário.
double contadorRegressivoInicial; // Contador para a contagem regressiva no início da partida.
double tempoChuvaAtual;          // Tempo restante para o efeito de chuva (em segundos).
bool chovendo;                   // true se o efeito de chuva está ativo, false caso contrário.
int totalCapivaras;              // O número total de capivaras nesta partida.

// --- DEFINIÇÕES DAS FUNÇÕES ---

// Função auxiliar para desenhar e gerenciar a interatividade de um botão genérico.

bool DesenharBotao(Rectangle rect, const char* texto, Vector2 mousePos) {
    Color cor = Fade(RED, 0.5f); // Cor padrão do botão (vermelho semi-transparente).
    bool clicado = false;        // Indicar se o botão foi clicado.

    // Verifica se a posição do mouse está sobre o retângulo do botão.
    if (CheckCollisionPointRec(mousePos, rect)) {
        cor = BROWN; // Se o mouse estiver em cima, a cor muda para marrom.
        // Verifica se o botão esquerdo do mouse foi pressionado.
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            clicado = true;
        }
    }
    DrawRectangleRec(rect, cor); // Desenha o retângulo do botão com a cor definida.
    // Desenha o texto do botão, centralizado horizontalmente e ajustado verticalmente.
    DrawText(texto, rect.x + (rect.width - MeasureText(texto, 30)) / 2, rect.y + 15, 30, WHITE);
    return clicado; // Retorna o estado do clique.
}

// Função responsável por inicializar todas as variáveis e estados para uma nova partida.
void InicializarJogo(Dificuldade dificuldade) {
    srand(time(NULL)); // Inicializa o gerador de números aleatórios. As capivaras apareçam em posições, tipos e tempos aleatorios
                       // 'time(NULL)' fornece uma semente diferente a cada execução, garantindo aleatoriedade.
    dificuldadeAtual = dificuldade; // Define a dificuldade escolhida para a partida.
    pontos = 0;                     // Reseta a pontuação.
    tempoRestanteJogo = 2 * 60;     // Define o tempo total de jogo para 2 minutos (120 segundos).
    jogoIniciado = false;           // O jogo não começa imediatamente, espera a contagem regressiva.
    jogoAcabou = false;             // O jogo não está acabado no início.
    contadorRegressivoInicial = 3.99; // Inicia a contagem regressiva (3, 2, 1, GO!). O .99 é para garantir que o "GO!" apareça no tempo certo.
    chovendo = false;               // A chuva não está ativa no início.
    tempoChuvaAtual = 0.0;          // Reseta o tempo da chuva.
    totalCapivaras = CAPIVARAS_POR_DIFICULDADE[dificuldade]; // Define o número de slots de capivara com base na dificuldade.

    // --- ALOCAÇÃO DINÂMICA DE MEMÓRIA PARA AS CAPIVARAS ---
    // É crucial liberar a memória antiga antes de alocar uma nova, especialmente ao reiniciar o jogo, para evitar vazamentos de memória.
    if (capivaras != NULL) { 
        free(capivaras);    
        capivaras = NULL;    
    }
    // Aloca um array de 'totalCapivaras' elementos do tipo Capivara.
    capivaras = (Capivara*)malloc(totalCapivaras * sizeof(Capivara));

    int larguraTela = GetScreenWidth(); // Obtém a largura atual da janela do jogo.
    int larguraCapivara = 150;          // Largura base para a textura da capivara.
    int alturaCapivara = 120;           // Altura base para a textura da capivara.
    int espacamentoCapivara = 50;               // Espaçamento horizontal entre cada capivara.
    // Calcula a largura total que todas as capivaras e seus espaçamentos ocuparão.
    int larguraTotal = totalCapivaras * larguraCapivara + (totalCapivaras - 1) * espacamentoCapivara; // Onde N-1 é o espaço entre elas
    // Calcula a posição X inicial para centralizar o bloco de capivaras na tela.
    int startX = (larguraTela - larguraTotal) / 2;
    int yPos = GetScreenHeight() / 2 + 120; // Posição Y fixa onde as capivaras aparecerão(divide a tela no meio)

    // Inicializa cada estrutura Capivara dentro do array alocado.
    for (int i = 0; i < totalCapivaras; i++) {
        // Define o retângulo de posição e tamanho para cada capivara.
        capivaras[i].rect = (Rectangle){ (float)(startX + i * (larguraCapivara + espacamentoCapivara)), (float)yPos, (float)larguraCapivara, (float)alturaCapivara };
        // Inicializa todos os outros campos da estrutura Capivara para seus valores padrão.
        // Isso garante que cada capivara comece invisível, não acertada, não machucada e do tipo NORMAL.
        capivaras[i] = (Capivara){capivaras[i].rect, false, 0.0, 0.0, false, 0.0, false, NORMAL};
    }
}

// Função responsável por toda a lógica de atualização do jogo a cada frame.
// Inclui contagem de tempo, aparecimento de capivaras, detecção de cliques, etc.
void AtualizarJogo() {
    if (jogoAcabou) // Se o jogo já terminou, não há mais lógica para atualizar.
        return;

    // Lógica para a contagem regressiva antes do início do jogo.
    if (!jogoIniciado) {
        contadorRegressivoInicial -= GetFrameTime(); // Decrementa o contador pelo tempo do último frame.
        if (contadorRegressivoInicial <= 0)         // Se o contador chegou a zero (ou menos).
            jogoIniciado = true;                    // O jogo é marcado como iniciado.
        return;                                     // Sai da função, não atualiza o jogo real ainda.
    }

    // Lógica principal do tempo de jogo.
    tempoRestanteJogo -= GetFrameTime(); // Decrementa o tempo restante do jogo.
    if (tempoRestanteJogo <= 0) {       // Se o tempo acabou.
        tempoRestanteJogo = 0;          // Garante que o tempo não fique negativo na exibição.
        jogoAcabou = true;              // Marca o jogo como terminado.
        return;                         // Sai da função, o jogo acabou.
    }

    // Lógica principal da chuva do jogo.
    if (chovendo) {                       // Se o estado de chuva está ativo.
        tempoChuvaAtual -= GetFrameTime(); // Decrementa o tempo restante da chuva.
        if (tempoChuvaAtual <= 0) {        // Se o tempo da chuva acabou.
            chovendo = false;              // Desativa o estado de chuva.
        }
    }

    // Obtém os parâmetros de dificuldade correspondentes à dificuldade atual.
    float intervaloMin = INTERVALO_MIN[dificuldadeAtual];
    float tempoVisivelMin = TEMPO_VISIVEL_MIN[dificuldadeAtual];
    float tempoVisivelMax = TEMPO_VISIVEL_MAX[dificuldadeAtual];
    int maxCapivarasSimultaneas = MAX_SIMULTANEAS_INICIAL[dificuldadeAtual];
    
    int capivarasVisiveisAtualmente = 0; // Contador para saber quantas capivaras estão visíveis.

    // Loop para iterar sobre cada capivara (ou slot) e atualizar seu estado.
    for (int i = 0; i < totalCapivaras; i++) {
        // Conta as capivaras que estão visíveis e não estão no estado de "atordoadas".
        if (capivaras[i].visivel && !capivaras[i].machucada)
            capivarasVisiveisAtualmente++;

        // Lógica para capivaras que foram acertadas (estado "atordoada").
        if (capivaras[i].machucada) {
            capivaras[i].tempoAcertada += GetFrameTime(); // Incrementa o tempo que o animal está machucado.
            if (capivaras[i].tempoAcertada >= 0.5) {      // Após 0.5 segundos (tempo da animação de atordoado).
                // Reseta o estado da capivara: invisível, não acertada, não machucada, e volta ao tipo NORMAL para a próxima aparição.
                capivaras[i] = (Capivara){capivaras[i].rect, false, 0.0, 0.0, false, 0.0, false, NORMAL};
            }
            continue; // Pula para a próxima capivara no loop, pois esta já foi tratada neste frame.
        }

        // Lógica para capivaras que estão visíveis (mas não machucadas) ou para fazer novas aparecerem.
        if (capivaras[i].visivel) {
            capivaras[i].tempoVisivel += GetFrameTime(); // Incrementa o tempo que a capivara está visível.
            // Se o tempo de visibilidade total foi atingido.
            if (capivaras[i].tempoVisivel >= capivaras[i].tempoTotalVisivel) {
                capivaras[i].visivel = false; // Torna a capivara invisível.
                capivaras[i].tipo = NORMAL;   // Reseta seu tipo para NORMAL para a próxima vez que aparecer.
            }
        }
        // Condição para fazer uma nova capivara aparecer:
        // 1. O slot atual não está visível.
        // 2. O número de capivaras visíveis é menor que o máximo permitido.
        // 3. NÃO ESTÁ CHOVENDO (capivaras normais/douradas não aparecem na chuva).
        else if (capivarasVisiveisAtualmente < maxCapivarasSimultaneas && !chovendo) {
            // Lógica para determinar a chance de uma nova capivara aparecer.
            // GetRandomValue(0, 1000) cria um "tick"(probabilidade acumulada para um evento ocorrer em um determinado frame) para cada milissegundo de FrameTime,
            // tornando a chance de aparição mais consistente independente do FPS.
            if (GetRandomValue(0, 1000) < (int)(GetFrameTime() * 1000 / intervaloMin)) {
                TipoCapivara novoTipo = NORMAL; // Define o tipo padrão como NORMAL.
                int chance = GetRandomValue(1, 100); // Gera um número aleatório para determinar o tipo do animal.

                // Lógica de sorteio de tipo de capivara baseada na dificuldade.
                switch(dificuldadeAtual) {
                    case FACIL:    
                        if (chance <= 10) // 10% de chance de aparecer uma CUTIA.
                            novoTipo = CUTIA; break;
                    case MEDIO:    
                        if (chance <= 15) // 15% de chance de aparecer uma CUTIA.
                            novoTipo = CUTIA;
                        else if (chance <= 18) // 3% de chance de aparecer uma DOURADA (18 - 15 = 3).
                            novoTipo = DOURADA; break;
                    case DIFICIL:
                        if (chance <= 20) // 20% de chance de aparecer uma CUTIA.
                            novoTipo = CUTIA;
                        else if (chance <= 25) // 5% de chance de aparecer uma DOURADA (25 - 20 = 5).
                            novoTipo = DOURADA; break;
                }

                capivaras[i].visivel = true;     // Torna a capivara neste slot visível.
                capivaras[i].tempoVisivel = 0.0; // Reseta o tempo visível dela.
                // Define um tempo total de visibilidade aleatório, dentro dos limites da dificuldade.
                capivaras[i].tempoTotalVisivel = (double)GetRandomValue((int)(tempoVisivelMin * 1000), (int)(tempoVisivelMax * 1000)) / 1000.0;
                capivaras[i].hit = false;        // Garante que ela não esteja marcada como acertada.
                capivaras[i].tipo = novoTipo;    // Atribui o tipo de capivara sorteado.
            }
        }

        // --- DETECÇÃO DE CLIQUE DO JOGADOR ---
        // Verifica se:
        // 1. A capivara está visível.
        // 2. Não foi acertada ainda neste aparecimento.
        // 3. O botão esquerdo do mouse foi pressionado neste frame.
        // 4. A posição do mouse colide com o retângulo da capivara.
        if (capivaras[i].visivel && !capivaras[i].hit && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), capivaras[i].rect)) {
            capivaras[i].hit = true;        // Marca a capivara como acertada.
            capivaras[i].machucada = true;  // Inicia o estado de "atordoada"
            capivaras[i].tempoAcertada = 0.0; // Reseta o contador para o tempo de "atordoada".

            // Lógica para adicionar ou subtrair pontos com base no tipo de animal acertado.
            switch(capivaras[i].tipo) {
                case NORMAL:
                    pontos += 1; // Capivara normal: +1 ponto.
                break;
                case DOURADA:
                    pontos += 2; // Capivara dourada: +2 pontos.
                break;
                case CUTIA:
                    pontos += PENALIDADE_CUTIA[dificuldadeAtual]; // Cutia: perde pontos conforme a dificuldade.
                    if(pontos < 0) // Garante que a pontuação nunca seja negativa.
                        pontos = 0;
                    if(dificuldadeAtual == DIFICIL){
                        chovendo = true;      // Ativa o estado de chuva.
                        tempoChuvaAtual = 5.0; // Define que a chuva durará 5 segundos.
                    }
                break;
            }
        }
    }
}

// Função responsável por desenhar todos os elementos visuais do jogo na tela.
void DesenharJogo() {
    // Desenha o fundo do jogo. A função DrawTexturePro é usada para redimensionar a textura
    // para preencher toda a tela, independentemente do tamanho original da imagem.
    DrawTexturePro(fundoJogo, (Rectangle){0,0, (float)fundoJogo.width, (float)fundoJogo.height}, (Rectangle){0,0, (float)GetScreenWidth(), (float)GetScreenHeight()}, (Vector2){0,0}, 0.0f, WHITE);
    
    // Desenha a textura da chuva se o estado 'chovendo' for true.
    if (chovendo)
        DrawTexture(texturaChuva, 0, 0, WHITE); // Desenha a textura de chuva na posição (0,0).

    // Loop para desenhar cada capivara (ou animal) na tela.
    for (int i = 0; i < totalCapivaras; i++) {
        // Se a capivara não está visível e não está machucada, não há necessidade de desenhá-la.
        if (!capivaras[i].visivel && !capivaras[i].machucada)
            continue; // Pula para a próxima iteração do loop.

        Texture2D* texAtual = &texturaCapivaraLevantada; // Por padrão, usa a textura da capivara normal.
        // Seleciona a textura correta com base no tipo de animal.
        if (capivaras[i].tipo == DOURADA) {
            texAtual = &texturaCapivaraDourada; // Se for dourada, usa a textura da capivara dourada.
        } else if (capivaras[i].tipo == CUTIA) {
            texAtual = &texturaCutia; // Se for cutia, usa a textura da cutia.
        }
        
        Rectangle destRec = capivaras[i].rect; // O retângulo de destino onde a textura será desenhada.
        
        // Ajusta o tamanho da cutia ao desenhar.
        if (capivaras[i].tipo == CUTIA) {
            float scale = 0.9f; // Fator de escala: 90% do tamanho do slot da capivara.
            destRec.width *= scale;  // Reduz a largura.
            destRec.height *= scale; // Reduz a altura.
            // Centraliza a cutia dentro do espaço original do slot (capivaras[i].rect).
            destRec.x += (capivaras[i].rect.width - destRec.width) / 2;
            destRec.y += (capivaras[i].rect.height - destRec.height) / 2;
        }

        // Desenha a textura apropriada: atordoada se machucada, ou a textura normal/dourada/cutia se visível.
        if (capivaras[i].machucada) {
            // Se estiver machucada, sempre desenha a textura de atordoado.
            DrawTexturePro(texturaAnimalAtordoado, (Rectangle){0,0, (float)texturaAnimalAtordoado.width, (float)texturaAnimalAtordoado.height}, destRec, (Vector2){0,0}, 0.0f, WHITE);
        } else if (capivaras[i].visivel) {
            // Se estiver visível e não machucada, desenha a textura do tipo de animal.
            DrawTexturePro(*texAtual, (Rectangle){0,0, (float)texAtual->width, (float)texAtual->height}, destRec, (Vector2){0,0}, 0.0f, WHITE);
        }
    }

    // --- UI (Interface do Usuário) do Jogo ---

    // Desenha o tempo restante no canto superior direito, formatado como "MM:SS".
    DrawText(TextFormat("Tempo: %02d:%02d", (int)tempoRestanteJogo / 60, (int)tempoRestanteJogo % 60), GetScreenWidth() - 200, 20, 30, WHITE);
    // Desenha a pontuação no canto superior esquerdo.
    DrawText(TextFormat("Pontos: %d", pontos), 20, 20, 30, WHITE);
    
    // Desenha o botão de PAUSE no centro superior da tela.
    Rectangle btnPausaRect = {GetScreenWidth() / 2.0f - 60, 20, 120, 40};
    // A cor do botão muda ao passar o mouse.
    DrawRectangleRec(btnPausaRect, CheckCollisionPointRec(GetMousePosition(), btnPausaRect) ? BROWN : Fade(RED, 0.5f));
    DrawText("PAUSE", btnPausaRect.x + 25, btnPausaRect.y + 10, 20, WHITE);

    // --- Contagem Regressiva Inicial ---
    // Exibe a contagem regressiva antes do jogo realmente começar.
    if (!jogoIniciado) {
        const char* textoContador = "";
        if (contadorRegressivoInicial > 1) // Se o contador é maior que 1 (ex: 3, 2).
            textoContador = TextFormat("%d", (int)contadorRegressivoInicial); // Exibe o número inteiro.
        else if (contadorRegressivoInicial > 0) // Se o contador está entre 0 e 1 (quase no fim).
            textoContador = "GO!"; // Exibe "GO!".
        
        // Desenha o texto da contagem regressiva, centralizado na tela.
        if (strlen(textoContador) > 0) { // Garante que só desenhe se houver texto.
            DrawText(textoContador, GetScreenWidth() / 2 - MeasureText(textoContador, 100) / 2, GetScreenHeight() / 2 - 50, 100, GOLD);
        }
    }
    
    // --- Tela de Fim de Jogo (Overlay) ---
    // É desenhada sobre o resto do jogo quando 'jogoAcabou' é true.
    if (jogoAcabou) {
        // Desenha um retângulo semi-transparente que escurece a tela, dando um efeito de overlay.
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.7f));
        const char* fimTexto = "Fim de jogo!";
        const char* pontuacaoFinalTexto = TextFormat("Pontuação final: %d", pontos);
        
        // Desenha as mensagens de "Fim de jogo!" e a pontuação final, centralizadas.
        DrawText(fimTexto, GetScreenWidth() / 2 - MeasureText(fimTexto, 60) / 2, GetScreenHeight() / 2 - 100, 60, WHITE);
        DrawText(pontuacaoFinalTexto, GetScreenWidth() / 2 - MeasureText(pontuacaoFinalTexto, 40) / 2, GetScreenHeight() / 2 - 30, 40, WHITE);
    }
}

// --- FUNÇÃO PRINCIPAL ---
// O ponto de entrada do programa. Aqui o jogo é inicializado e o loop principal é executado.
int main(void) {
    const int larguraTela = 1500; // Define a largura da janela do jogo.
    const int alturaTela = 800;   // Define a altura da janela do jogo.
    InitWindow(larguraTela, alturaTela, "Barigueira Attack!"); // Inicializa a janela da Raylib com título.
    SetTargetFPS(60); // Define o limite de quadros por segundo (FPS) para 60, para uma experiência de jogo suave.

    // --- CARREGAMENTO INICIAL DE TEXTURAS ---
    // Todas as texturas são carregadas, para evitar atrasos durante o jogo e otimizar o uso da memória.
    fundoJogo = LoadTexture("fundoJogo.png");                 // Carrega a imagem de fundo do jogo.
    texturaCapivaraLevantada = LoadTexture("capivaraNormal.png"); // Carrega a imagem da capivara comum.
    texturaCapivaraDourada = LoadTexture("capivaraDourada.png"); // Carrega a imagem da capivara dourada.
    texturaAnimalAtordoado = LoadTexture("animalAtordoado.png"); // Carrega a imagem do animal atordoado (acertado).
    texturaCutia = LoadTexture("cutiaJogo.png");                 // Carrega a imagem da cutia.
    texturaChuva = LoadTexture("fundoChuvoso.png");             // Carrega a imagem do efeito de chuva.
    Texture2D fundoPrincipal = LoadTexture("fundoPrincipal.png"); // Fundo da tela inicial.
    Texture2D fundoMenu = LoadTexture("fundoMenu.png");           // Fundo do menu principal e seleção de dificuldade.
    Texture2D fundoCreditos = LoadTexture("fundoCreditos.png");   // Fundo da tela de créditos.

    Tela telaAtual = TELA_INICIAL; // Define o estado inicial do jogo para a tela de abertura.

    // Inicializa o ponteiro como NULL para que a função `free(capivaras)` não tente, liberar memória de um ponteiro não inicializado, o que causaria um erro.
    capivaras = NULL;

    // --- LOOP PRINCIPAL DO JOGO ---
    // 'WindowShouldClose()' retorna true quando o usuário clica no 'X' da janela ou pressiona ESC.
    
    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition(); // Obtém a posição atual do cursor do mouse a cada frame.

        BeginDrawing(); // Inicia o modo de desenho da Raylib para este frame.
        ClearBackground(BEIGE); // Limpa a tela com uma cor base (útil se não houver um fundo ocupando 100%).

        // --- MÁQUINA DE ESTADOS ---
        // Um switch-case é usado para gerenciar as diferentes telas (estados) do jogo.
        // A lógica e o desenho executados dependem da 'telaAtual'.
        switch (telaAtual) {
            case TELA_INICIAL: {
                DrawTexture(fundoPrincipal, 0, 0, WHITE); // Desenha o fundo da tela inicial.
                const char* titulo = "BARIGUEIRA ATTACK!";
                // Desenha o título do jogo, centralizado.
                DrawText(titulo, larguraTela / 2 - MeasureText(titulo, 80) / 2, alturaTela / 2 - 150, 80, BEIGE);
                // Desenha o botão "INICIAR". Se clicado, muda para a tela de menu.
                if (DesenharBotao((Rectangle){larguraTela / 2 - 150, 490, 300, 60}, "INICIAR", mouse)) {
                    telaAtual = TELA_MENU;
                }
            } break; 

            case TELA_MENU: {
                DrawTexture(fundoMenu, 0, 0, WHITE); // Desenha o fundo do menu.
                const char* titulo = "MENU DO JOGO";
                // Desenha o título do menu, centralizado.
                DrawText(titulo, larguraTela / 2 - MeasureText(titulo, 60) / 2, 150, 60, BEIGE);
                // Botão "JOGAR": Se clicado, vai para a seleção de dificuldade.
                if (DesenharBotao((Rectangle){larguraTela / 2 - 150, 300, 300, 60}, "JOGAR", mouse)) {
                    telaAtual = TELA_SELECAO_DIFICULDADE;
                }
                // Botão "CRÉDITOS": Se clicado, vai para a tela de créditos.
                if (DesenharBotao((Rectangle){larguraTela / 2 - 150, 385, 300, 60}, "CRÉDITOS", mouse)) {
                    telaAtual = TELA_CREDITOS;
                }
                // Botão "SAIR": Se clicado, fecha a janela do jogo, encerrando o loop principal.
                if (DesenharBotao((Rectangle){larguraTela / 2 - 150, 470, 300, 60}, "SAIR", mouse)) {
                    CloseWindow();
                }
            } break; 

            case TELA_SELECAO_DIFICULDADE: {
                DrawTexture(fundoMenu, 0, 0, WHITE); // Usa o mesmo fundo do menu.
                const char* titulo = "SELECIONE A DIFICULDADE";
                // Desenha o título da tela de seleção, centralizado.
                DrawText(titulo, larguraTela / 2 - MeasureText(titulo, 60) / 2, 150, 60, BEIGE);

                // Botão "FÁCIL": Inicializa o jogo com a dificuldade FACIL e transiciona para a tela de jogo.
                if (DesenharBotao((Rectangle){larguraTela / 2 - 150, 300, 300, 60}, "FÁCIL", mouse)) {
                    InicializarJogo(FACIL);
                    telaAtual = TELA_JOGO;
                }
                // Botão "MÉDIO": Inicializa o jogo com a dificuldade MEDIO.
                if (DesenharBotao((Rectangle){larguraTela / 2 - 150, 385, 300, 60}, "MÉDIO", mouse)) {
                    InicializarJogo(MEDIO);
                    telaAtual = TELA_JOGO;
                }
                // Botão "DIFÍCIL": Inicializa o jogo com a dificuldade DIFICIL.
                if (DesenharBotao((Rectangle){larguraTela / 2 - 150, 470, 300, 60}, "DIFÍCIL", mouse)) {
                    InicializarJogo(DIFICIL);
                    telaAtual = TELA_JOGO;
                }
                // Botão "VOLTAR": Retorna ao menu principal.
                if (DesenharBotao((Rectangle){larguraTela / 2 - 150, 555, 300, 60}, "VOLTAR", mouse)) {
                    telaAtual = TELA_MENU;
                }
            } break; 

            case TELA_JOGO: {
                // A lógica do jogo só é atualizada se o jogo ainda não acabou.
                if (!jogoAcabou) {
                    AtualizarJogo(); // Chama a função que contém toda a lógica do jogo.
                    // Verifica se o botão de pause foi clicado
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, (Rectangle){larguraTela / 2.0f - 60, 20, 120, 40})) {
                        telaAtual = TELA_PAUSA; // Se clicado, transiciona para a tela de pausa.
                    }
                }
                DesenharJogo(); // Chama a função que desenha todos os elementos visuais do jogo (incluindo o overlay de fim de jogo se aplicável).

                // Botões específicos que só aparecem quando o jogo termina.
                if (jogoAcabou) {
                    // Botão "REINICIAR": Reinicia o jogo com a mesma dificuldade.
                    if (DesenharBotao((Rectangle){larguraTela / 2.0f - 260, alturaTela / 2.0f + 50, 250, 60}, "REINICIAR", mouse)) {
                        InicializarJogo(dificuldadeAtual); // Chama a função de inicialização novamente.
                    }
                    // Botão "MENU INICIAL": Volta ao menu principal.
                    if (DesenharBotao((Rectangle){larguraTela / 2.0f + 10, alturaTela / 2.0f + 50, 250, 60}, "MENU INICIAL", mouse)) {
                        // Antes de voltar ao menu principal, é necessario liberar a memória alocada para as capivaras.
                        if (capivaras != NULL) {
                            free(capivaras);     // Libera o bloco de memória.
                            capivaras = NULL;    // Define o ponteiro para NULL para evitar acesso a memória já liberada
                        }
                        telaAtual = TELA_MENU; // Transiciona para o menu.
                    }
                }
            } break;

            case TELA_PAUSA: {
                DesenharJogo(); // Desenha o jogo em segundo plano (como estava antes de pausar).
                // Desenha um retângulo semi-transparente para criar um efeito de escurecimento sobre o jogo pausado.
                DrawRectangle(0, 0, larguraTela, alturaTela, ColorAlpha(BLACK, 0.5f));
                const char* textoPausa = "JOGO PAUSADO";
                // Desenha o texto "JOGO PAUSADO", centralizado.
                DrawText(textoPausa, larguraTela / 2 - MeasureText(textoPausa, 60) / 2, 150, 60, BEIGE);

                // Botão "CONTINUAR": Despausa o jogo. Pode ser clicado ou pressionando ESC.
                if (DesenharBotao((Rectangle){larguraTela / 2.0f - 150, 300, 300, 60}, "CONTINUAR", mouse) || IsKeyPressed(KEY_ESCAPE)) {
                    telaAtual = TELA_JOGO; // Retorna à tela de jogo.
                }
                // Botão "REINICIAR": Reinicia a partida atual.
                if (DesenharBotao((Rectangle){larguraTela / 2.0f - 150, 385, 300, 60}, "REINICIAR", mouse)) {
                    InicializarJogo(dificuldadeAtual); // Re-inicializa o jogo.
                    telaAtual = TELA_JOGO;
                }
                // Botão "MENU PRINCIPAL": Volta ao menu.
                if (DesenharBotao((Rectangle){larguraTela / 2.0f - 150, 470, 300, 60}, "MENU PRINCIPAL", mouse)) {
                    // Libera a memória das capivaras antes de sair do jogo para o menu.
                    if (capivaras != NULL) {
                        free(capivaras);
                        capivaras = NULL;
                    }
                    telaAtual = TELA_MENU;
                }
            } break; 

            case TELA_CREDITOS: {
                DrawTexture(fundoCreditos, 0, 0, WHITE); // Desenha o fundo da tela de créditos.
                const char* titulo = "CRÉDITOS";
                // Desenha o título "CRÉDITOS", centralizado.
                DrawText(titulo, larguraTela / 2 - MeasureText(titulo, 60) / 2, 80, 60, BEIGE);

                // Array de strings contendo todo o texto dos créditos.
                const char* creditosTexto[] = {
                    "ALUNOS:",
                    "ANA WALTRICK | ÂNGELO MIRANDA | VITOR KLUPPELL",
                    "", 
                    "INSTITUIÇÃO: CENTRO UNIVERSITÁRIO AUTÔNOMO DO BRASIL",
                    "DISCIPLINA: PROGRAMAÇÃO AVANÇADA",
                    "PROFESSOR: FABIO BETTIO",
                    "",
                    "INSPIRAÇÃO:",
                    "JOGO WHACK-A-MOLE | CURITIBA | PARQUE BARIGUI | ANIMAIS",
                    "",
                    "OBRIGADO POR JOGAR!"
                };
                int yPos = 180; // Posição Y inicial para a primeira linha de crédito.
                // Loop para desenhar cada linha de texto dos créditos.
                for (int i = 0; i < sizeof(creditosTexto)/sizeof(creditosTexto[0]); i++) {
                    // Desenha o texto, centralizado horizontalmente.
                    DrawText(creditosTexto[i], larguraTela / 2 - MeasureText(creditosTexto[i], 28) / 2, yPos, 28, BEIGE);
                    yPos += 40; // Incrementa a posição Y para a próxima linha.
                }

                // Botão "VOLTAR": Retorna ao menu principal.
                if (DesenharBotao((Rectangle){larguraTela / 2.0f - 150, alturaTela - 100, 300, 60}, "VOLTAR", mouse)) {
                    telaAtual = TELA_MENU;
                }
            } break; 
        }
        EndDrawing(); // Finaliza o modo de desenho, apresentando o frame renderizado na tela.
    }

    // --- DESALOCAÇÃO FINAL DA MEMÓRIA E RECURSOS AO FECHAR A JANELA ---
    // Liberar todos os recursos alocados para evitar vazamentos de memória e garantir um encerramento limpo do programa.

    if (capivaras != NULL) {
        free(capivaras); 
        capivaras = NULL; 
    }

    // Descarrega todas as texturas da memória.
    UnloadTexture(fundoPrincipal);
    UnloadTexture(fundoMenu);
    UnloadTexture(fundoCreditos);
    UnloadTexture(fundoJogo);
    UnloadTexture(texturaCapivaraLevantada);
    UnloadTexture(texturaAnimalAtordoado);
    UnloadTexture(texturaCutia);
    UnloadTexture(texturaChuva);
    UnloadTexture(texturaCapivaraDourada);

    CloseWindow(); // Fecha a janela da Raylib e libera seus recursos internos.
    return 0;      // Retorna 0 para indicar que o programa foi executado com sucesso.
}

