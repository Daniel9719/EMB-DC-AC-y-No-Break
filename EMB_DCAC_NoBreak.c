#include<stdbool.h>
#include<stdint.h>
#include"inc/tm4c1294ncpdt.h"

static unsigned char Seccion=1;

//----------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%    DECLARACIÓN DE PUERTOS    %%%%%%%%%%%%%%%%%%%%
//----------------------------------------------------------------------
void Inic_puertos(void){
    SYSCTL_RCGCGPIO_R=0x219;                    //Reloj Puerto A, D, E, K
    while((SYSCTL_RCGCGPIO_R&0x219)==0){}
    //PUERTO E  MIC y Dedos
    GPIO_PORTE_AHB_DIR_R|=0x0;                  //PE2 entradas
    GPIO_PORTE_AHB_AFSEL_R|=0x4;                //PE2 función alterna
    GPIO_PORTE_AHB_DEN_R&=~0x4;                 //Sin modo digital
    GPIO_PORTE_AHB_AMSEL_R|=0x4;                //Modo analógico hab

}

//--------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%    INICIALIZACIÓN TIMER6    %%%%%%%%%%%%%%%%%%%%
//--------------------------------------------------------------------
void Inicial_Timer(void){
    SYSCTL_RCGCTIMER_R|=0x20;             //Reloj del Timer 0
    while((SYSCTL_RCGCTIMER_R&0x20)==0){}
    TIMER6_CTL_R=0x0;                   //Deshab Timer 0
    TIMER6_CFG_R=0x0;                   //Uso del timer con 32 bits
    TIMER6_TAMR_R=0x11;                 //Conteo ascendente, Modo One shot: Al terminar de contar termina la operación
    TIMER6_ICR_R=0x1;                   //Limpieza de la bandera de interrupción
//    NVIC_PRI4_R=(NVIC_PRI4_R&0x00FFFFFF)|0x20000000;  //Prioridad 1 (Menor)
    //NVIC_EN0_R|=((1<<19) & 0xFFFFFFFF); //Hab interrupción
}

//--------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%    INICIALIZACIÓN ADC0    %%%%%%%%%%%%%%%%%%%%
//--------------------------------------------------------------------
void Inicial_ADC0(void){
    SYSCTL_RCGCADC_R=0x1;                       //Reloj ADC
    while((SYSCTL_RCGCADC_R&0x1)==0){}
    ADC0_PC_R=0x7;                              //Tasa completa de conversión
    ADC0_ACTSS_R=~0x9;                          //Seq 3 y Seq 0 Deshab
    //SECUENCIADOR 0  (Dedos)
    ADC0_EMUX_R=0xF;                            //Siempre se encuentra muestreando
    ADC0_SSEMUX3_R=0x0;                         //AIN[15..0]
    ADC0_SSMUX3_R|=0x1;                         //Orden de AIN para el muestreo
                                                //1:AIN1  (PE2)
    ADC0_SSCTL3_R|=0x6;                         //No: Sens temp, Muestra diferencial
                                                //Si: Interrupción, PE5 indica el final secuencia
    ADC0_SSTSH3_R|=0xC;                         //Sample and Hold Nsh=256    Fconv=60[kHz]
    ADC0_IM_R|=0x8;                             //Hab interrupción del secuenciador 3
    ADC0_SSOP3|=0x1;                            //Los datos de la FIFO se evían al comparador digital
    ADC0_SSDC3_R|=0x0;                          //Limites se definen por DCCCMP0
    ADC0_DCCMP0_R|=0x8000800;                   //COMP1=COMP2=0x800=2048 => 1.65 [V];
    ADC0_DCCTL0|=0x1D;                          //Comparador digital 0 genera una interrupción si se está en banda alta
                                                //y se emplea modo Once

//    ADC0_DCCTL0|=0x14;                         //Comparador digital 0 genera una interrupción si se está en banda media
                                                //y se emplea modo Always
    //----------------------
    ADC0_ACTSS_R=0x8;                               //Seq 3 Hab
    SYSCTL_PLLFREQ0_R |= SYSCTL_PLLFREQ0_PLLPWR;    // encender PLL
    while((SYSCTL_PLLSTAT_R&0x01)==0);              // espera a que el PLL fije su frecuencia
    SYSCTL_PLLFREQ0_R &= ~SYSCTL_PLLFREQ0_PLLPWR;   // apagar PLL

    NVIC_PRI1_R=(NVIC_PRI1_R&0xFF00FFFF)|0x00000000;  //6 Prioridad 0 (Mayor)
    NVIC_EN0_R|=((1<<6) & 0xFFFFFFFF);          //Hab Interrupción
}


//--------------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%    INTERRUPCIÓN DEL ADC 0  %%%%%%%%%%%%%%%%%%%%
//--------------------------------------------------------------------------
void Inter_ADC_Seq3(void){
    static int Periodo=0;
    ADC0_IM_R|=0x1;                                 //Limpieza de la bandera de interrupción
    switch(Seccion){
    case 1:
        TIMER6_CTL_R=0x0;                           //Deshab Timer 6
        Periodo=(Periodo+TIMER0_TAV_R)/32;          //Promedio del periodo de la señal
        TIMER6_TAV_R=0;                             //Reinicia conteo a cero cada que entre a la interrupción
        if(Ingreso==32){
            TIMER6_CFG_R=0x0;                   //Uso del timer con 32 bits
            TIMER6_TAMR_R=0x12;                 //Conteo ascendente, Modo periódico: Reinicia conteo tras finalizar
            TIMER6_TAILR_R=round(Periodo);
            Seccion=2;
        }else{
            TIMER6_TAILR_R=300000;                      //(16.66[ms] Periodo) 266667
            TIMER6_CTL_R=0x1;                           //Hab Timer 0
        }
        TIMER6_CTL_R=0x1;                           //Hab Timer 0
        break;
    case 2:
        break:
    case 3:
        break;
    default:
        break;
    }
}


//------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%    PROGRAMA PRINCIPAL    %%%%%%%%%%%%%%%%%%%%
//------------------------------------------------------------------
int main(void)
{
	return 0;
}
