# Relatório do Projeto CCP020 — Tocador de Música com Arduino

**Disciplina:** CCP020 — Circuitos e Sistemas Digitais  
**Aluno:** Vinicius Alves de Souza  
**RA:** 52.126.009-1  
**Instituição:** Centro Universitário FEI — Campus São Bernardo do Campo  
**Data:** Novembro de 2026  

---

## 1. Descrição do Projeto

O projeto consiste em um **tocador de música** implementado em Arduino UNO, capaz de reproduzir 5 melodias pré-programadas por meio de um buzzer piezoelétrico. O sistema conta com interface de usuário via display LCD 16×2, 4 botões de controle (navegar para cima, navegar para baixo, play/pause e stop) e 2 LEDs indicadores de estado (reproduzindo e pausado).

### Componentes utilizados

| Componente | Quantidade | Observações |
|---|---|---|
| Arduino UNO R3 | 1 | Microcontrolador principal |
| Display LCD 16×2 | 1 | Interface de texto (modo 4 bits) |
| Buzzer piezoelétrico | 1 | Saída de áudio |
| Botão push-button | 4 | Controle: UP, DOWN, PLAY, STOP |
| LED vermelho | 1 | Indicador de pausa |
| LED verde | 1 | Indicador de reprodução |
| Resistor 220 Ω | 2 | Limitação de corrente dos LEDs |
| Potenciômetro | 1 | Controle de contraste do LCD |
| Protoboard | 1 | Montagem dos componentes |

---

## 2. Mapeamento de Pinos (versão corrigida)

| Sinal | Pino Arduino | Observação |
|---|---|---|
| LCD RS | 12 | — |
| LCD E (Enable) | 11 | — |
| LCD D4 | 5 | — |
| LCD D5 | 4 | — |
| LCD D6 | 3 | — |
| LCD D7 | **2** | Corrigido: era pino 7 |
| BTN_UP | 8 | INPUT_PULLUP |
| BTN_DOWN | 9 | INPUT_PULLUP |
| BTN_PLAY | 10 | INPUT_PULLUP |
| BTN_STOP | **13** | Corrigido: era pino 2 |
| LED Verde (PLAY) | A1 | 220 Ω em série |
| LED Vermelho (PAUSE) | A0 | 220 Ω em série |
| Buzzer | 6 | Saída PWM |

---

## 3. Bugs Identificados e Corrigidos

### Bug 1 — Truncamento na duração das notas
**Problema:** A expressão `duracaoNota * 1.30` realizava multiplicação `int × float` sem cast explícito, resultando em truncamento silencioso para `int`. Notas mais longas tocavam com duração incorreta.  
**Correção:** Cast explícito para `(unsigned long)((float)duracaoNota * 1.30f)` garante aritmética de ponto flutuante e armazena o resultado corretamente.

### Bug 2 — Acesso incorreto à memória PROGMEM
**Problema:** As melodias eram declaradas com `const int*` e acessadas com indexação de ponteiro (`melodiaSel + notaAtual`). No AVR, `PROGMEM` reside no espaço de endereçamento de flash, separado da RAM. Um ponteiro `const int*` aponta para RAM, não para flash, causando leitura de dados corrompidos.  
**Correção:** Uso de `PGM_P` (ponteiro para flash) com aritmética de byte explícita (`+ notaAtual * 2`) e leitura via `pgm_read_word_near()`.

### Bug 3 — Debounce bloqueante com `delay()`
**Problema:** O debounce dos botões era implementado com `delay(200)`, bloqueando a execução do loop inteiro. Durante esse tempo, notas não eram processadas e o sistema ficava sem resposta.  
**Correção:** Debounce não-bloqueante com `millis()`: a função `leuBotao()` compara o timestamp atual com o último acionamento sem bloquear a execução.

### Bug 4 — Timer não resetado ao retomar reprodução após pausa
**Problema:** Ao pausar e retomar a reprodução, `tempoAnterior` mantinha o valor antigo. Com `millis()` avançando durante a pausa, a condição `millis() - tempoAnterior >= duracaoTotal` era verdadeira imediatamente, pulando a nota atual.  
**Correção:** `tempoAnterior = millis()` é atribuído no momento exato em que a reprodução é retomada em `alternarPlayPause()`.

---

## 4. Melhorias Implementadas

### Melhoria 1 — Armazenamento de melodias em PROGMEM
Todas as melodias e durações foram movidas para a memória flash com `PROGMEM`, liberando a RAM (limitada a 2 KB no Arduino UNO) para uso do programa. Sem isso, 5 melodias com arrays de int esgotariam a RAM disponível.

### Melhoria 2 — Estados do sistema com `enum`
Introdução de `enum EstadoSistema { MENU, REPRODUZINDO, PAUSADO }` para controle de fluxo explícito, substituindo variáveis booleanas soltas. Torna o código mais legível e menos propenso a estados inconsistentes.

### Melhoria 3 — Reprodução não-bloqueante com `millis()`
Substituição de `delay()` por lógica baseada em `millis()` em toda a função de reprodução, permitindo que o sistema continue verificando botões enquanto toca.

### Melhoria 4 — Funções dedicadas para cada ação
Separação do `loop()` em funções: `iniciarReproducao()`, `alternarPlayPause()`, `pararReproducao()`, `tocarMusicaNaoBloqueante()` e `atualizarMenu()`. Melhora a manutenibilidade e legibilidade.

### Melhoria 5 — Navegação circular no menu
O índice da música navega de forma circular: ao passar da última música, volta para a primeira (e vice-versa), usando aritmética modular.

### Melhoria 6 — Resistores de proteção para os LEDs
Adição de resistores de 220 Ω em série com cada LED. Sem o resistor, a corrente seria limitada apenas pela resistência interna do LED (~50–100 mA), acima do limite de 40 mA por pino do Arduino UNO, podendo danificar o pino ou o LED.  
Cálculo: R = (Vcc − Vf) / I = (5 − 2) / 0,015 = **200 Ω** → valor padrão **220 Ω**.

### Melhoria 7 — Relocação do pino LCD D7 (pino 7 → pino 2)
O pino 7 é adjacente ao pino 6 (buzzer). Ruído elétrico gerado pela saída PWM do buzzer pode induzir falsos níveis lógicos no pino 7, corrompendo dados do LCD. A relocação do D7 para o pino 2 (distante do buzzer e contíguo com D4–D6 nos pinos 2–5) elimina essa interferência.

---

## 5. Melodias Implementadas

| # | Nome | Notas |
|---|---|---|
| 01 | Brilha, Brilha Estrelinha | 15 notas |
| 02 | Ode à Alegria (Beethoven) | 16 notas |
| 03 | Parabéns pra Você | 25 notas |
| 04 | Asa Branca (Luiz Gonzaga) | 20 notas |
| 05 | Escala de Dó (teste) | 8 notas |

---

## 6. Diagrama de Estados

```
        [MENU]
           |
    BTN_PLAY pressionado
           |
           v
    [REPRODUZINDO] <----> [PAUSADO]
           |            BTN_PLAY
           |
    BTN_STOP pressionado
           |
           v
        [MENU]
```

---

## 7. Link do Circuito no Tinkercad

[Copy of tocador de musica — Tinkercad](https://www.tinkercad.com/things/2GIkCjrHh6Y-copy-of-tocador-de-musica)

---

## 8. Referências

- Documentação oficial Arduino: https://www.arduino.cc/reference/
- Biblioteca LiquidCrystal: https://www.arduino.cc/en/Reference/LiquidCrystal
- AVR LibC — avr/pgmspace.h: https://www.nongnu.org/avr-libc/user-manual/group__avr__pgmspace.html
- Datasheet ATmega328P: https://ww1.microchip.com/downloads/en/DeviceDoc/Atmega48A-PA-88A-PA-168A-PA-328-P-DS-DS40002061B.pdf
