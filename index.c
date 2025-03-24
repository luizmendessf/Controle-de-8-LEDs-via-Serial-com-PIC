// Definição de constantes para as teclas de controle
#define F 70  // Tecla 'F' (FRENTE) - ASCII 70
#define A 65  // Tecla 'A' (ATRAS)  - ASCII 65
#define D 68  // Tecla 'D' (PISCAR MENOS SIGNIFICATIVO) - ASCII 68
#define E 69  // Tecla 'E' (PISCAR MAIS SIGNIFICATIVO)  - ASCII 69
#define P 80  // Tecla 'P' (AUMENTAR INTENSIDADE)       - ASCII 80
#define L 76  // Tecla 'L' (DIMINUIR INTENSIDADE)       - ASCII 76

// Variável para armazenar o dado recebido pela serial
unsigned char dado;

// Cálculo do valor para configurar o Timer0 para PWM (dependente da frequência do oscilador e prescaler)
// valor = (0.0001 * Fosc)/(Prescaler * Postscaler)
// Assumindo Fosc = 20MHz e Prescaler = 32 e Postscaler = 4 (configurado posteriormente)
unsigned int valor = (0.0001 * 20000000) / (32 * 4);

// Variável para controlar o duty cycle do PWM (inicializado em 10%)
unsigned int duty = 10;

// Contadores utilizados para controlar a execução única do aumento/diminuição do duty cycle
int cnt1 = 0; // Contador para a tecla 'P'
int cnt2 = 0; // Contador para a tecla 'L'

/*----------------- Função para transmitir um caractere pela USART ----------------------*/
void TXCHR_USART(char dado1) {
  while (TXIF_bit == 0); // Espera até que o buffer de transmissão esteja vazio (TXIF flag = 1)
  TXIF_bit = 0;          // Limpa a flag de interrupção de transmissão
  TXREG = dado1;         // Carrega o caractere para o registrador de transmissão
}

/******************** Função para transmitir uma string pela USART ************/
void TX_STRING_USART(const char *dado) {
  unsigned int i = 0;
  while (dado[i] != '\0') // Loop até encontrar o caractere nulo (fim da string)
    TXCHR_USART(dado[i++]); // Transmite cada caractere da string
} // Fim da função TX_STRING_USART()

/**************** Função INICIAR_USART ****************************/
void INICIAR_USART(const unsigned long Baud_Rate) {
  // Calcula o valor para o registrador SPBRG (Baud Rate Generator)
  SPBRG = (20000000UL / (long)(16 * Baud_Rate)) - 1; // Para BRGH = 1 (alta velocidade)

  SYNC_bit = 0; // Habilita o modo assíncrono (para comunicação serial com PC)
  SPEN_bit = 1; // Habilita os pinos da porta serial (TX e RX)
  CREN_bit = 1; // Habilita a recepção de dados
  TXIE_bit = 0; // Desabilita a interrupção de transmissão (não utilizada neste exemplo)
  RCIE_bit = 0; // Desabilita a interrupção de recepção (habilitada posteriormente)
  TX9_bit = 0; // Configura para transmissão de 8 bits de dados
  RX9_bit = 0; // Configura para recepção de 8 bits de dados
  TXEN_bit = 1; // Habilita a transmissão
}

// --- Rotina de Interrupção ---
void interrupt() {
  if (RCIF_bit) // Houve interrupção na USART (recepção de dados)?
  {             // Sim...
    RCIF_bit = 0; // Limpa a flag de interrupção de recepção

    // Verifica se ocorreu erro de framing ou overrun
    if (FERR_bit || OERR_bit) {
      // Sim...
      CREN_bit = 0; // Limpa o status de recebimento
      CREN_bit = 1; // Reinicia o modo de recepção
    } // Fim do teste de erro

    // TXREG = RCREG; // Eco: Transmite de volta o que foi recebido (para debugging)
    dado = RCREG; // A variável 'dado' recebe o valor do byte recebido
  } // Fim do if(RCIF_bit)
} // Fim da interrupção

void main() {
  // Mensagem inicial a ser enviada para o PC
  const char message= "Pressionar as teclas F, A, D, E, P ou L";

  // Configurar pinos ANx como digitais (desabilita entradas analógicas)
  ANSEL = 0;
  ANSELH = 0;

  // Desabilita os comparadores (se presentes no PIC)
  C1ON_bit = 0;
  C2ON_bit = 0;

  // Configura a Porta B como saída (para os LEDs)
  TRISB = 0;
  // Inicializa a Porta B com todos os LEDs apagados
  PORTB = 0;

  // Configura o EUSART para alta velocidade (BRGH = 1)
  BRGH_bit = 1;

  // Inicializa a comunicação serial com a taxa de 19200 bps
  INICIAR_USART(19200);

  // Pequeno delay para estabilização do módulo USART
  Delay_ms(100);

  // Transmite a mensagem inicial para o PC
  TX_STRING_USART(message);
  TXCHR_USART(10); // Envia um caractere de Line Feed (LF) para mudar de linha no terminal
  TXCHR_USART(13); // Envia um caractere de Carriage Return (CR) para voltar ao início da linha

  // Inicializa a Porta B com um valor (apenas para testar os LEDs inicialmente)
  PORTB = 1;

  // Configuração do Timer0 para gerar o PWM (controle de intensidade)
  OPTION_REG = 0x84; // Configurações do Timer0: Prescaler = 1:32
  INTCON = 0xA0;     // Habilita interrupções globais e de periféricos

  // Define o valor inicial do Timer0 para o duty cycle inicial
  TMR0 = 256 - (duty * valor);

  /************************ Configuração da interrupção de recepção USART
   * Esta configuração deve vir depois da função INICIAR_USART porque nesta
   * estamos desabilitando a interrupção de recepção USART
   ***********************************************************************/
  RCIF_bit = 0; // Desabilita o flag de interrupção de recepção USART (limpa qualquer interrupção pendente)
  RCIE_bit = 1; // Habilita a interrupção de recepção USART
  GIE_bit = 1;  // Habilita a interrupção global
  PEIE_bit = 1; // Habilita a interrupção de periféricos
  // Fim das configurações de interrupção de recepção USART

  // Loop infinito para processar os comandos recebidos
  while (1) {
    switch (dado) {
      case F: // Tecla 'F' pressionada: LEDs acendem em sequência (FRENTE)
        cnt1 = 0;
        cnt2 = 0;
        TX_STRING_USART("FRENTE");
        TXCHR_USART(10);
        TXCHR_USART(13);
        if (PORTB >= 128) {
          PORTB = 1; // Reinicia a sequência se o último LED estiver aceso
          Delay_ms(500);
        } else {
          PORTB = PORTB << 1; // Desloca os bits para a esquerda (acende o próximo LED)
          Delay_ms(500);
        }
        break;

      case A: // Tecla 'A' pressionada: LEDs acendem em sequência inversa (ATRAS)
        cnt1 = 0;
        cnt2 = 0;
        TX_STRING_USART("ATRAS");
        TXCHR_USART(10);
        TXCHR_USART(13);
        if (PORTB == 0) {
          PORTB = 128; // Reinicia a sequência se todos os LEDs estiverem apagados
          Delay_ms(500);
        } else {
          PORTB = PORTB >> 1; // Desloca os bits para a direita (acende o LED anterior)
          Delay_ms(500);
        }
        break;

      case D: // Tecla 'D' pressionada: LEDs menos significativos piscam
        cnt1 = 0;
        cnt2 = 0;
        TX_STRING_USART("PISCANDO MENOS SIGNIFICATIVO");
        TXCHR_USART(10);
        TXCHR_USART(13);
        {
          // Inverte o estado dos 4 LEDs menos significativos (RB0 a RB3)
          PORTB.RB0 = ~PORTB.RB0;
          PORTB.RB1 = ~PORTB.RB1;
          PORTB.RB2 = ~PORTB.RB2;
          PORTB.RB3 = ~PORTB.RB3;
          Delay_ms(500);
        }
        break;

      case E: // Tecla 'E' pressionada: LEDs mais significativos piscam
        TX_STRING_USART("PISCANDO MAIS SIGNIFICATIVO");
        TXCHR_USART(10);
        TXCHR_USART(13);
        cnt1 = 0;
        cnt2 = 0;
        {
          // Inverte o estado dos 4 LEDs mais significativos (RB4 a RB7)
          PORTB.RB4 = ~PORTB.RB4;
          PORTB.RB5 = ~PORTB.RB5;
          PORTB.RB6 = ~PORTB.RB6;
          PORTB.RB7 = ~PORTB.RB7;
          Delay_ms(500);
          break;
        }

      case P: // Tecla 'P' pressionada: Aumenta o duty cycle (intensidade)
        {
          if (cnt1 == 0) {
            duty += 1;
            if (duty >= 10) duty = 10; // Limita o duty cycle ao máximo de 10 (100%)
            TMR0 = 256 - (duty * valor); // Atualiza o valor do Timer0 para o novo duty cycle
            TX_STRING_USART("Duty Cycle: ");
            // Converte o valor do duty (multiplicado por 10 para representar a porcentagem) para string e envia
            char duty_str[4];
            IntToStr(duty * 10, duty_str);
            TX_STRING_USART(duty_str);
            TX_STRING_USART("%");
            TXCHR_USART(10);
            TXCHR_USART(13);
          }
          cnt1++;
          break;
        }

      case L: // Tecla 'L' pressionada: Diminui o duty cycle (intensidade)
        {
          if (cnt2 == 0) {
            duty -= 1;
            if (duty <= 0) duty = 0; // Limita o duty cycle ao mínimo de 0 (0%)
            TMR0 = 256 - (duty * valor); // Atualiza o valor do Timer0
            TX_STRING_USART("Duty Cycle: ");
            // Converte o valor do duty (multiplicado por 10 para representar a porcentagem) para string e envia
            char duty_str[4];
            IntToStr(duty * 10, duty_str);
            TX_STRING_USART(duty_str);
            TX_STRING_USART("%");
            TXCHR_USART(10);
            TXCHR_USART(13);
          }
          cnt2++;
          break;
        }

      default: // Nenhuma tecla reconhecida pressionada
        {
          dado = 0; // Reseta a variável 'dado'
          // Envia uma mensagem vazia ou um status padrão para o PC (opcional)
          // TX_STRING_USART("");
          // TXCHR_USART(10);
          // TXCHR_USART(13);
        }
        break;
    } // Fim do switch

    // Reseta os contadores para permitir novos ajustes de intensidade ao pressionar as teclas novamente
    if (dado != P) cnt1 = 0;
    if (dado != L) cnt2 = 0;

  } // Fim do while (1)
} // Fim da função main