#include "raylib.h"
#include <stdbool.h> 
#include <string.h> 
#include <stdio.h>
#include <stdlib.h> // Fornece fun��es utilit�rias como aloca��o de mem�ria din�mica, usado para gerenciar as capivaras.
#include <time.h> // Permite manipular e medir tempo, al�m de inicializar o gerador de n�meros aleat�rios.

// ENUMS
typedef enum Tela {
    TELA_INICIAL,              // Tela de abertura do jogo
    TELA_MENU,                 // Menu principal
    TELA_CREDITOS,             // Tela que exibe os nomes dos criadores e informa��es
    TELA_JOGO,                 // Tela onde o jogo acontece
    TELA_SELECAO_DIFICULDADE,  // Tela para o jogador escolher o n�vel de dificuldade.
    TELA_PAUSA                 // Tela exibida quando o jogo est� pausado.
} Tela;

typedef enum Dificuldade {
    FACIL,  
    MEDIO, 
    DIFICIL
} Dificuldade;

typedef enum TipoCapivara {
    NORMAL,  // Capivara comum, d� pontos positivos.
    DOURADA, // Capivara dourada, d� mais pontos.
    CUTIA    // Cutia resulta em penalidade de pontos e ativa o efeito de chuva no modo dificil.
} TipoCapivara;

// STRUCTS
typedef struct {
    Rectangle rect;          // Posi��o e tamanho do animal na tela (x, y, largura, altura). Usado para desenho e detec��o de colis�o.
    bool visivel;            // Indica se o animal est� atualmente vis�vel para ser clicado.
    double tempoVisivel;     // Tempo decorrido desde que o animal ficou vis�vel (para controle de dura��o).
    double tempoTotalVisivel; // Tempo total que o animal deve permanecer vis�vel antes de desaparecer.
    bool hit;                // Indica se o animal foi clicado (acertado).
    double tempoAcertada;    // Tempo decorrido desde que o animal foi acertado (para anima��o de "atordoado").
    bool machucada;          // Indica se o animal est� no estado de "atordoado" ap�s ser acertado.
    TipoCapivara tipo;       // O tipo espec�fico do animal (NORMAL, DOURADA, CUTIA), para determinar a textura e a pontua��o.
}CAPIVARA;

// --- CONSTANTES DE DIFICULDADE ---
// Estes arrays armazenam valores espec�ficos para cada n�vel de dificuldade(FACIL = 0, MEDIO = 1, DIFICIL = 2).

const int CAPIVARAS_POR_DIFICULDADE[] = {3, 4, 5};       // N�mero total de posi��es para capivaras por dificuldade.
const int MAX_SIMULTANEAS_INICIAL[] = {1, 2, 3};     // M�ximo de capivaras vis�veis simultaneamente no in�cio do jogo.
const float INTERVALO_MIN[] = {1.5f, 1.0f, 1.0f};    // Intervalo m�nimo de tempo para uma nova capivara aparecer.
const float INTERVALO_MAX[] = {3.0f, 2.0f, 1.5f};    // Intervalo m�ximo de tempo para uma nova capivara aparecer.
const float TEMPO_VISIVEL_MIN[] = {1.0f, 0.8f, 0.5f}; // Tempo m�nimo que uma capivara permanece vis�vel.
const float TEMPO_VISIVEL_MAX[] = {1.8f, 1.5f, 0.8f}; // Tempo m�ximo que uma capivara permanece vis�vel.
const int PENALIDADE_CUTIA[] = {-1, -2, -3};         // Pontos perdidos ao acertar uma cutia, por dificuldade.

// --- VARI�VEIS GLOBAIS DE TEXTURA ---
// As texturas s�o carregadas uma �nica vez na mem�ria da GPU no in�cio do programa para otimiza��o.

Texture2D fundoJogo;                 // Textura para o fundo da tela de jogo.
Texture2D texturaCapivaraLevantada;  // Textura da capivara normal aparecendo.
Texture2D texturaAnimalAtordoado;    // Textura gen�rica para qualquer animal que foi acertado.
Texture2D texturaCutia;              // Textura da cutia.
Texture2D texturaChuva;              // Textura do efeito de chuva.
Texture2D texturaCapivaraDourada;    // Textura da capivara dourada.

// --- VARI�VEIS GLOBAIS DE ESTADO DO JOGO ---
// Estas vari�veis controlam o estado atual e os dados gerais do jogo.

Capivara* capivaras;             // Ponteiro para um array din�mico de estruturas Capivara. Ser� alocado dinamicamente com base no n�mero de capivaras por dificuldade.

Dificuldade dificuldadeAtual;    // Armazena a dificuldade selecionada para a partida atual.
double tempoRestanteJogo;        // Tempo restante da partida em segundos.
int pontos;                      // Pontua��o atual do jogador.
bool jogoIniciado;               // true se o jogo come�ou ap�s a contagem regressiva, false caso contr�rio.
bool jogoAcabou;                 // true se o tempo de jogo acabou, false caso contr�rio.
double contadorRegressivoInicial; // Contador para a contagem regressiva no in�cio da partida.
double tempoChuvaAtual;          // Tempo restante para o efeito de chuva (em segundos).
bool chovendo;                   // true se o efeito de chuva est� ativo, false caso contr�rio.
int totalCapivaras;              // O n�mero total de capivaras nesta partida.

// --- DEFINI��ES DAS FUN��ES ---

// Fun��o auxiliar para desenhar e gerenciar a interatividade de um bot�o gen�rico.

bool DesenharBotao(Rectangle rect, const char* texto, Vector2 mousePos) {
    Color cor = Fade(RED, 0.5f); // Cor padr�o do bot�o (vermelho semi-transparente).
    bool clicado = false;        // Indicar se o bot�o foi clicado.

    // Verifica se a posi��o do mouse est� sobre o ret�ngulo do bot�o.
    if (CheckCollisionPointRec(mousePos, rect)) {
        cor = BROWN; // Se o mouse estiver em cima, a cor muda para marrom.
        // Verifica se o bot�o esquerdo do mouse foi pressionado.
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            clicado = true;
        }
    }
    DrawRectangleRec(rect, cor); // Desenha o ret�ngulo do bot�o com a cor definida.
    // Desenha o texto do bot�o, centralizado horizontalmente e ajustado verticalmente.
    DrawText(texto, rect.x + (rect.width - MeasureText(texto, 30)) / 2, rect.y + 15, 30, WHITE);
    return clicado; // Retorna o estado do clique.
}

// Fun��o respons�vel por inicializar todas as vari�veis e estados para uma nova partida.
void InicializarJogo(Dificuldade dificuldade) {
    srand(time(NULL)); // Inicializa o gerador de n�meros aleat�rios. As capivaras apare�am em posi��es, tipos e tempos aleatorios
                       // 'time(NULL)' fornece uma semente diferente a cada execu��o, garantindo aleatoriedade.
    dificuldadeAtual = dificuldade; // Define a dificuldade escolhida para a partida.
    pontos = 0;                     // Reseta a pontua��o.
    tempoRestanteJogo = 2 * 60;     // Define o tempo total de jogo para 2 minutos (120 segundos).
    jogoIniciado = false;           // O jogo n�o come�a imediatamente, espera a contagem regressiva.
    jogoAcabou = false;             // O jogo n�o est� acabado no in�cio.
    contadorRegressivoInicial = 3.99; // Inicia a contagem regressiva (3, 2, 1, GO!). O .99 � para garantir que o "GO!" apare�a no tempo certo.
    chovendo = false;               // A chuva n�o est� ativa no in�cio.
    tempoChuvaAtual = 0.0;          // Reseta o tempo da chuva.
    totalCapivaras = CAPIVARAS_POR_DIFICULDADE[dificuldade]; // Define o n�mero de slots de capivara com base na dificuldade.

    // --- ALOCA��O DIN�MICA DE MEM�RIA PARA AS CAPIVARAS ---
    // � crucial liberar a mem�ria antiga antes de alocar uma nova, especialmente ao reiniciar o jogo, para evitar vazamentos de mem�ria.
    if (capivaras != NULL) { 
        free(capivaras);    
        capivaras = NULL;    
    }
    // Aloca um array de 'totalCapivaras' elementos do tipo Capivara.
    capivaras = (Capivara*)malloc(totalCapivaras * sizeof(Capivara));

    int larguraTela = GetScreenWidth(); // Obt�m a largura atual da janela do jogo.
    int larguraCapivara = 150;          // Largura base para a textura da capivara.
    int alturaCapivara = 120;           // Altura base para a textura da capivara.
    int espacamentoCapivara = 50;               // Espa�amento horizontal entre cada capivara.
    // Calcula a largura total que todas as capivaras e seus espa�amentos ocupar�o.
    int larguraTotal = totalCapivaras * larguraCapivara + (totalCapivaras - 1) * espacamentoCapivara; // Onde N-1 � o espa�o entre elas
    // Calcula a posi��o X inicial para centralizar o bloco de capivaras na tela.
    int startX = (larguraTela - larguraTotal) / 2;
    int yPos = GetScreenHeight() / 2 + 120; // Posi��o Y fixa onde as capivaras aparecer�o(divide a tela no meio)

    // Inicializa cada estrutura Capivara dentro do array alocado.
    for (int i = 0; i < totalCapivaras; i++) {
        // Define o ret�ngulo de posi��o e tamanho para cada capivara.
        capivaras[i].rect = (Rectangle){ (float)(startX + i * (larguraCapivara + espacamentoCapivara)), (float)yPos, (float)larguraCapivara, (float)alturaCapivara };
        // Inicializa todos os outros campos da estrutura Capivara para seus valores padr�o.
        // Isso garante que cada capivara comece invis�vel, n�o acertada, n�o machucada e do tipo NORMAL.
        capivaras[i] = (Capivara){capivaras[i].rect, false, 0.0, 0.0, false, 0.0, false, NORMAL};
    }
}

// Fun��o respons�vel por toda a l�gica de atualiza��o do jogo a cada frame.
// Inclui contagem de tempo, aparecimento de capivaras, detec��o de cliques, etc.
void AtualizarJogo() {
    if (jogoAcabou) // Se o jogo j� terminou, n�o h� mais l�gica para atualizar.
        return;

    // L�gica para a contagem regressiva antes do in�cio do jogo.
    if (!jogoIniciado) {
        contadorRegressivoInicial -= GetFrameTime(); // Decrementa o contador pelo tempo do �ltimo frame.
        if (contadorRegressivoInicial <= 0)         // Se o contador chegou a zero (ou menos).
            jogoIniciado = true;                    // O jogo � marcado como iniciado.
        return;                                     // Sai da fun��o, n�o atualiza o jogo real ainda.
    }

    // L�gica principal do tempo de jogo.
    tempoRestanteJogo -= GetFrameTime(); // Decrementa o tempo restante do jogo.
    if (tempoRestanteJogo <= 0) {       // Se o tempo acabou.
        tempoRestanteJogo = 0;          // Garante que o tempo n�o fique negativo na exibi��o.
        jogoAcabou = true;              // Marca o jogo como terminado.
        return;                         // Sai da fun��o, o jogo acabou.
    }

    // L�gica principal da chuva do jogo.
    if (chovendo) {                       // Se o estado de chuva est� ativo.
        tempoChuvaAtual -= GetFrameTime(); // Decrementa o tempo restante da chuva.
        if (tempoChuvaAtual <= 0) {        // Se o tempo da chuva acabou.
            chovendo = false;              // Desativa o estado de chuva.
        }
    }

    // Obt�m os par�metros de dificuldade correspondentes � dificuldade atual.
    float intervaloMin = INTERVALO_MIN[dificuldadeAtual];
    float tempoVisivelMin = TEMPO_VISIVEL_MIN[dificuldadeAtual];
    float tempoVisivelMax = TEMPO_VISIVEL_MAX[dificuldadeAtual];
    int maxCapivarasSimultaneas = MAX_SIMULTANEAS_INICIAL[dificuldadeAtual];
    
    int capivarasVisiveisAtualmente = 0; // Contador para saber quantas capivaras est�o vis�veis.

    // Loop para iterar sobre cada capivara (ou slot) e atualizar seu estado.
    for (int i = 0; i < totalCapivaras; i++) {
        // Conta as capivaras que est�o vis�veis e n�o est�o no estado de "atordoadas".
        if (capivaras[i].visivel && !capivaras[i].machucada)
            capivarasVisiveisAtualmente++;

        // L�gica para capivaras que foram acertadas (estado "atordoada").
        if (capivaras[i].machucada) {
            capivaras[i].tempoAcertada += GetFrameTime(); // Incrementa o tempo que o animal est� machucado.
            if (capivaras[i].tempoAcertada >= 0.5) {      // Ap�s 0.5 segundos (tempo da anima��o de atordoado).
                // Reseta o estado da capivara: invis�vel, n�o acertada, n�o machucada, e volta ao tipo NORMAL para a pr�xima apari��o.
                capivaras[i] = (Capivara){capivaras[i].rect, false, 0.0, 0.0, false, 0.0, false, NORMAL};
            }
            continue; // Pula para a pr�xima capivara no loop, pois esta j� foi tratada neste frame.
        }

        // L�gica para capivaras que est�o vis�veis (mas n�o machucadas) ou para fazer novas aparecerem.
        if (capivaras[i].visivel) {
            capivaras[i].tempoVisivel += GetFrameTime(); // Incrementa o tempo que a capivara est� vis�vel.
            // Se o tempo de visibilidade total foi atingido.
            if (capivaras[i].tempoVisivel >= capivaras[i].tempoTotalVisivel) {
                capivaras[i].visivel = false; // Torna a capivara invis�vel.
                capivaras[i].tipo = NORMAL;   // Reseta seu tipo para NORMAL para a pr�xima vez que aparecer.
            }
        }
        // Condi��o para fazer uma nova capivara aparecer:
        // 1. O slot atual n�o est� vis�vel.
        // 2. O n�mero de capivaras vis�veis � menor que o m�ximo permitido.
        // 3. N�O EST� CHOVENDO (capivaras normais/douradas n�o aparecem na chuva).
        else if (capivarasVisiveisAtualmente < maxCapivarasSimultaneas && !chovendo) {
            // L�gica para determinar a chance de uma nova capivara aparecer.
            // GetRandomValue(0, 1000) cria um "tick"(probabilidade acumulada para um evento ocorrer em um determinado frame) para cada milissegundo de FrameTime,
            // tornando a chance de apari��o mais consistente independente do FPS.
            if (GetRandomValue(0, 1000) < (int)(GetFrameTime() * 1000 / intervaloMin)) {
                TipoCapivara novoTipo = NORMAL; // Define o tipo padr�o como NORMAL.
                int chance = GetRandomValue(1, 100); // Gera um n�mero aleat�rio para determinar o tipo do animal.

                // L�gica de sorteio de tipo de capivara baseada na dificuldade.
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

                capivaras[i].visivel = true;     // Torna a capivara neste slot vis�vel.
                capivaras[i].tempoVisivel = 0.0; // Reseta o tempo vis�vel dela.
                // Define um tempo total de visibilidade aleat�rio, dentro dos limites da dificuldade.
                capivaras[i].tempoTotalVisivel = (double)GetRandomValue((int)(tempoVisivelMin * 1000), (int)(tempoVisivelMax * 1000)) / 1000.0;
                capivaras[i].hit = false;        // Garante que ela n�o esteja marcada como acertada.
                capivaras[i].tipo = novoTipo;    // Atribui o tipo de capivara sorteado.
            }
        }

        // --- DETEC��O DE CLIQUE DO JOGADOR ---
        // Verifica se:
        // 1. A capivara est� vis�vel.
        // 2. N�o foi acertada ainda neste aparecimento.
        // 3. O bot�o esquerdo do mouse foi pressionado neste frame.
        // 4. A posi��o do mouse colide com o ret�ngulo da capivara.
        if (capivaras[i].visivel && !capivaras[i].hit && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), capivaras[i].rect)) {
            capivaras[i].hit = true;        // Marca a capivara como acertada.
            capivaras[i].machucada = true;  // Inicia o estado de "atordoada"
            capivaras[i].tempoAcertada = 0.0; // Reseta o contador para o tempo de "atordoada".

            // L�gica para adicionar ou subtrair pontos com base no tipo de animal acertado.
            switch(capivaras[i].tipo) {
                case NORMAL:
                    pontos += 1; // Capivara normal: +1 ponto.
                break;
                case DOURADA:
                    pontos += 2; // Capivara dourada: +2 pontos.
                break;
                case CUTIA:
                    pontos += PENALIDADE_CUTIA[dificuldadeAtual]; // Cutia: perde pontos conforme a dificuldade.
                    if(pontos < 0) // Garante que a pontua��o nunca seja negativa.
                        pontos = 0;
                    if(dificuldadeAtual == DIFICIL){
                        chovendo = true;      // Ativa o estado de chuva.
                        tempoChuvaAtual = 5.0; // Define que a chuva durar� 5 segundos.
                    }
                break;
            }
        }
    }
}

// Fun��o respons�vel por desenhar todos os elementos visuais do jogo na tela.
void DesenharJogo() {
    // Desenha o fundo do jogo. A fun��o DrawTexturePro � usada para redimensionar a textura
    // para preencher toda a tela, independentemente do tamanho original da imagem.
    DrawTexturePro(fundoJogo, (Rectangle){0,0, (float)fundoJogo.width, (float)fundoJogo.height}, (Rectangle){0,0, (float)GetScreenWidth(), (float)GetScreenHeight()}, (Vector2){0,0}, 0.0f, WHITE);
    
    // Desenha a textura da chuva se o estado 'chovendo' for true.
    if (chovendo)
        DrawTexture(texturaChuva, 0, 0, WHITE); // Desenha a textura de chuva na posi��o (0,0).

    // Loop para desenhar cada capivara (ou animal) na tela.
    for (int i = 0; i < totalCapivaras; i++) {
        // Se a capivara n�o est� vis�vel e n�o est� machucada, n�o h� necessidade de desenh�-la.
        if (!capivaras[i].visivel && !capivaras[i].machucada)
            continue; // Pula para a pr�xima itera��o do loop.

        Texture2D* texAtual = &texturaCapivaraLevantada; // Por padr�o, usa a textura da capivara normal.
        // Seleciona a textura correta com base no tipo de animal.
        if (capivaras[i].tipo == DOURADA) {
            texAtual = &texturaCapivaraDourada; // Se for dourada, usa a textura da capivara dourada.
        } else if (capivaras[i].tipo == CUTIA) {
            texAtual = &texturaCutia; // Se for cutia, usa a textura da cutia.
        }
        
        Rectangle destRec = capivaras[i].rect; // O ret�ngulo de destino onde a textura ser� desenhada.
        
        // Ajusta o tamanho da cutia ao desenhar.
        if (capivaras[i].tipo == CUTIA) {
            float scale = 0.9f; // Fator de escala: 90% do tamanho do slot da capivara.
            destRec.width *= scale;  // Reduz a largura.
            destRec.height *= scale; // Reduz a altura.
            // Centraliza a cutia dentro do espa�o original do slot (capivaras[i].rect).
            destRec.x += (capivaras[i].rect.width - destRec.width) / 2;
            destRec.y += (capivaras[i].rect.height - destRec.height) / 2;
        }

        // Desenha a textura apropriada: atordoada se machucada, ou a textura normal/dourada/cutia se vis�vel.
        if (capivaras[i].machucada) {
            // Se estiver machucada, sempre desenha a textura de atordoado.
            DrawTexturePro(texturaAnimalAtordoado, (Rectangle){0,0, (float)texturaAnimalAtordoado.width, (float)texturaAnimalAtordoado.height}, destRec, (Vector2){0,0}, 0.0f, WHITE);
        } else if (capivaras[i].visivel) {
            // Se estiver vis�vel e n�o machucada, desenha a textura do tipo de animal.
            DrawTexturePro(*texAtual, (Rectangle){0,0, (float)texAtual->width, (float)texAtual->height}, destRec, (Vector2){0,0}, 0.0f, WHITE);
        }
    }

    // --- UI (Interface do Usu�rio) do Jogo ---

    // Desenha o tempo restante no canto superior direito, formatado como "MM:SS".
    DrawText(TextFormat("Tempo: %02d:%02d", (int)tempoRestanteJogo / 60, (int)tempoRestanteJogo % 60), GetScreenWidth() - 200, 20, 30, WHITE);
    // Desenha a pontua��o no canto superior esquerdo.
    DrawText(TextFormat("Pontos: %d", pontos), 20, 20, 30, WHITE);
    
    // Desenha o bot�o de PAUSE no centro superior da tela.
    Rectangle btnPausaRect = {GetScreenWidth() / 2.0f - 60, 20, 120, 40};
    // A cor do bot�o muda ao passar o mouse.
    DrawRectangleRec(btnPausaRect, CheckCollisionPointRec(GetMousePosition(), btnPausaRect) ? BROWN : Fade(RED, 0.5f));
    DrawText("PAUSE", btnPausaRect.x + 25, btnPausaRect.y + 10, 20, WHITE);

    // --- Contagem Regressiva Inicial ---
    // Exibe a contagem regressiva antes do jogo realmente come�ar.
    if (!jogoIniciado) {
        const char* textoContador = "";
        if (contadorRegressivoInicial > 1) // Se o contador � maior que 1 (ex: 3, 2).
            textoContador = TextFormat("%d", (int)contadorRegressivoInicial); // Exibe o n�mero inteiro.
        else if (contadorRegressivoInicial > 0) // Se o contador est� entre 0 e 1 (quase no fim).
            textoContador = "GO!"; // Exibe "GO!".
        
        // Desenha o texto da contagem regressiva, centralizado na tela.
        if (strlen(textoContador) > 0) { // Garante que s� desenhe se houver texto.
            DrawText(textoContador, GetScreenWidth() / 2 - MeasureText(textoContador, 100) / 2, GetScreenHeight() / 2 - 50, 100, GOLD);
        }
    }
    
    // --- Tela de Fim de Jogo (Overlay) ---
    // � desenhada sobre o resto do jogo quando 'jogoAcabou' � true.
    if (jogoAcabou) {
        // Desenha um ret�ngulo semi-transparente que escurece a tela, dando um efeito de overlay.
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.7f));
        const char* fimTexto = "Fim de jogo!";
        const char* pontuacaoFinalTexto = TextFormat("Pontua��o final: %d", pontos);
        
        // Desenha as mensagens de "Fim de jogo!" e a pontua��o final, centralizadas.
        DrawText(fimTexto, GetScreenWidth() / 2 - MeasureText(fimTexto, 60) / 2, GetScreenHeight() / 2 - 100, 60, WHITE);
        DrawText(pontuacaoFinalTexto, GetScreenWidth() / 2 - MeasureText(pontuacaoFinalTexto, 40) / 2, GetScreenHeight() / 2 - 30, 40, WHITE);
    }
}

// --- FUN��O PRINCIPAL ---
// O ponto de entrada do programa. Aqui o jogo � inicializado e o loop principal � executado.
int main(void) {
    const int larguraTela = 1500; // Define a largura da janela do jogo.
    const int alturaTela = 800;   // Define a altura da janela do jogo.
    InitWindow(larguraTela, alturaTela, "Barigueira Attack!"); // Inicializa a janela da Raylib com t�tulo.
    SetTargetFPS(60); // Define o limite de quadros por segundo (FPS) para 60, para uma experi�ncia de jogo suave.

    // --- CARREGAMENTO INICIAL DE TEXTURAS ---
    // Todas as texturas s�o carregadas, para evitar atrasos durante o jogo e otimizar o uso da mem�ria.
    fundoJogo = LoadTexture("fundoJogo.png");                 // Carrega a imagem de fundo do jogo.
    texturaCapivaraLevantada = LoadTexture("capivaraNormal.png"); // Carrega a imagem da capivara comum.
    texturaCapivaraDourada = LoadTexture("capivaraDourada.png"); // Carrega a imagem da capivara dourada.
    texturaAnimalAtordoado = LoadTexture("animalAtordoado.png"); // Carrega a imagem do animal atordoado (acertado).
    texturaCutia = LoadTexture("cutiaJogo.png");                 // Carrega a imagem da cutia.
    texturaChuva = LoadTexture("fundoChuvoso.png");             // Carrega a imagem do efeito de chuva.
    Texture2D fundoPrincipal = LoadTexture("fundoPrincipal.png"); // Fundo da tela inicial.
    Texture2D fundoMenu = LoadTexture("fundoMenu.png");           // Fundo do menu principal e sele��o de dificuldade.
    Texture2D fundoCreditos = LoadTexture("fundoCreditos.png");   // Fundo da tela de cr�ditos.

    Tela telaAtual = TELA_INICIAL; // Define o estado inicial do jogo para a tela de abertura.

    // Inicializa o ponteiro como NULL para que a fun��o `free(capivaras)` n�o tente, liberar mem�ria de um ponteiro n�o inicializado, o que causaria um erro.
    capivaras = NULL;

    // --- LOOP PRINCIPAL DO JOGO ---
    // 'WindowShouldClose()' retorna true quando o usu�rio clica no 'X' da janela ou pressiona ESC.
    
    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition(); // Obt�m a posi��o atual do cursor do mouse a cada frame.

        BeginDrawing(); // Inicia o modo de desenho da Raylib para este frame.
        ClearBackground(BEIGE); // Limpa a tela com uma cor base (�til se n�o houver um fundo ocupando 100%).

        // --- M�QUINA DE ESTADOS ---
        // Um switch-case � usado para gerenciar as diferentes telas (estados) do jogo.
        // A l�gica e o desenho executados dependem da 'telaAtual'.
        switch (telaAtual) {
            case TELA_INICIAL: {
                DrawTexture(fundoPrincipal, 0, 0, WHITE); // Desenha o fundo da tela inicial.
                const char* titulo = "BARIGUEIRA ATTACK!";
                // Desenha o t�tulo do jogo, centralizado.
                DrawText(titulo, larguraTela / 2 - MeasureText(titulo, 80) / 2, alturaTela / 2 - 150, 80, BEIGE);
                // Desenha o bot�o "INICIAR". Se clicado, muda para a tela de menu.
                if (DesenharBotao((Rectangle){larguraTela / 2 - 150, 490, 300, 60}, "INICIAR", mouse)) {
                    telaAtual = TELA_MENU;
                }
            } break; 

            case TELA_MENU: {
                DrawTexture(fundoMenu, 0, 0, WHITE); // Desenha o fundo do menu.
                const char* titulo = "MENU DO JOGO";
                // Desenha o t�tulo do menu, centralizado.
                DrawText(titulo, larguraTela / 2 - MeasureText(titulo, 60) / 2, 150, 60, BEIGE);
                // Bot�o "JOGAR": Se clicado, vai para a sele��o de dificuldade.
                if (DesenharBotao((Rectangle){larguraTela / 2 - 150, 300, 300, 60}, "JOGAR", mouse)) {
                    telaAtual = TELA_SELECAO_DIFICULDADE;
                }
                // Bot�o "CR�DITOS": Se clicado, vai para a tela de cr�ditos.
                if (DesenharBotao((Rectangle){larguraTela / 2 - 150, 385, 300, 60}, "CR�DITOS", mouse)) {
                    telaAtual = TELA_CREDITOS;
                }
                // Bot�o "SAIR": Se clicado, fecha a janela do jogo, encerrando o loop principal.
                if (DesenharBotao((Rectangle){larguraTela / 2 - 150, 470, 300, 60}, "SAIR", mouse)) {
                    CloseWindow();
                }
            } break; 

            case TELA_SELECAO_DIFICULDADE: {
                DrawTexture(fundoMenu, 0, 0, WHITE); // Usa o mesmo fundo do menu.
                const char* titulo = "SELECIONE A DIFICULDADE";
                // Desenha o t�tulo da tela de sele��o, centralizado.
                DrawText(titulo, larguraTela / 2 - MeasureText(titulo, 60) / 2, 150, 60, BEIGE);

                // Bot�o "F�CIL": Inicializa o jogo com a dificuldade FACIL e transiciona para a tela de jogo.
                if (DesenharBotao((Rectangle){larguraTela / 2 - 150, 300, 300, 60}, "F�CIL", mouse)) {
                    InicializarJogo(FACIL);
                    telaAtual = TELA_JOGO;
                }
                // Bot�o "M�DIO": Inicializa o jogo com a dificuldade MEDIO.
                if (DesenharBotao((Rectangle){larguraTela / 2 - 150, 385, 300, 60}, "M�DIO", mouse)) {
                    InicializarJogo(MEDIO);
                    telaAtual = TELA_JOGO;
                }
                // Bot�o "DIF�CIL": Inicializa o jogo com a dificuldade DIFICIL.
                if (DesenharBotao((Rectangle){larguraTela / 2 - 150, 470, 300, 60}, "DIF�CIL", mouse)) {
                    InicializarJogo(DIFICIL);
                    telaAtual = TELA_JOGO;
                }
                // Bot�o "VOLTAR": Retorna ao menu principal.
                if (DesenharBotao((Rectangle){larguraTela / 2 - 150, 555, 300, 60}, "VOLTAR", mouse)) {
                    telaAtual = TELA_MENU;
                }
            } break; 

            case TELA_JOGO: {
                // A l�gica do jogo s� � atualizada se o jogo ainda n�o acabou.
                if (!jogoAcabou) {
                    AtualizarJogo(); // Chama a fun��o que cont�m toda a l�gica do jogo.
                    // Verifica se o bot�o de pause foi clicado
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, (Rectangle){larguraTela / 2.0f - 60, 20, 120, 40})) {
                        telaAtual = TELA_PAUSA; // Se clicado, transiciona para a tela de pausa.
                    }
                }
                DesenharJogo(); // Chama a fun��o que desenha todos os elementos visuais do jogo (incluindo o overlay de fim de jogo se aplic�vel).

                // Bot�es espec�ficos que s� aparecem quando o jogo termina.
                if (jogoAcabou) {
                    // Bot�o "REINICIAR": Reinicia o jogo com a mesma dificuldade.
                    if (DesenharBotao((Rectangle){larguraTela / 2.0f - 260, alturaTela / 2.0f + 50, 250, 60}, "REINICIAR", mouse)) {
                        InicializarJogo(dificuldadeAtual); // Chama a fun��o de inicializa��o novamente.
                    }
                    // Bot�o "MENU INICIAL": Volta ao menu principal.
                    if (DesenharBotao((Rectangle){larguraTela / 2.0f + 10, alturaTela / 2.0f + 50, 250, 60}, "MENU INICIAL", mouse)) {
                        // Antes de voltar ao menu principal, � necessario liberar a mem�ria alocada para as capivaras.
                        if (capivaras != NULL) {
                            free(capivaras);     // Libera o bloco de mem�ria.
                            capivaras = NULL;    // Define o ponteiro para NULL para evitar acesso a mem�ria j� liberada
                        }
                        telaAtual = TELA_MENU; // Transiciona para o menu.
                    }
                }
            } break;

            case TELA_PAUSA: {
                DesenharJogo(); // Desenha o jogo em segundo plano (como estava antes de pausar).
                // Desenha um ret�ngulo semi-transparente para criar um efeito de escurecimento sobre o jogo pausado.
                DrawRectangle(0, 0, larguraTela, alturaTela, ColorAlpha(BLACK, 0.5f));
                const char* textoPausa = "JOGO PAUSADO";
                // Desenha o texto "JOGO PAUSADO", centralizado.
                DrawText(textoPausa, larguraTela / 2 - MeasureText(textoPausa, 60) / 2, 150, 60, BEIGE);

                // Bot�o "CONTINUAR": Despausa o jogo. Pode ser clicado ou pressionando ESC.
                if (DesenharBotao((Rectangle){larguraTela / 2.0f - 150, 300, 300, 60}, "CONTINUAR", mouse) || IsKeyPressed(KEY_ESCAPE)) {
                    telaAtual = TELA_JOGO; // Retorna � tela de jogo.
                }
                // Bot�o "REINICIAR": Reinicia a partida atual.
                if (DesenharBotao((Rectangle){larguraTela / 2.0f - 150, 385, 300, 60}, "REINICIAR", mouse)) {
                    InicializarJogo(dificuldadeAtual); // Re-inicializa o jogo.
                    telaAtual = TELA_JOGO;
                }
                // Bot�o "MENU PRINCIPAL": Volta ao menu.
                if (DesenharBotao((Rectangle){larguraTela / 2.0f - 150, 470, 300, 60}, "MENU PRINCIPAL", mouse)) {
                    // Libera a mem�ria das capivaras antes de sair do jogo para o menu.
                    if (capivaras != NULL) {
                        free(capivaras);
                        capivaras = NULL;
                    }
                    telaAtual = TELA_MENU;
                }
            } break; 

            case TELA_CREDITOS: {
                DrawTexture(fundoCreditos, 0, 0, WHITE); // Desenha o fundo da tela de cr�ditos.
                const char* titulo = "CR�DITOS";
                // Desenha o t�tulo "CR�DITOS", centralizado.
                DrawText(titulo, larguraTela / 2 - MeasureText(titulo, 60) / 2, 80, 60, BEIGE);

                // Array de strings contendo todo o texto dos cr�ditos.
                const char* creditosTexto[] = {
                    "ALUNOS:",
                    "ANA WALTRICK | �NGELO MIRANDA | VITOR KLUPPELL",
                    "", 
                    "INSTITUI��O: CENTRO UNIVERSIT�RIO AUT�NOMO DO BRASIL",
                    "DISCIPLINA: PROGRAMA��O AVAN�ADA",
                    "PROFESSOR: FABIO BETTIO",
                    "",
                    "INSPIRA��O:",
                    "JOGO WHACK-A-MOLE | CURITIBA | PARQUE BARIGUI | ANIMAIS",
                    "",
                    "OBRIGADO POR JOGAR!"
                };
                int yPos = 180; // Posi��o Y inicial para a primeira linha de cr�dito.
                // Loop para desenhar cada linha de texto dos cr�ditos.
                for (int i = 0; i < sizeof(creditosTexto)/sizeof(creditosTexto[0]); i++) {
                    // Desenha o texto, centralizado horizontalmente.
                    DrawText(creditosTexto[i], larguraTela / 2 - MeasureText(creditosTexto[i], 28) / 2, yPos, 28, BEIGE);
                    yPos += 40; // Incrementa a posi��o Y para a pr�xima linha.
                }

                // Bot�o "VOLTAR": Retorna ao menu principal.
                if (DesenharBotao((Rectangle){larguraTela / 2.0f - 150, alturaTela - 100, 300, 60}, "VOLTAR", mouse)) {
                    telaAtual = TELA_MENU;
                }
            } break; 
        }
        EndDrawing(); // Finaliza o modo de desenho, apresentando o frame renderizado na tela.
    }

    // --- DESALOCA��O FINAL DA MEM�RIA E RECURSOS AO FECHAR A JANELA ---
    // Liberar todos os recursos alocados para evitar vazamentos de mem�ria e garantir um encerramento limpo do programa.

    if (capivaras != NULL) {
        free(capivaras); 
        capivaras = NULL; 
    }

    // Descarrega todas as texturas da mem�ria.
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

