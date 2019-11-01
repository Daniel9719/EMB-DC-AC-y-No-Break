#include<stdbool.h>
#include<stdint.h>
#include"inc/tm4c1294ncpdt.h"

int Periodo=0;
unsigned char FETp_FETn=0;
int Porc_Pot=50, Match=0;
//----------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%    DECLARACIÓN DE PUERTOS    %%%%%%%%%%%%%%%%%%%%
//----------------------------------------------------------------------
void Inic_puertos(void){
    SYSCTL_RCGCGPIO_R=0x1110;                       //Reloj Puerto E, J y N
    while((SYSCTL_RCGCGPIO_R&0x1110)==0){}

    //PUERTO E (ADC de Muestreo)
    GPIO_PORTE_AHB_DIR_R|=0x0;                      //PE2 entrada
    GPIO_PORTE_AHB_AFSEL_R|=0x4;                    //PE2 función alterna
    GPIO_PORTE_AHB_DEN_R&=~0x4;                     //Sin modo digital
    GPIO_PORTE_AHB_AMSEL_R|=0x4;                    //Modo analógico hab

    //PUERTO J0 (Falla CFE)
    GPIO_PORTJ_AHB_DIR_R|=0x0;                      //PJ0 entrada
    GPIO_PORTJ_AHB_DEN_R|=0x1;                      //PJ0 Sin modo digital
    GPIO_PORTJ_AHB_IM_R|=0x1;                       //PJ0 Genera una interrupción
    GPIO_PORTJ_AHB_IBE_R|=0x1;                      //PJ0 Ambos flancos generan una interrupción
    NVIC_PRI12_R=(NVIC_PRI12_R&0x0FFFFFFF)|0x00000000;//Prioridad 0 (Menor)
    NVIC_EN1_R|=((1<<(51-32)) & 0xFFFFFFFF);        //Hab interrupción del Puerto J

    //PUERTO N2 y N3 (MOSFETs)
    GPIO_PORTN_DIR_R|=0xC;                          //PN2 y PN3 salidas
    GPIO_PORTN_DEN_R|=0xC;                          //PN2 y PN3 Con modo digital
}

//--------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%    INICIALIZACIÓN TIMER6    %%%%%%%%%%%%%%%%%%%%
//--------------------------------------------------------------------
void Inicial_Timer(void){
    SYSCTL_RCGCTIMER_R|=0x78;                       //Reloj del Timer 6, 5, 4 y 3
    while((SYSCTL_RCGCTIMER_R&0x60)==0){}
    //Reloj de muestreo
    TIMER6_CTL_R=0x0;                               //Deshab Timer 6
    TIMER6_CFG_R=0x0;                               //Uso del timer con 32 bits
    TIMER6_TAMR_R=0x92;                             //Conteo ascendente, Modo periódico: Reinicia conteo tras finalizar, Modo Snap Shot
    TIMER6_ICR_R=0x1;                               //Limpieza de la bandera de interrupción

    //PWM Descendente
    TIMER5_CTL_R=0x0;                               //Deshab Timer 5
    TIMER5_CFG_R=0x0;                               //Uso del timer con 32 bits
    TIMER5_TAMR_R=0x561;                            //Módulo A: Act Match en timeout, actualiza TAIL en timeout
                                                    //Conteo descendente, Wait for Trigger, Inter Match
                                                    //Modo OneShot: Reinicia conteo tras finalizar
    TIMER5_ICR_R=0x1;                               //Limpieza de la bandera de interrupción
    NVIC_PRI16_R=(NVIC_PRI16_R&0xFFFFFF0F)|0x00000020;//Prioridad 1 (Menor)
    NVIC_EN2_R|=((1<<(65-64)) & 0xFFFFFFFF);        //Hab interrupción del Timer 5

    //PWM Ascendente
    TIMER4_CTL_R=0x0;                               //Deshab Timer 4
    TIMER4_CFG_R=0x0;                               //Uso del timer con 32 bits
    TIMER4_TAMR_R=0x571;                            //Módulo A: Act Match en timeout, actualiza TAIL en timeout
                                                    //Conteo ascendente, Wait for Trigger, Inter Match
                                                    //Modo OneShot: Reinicia conteo tras finalizar
    TIMER4_ICR_R=0x1;                               //Limpieza de la bandera de interrupción
    NVIC_PRI15_R=(NVIC_PRI15_R&0x0FFFFFFF)|0x20000000;//Prioridad 1 (Menor)
    NVIC_EN1_R|=((1<<(63-32)) & 0xFFFFFFFF);        //Hab interrupción del Timer 4

    //Reloj Síncrono
    TIMER3_CTL_R=0x0;                               //Deshab Timer 3
    TIMER3_CFG_R=0x0;                               //Uso del timer con 32 bits
    TIMER3_TAMR_R=0x92;                             //Conteo ascendente, Modo periódico: Reinicia conteo tras finalizar, Modo Snap Shot
    TIMER3_ICR_R=0x1;                               //Limpieza de la bandera de interrupción
}

//-------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%    INICIALIZACIÓN ADC0    %%%%%%%%%%%%%%%%%%%%
//-------------------------------------------------------------------
void Inicial_ADC0(void){
    SYSCTL_RCGCADC_R=0x1;                           //Reloj ADC0
    while((SYSCTL_RCGCADC_R&0x1)==0){}
    ADC0_PC_R=0x7;                                  //Tasa completa de conversión
    ADC0_ACTSS_R&=~0x8;                             //Seq 3 Deshab
    //SECUENCIADOR 0  (Dedos)
    ADC0_EMUX_R=0xF000;                                //Siempre se encuentra muestreando
    ADC0_SSEMUX3_R=0x0;                             //AIN[15..0]
    ADC0_SSMUX3_R|=0x1;                             //Orden de AIN para el muestreo
                                                    //1:AIN1  (PE2)
    ADC0_SSCTL3_R|=0x2;                             //No: Sens temp, Muestra diferencial, Interrupción al fin del muestreo
                                                    //Si: PE2 indica el final secuencia
    ADC0_SSTSH3_R|=0xC;                             //Sample and Hold Nsh=256    Fconv=60[kHz]
    ADC0_IM_R|=0x80000;                             //Hab interrupción del comparador digital del Sec 3
    ADC0_SSOP3_R|=0x1;                              //Los datos de la FIFO se evían al comparador digital
    ADC0_SSDC3_R|=0x0;                              //Limites se definen por DCCCMP0
    ADC0_DCCMP0_R|=0x8000800;                       //COMP1=COMP2=0x800=2048 => 1.65 [V];
    ADC0_DCCTL0_R|=0x1D;                              //Comparador digital 0 genera una interrupción si se está en banda alta
                                                    //y se emplea modo Once
    NVIC_PRI4_R=(NVIC_PRI4_R&0xFFFF0FFF)|0x00002000;//Prioridad 1 (Menor)
    NVIC_EN0_R|=((1<<17) & 0xFFFFFFFF);             //Hab Interrupción Secuenciador 3

    //----------------------
    ADC0_ACTSS_R=0x8;                               //Seq 3 Hab
    SYSCTL_PLLFREQ0_R |= SYSCTL_PLLFREQ0_PLLPWR;    // encender PLL
    while((SYSCTL_PLLSTAT_R&0x01)==0);              // espera a que el PLL fije su frecuencia
    SYSCTL_PLLFREQ0_R &= ~SYSCTL_PLLFREQ0_PLLPWR;   // apagar PLL

    ADC0_PSSI_R=0x8;                                //Inicio conversión Seq3
}

//--------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%    INTERRUPCIÓN DEL ADC 0  %%%%%%%%%%%%%%%%%%%%
//--------------------------------------------------------------------
void Inter_ADC_Seq3(void){
    static int Ingreso=0;
    ADC0_ISC_R=0x80000;                             //Limpieza bandera MIS ADC0 Seq3
    ADC0_DCISC_R=0x1;                               //Limpieza bandera Comparador Digital 0
    switch(Ingreso){
        case 0:
            Ingreso++;
            TIMER6_TAILR_R=300000;                  //(16.66[ms] Periodo) 266667
            TIMER6_CTL_R=0x1;                       //Hab Timer 0
            break;
        case 32:
            Periodo=Periodo+(TIMER6_TAV_R)/32;      //Promedio del periodo de la señal
            TIMER6_TAV_R=0;                         //Reinicio a cero del Reloj de muestreo
            TIMER3_TAILR_R=Periodo/2;               //Actualización de valor de cuenta de Reloj Síncrono
            TIMER3_CTL_R=0x1;                       //Hab Timer 3 Reloj Síncrono
            TIMER4_TAILR_R=Periodo/4;               //Valor de cuenta del PWMa
            TIMER4_CTL_R=0x1;                       //Hab Timer 4 PWM
            TIMER5_TAILR_R=Periodo/4;               //Valor de cuenta del PWMd
            TIMER5_CTL_R=0x1;                       //Hab Timer 5 PWM
            Match=(Porc_Pot*(Periodo/4)-16)/100+8;
            TIMER4_TAMATCHR_R=Match;
            TIMER5_TAMATCHR_R=Match;
            Ingreso=0;
            Periodo=0;
            break;
        default:
            Periodo=Periodo+(TIMER6_TAV_R)/32;      //Promedio del periodo de la señal
            TIMER6_TAV_R=0;
            Ingreso++;
            break;
    }
}

//------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%    INTERRUCPIÓN GPIOJ    %%%%%%%%%%%%%%%%%%%%
//------------------------------------------------------------------
void Inter_PortJ_Falla(void){
    GPIO_PORTJ_AHB_ICR_R|=0x1;                      //Limpieza bandera de interrupción
    if(GPIO_PORTJ_AHB_DATA_R&0x1){                  //Termina la falla
        TIMER4_IMR_R=0x0;                            //Deshab interrupción Match Timer A
        TIMER5_IMR_R=0x0;                            //Deshab interrupción Match Timer A
        GPIO_PORTN_DATA_R&=~0xC;                   //Apagar MOSFETs
        ADC0_IM_R|=0x80000;                         //Hab interrupción del comparador digital del Sec 3
    }else{                                          //Ocurre la falla
        ADC0_IM_R&=~0x80000;                        //Deshab interrupción del comparador digital del Sec 3
        if(TIMER6_TAV_R<(Periodo/2)){
            FETp_FETn=0x8;
        }else{
            FETp_FETn=0x4;
        }
        TIMER6_CTL_R=0x0;                           //Deshab Timer 6 Reloj Síncrono
        if(TIMER3_TAV_R<(Periodo/4)){
            if(TIMER4_TAV_R>Match){
                GPIO_PORTN_DATA_R=FETp_FETn;        //Encender N2 o N3 (MOSFET)
            }
        }else{
            if(TIMER5_TAV_R>Match){
                GPIO_PORTN_DATA_R=FETp_FETn;        //Encender N2 o N3 (MOSFET)
            }
        }
        TIMER4_IMR_R|=0x10;                          //Hab interrupción Match Timer A
        TIMER5_IMR_R|=0x10;                          //Hab interrupción Match Timer A
    }
}

//--------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%    INTERRUPCIÓN DEL TIMER4  %%%%%%%%%%%%%%%%%%%
//--------------------------------------------------------------------
void Inter_TIMER4_Asc(void){
    GPIO_PORTN_DATA_R=FETp_FETn;                   //Encender MOSFETs
    FETp_FETn^=0xC;                                 //Conmutación de los MOSFETs
    TIMER4_ICR_R=0x10;                              //Limpieza de la bandera de interrupción Match
}

//--------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%    INTERRUPCIÓN DEL TIMER5  %%%%%%%%%%%%%%%%%%%
//--------------------------------------------------------------------
void Inter_TIMER5_Des(void){
    GPIO_PORTN_DATA_R&=~0xC;                        //Apagar MOSFETs
    TIMER5_ICR_R=0x10;                              //Limpieza de la bandera de interrupción Match
}

//------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%    PROGRAMA PRINCIPAL    %%%%%%%%%%%%%%%%%%%%
//------------------------------------------------------------------
void main(void){
    Inic_puertos();
    Inicial_Timer();
    Inicial_ADC0();
	while(1){}
}
