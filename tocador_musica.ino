// CCP020 — Projeto: Tocador de Música com Arduino UNO
// Aluno: Vinicius Alves de Souza | RA: 52.126.009-1
// Centro Universitário FEI — Nov/2026

#include <LiquidCrystal.h>
#include <avr/pgmspace.h>

// --- Definição de notas (frequências em Hz) ---
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define REST     0

// --- LCD: RS, E, D4, D5, D6, D7 ---
// MELHORIA 7: D7 no pino 2 (longe do buzzer no pino 6)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// --- Pinos dos botões (INPUT_PULLUP: LOW = pressionado) ---
const int PIN_BTN_UP   = 8;
const int PIN_BTN_DOWN = 9;
const int PIN_BTN_PLAY = 10;
const int PIN_BTN_STOP = 13;  // MELHORIA 7: era pino 2, liberado para LCD D7

// --- Pinos dos LEDs e buzzer ---
const int PIN_LED_PAUSE = A0;  // LED Vermelho — requer resistor 220Ω
const int PIN_LED_PLAY  = A1;  // LED Verde    — requer resistor 220Ω
const int PIN_BUZZER    = 6;

// --- Músicas ---
int musicaAtual = 0;
const int totalMusicas = 5;
const char* nomesMusicas[5] = {
  "01. Brilha Est.", "02. Ode Alegria", "03. Parabens",
  "04. Asa Branca",  "05. Escala Teste"
};

// --- Máquina de estados ---
enum EstadoSistema { MENU, REPRODUZINDO, PAUSADO };
EstadoSistema estadoAtual = MENU;
unsigned int  notaAtual     = 0;
unsigned long tempoAnterior = 0;

// --- Debounce não-bloqueante (MELHORIA 3 / BUG 3 FIX) ---
const unsigned long DEBOUNCE_MS = 200;
unsigned long ultimoPressUp=0, ultimoPressDown=0, ultimoPressPlay=0, ultimoPressStop=0;

bool leuBotao(int pino, unsigned long &ultimo) {
  if (digitalRead(pino) == LOW && millis() - ultimo > DEBOUNCE_MS) {
    ultimo = millis();
    return true;
  }
  return false;
}

// ============================================================
// MELODIAS — armazenadas em flash (PROGMEM) [MELHORIA 1]
// ============================================================

// 01. Brilha, Brilha Estrelinha
const int melodia1[] PROGMEM = {
  NOTE_C4,NOTE_C4,NOTE_G4,NOTE_G4,NOTE_A4,NOTE_A4,NOTE_G4,
  REST,
  NOTE_F4,NOTE_F4,NOTE_E4,NOTE_E4,NOTE_D4,NOTE_D4,NOTE_C4
};
const int duracao1[] PROGMEM = { 4,4,4,4,4,4,2,4,4,4,4,4,4,4,2 };
const int tam1 = 15;

// 02. Ode à Alegria
const int melodia2[] PROGMEM = {
  NOTE_E4,NOTE_E4,NOTE_F4,NOTE_G4,NOTE_G4,NOTE_F4,NOTE_E4,NOTE_D4,
  NOTE_C4,NOTE_C4,NOTE_D4,NOTE_E4,NOTE_E4,NOTE_D4,NOTE_D4,REST
};
const int duracao2[] PROGMEM = { 4,4,4,4,4,4,4,4,4,4,4,4,4,4,2,4 };
const int tam2 = 16;

// 03. Parabéns pra Você
const int melodia3[] PROGMEM = {
  NOTE_C4,NOTE_C4,NOTE_D4,NOTE_C4,NOTE_F4,NOTE_E4,REST,
  NOTE_C4,NOTE_C4,NOTE_D4,NOTE_C4,NOTE_G4,NOTE_F4,REST,
  NOTE_C4,NOTE_C4,NOTE_C5,NOTE_A4,NOTE_F4,NOTE_E4,NOTE_D4,REST,
  NOTE_AS4,NOTE_AS4,NOTE_A4,NOTE_F4,NOTE_G4,NOTE_F4
};
const int duracao3[] PROGMEM = {
  8,8,4,4,4,2,4,
  8,8,4,4,4,2,4,
  8,8,4,4,4,4,4,4,
  8,8,4,4,4,2
};
const int tam3 = 26;

// 04. Asa Branca
const int melodia4[] PROGMEM = {
  NOTE_G4,NOTE_A4,NOTE_G4,NOTE_E4,NOTE_D4,NOTE_E4,NOTE_G4,NOTE_A4,
  NOTE_G4,REST,
  NOTE_G4,NOTE_A4,NOTE_G4,NOTE_E4,NOTE_D4,NOTE_E4,NOTE_G4,NOTE_E4,
  NOTE_D4,REST
};
const int duracao4[] PROGMEM = {
  8,8,8,8,8,8,4,8,2,8,
  8,8,8,8,8,8,4,8,2,4
};
const int tam4 = 20;

// 05. Escala de Dó (teste de hardware)
const int melodia5[] PROGMEM = {
  NOTE_C4,NOTE_D4,NOTE_E4,NOTE_F4,NOTE_G4,NOTE_A4,NOTE_B4,NOTE_C5
};
const int duracao5[] PROGMEM = { 4,4,4,4,4,4,4,4 };
const int tam5 = 8;

// ============================================================
// SETUP
// ============================================================
void setup() {
  pinMode(PIN_BTN_UP,   INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  pinMode(PIN_BTN_PLAY, INPUT_PULLUP);
  pinMode(PIN_BTN_STOP, INPUT_PULLUP);
  pinMode(PIN_LED_PLAY,  OUTPUT);
  pinMode(PIN_LED_PAUSE, OUTPUT);
  pinMode(PIN_BUZZER,    OUTPUT);
  lcd.begin(16, 2);
  atualizarMenu();
}

// ============================================================
// LOOP PRINCIPAL
// ============================================================
void loop() {
  if (estadoAtual == MENU) {
    if (leuBotao(PIN_BTN_UP,   ultimoPressUp))   { musicaAtual = (musicaAtual - 1 + totalMusicas) % totalMusicas; atualizarMenu(); }
    if (leuBotao(PIN_BTN_DOWN, ultimoPressDown))  { musicaAtual = (musicaAtual + 1) % totalMusicas; atualizarMenu(); }
    if (leuBotao(PIN_BTN_PLAY, ultimoPressPlay))  { iniciarReproducao(); }
  } else {
    if (leuBotao(PIN_BTN_PLAY, ultimoPressPlay))  { alternarPlayPause(); }
    if (leuBotao(PIN_BTN_STOP, ultimoPressStop))  { pararReproducao(); }
    if (estadoAtual == REPRODUZINDO) { tocarMusicaNaoBloqueante(); }
  }
}

// ============================================================
// FUNÇÕES DE CONTROLE
// ============================================================

void atualizarMenu() {
  noTone(PIN_BUZZER);
  digitalWrite(PIN_LED_PLAY,  LOW);
  digitalWrite(PIN_LED_PAUSE, LOW);
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Selecione:");
  lcd.setCursor(0, 1); lcd.print(nomesMusicas[musicaAtual]);
}

void iniciarReproducao() {
  estadoAtual = REPRODUZINDO;
  notaAtual = 0;
  tempoAnterior = millis();  // BUG 4 FIX: inicia o timer no momento exato
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Tocando agora:");
  lcd.setCursor(0, 1); lcd.print(nomesMusicas[musicaAtual]);
  digitalWrite(PIN_LED_PLAY,  HIGH);
  digitalWrite(PIN_LED_PAUSE, LOW);
}

void alternarPlayPause() {
  if (estadoAtual == REPRODUZINDO) {
    estadoAtual = PAUSADO;
    noTone(PIN_BUZZER);
    digitalWrite(PIN_LED_PLAY,  LOW);
    digitalWrite(PIN_LED_PAUSE, HIGH);
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Pausado");
    lcd.setCursor(0, 1); lcd.print(nomesMusicas[musicaAtual]);
  } else if (estadoAtual == PAUSADO) {
    estadoAtual = REPRODUZINDO;
    tempoAnterior = millis();  // BUG 4 FIX: reseta timer ao retomar
    digitalWrite(PIN_LED_PLAY,  HIGH);
    digitalWrite(PIN_LED_PAUSE, LOW);
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Tocando agora:");
    lcd.setCursor(0, 1); lcd.print(nomesMusicas[musicaAtual]);
  }
}

void pararReproducao() {
  estadoAtual = MENU;
  noTone(PIN_BUZZER);
  atualizarMenu();
}

// ============================================================
// REPRODUÇÃO NÃO-BLOQUEANTE
// ============================================================
void tocarMusicaNaoBloqueante() {
  // BUG 2 FIX: PGM_P com aritmética de byte (*2) para arrays int em PROGMEM
  PGM_P melodiaSel; PGM_P duracaoSel; int tamanho;
  switch (musicaAtual) {
    case 0: melodiaSel=(PGM_P)melodia1; duracaoSel=(PGM_P)duracao1; tamanho=tam1; break;
    case 1: melodiaSel=(PGM_P)melodia2; duracaoSel=(PGM_P)duracao2; tamanho=tam2; break;
    case 2: melodiaSel=(PGM_P)melodia3; duracaoSel=(PGM_P)duracao3; tamanho=tam3; break;
    case 3: melodiaSel=(PGM_P)melodia4; duracaoSel=(PGM_P)duracao4; tamanho=tam4; break;
    case 4: melodiaSel=(PGM_P)melodia5; duracaoSel=(PGM_P)duracao5; tamanho=tam5; break;
    default: return;
  }

  int nota           = (int)pgm_read_word_near(melodiaSel  + notaAtual * 2);
  int divisorDuracao = (int)pgm_read_word_near(duracaoSel  + notaAtual * 2);
  int duracaoNota    = 1000 / divisorDuracao;

  // BUG 1 FIX: cast explícito evita truncamento int×float
  unsigned long duracaoTotal = (unsigned long)((float)duracaoNota * 1.30f);

  unsigned long tempoAtual = millis();
  if (tempoAtual - tempoAnterior >= duracaoTotal) {
    tempoAnterior = tempoAtual;
    if (nota == REST) noTone(PIN_BUZZER);
    else              tone(PIN_BUZZER, nota, duracaoNota);
    notaAtual++;
    if (notaAtual >= (unsigned int)tamanho) notaAtual = 0;
  }
}
